#include "core.h"
#include "core/Application.h"
#include "core/Input.h"
#include "scripting/LuauLayer.h"
#include "scripting/GlobalApi.h"
#include "scripting/ScriptAnalyzer.h"
#include "platform/Platform.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "editor/panels/ConsoleLog.h"
#include "editor/panels/CodeEditorPanel.h"
#include "editor/panels/CodeEditorPanelManager.h"
#include "svg/Svg.h"

#pragma warning( push )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4324 )
#pragma warning( disable : 4324 )
#include <lua.h>
#include <lualib.h>
#include <luacode.h>
#pragma warning ( pop )

namespace MathAnim
{
	struct Bytecode
	{
		std::string scriptFilepath;
		char* bytes;
		size_t size;
	};

	struct ParsedError
	{
		std::string filepath;
		int lineNumber;
		std::string message;
	};

	enum class LoadBytecodeOptions : uint8
	{
		None = 1,
		PopBytecode = None << 1,
	};

	struct AnimObjectDebugData
	{
		CodeEditorPanelData* editor;
		AnimObjId object;
	};

	namespace LuauLayer
	{
		// ---------- Internal Functions ----------
		static void* luaAllocWrapper(void* ud, void* ptr, size_t osize, size_t nsize);
		static ParsedError parseError(const char* luaRuntimeErrorMessage);
		static void tryRemoveCachedBytecode(const std::string& filename);
		static bool analyzeScriptFile(const std::string& filename);
		static bool analyzeScriptSource(const std::string& sourceCode, const std::string& scriptName);
		static Bytecode compileToBytecode(const char* data, size_t dataSize, std::string const& scriptName);
		static int loadBytecode(Bytecode const& bytecode, LoadBytecodeOptions options = LoadBytecodeOptions::PopBytecode);
		static bool readFile(std::string const& scriptPath, RawMemory* memory);

		// ---------- Internal Variables ----------
		ScriptAnalyzer* analyzer = nullptr;
		lua_State* luaState = nullptr;
		std::unordered_map<std::string, Bytecode> cachedBytecode;
		std::filesystem::path scriptDirectory = "";
		const Bytecode* currentExecutingScript = nullptr;

		static int lastLineWeWereDebugging = -1;
		static bool skipDebugStep = false;
		static int frameAdd = 1;
		static void debugBreak(lua_State* L, lua_Debug* ar)
		{
			lua_singlestep(L, true);
			if (lastLineWeWereDebugging != ar->currentline)
			{
				lastLineWeWereDebugging = ar->currentline;
				skipDebugStep = false;

				auto* callbacks = lua_callbacks(luaState);
				auto* debug = (AnimObjectDebugData*)callbacks->userdata;
				debug->editor->currentExecutingLine = ar->currentline;
				debug->editor->debuggingSessionActive = true;

				lua_yield(L, 0);
			}
		}

		static void debugStep(lua_State* L, lua_Debug* ar)
		{
			if (!skipDebugStep && lastLineWeWereDebugging != ar->currentline)
			{
				auto* callbacks = lua_callbacks(luaState);
				auto* debug = (AnimObjectDebugData*)callbacks->userdata;
				debug->editor->currentExecutingLine = ar->currentline;
				lua_yield(L, 0);
			}

			lastLineWeWereDebugging = ar->currentline;
		}

		static void debugInterrupt(lua_State*, lua_Debug* ar)
		{
			g_logger_info("Debug Interrupting. Current line: {}", ar->currentline);
		}

		static void debugProtectedError(lua_State*)
		{
			g_logger_info("Debug Protected error!");
		}

		void init(const std::filesystem::path& inScriptDirectory, AnimationManagerData* am)
		{
			Platform::createDirIfNotExists(inScriptDirectory.string().c_str());

			luaState = lua_newstate(luaAllocWrapper, NULL);
			ScriptApi::registerGlobalFunctions(luaState, am);
			scriptDirectory = inScriptDirectory;

			analyzer = g_memory_new ScriptAnalyzer(inScriptDirectory);

			auto* callbacks = lua_callbacks(luaState);
			callbacks->debugbreak = debugBreak;
			callbacks->debugstep = debugStep;
			callbacks->debuginterrupt = debugInterrupt;
			callbacks->debugprotectederror = debugProtectedError;
		}

