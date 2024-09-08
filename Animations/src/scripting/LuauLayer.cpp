#include "core.h"
#include "core/Application.h"
#include "core/Input.h"
#include "core/Profiling.h"
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
		static void registerScriptsInDir(std::filesystem::path const& dir);

		// ---------- Internal Variables ----------
		ScriptAnalyzer* analyzer = nullptr;
		lua_State* luaState = nullptr;
		lua_State* scriptState = nullptr;
		std::unordered_map<std::string, Bytecode> cachedBytecode;
		std::filesystem::path scriptDirectory = "";
		const Bytecode* currentExecutingScript = nullptr;
		bool isDebuggingCurrentScript = false;

		static const char* defaultAnimObjScriptsDir = "./assets/defaultScripts/animObjectScripts";

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

				g_logger_assert(scriptState != nullptr, "How are we debugging a script if it's not loaded?");
				auto* callbacks = lua_callbacks(scriptState);
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
				g_logger_assert(scriptState != nullptr, "How are we debugging a script if it's not loaded?");
				auto* callbacks = lua_callbacks(scriptState);
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

		void init(const std::filesystem::path& inScriptDirectory, AnimationManagerData* am)
		{
			Platform::createDirIfNotExists(inScriptDirectory.string().c_str());

			luaState = lua_newstate(luaAllocWrapper, NULL);
			ScriptApi::registerGlobalFunctions(luaState, am);
			scriptDirectory = inScriptDirectory;

			analyzer = g_memory_new ScriptAnalyzer(inScriptDirectory);

			registerScriptsInDir(inScriptDirectory);
			registerScriptsInDir(defaultAnimObjScriptsDir);
		}

		void update(AnimationManagerData* am)
		{
			bool runScriptDebugCode = false;

			if (currentExecutingScript && scriptState && Input::keyPressed(GLFW_KEY_F5))
			{
				skipDebugStep = true;
				runScriptDebugCode = true;
			}

			if (currentExecutingScript && scriptState && Input::keyRepeatedOrDown(GLFW_KEY_F10))
			{
				runScriptDebugCode = true;
			}

			if (runScriptDebugCode)
			{
				int result = lua_resume(scriptState, NULL, 0);
				auto* callbacks = lua_callbacks(scriptState);
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
					isDebuggingCurrentScript = false;
					lastLineWeWereDebugging = -1;
					lua_singlestep(scriptState, false);
					g_memory_free(debug);

					scriptState = nullptr;
					lua_pop(luaState, 1);
				}
				else if (result != LUA_YIELD)
				{
					ParsedError error = parseError(lua_tostring(scriptState, -1));
					ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
					lua_pop(scriptState, 1);

					currentExecutingScript = nullptr;
					isDebuggingCurrentScript = false;
					lua_singlestep(scriptState, false);
					g_memory_free(debug);

					scriptState = nullptr;
					lua_pop(luaState, 1);
				}
			}
		}

		bool compile(const std::string& filename)
		{
			if (!analyzer)
			{
				return false;
			}

			if (!analyzeScriptFile(filename))
			{
				return false;
			}

			std::optional<Luau::SourceCode> maybeSourceCode = analyzer->resolveFile(filename);
			if (!maybeSourceCode.has_value())
			{
				return false;
			}
			auto const& sourceCode = maybeSourceCode.value();
			Bytecode bytecode = compileToBytecode((const char*)sourceCode.source.c_str(), sourceCode.source.size(), filename);

			if (!bytecode.isValid)
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

		Bytecode getBytecode(const std::string& filename)
		{
			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				// Try to compile
				if (!compile(filename))
				{
					Bytecode res = {};
					res.isValid = false;
					return res;
				}

				iter = cachedBytecode.find(filename);
			}

			return iter->second;
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
				lua_pop(scriptState, 1);
				currentExecutingScript = nullptr;
				return false;
			}

			for (auto const breakpointLine : editor->breakpoints)
			{
				lua_breakpoint(scriptState, -1, breakpointLine, true);
			}

			// Setup user data so we can tell our code editor what line is currently executing
			auto* callbacks = lua_callbacks(scriptState);
			callbacks->debugbreak = debugBreak;
			callbacks->debuginterrupt = debugInterrupt;
			callbacks->debugstep = debugStep;

			auto debug = (AnimObjectDebugData*)g_memory_allocate(sizeof(AnimObjectDebugData));
			debug->editor = editor;
			debug->object = NULL_ANIM_OBJECT;
			callbacks->userdata = debug;

			// NOTE: lua_resume will begin a coroutine, this will suspend execution
			//       if it YIELDS, which should happen once it hits a breakpoint
			result = lua_resume(scriptState, NULL, 0);
			if (result != LUA_YIELD && result != LUA_OK)
			{
				ParsedError error = parseError(lua_tostring(scriptState, -1));
				ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
				lua_pop(scriptState, 1);

				currentExecutingScript = nullptr;
				g_memory_free(debug);

				scriptState = nullptr;
				lua_pop(luaState, 1);
				return false;
			}

			if (result == LUA_OK)
			{
				g_memory_free(debug);
			}

			return true;
		}

		bool pushBytecode(const std::string& filename)
		{
			if (currentExecutingScript || isDebuggingCurrentScript)
			{
				return false;
			}

			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				return false;
			}

			int result = LUA_OK;
			{
				MP_PROFILE_EVENT("LuauLayer_LoadBytecode");
				const Bytecode& bytecode = iter->second;
				currentExecutingScript = &bytecode;
				scriptState = lua_newthread(luaState);
				luaL_sandboxthread(scriptState);
				result = luau_load(scriptState, filename.c_str(), bytecode.bytes, bytecode.size, 0);
				if (result != LUA_OK)
				{
					scriptState = nullptr;
					lua_pop(luaState, 1);
					return false;
				}
			}

			return true;
		}

		bool executeBytecode()
		{
			if (!currentExecutingScript || isDebuggingCurrentScript || !scriptState)
			{
				return false;
			}

			MP_PROFILE_EVENT("LuauLayer_ExecuteBytecode");
			int result = lua_pcall(scriptState, 0, LUA_MULTRET, 0);
			if (result != LUA_OK)
			{
				ParsedError error = parseError(lua_tostring(scriptState, -1));
				ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
				lua_pop(scriptState, 1);
				return false;
			}

			return true;
		}

		bool popBytecode()
		{
			if (scriptState)
			{
				// Pop thread from stack
				lua_pop(luaState, 1);
				scriptState = nullptr;
			}

			if (!currentExecutingScript)
			{
				return false;
			}

			currentExecutingScript = nullptr;
			return true;
		}

		bool executeOnAnimate(const std::string& functionName, AnimationManagerData* am, AnimObjId id, float t)
		{
			MP_PROFILE_EVENT("LuauLayer_ExecuteOnAnimObj");
			// Can't execute a script while one is already being debugged
			if (isDebuggingCurrentScript)
			{
				return false;
			}

			if (!currentExecutingScript || !scriptState)
			{
				g_logger_warning("Tried to execute script on anim object, but no script was loaded.");
				return false;
			}

			AnimObject* obj = AnimationManager::getMutableObject(am, id);
			if (!obj)
			{
				g_logger_error("Cannot run script on null anim object. Object '{}' does not exist.", id);
				return false;
			}

			int result = LUA_OK;
			{
				MP_PROFILE_EVENT("LuauLayer_ExecuteFunction");
				{
					MP_PROFILE_EVENT("LuauLayer_PushFunctionToStack");
					// Get the function and push it on top of the stack
					lua_getfield(scriptState, LUA_GLOBALSINDEX, functionName.c_str());
				}
				lua_pushnumber(scriptState, (double)t);
				{
					MP_PROFILE_EVENT("LuauLayer_PushAnimObjectToStack");
					// Push anim object to top of the stack
					ScriptApi::pushAnimObject(scriptState, *obj);
				}
				{
					MP_PROFILE_EVENT("LuauLayer_PCallFunction");
					result = lua_pcall(scriptState, 2, 0, 0);
				}

				if (result)
				{
					ParsedError error = parseError(lua_tostring(scriptState, -1));
					ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
					lua_pop(scriptState, 1);
					return false;
				}
			}

			{
				MP_PROFILE_EVENT("LuauLayer_FinalizeAnimObjects");
				// TODO: Just make an oninspector function and generate function in C++ code instead of this abstract execute thing
				if (functionName == "generate")
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

					obj->_svgObjectStart->finalize();
					obj->retargetSvgScale();
				}

				return true;
			}
		}

		bool executeOnAnimObj(const std::string& functionName, AnimationManagerData* am, AnimObjId id)
		{
			MP_PROFILE_EVENT("LuauLayer_ExecuteOnAnimObj");
			// Can't execute a script while one is already being debugged
			if (isDebuggingCurrentScript)
			{
				return false;
			}

			if (!currentExecutingScript || !scriptState)
			{
				g_logger_warning("Tried to execute script on anim object, but no script was loaded.");
				return false;
			}

			AnimObject* obj = AnimationManager::getMutableObject(am, id);
			if (!obj)
			{
				g_logger_error("Cannot run script on null anim object. Object '{}' does not exist.", id);
				return false;
			}

			int result = LUA_OK;
			{
				MP_PROFILE_EVENT("LuauLayer_ExecuteFunction");
				{
					MP_PROFILE_EVENT("LuauLayer_PushFunctionToStack");
					// Get the function and push it on top of the stack
					lua_getfield(scriptState, LUA_GLOBALSINDEX, functionName.c_str());
				}
				{
					MP_PROFILE_EVENT("LuauLayer_PushAnimObjectToStack");
					// Push anim object to top of the stack
					ScriptApi::pushAnimObject(scriptState, *obj);
				}
				{
					MP_PROFILE_EVENT("LuauLayer_PCallFunction");
					result = lua_pcall(scriptState, 1, 0, 0);
				}

				if (result)
				{
					ParsedError error = parseError(lua_tostring(scriptState, -1));
					ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
					lua_pop(scriptState, 1);
					return false;
				}
			}

			{
				MP_PROFILE_EVENT("LuauLayer_FinalizeAnimObjects");
				// TODO: Just make an oninspector function and generate function in C++ code instead of this abstract execute thing
				if (functionName == "generate")
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

					obj->_svgObjectStart->finalize();
					obj->retargetSvgScale();
				}

				return true;
			}
		}

		bool executeFn(const std::string& functionName)
		{
			MP_PROFILE_EVENT("LuauLayer_ExecuteOnAnimObj");
			// Can't execute a script while one is already being debugged
			if (isDebuggingCurrentScript)
			{
				return false;
			}

			if (!currentExecutingScript || !scriptState)
			{
				g_logger_warning("Tried to execute script on anim object, but no script was loaded.");
				return false;
			}

			int result = LUA_OK;
			int fieldType = LUA_TNIL;
			{
				MP_PROFILE_EVENT("LuauLayer_ExecuteFunction");
				{
					MP_PROFILE_EVENT("LuauLayer_PushFunctionToStack");
					// Get the function and push it on top of the stack
					fieldType = lua_getfield(scriptState, LUA_GLOBALSINDEX, functionName.c_str());
				}
				{
					MP_PROFILE_EVENT("LuauLayer_PCallFunction");
					if (fieldType != LUA_TNIL)
					{
						result = lua_pcall(scriptState, 0, 0, 0);
					}
				}

				if (result)
				{
					ParsedError error = parseError(lua_tostring(scriptState, -1));
					ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
					lua_pop(scriptState, 1);
					return false;
				}
			}

			return true;
		}

		bool debugGenerateAnimObj(const std::string& filename, AnimationManagerData* am, AnimObjId id)
		{
			if (currentExecutingScript)
			{
				g_logger_warning("Tried to debug script on anim object, a script '{}' was already loaded.", currentExecutingScript->scriptFilepath);
				return false;
			}

			if (!pushBytecode(filename))
			{
				return false;
			}

			const AnimObject* obj = AnimationManager::getObject(am, id);
			if (!obj)
			{
				g_logger_error("Cannot debug script on null anim object. Object '{}' does not exist.", id);
				popBytecode();
				return false;
			}

			CodeEditorPanelData* editor = CodeEditorPanelManager::getEditor(currentExecutingScript->scriptFilepath);
			if (!editor)
			{
				// If no editor is open, there's no way to debug the script because no breakpoints will have been set
				popBytecode();
				return false;
			}

			for (auto const breakpointLine : editor->breakpoints)
			{
				lua_breakpoint(scriptState, 1, breakpointLine, true);
			}

			if (!executeBytecode())
			{
				g_logger_warning("Failed to execute bytecode for script '{}' while debugging.", currentExecutingScript->scriptFilepath);
				popBytecode();
				return false;
			}

			// Setup user data so we can tell our code editor what line is currently executing
			auto* callbacks = lua_callbacks(scriptState);
			callbacks->debugbreak = debugBreak;
			callbacks->debuginterrupt = debugInterrupt;
			callbacks->debugstep = debugStep;

			auto debugData = (AnimObjectDebugData*)g_memory_allocate(sizeof(AnimObjectDebugData));
			debugData->object = id;
			debugData->editor = editor;
			callbacks->userdata = debugData;

			// Get the function and push it on top of the stack
			lua_getfield(scriptState, LUA_GLOBALSINDEX, "onInspector");
			// Push anim object to top of the stack
			ScriptApi::pushAnimObject(scriptState, *obj);

			int result = lua_pcall(scriptState, 1, 0, 0);
			if (result != LUA_OK)
			{
				ParsedError error = parseError(lua_tostring(scriptState, -1));
				ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
				g_memory_free(debugData);
				lua_pop(scriptState, 1);
				popBytecode();
				return false;
			}

			// Get the function and push it on top of the stack
			lua_getfield(scriptState, LUA_GLOBALSINDEX, "generate");
			// Push anim object to top of the stack
			ScriptApi::pushAnimObject(scriptState, *obj);

			// NOTE: lua_resume will begin a coroutine, this will suspend execution
			//       if it YIELDS, which should happen once it hits a breakpoint
			result = lua_resume(scriptState, NULL, 1);
			if (result != LUA_YIELD && result != LUA_OK)
			{
				ParsedError error = parseError(lua_tostring(scriptState, -1));
				ConsoleLog::error(error.filepath.c_str(), error.lineNumber, "%s", error.message.c_str());
				g_memory_free(debugData);
				lua_pop(scriptState, 1);
				popBytecode();
				return false;
			}

			isDebuggingCurrentScript = true;

			// Free debug data if we finished execution without hitting a breakpoint
			if (result == LUA_OK)
			{
				popBytecode();
				isDebuggingCurrentScript = false;
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

		bool execute(const std::string& filename)
		{
			if (pushBytecode(filename))
			{
				if (executeBytecode())
				{
					return popBytecode();
				}
			}

			return false;
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
				g_logger_assert(scriptState != nullptr, "How do we have an executing script with no script state?");
				auto* callbacks = lua_callbacks(scriptState);
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
			scriptState = nullptr;
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
				bytecodeSize,
				true
			};
		}

		static int loadBytecode(Bytecode const& bytecode, LoadBytecodeOptions options)
		{
			scriptState = lua_newthread(luaState);
			luaL_sandboxthread(scriptState);
			int result = luau_load(scriptState, bytecode.scriptFilepath.c_str(), bytecode.bytes, bytecode.size, 0);

			bool popBytecode = ((uint8)options & (uint8)LoadBytecodeOptions::PopBytecode);
			if (popBytecode)
			{
				// Pop the bytecode off the stack
				lua_pop(scriptState, 1);
			}

			return result;
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

		static void registerScriptsInDir(std::filesystem::path const& dir)
		{
			for (auto file : std::filesystem::directory_iterator(dir))
			{
				if (!file.is_regular_file())
				{
					continue;
				}

				// TODO: This should be renamed... It's always the full filepath and this is super confusing in code
				std::string const& filename = file.path().string();
				LuauLayer::compile(filename);
				LuauLayer::pushBytecode(filename);
				LuauLayer::executeBytecode();
				LuauLayer::executeFn("register");
				LuauLayer::popBytecode();
			}
		}
	}
}
