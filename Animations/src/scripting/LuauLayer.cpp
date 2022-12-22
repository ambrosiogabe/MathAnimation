#include "core.h"
#include "scripting/LuauLayer.h"
#include "scripting/GlobalApi.h"
#include "scripting/ScriptAnalyzer.h"
#include "platform/Platform.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"

#include <lua.h>
#include <lualib.h>
#include <luacode.h>

namespace MathAnim
{
	struct Bytecode
	{
		std::string scriptFilepath;
		char* bytes;
		size_t size;
	};

	namespace LuauLayer
	{
		// ---------- Internal Functions ----------
		static void* luaAllocWrapper(void* ud, void* ptr, size_t osize, size_t nsize);

		// ---------- Internal Variables ----------
		ScriptAnalyzer* analyzer = nullptr;
		lua_State* luaState = nullptr;
		std::unordered_map<std::string, Bytecode> cachedBytecode;
		std::filesystem::path scriptDirectory = "";
		const Bytecode* currentExecutingScript = nullptr;

		void init(const std::string& inScriptDirectory, AnimationManagerData* am)
		{
			Platform::createDirIfNotExists(inScriptDirectory.c_str());

			luaState = lua_newstate(luaAllocWrapper, NULL);
			ScriptApi::registerGlobalFunctions(luaState, am);
			scriptDirectory = inScriptDirectory;

			analyzer = (ScriptAnalyzer*)g_memory_allocate(sizeof(ScriptAnalyzer));
			new(analyzer)ScriptAnalyzer(inScriptDirectory);
		}

		void update()
		{

		}

		bool compile(const std::string& filename)
		{
			if (!analyzer->analyze(filename))
			{
				// If the bytecode exists, free the bytes since the most recent code is broken
				auto iter = cachedBytecode.find(filename);
				if (iter != cachedBytecode.end())
				{
					::free(iter->second.bytes);
					cachedBytecode.erase(iter);
				}
				return false;
			}

			std::string scriptPath = (scriptDirectory / filename).make_preferred().string();
			FILE* fp = fopen(scriptPath.c_str(), "rb");
			if (!fp)
			{
				g_logger_warning("Could not open file '%s', error opening file.", filename.c_str());
				return false;
			}

			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			RawMemory memory;
			memory.init(fileSize + 1);
			fread(memory.data, fileSize, 1, fp);
			memory.data[fileSize] = '\0';
			fclose(fp);

			lua_CompileOptions compileOptions = {};
			compileOptions.optimizationLevel = 1;
			compileOptions.debugLevel = 1;
			size_t bytecodeSize = 0;
			char* bytecode = luau_compile((const char*)memory.data, memory.size, &compileOptions, &bytecodeSize);
			int result = luau_load(luaState, filename.c_str(), bytecode, bytecodeSize, 0);

			memory.free();

			// Pop the bytecode off the stack
			lua_pop(luaState, 1);

			if (result != 0)
			{
				::free(bytecode);
				return false;
			}

			{
				// If the bytecode exists, free the bytes since it's about to be
				// replaced
				auto iter = cachedBytecode.find(filename);
				if (iter != cachedBytecode.end())
				{
					::free(iter->second.bytes);
				}
			}

			Bytecode res;
			res.bytes = bytecode;
			res.size = bytecodeSize;
			res.scriptFilepath = scriptPath;
			cachedBytecode[filename] = res;

			return true;
		}

		bool compile(const std::string& sourceCode, const std::string& scriptName)
		{
			if (!analyzer->analyze(sourceCode, scriptName))
			{
				return false;
			}

			lua_CompileOptions compileOptions = {};
			compileOptions.optimizationLevel = 1;
			compileOptions.debugLevel = 1;
			size_t bytecodeSize = 0;
			char* bytecode = luau_compile(sourceCode.c_str(), sourceCode.length(), &compileOptions, &bytecodeSize);
			int result = luau_load(luaState, scriptName.c_str(), bytecode, bytecodeSize, 0);

			// Pop the bytecode off the stack
			lua_pop(luaState, 1);

			if (result != 0)
			{
				::free(bytecode);
				return false;
			}

			// If the bytecode exists, free the bytes since it's about to be
			// replaced
			auto iter = cachedBytecode.find(scriptName);
			if (iter != cachedBytecode.end())
			{
				::free(iter->second.bytes);
			}

			Bytecode res;
			res.bytes = bytecode;
			res.size = bytecodeSize;
			res.scriptFilepath = scriptName;
			cachedBytecode[scriptName] = res;

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

		bool execute(const std::string& filename)
		{
			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				g_logger_warning("Tried to execute script '%s' which was never compiled successfully.", filename.c_str());
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
					g_logger_error("Failed to run script: %s", lua_tostring(luaState, -1));
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
				g_logger_error("Cannot run script on null anim object. Object '%d' does not exist.", id);
				return false;
			}

			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				g_logger_warning("Tried to execute script '%s' which was never compiled successfully.", filename.c_str());
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
					g_logger_error("Failed to load script: %s", lua_tostring(luaState, -1));
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
					g_logger_error("Failed to run function(%s): %s", functionName.c_str(), lua_tostring(luaState, -1));
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

		bool remove(const std::string& filename)
		{
			auto iter = cachedBytecode.find(filename);
			if (iter == cachedBytecode.end())
			{
				g_logger_warning("Tried to delete script '%s' which was never compiled successfully.", filename.c_str());
				return false;
			}

			cachedBytecode.erase(iter);
			return true;
		}

		void free()
		{
			if (analyzer)
			{
				analyzer->free();
				analyzer->~ScriptAnalyzer();
				g_memory_free(analyzer);
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
	}
}