		void update(AnimationManagerData* am)
		{
			bool runScriptDebugCode = false;

			if (currentExecutingScript && Input::keyPressed(GLFW_KEY_F5))
			{
				skipDebugStep = true;
				runScriptDebugCode = true;
			}

			if (currentExecutingScript && Input::keyPressed(GLFW_KEY_F10))
			{
				runScriptDebugCode = true;
			}

			if (runScriptDebugCode)
			{
				int result = lua_resume(luaState, NULL, 0);
				auto* callbacks = lua_callbacks(luaState);
				auto* debug = (AnimObjectDebugData*)callbacks->userdata;
				const AnimObject* obj = AnimationManager::getObject(am, debug->object);

				if (obj)
				{
					for (auto breadthFirstIter = obj->beginBreadthFirst(am); breadthFirstIter != obj->end(); ++breadthFirstIter)
					{
						AnimObject* childObj = AnimationManager::getMutableObject(am, *breadthFirstIter);
						if (childObj)
						{
							childObj->_svgObjectStart->finalize();
							childObj->retargetSvgScale();
						}
					}

					Application::setFrameIndex(Application::getFrameIndex() + frameAdd);
					frameAdd *= -1;
				}

				if (result == LUA_OK)
				{
					debug->editor->debuggingSessionActive = false;

					currentExecutingScript = nullptr;
					lastLineWeWereDebugging = -1;
					lua_singlestep(luaState, false);
					g_memory_free(debug);
				}
				else if (result != LUA_YIELD)
				{
					ParsedError error = parseError(lua_tostring(luaState, -1));
					ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
					lua_pop(luaState, 1);

					currentExecutingScript = nullptr;
					lua_singlestep(luaState, false);
					g_memory_free(debug);
				}
			}
		}

		bool compile(const std::string& filename)
		{
			if (!analyzeScriptFile(filename))
			{
				return false;
			}

			std::string scriptPath = (scriptDirectory / filename).make_preferred().lexically_normal().string();
			RawMemory memory;
			if (!readFile(scriptPath, &memory))
			{
				return false;
			}

			Bytecode bytecode = compileToBytecode((const char*)memory.data, memory.size, scriptPath);
			memory.free();

			int result = loadBytecode(bytecode);

			// If the script throws a runtime error, don't cache it
			if (result != 0)
			{
				::free(bytecode.bytes);
				return false;
			}

			// Remove the script first since we're about to cache new bytecode
			tryRemoveCachedBytecode(filename);
			cachedBytecode[filename] = bytecode;

			return true;
		}

		bool compile(const std::string& sourceCode, const std::string& scriptName)
		{
			if (!analyzeScriptSource(sourceCode, scriptName))
			{
				return false;
			}

			Bytecode bytecode = compileToBytecode(sourceCode.c_str(), sourceCode.length(), scriptName);
			bytecode.scriptFilepath = scriptName;
			int result = loadBytecode(bytecode);

			if (result != 0)
			{
				::free(bytecode.bytes);
				return false;
			}

			// If the bytecode exists, free the bytes since it's about to be replaced
			tryRemoveCachedBytecode(scriptName);
			cachedBytecode[scriptName] = bytecode;

			return true;
		}

		const std::string& getCurrentExecutingScriptFilepath()
		{
			if (currentExecutingScript)
			{
				return currentExecutingScript->scriptFilepath;
			}

			static const std::string dummyScriptName = "NULL_SCRIPT";
			return dummyScriptName;
		}

		bool startDebugging(const std::string& filename, CodeEditorPanelData* editor)
		{
			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				// Try to compile
				if (!compile(filename))
				{
					g_logger_warning("Tried to debug script '{}' which was never compiled successfully.", filename);
					return false;
				}

				iter = cachedBytecode.find(filename);
			}

			const Bytecode& bytecode = iter->second;
			currentExecutingScript = &bytecode;
			int result = loadBytecode(bytecode, LoadBytecodeOptions::None);

			if (result != 0)
			{
				lua_pop(luaState, 1);
				currentExecutingScript = nullptr;
				return false;
			}

			for (auto const breakpointLine : editor->breakpoints)
			{
				lua_breakpoint(luaState, -1, breakpointLine, true);
			}

