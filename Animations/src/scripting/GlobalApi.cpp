#include "core.h"
#include "scripting/GlobalApi.h"
#include "scripting/LuauLayer.h"
#include "editor/ConsoleLog.h"
#include "editor/SceneHierarchyPanel.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "svg/Svg.h"

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
	static Vec4 toVec4(lua_State* L, int index);
	static void pushVec4(lua_State* L, const Vec4& value);
	static Vec3 toVec3(lua_State* L, int index);
	static void pushVec3(lua_State* L, const Vec3& value);
	static Vec2 toVec2(lua_State* L, int index);
	static void pushVec2(lua_State* L, const Vec2& value);
	static void pushCFunction(lua_State* L, lua_CFunction fn, const char* debugName);
	static AnimationManagerData* getAnimationManagerData(lua_State* L);
	static SvgObject* checkIfSvgIsNull(lua_State* L, int index);

	// Print helpers
	static std::string getAsString(lua_State* L, int index = 1);
	static void luaPrintTable(lua_State* L, char* buffer, size_t bufferSize, int index, int tabDepth = 1, int maxTabDepth = 5);
	static std::string getAsString(lua_State* L, int index);
	static int luaPrintItem(lua_State* L, g_logger_level logLevel, const std::string& scriptFilepath, const lua_Debug& debugInfo, int index = 1);
	static int luaPrint(lua_State* L, g_logger_level logLevel);

	int global_printStackVar(lua_State* L, int index)
	{
		luaPrintItem(L, g_logger_level_Info, "Null", {}, index);

		return 0;
	}

	// ------- Logging Utilities -------
	int global_printWrapper(lua_State* L)
	{
		return luaPrint(L, g_logger_level_Log);
	}

	int global_logWrite(lua_State* L)
	{
		return luaPrint(L, g_logger_level_Log);
	}

	int global_logInfo(lua_State* L)
	{
		return luaPrint(L, g_logger_level_Info);
	}

	int global_logWarning(lua_State* L)
	{
		return luaPrint(L, g_logger_level_Warning);
	}

	int global_logError(lua_State* L)
	{
		return luaPrint(L, g_logger_level_Error);
	}

	// ------- Anim Objects -------
	int global_createAnimObjectFn(lua_State* L)
	{
		int nargs = lua_gettop(L);
		argumentCheck(L, 1, 1, "createAnimObject(AnimObject)", nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// First parameter is AnimObject
		lua_getfield(L, 1, "id");
		AnimObjId id = toU64(L, -1);
		lua_pop(L, 1);

		AnimObject newObject = AnimObject::createDefaultFromParent(am, AnimObjectTypeV1::SvgObject, id, true);
		newObject._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		newObject.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		(*newObject._svgObjectStart) = Svg::createDefault();
		(*newObject.svgObject) = Svg::createDefault();
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
		argumentCheckWithSelf(L, 1, setPosition({ x = xPos, y = yPos, z = zPos }), nargs);

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
			obj->position = obj->_positionStart;
		}

		return 0;
	}

	int global_setAnimObjColor(lua_State* L)
	{
		// setColor: (self: AnimObject, color: Vec4) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, setColor: (self: AnimObject, color : Vec4), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// AnimObj is first parameter
		lua_getfield(L, 1, "id");
		uint64 id = toU64(L, -1);
		lua_pop(L, 1);

		// Position is arg 2
		Vec4 color = toVec4(L, 2);

		AnimObject* obj = AnimationManager::getMutableObject(am, id);
		if (obj)
		{
			obj->_fillColorStart = glm::u8vec4(
				(uint8)(color.r),
				(uint8)(color.g),
				(uint8)(color.b),
				(uint8)(color.a)
			);
			obj->fillColor = obj->_fillColorStart;
		}

		return 0;
	}

	// ------- Svg Objects -------
	int global_svgBeginPath(lua_State* L)
	{
		// beginPath: (startPosition: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, beginPath: (startPosition: Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		if (!lua_islightuserdata(L, -1))
		{
			throwError(L, "SvgObject.ptr is not a pointer. Somehow the SvgObject got malformed.");
		}
		SvgObject* svgPtr = (SvgObject*)lua_tolightuserdata(L, -1);
		if (!svgPtr)
		{
			// Get the animObjId and assign it's startSvgObject to this pointer now
			lua_getfield(L, 1, "objId");
			AnimObjId objId = toU64(L, -1);
			AnimObject* obj = AnimationManager::getMutableObject(am, objId);
			if (obj)
			{
				obj->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
				(*obj->_svgObjectStart) = Svg::createDefault();
				obj->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
				(*obj->svgObject) = Svg::createDefault();
			}
			else
			{
				throwError(L, "SvgObject had an invalid AnimObject. Somehow this object didn't get constructured properly.");
			}

			svgPtr = obj->_svgObjectStart;
			lua_pushlightuserdata(L, (void*)svgPtr);
			lua_setfield(L, 1, "ptr");
		}

		// Second argument is vec2
		Vec2 startPos = toVec2(L, 2);
		Svg::beginPath(svgPtr, startPos);

		return 0;
	}

	int global_svgClosePath(lua_State* L)
	{
		// closePath: (connectLastPoint: boolean) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, closePath: (connectLastPoint: boolean), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		if (!lua_isboolean(L, 2))
		{
			throwError(L, "SvgObject.closePath(connectLastPoint: boolean) expects a boolean as the first arg. Got something else instead.");
		}
		bool connectToFirstPoint = lua_toboolean(L, 2);

		Svg::closePath(svgPtr, connectToFirstPoint);
	}

	int global_svgSetPathAsHole(lua_State* L)
	{
		// setPathAsHole: () -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 0, setPathAsHole: (), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		if (svgPtr->numPaths > 0)
		{
			svgPtr->paths[svgPtr->numPaths - 1].isHole = true;
		}

		return 0;
	}

	// Absolute Commands
	int global_svgMoveTo(lua_State* L)
	{
		// moveTo: (position: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, moveTo: (connectLastPoint: boolean), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// Position is arg 2
		Vec2 pos = toVec2(L, 2);
		Svg::moveTo(svgPtr, pos);

		return 0;
	}

	int global_svgLineTo(lua_State* L)
	{
		// lineTo: (p0: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, lineTo: (p0: Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// p0 is arg 2
		Vec2 p0 = toVec2(L, 2);
		Svg::lineTo(svgPtr, p0);

		return 0;
	}

	int global_svgVtLineTo(lua_State* L)
	{
		// vtLineTo: (y0: number) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, vtLineTo: (y0: number), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// y0 is arg 2
		if (!lua_isnumber(L, 2))
		{
			throwError(L, "Expected number as first argument in SvgObject.vtLineTo(y0: number)");
		}
		float y0 = lua_tonumber(L, 2);
		Svg::vtLineTo(svgPtr, y0);

		return 0;
	}

	int global_svgHzLineTo(lua_State* L)
	{
		// hzLineTo: (x0: number) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, hzLineTo: (x0: number), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// x0 is arg 2
		if (!lua_isnumber(L, 2))
		{
			throwError(L, "Expected number as first argument in SvgObject.hzLineTo(x0: number)");
		}
		float x0 = lua_tonumber(L, 2);
		Svg::hzLineTo(svgPtr, x0);

		return 0;
	}

	int global_svgQuadTo(lua_State* L)
	{
		// quadTo: (p0: Vec2, p1: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 2, quadTo: (p0: Vec2, p1 : Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// p0 is arg 2
		Vec2 p0 = toVec2(L, 2);

		// p1 is arg 3
		Vec2 p1 = toVec2(L, 3);

		Svg::bezier2To(svgPtr, p0, p1);

		return 0;
	}

	int global_svgCubicTo(lua_State* L)
	{
		// cubicTo: (p0: Vec2, p1: Vec2, p2: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 3, cubicTo: (p0: Vec2, p1 : Vec2, p2 : Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// p0 is arg 2
		Vec2 p0 = toVec2(L, 2);

		// p1 is arg 3
		Vec2 p1 = toVec2(L, 3);

		// p2 is arg 4
		Vec2 p2 = toVec2(L, 4);

		Svg::bezier3To(svgPtr, p0, p1, p2);

		return 0;
	}

	int global_svgArcTo(lua_State* L)
	{
		// arcTo: (radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 5, arcTo: (radius: Vec2, xAxisRot : number, largeArcFlag : boolean, sweepFlag : boolean, p0 : Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// radius is arg 2
		Vec2 radius = toVec2(L, 2);

		// xAxisRot is arg3
		if (!lua_isnumber(L, 3))
		{
			throwError(L, "Expected number as argument 3 in Svg.arcTo");
		}
		float xAxisRot = (float)lua_tonumber(L, 3);

		// largeArcFlag is arg4
		if (!lua_isboolean(L, 4))
		{
			throwError(L, "Expected boolean as argument 4 in Svg.arcTo");
		}
		bool largeArcFlag = lua_toboolean(L, 4);

		// sweepFlag is arg5
		if (!lua_isboolean(L, 5))
		{
			throwError(L, "Expected boolean as argument 5 in Svg.arcTo");
		}
		bool sweepFlag = lua_toboolean(L, 5);

		// p0 is arg6
		Vec2 p0 = toVec2(L, 6);

		Svg::arcTo(svgPtr, radius, xAxisRot, largeArcFlag, sweepFlag, p0);

		return 0;
	}

	// Relative Commands
	int global_svgMoveToRel(lua_State* L)
	{
		// moveTo: (position: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, moveTo: (connectLastPoint: boolean), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// Position is arg 2
		Vec2 pos = toVec2(L, 2);
		Svg::moveTo(svgPtr, pos, false);

		return 0;
	}

	int global_svgLineToRel(lua_State* L)
	{
		// lineTo: (p0: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, lineTo: (p0: Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// p0 is arg 2
		Vec2 p0 = toVec2(L, 2);
		Svg::lineTo(svgPtr, p0, false);

		return 0;
	}

	int global_svgVtLineToRel(lua_State* L)
	{
		// vtLineTo: (y0: number) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, vtLineTo: (y0: number), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// y0 is arg 2
		if (!lua_isnumber(L, 2))
		{
			throwError(L, "Expected number as first argument in SvgObject.vtLineTo(y0: number)");
		}
		float y0 = lua_tonumber(L, 2);
		Svg::vtLineTo(svgPtr, y0, false);

		return 0;
	}

	int global_svgHzLineToRel(lua_State* L)
	{
		// hzLineTo: (x0: number) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 1, hzLineTo: (x0: number), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// x0 is arg 2
		if (!lua_isnumber(L, 2))
		{
			throwError(L, "Expected number as first argument in SvgObject.hzLineTo(x0: number)");
		}
		float x0 = lua_tonumber(L, 2);
		Svg::hzLineTo(svgPtr, x0, false);

		return 0;
	}

	int global_svgQuadToRel(lua_State* L)
	{
		// quadTo: (p0: Vec2, p1: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 2, quadTo: (p0: Vec2, p1 : Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// p0 is arg 2
		Vec2 p0 = toVec2(L, 2);

		// p1 is arg 3
		Vec2 p1 = toVec2(L, 3);

		Svg::bezier2To(svgPtr, p0, p1, false);

		return 0;
	}

	int global_svgCubicToRel(lua_State* L)
	{
		// cubicTo: (p0: Vec2, p1: Vec2, p2: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 3, cubicTo: (p0: Vec2, p1 : Vec2, p2 : Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// p0 is arg 2
		Vec2 p0 = toVec2(L, 2);

		// p1 is arg 3
		Vec2 p1 = toVec2(L, 3);

		// p2 is arg 4
		Vec2 p2 = toVec2(L, 4);

		Svg::bezier3To(svgPtr, p0, p1, p2, false);

		return 0;
	}

	int global_svgArcToRel(lua_State* L)
	{
		// arcTo: (radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> ()
		int nargs = lua_gettop(L);
		argumentCheckWithSelf(L, 5, arcTo: (radius: Vec2, xAxisRot : number, largeArcFlag : boolean, sweepFlag : boolean, p0 : Vec2), nargs);

		AnimationManagerData* am = getAnimationManagerData(L);

		// SvgObject is first arg
		lua_getfield(L, 1, "ptr");
		SvgObject* svgPtr = checkIfSvgIsNull(L, -1);
		if (!svgPtr)
		{
			return -1;
		}

		// radius is arg 2
		Vec2 radius = toVec2(L, 2);

		// xAxisRot is arg3
		if (!lua_isnumber(L, 3))
		{
			throwError(L, "Expected number as argument 3 in Svg.arcTo");
		}
		float xAxisRot = (float)lua_tonumber(L, 3);

		// largeArcFlag is arg4
		if (!lua_isboolean(L, 4))
		{
			throwError(L, "Expected boolean as argument 4 in Svg.arcTo");
		}
		bool largeArcFlag = lua_toboolean(L, 4);

		// sweepFlag is arg5
		if (!lua_isboolean(L, 5))
		{
			throwError(L, "Expected boolean as argument 5 in Svg.arcTo");
		}
		bool sweepFlag = lua_toboolean(L, 5);

		// p0 is arg6
		Vec2 p0 = toVec2(L, 6);

		Svg::arcTo(svgPtr, radius, xAxisRot, largeArcFlag, sweepFlag, p0, false);

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

	// --------------- Internal Functions ---------------
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

	static Vec4 toVec4(lua_State* L, int index)
	{
		Vec4 res = { NAN, NAN, NAN, NAN };

		// Vec4 could be x, y, z, w or r, g, b, a so try both
		lua_getfield(L, index, "x");
		if (!lua_isnumber(L, -1))
		{
			lua_pop(L, 1);
			lua_getfield(L, index, "r");
			if (!lua_isnumber(L, -1))
			{
				throwErrorNoReturn(L, "Vec4.x expected number. Instead got something else.");
				return res;
			}
		}
		res.x = (float)lua_tonumber(L, -1);

		lua_getfield(L, index, "y");
		if (!lua_isnumber(L, -1))
		{
			lua_pop(L, 1);
			lua_getfield(L, index, "g");
			if (!lua_isnumber(L, -1))
			{
				throwErrorNoReturn(L, "Vec4.y expected number. Instead got something else.");
				return res;
			}
		}
		res.y = (float)lua_tonumber(L, -1);

		lua_getfield(L, index, "z");
		if (!lua_isnumber(L, -1))
		{
			lua_pop(L, 1);
			lua_getfield(L, index, "b");
			if (!lua_isnumber(L, -1))
			{
				throwErrorNoReturn(L, "Vec4.z expected number. Instead got something else.");
				return res;
			}
		}
		res.z = (float)lua_tonumber(L, -1);

		lua_getfield(L, index, "w");
		if (!lua_isnumber(L, -1))
		{
			lua_pop(L, 1);
			lua_getfield(L, index, "a");
			if (!lua_isnumber(L, -1))
			{
				throwErrorNoReturn(L, "Vec.w expected number. Instead got something else.");
				return res;
			}
		}
		res.w = (float)lua_tonumber(L, -1);

		return res;
	}

	static void pushVec4(lua_State* L, const Vec4& value)
	{
		lua_createtable(L, 0, 4);

		lua_pushnumber(L, value.x);
		lua_setfield(L, -2, "x");

		lua_pushnumber(L, value.y);
		lua_setfield(L, -2, "y");

		lua_pushnumber(L, value.z);
		lua_setfield(L, -2, "z");

		lua_pushnumber(L, value.w);
		lua_setfield(L, -2, "w");
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

	static Vec2 toVec2(lua_State* L, int index)
	{
		Vec2 res = { NAN, NAN };

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

		return res;
	}

	static void pushVec2(lua_State* L, const Vec2& value)
	{
		lua_createtable(L, 0, 3);

		lua_pushnumber(L, value.x);
		lua_setfield(L, -2, "x");

		lua_pushnumber(L, value.y);
		lua_setfield(L, -2, "y");
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

	static SvgObject* checkIfSvgIsNull(lua_State* L, int index)
	{
		if (!lua_islightuserdata(L, -1))
		{
			throwErrorNoReturn(L, "SvgObject.ptr is not a pointer. Somehow the SvgObject got malformed.");
			return nullptr;
		}

		SvgObject* svgPtr = (SvgObject*)lua_tolightuserdata(L, -1);
		if (!svgPtr)
		{
			throwErrorNoReturn(L, "SvgObject.ptr is nullptr. Did you forget to call beginPath()?");
			return nullptr;
		}

		return svgPtr;
	}

	// Print helpers
	static void luaPrintTable(lua_State* L, char* buffer, size_t bufferSize, int index, int tabDepth, int maxTabDepth)
	{
		if (tabDepth >= maxTabDepth)
		{
			return;
		}

		// 10KB should be enough right?
		constexpr size_t recursiveBufferSize = 1024 * 10;
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
			// 10KB should be enough for a table...?
			constexpr size_t tableBufferSize = 1024 * 10;
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

	static int luaPrintItem(lua_State* L, g_logger_level logLevel, const std::string& scriptFilepath, const lua_Debug& debugInfo, int index)
	{
		std::string str = getAsString(L);
		if (logLevel == g_logger_level_Log)
		{
			ConsoleLog::log(scriptFilepath.c_str(), debugInfo.currentline, "%s", str.c_str());
		}
		else if (logLevel == g_logger_level_Info)
		{
			ConsoleLog::info(scriptFilepath.c_str(), debugInfo.currentline, "%s", str.c_str());
		}
		else if (logLevel == g_logger_level_Warning)
		{
			ConsoleLog::warning(scriptFilepath.c_str(), debugInfo.currentline, "%s", str.c_str());
		}
		else if (logLevel == g_logger_level_Error)
		{
			ConsoleLog::error(scriptFilepath.c_str(), debugInfo.currentline, "%s", str.c_str());
		}

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
			lua_createtable(L, 0, 5);

			pushU64(L, obj.id);
			lua_setfield(L, -2, "id");

			pushCFunction(L, global_setAnimObjName, "AnimObject.setName: (self: AnimObject, name: string) -> ()");
			lua_setfield(L, -2, "setName");

			pushCFunction(L, global_setAnimObjPosVec3, "AnimObject.setPositionVec: (self: AnimObject, position: Vec3) -> ()");
			lua_setfield(L, -2, "setPositionVec");

			pushCFunction(L, global_setAnimObjPosFloats, "AnimObject.setPosition: (self: AnimObject, x: number, y: number, z: number) -> ()");
			lua_setfield(L, -2, "setPosition");

			pushCFunction(L, global_setAnimObjColor, " setColor: (self: AnimObject, color: Vec4) -> ()");
			lua_setfield(L, -2, "setColor");

			pushNewSvgObject(L, obj);
			lua_setfield(L, -2, "svgObject");
		}

		void pushNewSvgObject(lua_State* L, const AnimObject& obj)
		{
			lua_createtable(L, 0, 18);

			lua_pushlightuserdata(L, obj._svgObjectStart);
			lua_setfield(L, -2, "ptr");

			pushU64(L, obj.id);
			lua_setfield(L, -2, "objId");

			pushCFunction(L, global_svgBeginPath, "Svg.beginPath: (startPosition: Vec2) -> ()");
			lua_setfield(L, -2, "beginPath");

			pushCFunction(L, global_svgClosePath, "closePath: (connectLastPoint: boolean) -> ()");
			lua_setfield(L, -2, "closePath");

			pushCFunction(L, global_svgSetPathAsHole, "setPathAsHole: () -> ()");
			lua_setfield(L, -2, "setPathAsHole");

			// Absolute svg commands
			pushCFunction(L, global_svgMoveTo, "moveTo: (position: Vec2) -> ()");
			lua_setfield(L, -2, "moveTo");

			pushCFunction(L, global_svgLineTo, "lineTo: (p0: Vec2) -> ()");
			lua_setfield(L, -2, "lineTo");

			pushCFunction(L, global_svgVtLineTo, "vtLineTo: (y0: number) -> ()");
			lua_setfield(L, -2, "vtLineTo");

			pushCFunction(L, global_svgHzLineTo, "hzLineTo: (x0: number) -> ()");
			lua_setfield(L, -2, "hzLineTo");

			pushCFunction(L, global_svgQuadTo, "quadTo: (p0: Vec2, p1: Vec2) -> ()");
			lua_setfield(L, -2, "quadTo");

			pushCFunction(L, global_svgCubicTo, "cubicTo: (p0: Vec2, p1: Vec2, p2: Vec2) -> ()");
			lua_setfield(L, -2, "cubicTo");

			pushCFunction(L, global_svgArcTo, "arcTo: (radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> ()");
			lua_setfield(L, -2, "arcTo");

			// Relative svg commands
			pushCFunction(L, global_svgMoveToRel, "moveToRel: (position: Vec2) -> ()");
			lua_setfield(L, -2, "moveToRel");

			pushCFunction(L, global_svgLineToRel, "lineToRel: (p0: Vec2) -> ()");
			lua_setfield(L, -2, "lineToRel");

			pushCFunction(L, global_svgVtLineToRel, "vtLineToRel: (y0: number) -> ()");
			lua_setfield(L, -2, "vtLineToRel");

			pushCFunction(L, global_svgHzLineToRel, "hzLineToRel: (x0: number) -> ()");
			lua_setfield(L, -2, "hzLineToRel");

			pushCFunction(L, global_svgQuadToRel, "quadToRel: (p0: Vec2, p1: Vec2) -> ()");
			lua_setfield(L, -2, "quadToRel");

			pushCFunction(L, global_svgCubicToRel, "cubicToRel: (p0: Vec2, p1: Vec2, p2: Vec2) -> ()");
			lua_setfield(L, -2, "cubicToRel");

			pushCFunction(L, global_svgArcToRel, "arcToRel: (radius: Vec2, xAxisRot: number, largeArcFlag: boolean, sweepFlag: boolean, p0: Vec2) -> ()");
			lua_setfield(L, -2, "arcToRel");
		}
	}
}