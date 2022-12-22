#include "core.h"
#include "scripting/GlobalApi.h"
#include "scripting/LuauLayer.h"
#include "editor/ConsoleLog.h"
#include "editor/SceneHierarchyPanel.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"

#include <lua.h>
#include <lualib.h>
#include <luaconf.h>

#define throwError(L, errorMessage) \
do { \
	lua_pushstring(L, errorMessage); \
	lua_error(L); \
    return -1; \
} while(false)

#define throwErrorNoReturn(L, errorMessage) \
do { \
	lua_pushstring(L, errorMessage); \
	lua_error(L); \
} while(false)

#define argumentCheck(L, start, end, fnSignature, actualNumArgs) \
do { \
	if (!(actualNumArgs >= start && actualNumArgs <= end)) { \
		ConsoleLog::error(L, "Invalid number of arguments passed to "#fnSignature". Expected >= "#start" && <= "#end" Instead got: %d", actualNumArgs); \
		throwError(L, "Invalid number of arguments passed to "#fnSignature); \
	} \
} while (false)

#define argumentCheckWithSelf(L, expectedNumArgs, fnSignature, actualNumArgs) \
do { \
	if (actualNumArgs != expectedNumArgs + 1) { \
		if (actualNumArgs == expectedNumArgs) { \
			ConsoleLog::error(L, "Invalid number of arguments passed to "#fnSignature". Did you call setName like obj."#fnSignature"? If you did, you need to change the syntax to obj:"#fnSignature" with the ':' instead of the '.'"); \
		} else { \
			ConsoleLog::error(L, "Invalid number of arguments passed to "#fnSignature". Expected %d number of arguments, instead got: %d", expectedNumArgs, actualNumArgs); \
		} \
		throwError(L, "Invalid number of arguments passed to "#fnSignature); \
	} \
} while(false)

extern "C"
{
	using namespace MathAnim;

	// --------------- Internal Variables ---------------
	static std::unordered_map<lua_CFunction, std::string> cFunctionDebugNames;

	// --------------- Internal Functions ---------------
	static uint64 toU64(lua_State* L, int index);
	static void pushU64(lua_State* L, uint64 value);
	static Vec3 toVec3(lua_State* L, int index);
	static void pushVec3(lua_State* L, const Vec3& value);
	static void pushCFunction(lua_State* L, lua_CFunction fn, const char* debugName);

	static AnimationManagerData* getAnimationManagerData(lua_State* L);
	static std::string getAsString(lua_State* L, int index = 1);

	static void luaPrintTable(lua_State* L, char* buffer, size_t bufferSize, int index, int tabDepth = 1, int maxTabDepth = 5);
	static void luaPrintTable(lua_State* L, char* buffer, size_t bufferSize, int index, int tabDepth, int maxTabDepth)
	{
		if (tabDepth >= maxTabDepth)
		{
			return;
		}

		constexpr size_t recursiveBufferSize = 128;
		char recursiveBuffer[recursiveBufferSize];

		char* bufferPtr = buffer;
		size_t sizeLeft = bufferSize;

		{
			int numBytesWritten = snprintf(buffer, sizeLeft, "{\n");
			if (numBytesWritten > 0 && numBytesWritten < sizeLeft)
			{
				sizeLeft -= numBytesWritten;
				bufferPtr += numBytesWritten;
			}
			else
			{
				buffer[bufferSize - 1] = '\0';
				return;
			}
		}

		// Push another reference to the table on top of the stack (so we know
		// where it is, and this function can work for negative, positive and
		// pseudo indices
		lua_pushvalue(L, index);
		// stack now contains: -1 => table
		lua_pushnil(L);
		// stack now contains: -1 => nil; -2 => table
		while (lua_next(L, -2))
		{
			// stack now contains: -1 => value; -2 => key; -3 => table
			// copy the key so that lua_tostring does not modify the original
			lua_pushvalue(L, -2);
			// stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
			const char* key = lua_tostring(L, -1);
			std::string value;
			if (lua_istable(L, -2))
			{
				luaPrintTable(L, recursiveBuffer, recursiveBufferSize, -2, tabDepth + 1);
				value = recursiveBuffer;
			}
			else
			{
				value = getAsString(L, -2);
			}
			int numBytesWritten = snprintf(bufferPtr, sizeLeft, "%*c%s => %s\n", tabDepth * 2, ' ', key, value.c_str());
			if (numBytesWritten > 0 && numBytesWritten < sizeLeft)
			{
				sizeLeft -= numBytesWritten;
				bufferPtr += numBytesWritten;
			}
			// pop value + copy of key, leaving original key
			lua_pop(L, 2);
			// stack now contains: -1 => key; -2 => table

			if (numBytesWritten < 0)
			{
				// Just in case
				buffer[bufferSize - 1] = '\0';
				break;
			}
		}

		if (sizeLeft > 0)
		{
			if (tabDepth > 1)
			{
				snprintf(bufferPtr, sizeLeft, "%*c}", (tabDepth - 1) * 2, ' ');
			}
			else
			{
				snprintf(bufferPtr, sizeLeft, "}");
			}
		}

		// Write out nullptr
		// Just in case
		buffer[bufferSize - 1] = '\0';
		// stack now contains: -1 => table (when lua_next returns 0 it pops the key
		// but does not push anything.)
		// Pop table
		lua_pop(L, 1);
		// Stack is now the same as it was on entry to this function
	}

	static std::string getAsString(lua_State* L, int index)
	{
		if (lua_isstring(L, index))
		{
			// Numbers also return true for "isstring" but I don't want to wrap them
			// in quotes so I'll just exit early here to avoid that
			if (lua_isnumber(L, index))
			{
				return lua_tostring(L, index);
			}

			/* Pop the next arg using lua_tostring(L, i) and do your print */
			return std::string("\"") + lua_tostring(L, index) + std::string("\"");
		}
		else if (lua_isboolean(L, index))
		{
			bool val = lua_toboolean(L, index);
			return val ? "true" : "false";
		}
		else if (lua_istable(L, index))
		{
			constexpr size_t tableBufferSize = 2048;
			char tableBuffer[tableBufferSize];
			luaPrintTable(L, tableBuffer, tableBufferSize, index);
			return tableBuffer;
		}
		else if (lua_iscfunction(L, index))
		{
			lua_CFunction func = lua_tocfunction(L, index);
			std::string fnName = "Unregistered C Function";
			auto iter = cFunctionDebugNames.find(func);
			if (iter != cFunctionDebugNames.end())
			{
				fnName = iter->second;
			}

			return fnName;
		}
		else if (lua_isnil(L, index))
		{
			return "nil";
		}
		else if (lua_islightuserdata(L, index))
		{
			void* ptr = lua_tolightuserdata(L, index);
			if (ptr)
			{
				std::stringstream stream;
				stream << "0x" << std::setfill('0') << std::setw(sizeof(uint64)) << std::hex << (uint64)ptr;
				return stream.str();
			}

			return "nullptr";
		}
		else if (lua_isfunction(L, index))
		{
			return "lua_function";
		}
		else
		{
			ConsoleLog::warning(L, "Lua Log tried to print non-string or boolean or number value.");
		}

		return "(null)";
	}

	static int luaPrintItem(lua_State* L, g_logger_level logLevel, const std::string& scriptFilepath, const lua_Debug& debugInfo, int index = 1)
	{
		std::string str = getAsString(L);
		if (logLevel == g_logger_level::Log)
		{
			ConsoleLog::log(scriptFilepath.c_str(), debugInfo.currentline, "%s", str.c_str());
		}
		else if (logLevel == g_logger_level::Info)
		{
			ConsoleLog::info(scriptFilepath.c_str(), debugInfo.currentline, "%s", str.c_str());
		}
		else if (logLevel == g_logger_level::Warning)
		{
			ConsoleLog::warning(scriptFilepath.c_str(), debugInfo.currentline, "%s", str.c_str());
		}
		else if (logLevel == g_logger_level::Error)
		{
			ConsoleLog::error(scriptFilepath.c_str(), debugInfo.currentline, "%s", str.c_str());
		}

		return 0;
	}

	int global_printStackVar(lua_State* L, int index)
	{
		luaPrintItem(L, g_logger_level::Info, "Null", {}, index);

		return 0;
	}

	static int luaPrint(lua_State* L, g_logger_level logLevel)
	{
		int stackFrame = lua_stackdepth(L) - 1;
		lua_Debug debugInfo = {};
		lua_getinfo(L, stackFrame, "l", &debugInfo);
		const std::string& scriptFilepath = LuauLayer::getCurrentExecutingScriptFilepath();

		int nargs = lua_gettop(L);

		for (int i = 1; i <= nargs; i++)
		{
			lua_getargument(L, 0, i);
			luaPrintItem(L, logLevel, scriptFilepath, debugInfo);
			lua_pop(L, 1);
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

	int global_createAnimObjectFn(lua_State* L)
	{
		int nargs = lua_gettop(L);
		argumentCheck(L, 1, 1, "createAnimObject(AnimObject)", nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// First parameter is AnimObject
		lua_getfield(L, 1, "id");
		AnimObjId id = toU64(L, -1);
		lua_pop(L, 1);

		AnimObject newObject = AnimObject::createDefaultFromParent(am, AnimObjectTypeV1::ScriptObject, id, true);
		ScriptApi::pushAnimObject(L, newObject);

		// Add the animation object to the scene
		AnimationManager::addAnimObject(am, newObject);
		// TODO: Ugly what do I do???
		SceneHierarchyPanel::addNewAnimObject(newObject);

		return 1;
	}

	int global_setAnimObjName(lua_State* L)
	{
		// setName: (self: AnimObject, name: string) -> (),
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, setName(nameString), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		if (!lua_istable(L, 1))
		{
			throwError(L, "Error: AnimObject.setName expects first argument to be of type AnimObject.");
		}

		if (!lua_isstring(L, 2))
		{
			throwError(L, "Error: AnimObject.setName expects second argument to be of type string.");
		}

		lua_getfield(L, 1, "id");
		uint64 id = toU64(L, -1);
		lua_pop(L, 1);
		const char* name = lua_tostring(L, 2);

		AnimObject* obj = AnimationManager::getMutableObject(am, id);
		if (obj)
		{
			obj->setName(name);
		}

		return 0;
	}

	int global_setAnimObjPosVec3(lua_State* L)
	{
		// setPositionVec: (self: AnimObject, position : Vec3) -> (),
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, setPosition({x = xPos, y = yPos, z = zPos}), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// AnimObj is first parameter
		lua_getfield(L, 1, "id");
		uint64 id = toU64(L, -1);
		lua_pop(L, 1);

		// Position is arg 2
		Vec3 position = toVec3(L, 2);

		AnimObject* obj = AnimationManager::getMutableObject(am, id);
		if (obj)
		{
			obj->_positionStart = position;
		}

		return 0;
	}

	int global_setAnimObjPosFloats(lua_State* L)
	{
		// setPosition: (self: AnimObject, x: number, y: number, z: number) -> (),
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 3, setPosition(x, y, z), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// AnimObj is first arg
		lua_getfield(L, 1, "id");
		uint64 id = toU64(L, -1);
		lua_pop(L, 1);

		float x = lua_tonumber(L, 2);
		if (!lua_isnumber(L, 2))
		{
			throwError(L, "Expected number as first argument in setPosition(x, y, z). Got something else instead.");
		}

		float y = lua_tonumber(L, 3);
		if (!lua_isnumber(L, 3))
		{
			throwError(L, "Expected number as second argument in setPosition(x, y, z). Got something else instead.");
		}

		float z = lua_tonumber(L, 4);
		if (!lua_isnumber(L, 4))
		{
			throwError(L, "Expected number as third argument in setPosition(x, y, z). Got something else instead.");
		}

		AnimObject* obj = AnimationManager::getMutableObject(am, id);
		if (obj)
		{
			obj->_positionStart = Vec3{ x, y, z };
		}

		return 0;
	}

	int global_require(lua_State* L)
	{
		int nargs = lua_gettop(L);
		argumentCheck(L, 1, 1, require(), nargs);

		if (nargs != 1)
		{
			return 0;
		}

		if (!lua_isstring(L, 1))
		{
			throwError(L, "Expected string as argument to require(someString). Instead got garbage.");
			return 0;
		}

		std::string file = lua_tostring(L, 1);
		if (file == "math-anim")
		{
			return global_loadMathAnimLib(L);
		}

		g_logger_warning("Requiring random files not supported yet for file: %s", file.c_str());
		return 0;
	}

	int global_loadMathAnimLib(lua_State* L)
	{
		lua_createtable(L, 0, 1);

		pushCFunction(L, global_createAnimObjectFn, "math-anim.createAnimObject: (parent: AnimObject) -> AnimObject");
		lua_setfield(L, -2, "createAnimObject");

		return 1;
	}

	static uint64 toU64(lua_State* L, int index)
	{
		uint64 res = 0;
		lua_pushnil(L);
		lua_getfield(L, index - 1, "low");
		uint32 low = lua_tointeger(L, index);
		lua_pop(L, 1);

		lua_getfield(L, index - 1, "high");
		uint64 high = (uint64)lua_tointeger(L, index);
		lua_pop(L, 1);

		res |= low;
		res |= (high << 32);
		return res;
	}

	static void pushU64(lua_State* L, uint64 val)
	{
		uint32 low = (uint32)val;
		uint32 high = (uint32)(val >> 32);
		lua_createtable(L, 0, 2);

		lua_pushinteger(L, low);
		lua_setfield(L, -2, "low");

		lua_pushinteger(L, high);
		lua_setfield(L, -2, "high");
	}

	static Vec3 toVec3(lua_State* L, int index)
	{
		Vec3 res = { NAN, NAN, NAN };

		lua_getfield(L, index, "x");
		if (!lua_isnumber(L, -1))
		{
			throwErrorNoReturn(L, "Vec3.x expected number. Instead got something else.");
			return res;
		}
		res.x = (float)lua_tonumber(L, -1);

		lua_getfield(L, index, "y");
		if (!lua_isnumber(L, -1))
		{
			throwErrorNoReturn(L, "Vec3.y expected number. Instead got something else.");
			return res;
		}
		res.y = (float)lua_tonumber(L, -1);

		lua_getfield(L, index, "z");
		if (!lua_isnumber(L, -1))
		{
			throwErrorNoReturn(L, "Vec3.z expected number. Instead got something else.");
			return res;
		}
		res.z = (float)lua_tonumber(L, -1);

		return res;
	}

	static void pushVec3(lua_State* L, const Vec3& value)
	{
		lua_createtable(L, 0, 3);

		lua_pushnumber(L, value.x);
		lua_setfield(L, -2, "x");

		lua_pushnumber(L, value.y);
		lua_setfield(L, -2, "y");

		lua_pushnumber(L, value.z);
		lua_setfield(L, -2, "z");
	}

	static void pushCFunction(lua_State* L, lua_CFunction fn, const char* debugName)
	{
		lua_pushcfunction(L, fn, debugName);
		cFunctionDebugNames[fn] = debugName;
	}

	static AnimationManagerData* getAnimationManagerData(lua_State* L)
	{
		lua_getglobal(L, "AnimationManagerData");
		AnimationManagerData* am = (AnimationManagerData*)lua_tolightuserdata(L, -1);
		return am;
	}
}

namespace MathAnim
{
	namespace ScriptApi
	{
		void registerGlobalFunctions(lua_State* L, AnimationManagerData* am)
		{
			luaL_openlibs(L);
			pushCFunction(L, global_printWrapper, "print: <T...>(...)");
			lua_setglobal(L, "print");

			lua_pushlightuserdata(L, (void*)am);
			lua_setglobal(L, "AnimationManagerData");

			// Create table to act as a logger
			// Table looks like:
			// logger {
			//   write()
			//   info()
			//   warn()
			//   error()
			// }
			lua_createtable(L, 0, 4);

			pushCFunction(L, global_logWrite, "logger.write: <T...>(...)");
			lua_setfield(L, -2, "write");

			pushCFunction(L, global_logInfo, "logger.info: <T...>(...)");
			lua_setfield(L, -2, "info");

			pushCFunction(L, global_logWarning, "logger.warn: <T...>(...)");
			lua_setfield(L, -2, "warn");

			pushCFunction(L, global_logError, "logger.error: <T...>(...)");
			lua_setfield(L, -2, "error");

			lua_setglobal(L, "logger");

			pushCFunction(L, global_require, "require: (module: string) -> any");
			lua_setglobal(L, "require");

			// TODO: We should do this to sandbox the scripts, but this prevents the scripts
			// from calling global functions
			// luaL_sandbox(L);
		}

		void pushAnimObject(lua_State* L, const AnimObject& obj)
		{
			/*
				id: u64,
				setName: (self: AnimObject, name: string) -> (),
				setPositionVec: (self: AnimObject, position: Vec3) -> (),
				setPosition: (self: AnimObject, x: number, y: number, z: number) -> (),
				svgObject: SvgObject
			*/
			lua_createtable(L, 0, 5);

			pushU64(L, obj.id);
			lua_setfield(L, -2, "id");

			pushCFunction(L, global_setAnimObjName, "AnimObject.setName: (self: AnimObject, name: string) -> ()");
			lua_setfield(L, -2, "setName");

			pushCFunction(L, global_setAnimObjPosVec3, "AnimObject.setPositionVec: (self: AnimObject, position: Vec3) -> ()");
			lua_setfield(L, -2, "setPositionVec");

			pushCFunction(L, global_setAnimObjPosFloats, "AnimObject.setPosition: (self: AnimObject, x: number, y: number, z: number) -> ()");
			lua_setfield(L, -2, "setPosition");

			lua_pushlightuserdata(L, nullptr);
			lua_setfield(L, -2, "svgObject");
		}
	}
}