			// Setup user data so we can tell our code editor what line is currently executing
			auto* callbacks = lua_callbacks(luaState);
			auto debug = (AnimObjectDebugData*)g_memory_allocate(sizeof(AnimObjectDebugData));
			debug->editor = editor;
			debug->object = NULL_ANIM_OBJECT;
			callbacks->userdata = debug;

			// NOTE: lua_resume will begin a coroutine, this will suspend execution
			//       if it YIELDS, which should happen once it hits a breakpoint
			result = lua_resume(luaState, NULL, 0);
			if (result != LUA_YIELD && result != LUA_OK)
			{
				ParsedError error = parseError(lua_tostring(luaState, -1));
				ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
				lua_pop(luaState, 1);

				currentExecutingScript = nullptr;
				g_memory_free(debug);
				return false;
			}

			if (result == LUA_OK)
			{
				g_memory_free(debug);
			}

			return true;
		}

		bool execute(const std::string& filename)
		{
			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				g_logger_warning("Tried to execute script '{}' which was never compiled successfully.", filename);
				return false;
			}

			const Bytecode& bytecode = iter->second;
			currentExecutingScript = &bytecode;
			int result = luau_load(luaState, filename.c_str(), bytecode.bytes, bytecode.size, 0);

			if (result == 0)
			{
				result = lua_pcall(luaState, 0, LUA_MULTRET, 0);
				if (result)
				{
					ParsedError error = parseError(lua_tostring(luaState, -1));
					ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
					lua_pop(luaState, 1);
					return false;
				}

				currentExecutingScript = nullptr;
				return true;
			}

