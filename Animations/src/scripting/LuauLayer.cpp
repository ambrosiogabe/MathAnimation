#include "core.h"
#include "scripting/LuauLayer.h"

#include <lua.h>
#include <luacode.h>

namespace MathAnim
{
	namespace LuauLayer
	{
		const char* testSourceChunkName = "GlobalEnv";
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

		static void* luaAllocWrapper(void* ud, void* ptr, size_t osize,
			size_t nsize)
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

		void init()
		{
			lua_State* luaState = lua_newstate(luaAllocWrapper, NULL);

			size_t bytecodeSize = 0;
			char* bytecode = luau_compile(testSourceCode, std::strlen(testSourceCode), NULL, &bytecodeSize);
			int result = luau_load(luaState, testSourceChunkName, bytecode, bytecodeSize, 0);
			::free(bytecode);

			if (result == 0)
			{
				g_logger_info("Success?");
			}
		}

		void update()
		{

		}

		void free()
		{

		}
	}
}
