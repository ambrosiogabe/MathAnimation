#include "core.h"
#include "scripting/GlobalApi.h"

#include <lua.h>
#include <lualib.h>

extern "C"
{
	int global_printWrapper(lua_State* L)
	{
		int nargs = lua_gettop(L);

		for (int i = 1; i <= nargs; i++)
		{
			if (lua_isstring(L, i))
			{
				/* Pop the next arg using lua_tostring(L, i) and do your print */
				const char* str = lua_tostring(L, i);
				g_logger_info("Lua Log: %s", str);
			}
			else if (lua_isboolean(L, i))
			{
				bool val = lua_toboolean(L, i);
				g_logger_info("Lua Log: %s", val ? "true" : "false");
			}
			else
			{
				g_logger_warning("Lua Log tried to print non-string or boolean or number value.");
			}
		}

		return 0;
	}
}

namespace MathAnim
{
	namespace ScriptApi
	{
		void registerGlobalFunctions(lua_State* luaState)
		{
			luaL_openlibs(luaState);
			lua_pushcfunction(luaState, global_printWrapper, "print");
			lua_setglobal(luaState, "print");
		}
	}
}