			lua_pop(luaState, 1);
			currentExecutingScript = nullptr;
			return false;
		}

		bool executeOnAnimObj(const std::string& filename, const std::string& functionName, AnimationManagerData* am, AnimObjId id)
		{
			const AnimObject* obj = AnimationManager::getObject(am, id);
			if (!obj)
			{
				g_logger_error("Cannot run script on null anim object. Object '{}' does not exist.", id);
				return false;
			}

			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				g_logger_warning("Tried to execute script '{}' which was never compiled successfully.", filename);
				return false;
			}

			const Bytecode& bytecode = iter->second;
			currentExecutingScript = &bytecode;
			int result = luau_load(luaState, filename.c_str(), bytecode.bytes, bytecode.size, 0);

			if (result == 0)
			{
				// Run the script to get all the function definitions loaded
				result = lua_pcall(luaState, 0, LUA_MULTRET, 0);
				if (result)
				{
					ParsedError error = parseError(lua_tostring(luaState, -1));
					ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
					lua_pop(luaState, 1);
					return false;
				}

				// Get the function and push it on top of the stack
				lua_getfield(luaState, LUA_GLOBALSINDEX, functionName.c_str());
				// Push anim object to top of the stack
				ScriptApi::pushAnimObject(luaState, *obj);
				result = lua_pcall(luaState, 1, 0, 0);
				if (result)
				{
					ParsedError error = parseError(lua_tostring(luaState, -1));
					ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
					lua_pop(luaState, 1);
					return false;
				}

				for (auto breadthFirstIter = obj->beginBreadthFirst(am); breadthFirstIter != obj->end(); ++breadthFirstIter)
				{
					AnimObject* childObj = AnimationManager::getMutableObject(am, *breadthFirstIter);
					if (childObj)
					{
						childObj->_svgObjectStart->finalize();
						childObj->retargetSvgScale();
					}
				}

				currentExecutingScript = nullptr;
				return true;
			}

			lua_pop(luaState, 1);
			currentExecutingScript = nullptr;
			return false;
		}

		bool debugOnAnimObj(const std::string& filename, const std::string& functionName, AnimationManagerData* am, AnimObjId id)
		{
			const AnimObject* obj = AnimationManager::getObject(am, id);
			if (!obj)
			{
				g_logger_error("Cannot run script on null anim object. Object '{}' does not exist.", id);
				return false;
			}

			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				g_logger_warning("Tried to execute script '{}' which was never compiled successfully.", filename);
				return false;
			}

			const Bytecode& bytecode = iter->second;
			currentExecutingScript = &bytecode;
			int result = luau_load(luaState, filename.c_str(), bytecode.bytes, bytecode.size, 0);

			CodeEditorPanelData* editor = CodeEditorPanelManager::getEditor(filename);
			if (editor)
			{
				for (auto const breakpointLine : editor->breakpoints)
				{
					lua_breakpoint(luaState, -1, breakpointLine, true);
				}
			}

			if (result != 0)
			{
				lua_pop(luaState, 1);
				currentExecutingScript = nullptr;
				return false;
			}

			// Run the script to get all the function definitions loaded
			result = lua_pcall(luaState, 0, LUA_MULTRET, 0);
			if (result)
			{
				ParsedError error = parseError(lua_tostring(luaState, -1));
				ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
				lua_pop(luaState, 1);
				return false;
			}

			// Get the function and push it on top of the stack
			lua_getfield(luaState, LUA_GLOBALSINDEX, functionName.c_str());
			// Push anim object to top of the stack
			ScriptApi::pushAnimObject(luaState, *obj);

			// Setup user data so we can tell our code editor what line is currently executing
			auto* callbacks = lua_callbacks(luaState);
			auto debugData = (AnimObjectDebugData*)g_memory_allocate(sizeof(AnimObjectDebugData));
			debugData->object = id;
			debugData->editor = editor;
			callbacks->userdata = debugData;

			// NOTE: lua_resume will begin a coroutine, this will suspend execution
			//       if it YIELDS, which should happen once it hits a breakpoint
			result = lua_resume(luaState, NULL, 1);
			if (result != LUA_YIELD && result != LUA_OK)
			{
				ParsedError error = parseError(lua_tostring(luaState, -1));
				ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
				lua_pop(luaState, 1);

				currentExecutingScript = nullptr;
				g_memory_free(debugData);
				return false;
			}

			// Free debug data if we finished execution without hitting a breakpoint
			if (result == LUA_OK)
			{
				g_memory_free(debugData);
			}

			for (auto breadthFirstIter = obj->beginBreadthFirst(am); breadthFirstIter != obj->end(); ++breadthFirstIter)
			{
				AnimObject* childObj = AnimationManager::getMutableObject(am, *breadthFirstIter);
				if (childObj)
				{
					childObj->_svgObjectStart->finalize();
					childObj->retargetSvgScale();
				}
			}

			return true;
		}

		bool remove(const std::string& filename)
		{
			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				g_logger_warning("Tried to delete script '{}' which was never compiled successfully.", filename);
				return false;
			}

			cachedBytecode.erase(iter);
			return true;
		}

		void free()
		{
			if (currentExecutingScript)
			{
				auto* callbacks = lua_callbacks(luaState);
				auto* debug = (AnimObjectDebugData*)callbacks->userdata;
				g_memory_free(debug);
			}

			if (analyzer)
			{
				analyzer->free();
				g_memory_delete(analyzer);
			}

			if (luaState)
			{
				// TODO: Do I need to free this?
				lua_close(luaState);
			}

			for (auto pair : cachedBytecode)
			{
				::free(pair.second.bytes);
			}
			cachedBytecode.clear();

			analyzer = nullptr;
			luaState = nullptr;
		}

		// ---------- Internal Functions ----------
		static void* luaAllocWrapper(void* ud, void* ptr, size_t osize, size_t nsize)
		{
			(void)ud;  (void)osize;  /* not used */
			if (nsize == 0)
			{
				::free(ptr);
				return NULL;
			}
			else
				return ::realloc(ptr, nsize);
		}

		static void tryRemoveCachedBytecode(const std::string& filename)
		{
			auto iter = cachedBytecode.find(filename);
			if (iter != cachedBytecode.end())
			{
				::free(iter->second.bytes);
				cachedBytecode.erase(iter);
			}
		}

		static bool analyzeScriptFile(const std::string& filename)
		{
			if (!analyzer->analyze(filename))
			{
				// If the bytecode exists, free the bytes since the most recent code is broken
				tryRemoveCachedBytecode(filename);
				return false;
			}

			return true;
		}

		static bool analyzeScriptSource(const std::string& sourceCode, const std::string& scriptName)
		{
			if (!analyzer->analyze(sourceCode, scriptName))
			{
				tryRemoveCachedBytecode(scriptName);
				return false;
			}

			return true;
		}

		static Bytecode compileToBytecode(const char* data, size_t dataSize, std::string const& scriptName)
		{
			lua_CompileOptions compileOptions = {};
			compileOptions.optimizationLevel = 1;
			compileOptions.debugLevel = 1;
			size_t bytecodeSize = 0;
			char* bytecode = luau_compile(data, dataSize, &compileOptions, &bytecodeSize);
			return {
				scriptName,
				bytecode,
				bytecodeSize
			};
		}

		static int loadBytecode(Bytecode const& bytecode, LoadBytecodeOptions options)
		{
			int result = luau_load(luaState, bytecode.scriptFilepath.c_str(), bytecode.bytes, bytecode.size, 0);

			bool popBytecode = ((uint8)options & (uint8)LoadBytecodeOptions::PopBytecode);
			if (popBytecode)
			{
				// Pop the bytecode off the stack
				lua_pop(luaState, 1);
			}

			return result;
		}

		static bool readFile(std::string const& scriptPath, RawMemory* memory)
		{
			// Read the file
			FILE* fp = fopen(scriptPath.c_str(), "rb");
			if (!fp)
			{
				g_logger_warning("Could not open file '{}', error opening file.", scriptPath.c_str());
				return false;
			}

			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			memory->init(fileSize + 1);
			fread(memory->data, fileSize, 1, fp);
			memory->data[fileSize] = '\0';
			fclose(fp);

			return true;
		}

		// Disgusting quick parsing to get the dumb error message in
		// a nicer struct
		static ParsedError parseError(const char* message)
		{
			ParsedError res = {};

			// Lua runtime error messages look like this:
			// [string "C:/SomeFile/Path.luau"]:10: Error Message
			size_t messageLength = std::strlen(message);
			constexpr const char stringKeyword[] = "string";

			bool foundStringKeyword = false;
			bool foundStartQuote = false;
			bool foundEndQuote = false;
			bool foundLineNumberStart = false;

			size_t filepathStart = 0;
			size_t filepathEnd = 0;
			size_t lineNumberStart = 0;
			size_t lineNumberEnd = 0;
			size_t errorMessageStart = 0;

			for (size_t i = 0; i < messageLength; i++)
			{
				if (!foundStringKeyword && message[i] == 's')
				{
					if (i + (sizeof(stringKeyword) - 1) < messageLength)
					{
						for (size_t j = 0; j < (sizeof(stringKeyword) - 1); j++)
						{
							if (stringKeyword[j] != message[i + j])
							{
								break;
							}
						}
						foundStringKeyword = true;
						i += sizeof(stringKeyword) - 1;
					}
				}

				if (foundStartQuote && !foundEndQuote && message[i] == '"')
				{
					foundEndQuote = true;
					filepathEnd = i;
				}

				if (foundStringKeyword && !foundStartQuote && message[i] == '"')
				{
					foundStartQuote = true;
					filepathStart = i + 1;
				}

				if (foundEndQuote && !foundLineNumberStart && message[i] >= '0' && message[i] <= '9')
				{
					foundLineNumberStart = true;
					lineNumberStart = i;
					for (size_t j = i; j < messageLength; j++)
					{
						if (message[j] < '0' || message[j] > '9')
						{
							lineNumberEnd = j;
							i = j;
							errorMessageStart = i + 1;
							break;
						}
					}
				}

				if (foundLineNumberStart)
				{
					break;
				}
			}

			// Sanitize inputs
			filepathStart = glm::clamp(filepathStart, (size_t)0, messageLength);
			filepathEnd = glm::clamp(filepathEnd, filepathStart, messageLength);
			lineNumberStart = glm::clamp(lineNumberStart, filepathEnd, messageLength);
			lineNumberEnd = glm::clamp(lineNumberEnd, lineNumberStart, messageLength);
			errorMessageStart = glm::clamp(errorMessageStart, lineNumberEnd, messageLength);

			// Get the strings
			if (foundStringKeyword && foundStartQuote && foundEndQuote && foundLineNumberStart)
			{
				res.filepath = std::string(message + filepathStart, message + filepathEnd);
				res.filepath = std::filesystem::path(res.filepath).lexically_normal().string();
				res.lineNumber = std::stoi(std::string(message + lineNumberStart, message + lineNumberEnd));
				res.message = std::string(message + errorMessageStart, message + messageLength);
			}
			else
			{
				res.filepath = getCurrentExecutingScriptFilepath();
				res.lineNumber = -1;
				res.message = message;
			}

			return res;
		}
	}
}
