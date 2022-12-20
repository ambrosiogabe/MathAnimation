#include "core.h"
#include "scripting/GlobalApi.h"
#include "scripting/LuauLayer.h"
#include "editor/ConsoleLog.h"

#include <lua.h>
#include <lualib.h>
#include <luaconf.h>

extern "C"
{
	int global_printWrapper(lua_State* L)
	{
		int stackFrame = lua_stackdepth(L) - 1;
		lua_Debug debugInfo = {};
		lua_getinfo(L, stackFrame, "l", &debugInfo);
		const std::string& scriptFilepath = MathAnim::LuauLayer::getCurrentExecutingScriptFilepath();

		int nargs = lua_gettop(L);

		for (int i = 1; i <= nargs; i++)
		{
			if (lua_isstring(L, i))
			{
				/* Pop the next arg using lua_tostring(L, i) and do your print */
				const char* str = lua_tostring(L, i);
				MathAnim::ConsoleLog::info(scriptFilepath.c_str(), debugInfo.currentline, "%s", str);
			}
			else if (lua_isboolean(L, i))
			{
				bool val = lua_toboolean(L, i);
				MathAnim::ConsoleLog::info(scriptFilepath.c_str(), debugInfo.currentline, "%s", val ? "true" : "false");
			}
			else
			{
				MathAnim::ConsoleLog::warning(scriptFilepath.c_str(), debugInfo.currentline,
					"Lua Log tried to print non-string or boolean or number value.");
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