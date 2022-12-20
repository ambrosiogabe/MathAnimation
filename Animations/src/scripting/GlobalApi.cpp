#include "core.h"
#include "scripting/GlobalApi.h"
#include "scripting/LuauLayer.h"
#include "editor/ConsoleLog.h"

#include <lua.h>
#include <lualib.h>
#include <luaconf.h>

extern "C"
{
	static int luaPrint(lua_State* L, g_logger_level logLevel)
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
				if (logLevel == g_logger_level::Log)
				{
					MathAnim::ConsoleLog::log(scriptFilepath.c_str(), debugInfo.currentline, "%s", str);
				}
				else if (logLevel == g_logger_level::Info)
				{
					MathAnim::ConsoleLog::info(scriptFilepath.c_str(), debugInfo.currentline, "%s", str);
				}
				else if (logLevel == g_logger_level::Warning)
				{
					MathAnim::ConsoleLog::warning(scriptFilepath.c_str(), debugInfo.currentline, "%s", str);
				}
				else if (logLevel == g_logger_level::Error)
				{
					MathAnim::ConsoleLog::error(scriptFilepath.c_str(), debugInfo.currentline, "%s", str);
				}
			}
			else if (lua_isboolean(L, i))
			{
				bool val = lua_toboolean(L, i);
				if (logLevel == g_logger_level::Log)
				{
					MathAnim::ConsoleLog::log(scriptFilepath.c_str(), debugInfo.currentline, "%s", val ? "true" : "false");
				} 
				else if (logLevel == g_logger_level::Info)
				{
					MathAnim::ConsoleLog::info(scriptFilepath.c_str(), debugInfo.currentline, "%s", val ? "true" : "false");
				}
				else if (logLevel == g_logger_level::Warning)
				{
					MathAnim::ConsoleLog::warning(scriptFilepath.c_str(), debugInfo.currentline, "%s", val ? "true" : "false");
				}
				else if (logLevel == g_logger_level::Error)
				{
					MathAnim::ConsoleLog::error(scriptFilepath.c_str(), debugInfo.currentline, "%s", val ? "true" : "false");
				}
			}
			else
			{
				MathAnim::ConsoleLog::warning(scriptFilepath.c_str(), debugInfo.currentline,
					"Lua Log tried to print non-string or boolean or number value.");
			}
		}

		return 0;
	}

	int global_printWrapper(lua_State* L)
	{
		return luaPrint(L, g_logger_level::Log);
	}

	int global_logWrite(lua_State* L)
	{
		return luaPrint(L, g_logger_level::Log);
	}

	int global_logInfo(lua_State* L)
	{
		return luaPrint(L, g_logger_level::Info);
	}

	int global_logWarning(lua_State* L)
	{
		return luaPrint(L, g_logger_level::Warning);
	}

	int global_logError(lua_State* L)
	{
		return luaPrint(L, g_logger_level::Error);
	}
}

namespace MathAnim
{
	namespace ScriptApi
	{
		void registerGlobalFunctions(lua_State* L)
		{
			luaL_openlibs(L);
			lua_pushcfunction(L, global_printWrapper, "print");
			lua_setglobal(L, "print");

			// Create table to act as a logger
			// Table looks like:
			// logger {
			//   write()
			//   info()
			//   warn()
			//   error()
			// }
			lua_createtable(L, 0, 4);

			lua_pushcfunction(L, global_logWrite, "logger.write");
			lua_setfield(L, -2, "write");

			lua_pushcfunction(L, global_logInfo, "logger.info");
			lua_setfield(L, -2, "info");

			lua_pushcfunction(L, global_logWarning, "logger.warn");
			lua_setfield(L, -2, "warn");

			lua_pushcfunction(L, global_logError, "logger.error");
			lua_setfield(L, -2, "error");

			lua_setglobal(L, "logger");

			// TODO: We should do this to sandbox the scripts, but this prevents the scripts
			// from calling global functions
			// luaL_sandbox(L);
		}
	}
}