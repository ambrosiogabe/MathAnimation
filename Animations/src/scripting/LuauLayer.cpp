#include "core.h"
#include "scripting/LuauLayer.h"
#include "scripting/GlobalApi.h"
#include "scripting/ScriptAnalyzer.h"
#include "platform/Platform.h"

#include <lua.h>
#include <luacode.h>
#include <lualib.h>
#include <Luau/Frontend.h>
#include <Luau/BuiltinDefinitions.h>

const char* testSourceChunkName = "Foo";
const char* testSourceCode = R"raw(
function ispositive(x)
    return x > 0
end

print(ispositive(1))
print(ispositive("2"))

function isfoo(a)
    return a == "foo"
end

print(isfoo("bar"))
print(isfoo(1))

)raw";

namespace MathAnim
{
	struct Bytecode
	{
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

		void init(const std::string& inScriptDirectory)
		{
			Platform::createDirIfNotExists(inScriptDirectory.c_str());

			luaState = lua_newstate(luaAllocWrapper, NULL);
			ScriptApi::registerGlobalFunctions(luaState);
			scriptDirectory = inScriptDirectory;

			analyzer = (ScriptAnalyzer*)g_memory_allocate(sizeof(ScriptAnalyzer));
			new(analyzer)ScriptAnalyzer(inScriptDirectory);
		}

		void update()
		{
			static bool runTest = true;
			if (runTest)
			{
				if (compile(testSourceCode, "Foo.lua"))
				{
					execute("Foo.lua");
				}

				runTest = false;
			}
		}

		bool compile(const std::string& filename)
		{
			if (!analyzer->analyze(filename))
			{
				return false;
			}

			std::string scriptPath = (scriptDirectory/filename).string();
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

			size_t bytecodeSize = 0;
			char* bytecode = luau_compile((const char*)memory.data, memory.size, NULL, &bytecodeSize);
			int result = luau_load(luaState, filename.c_str(), bytecode, bytecodeSize, 0);

			memory.free();

			// Pop the bytecode off the stack
			lua_pop(luaState, 1);

			if (result != 0)
			{
				::free(bytecode);
				return false;
			}

			// If the bytecode exists, free the bytes since it's about to be
			// replaced
			auto iter = cachedBytecode.find(filename);
			if (iter != cachedBytecode.end())
			{
				::free(iter->second.bytes);
			}

			Bytecode res;
			res.bytes = bytecode;
			res.size = bytecodeSize;
			cachedBytecode[filename] = res;

			return true;
		}

		bool compile(const std::string& sourceCode, const std::string& scriptName)
		{
			if (!analyzer->analyze(sourceCode, scriptName))
			{
				return false;
			}

			size_t bytecodeSize = 0;
			char* bytecode = luau_compile(sourceCode.c_str(), sourceCode.length(), NULL, &bytecodeSize);
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
			cachedBytecode[scriptName] = res;

			return true;
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
			int result = luau_load(luaState, filename.c_str(), bytecode.bytes, bytecode.size, 0);

			if (result == 0)
			{
				result = lua_pcall(luaState, 0, LUA_MULTRET, 0);
				if (result)
				{
					g_logger_error("Failed to run script: %s", lua_tostring(luaState, -1));
					lua_pop(luaState, 1);
				}

				return true;
			}

			lua_pop(luaState, 1);
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
