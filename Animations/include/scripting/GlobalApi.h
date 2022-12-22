#ifndef MATH_ANIM_GLOBAL_API_H
#define MATH_ANIM_GLOBAL_API_H

struct lua_State;

extern "C"
{
	// ------- Logging Utilities -------
	int global_printStackVar(lua_State* L, int index);
	int global_printWrapper(lua_State* L);
	int global_logWrite(lua_State* L);
	int global_logInfo(lua_State* L);
	int global_logWarning(lua_State* L);
	int global_logError(lua_State* L);

	// ------- Anim Objects -------
	int global_createAnimObjectFn(lua_State* L);
	int global_setAnimObjName(lua_State* L);
	int global_setAnimObjPosVec3(lua_State* L);
	int global_setAnimObjPosFloats(lua_State* L);
	int global_setAnimObjColor(lua_State* L);

	// ------- Svg Objects -------
	int global_svgBeginPath(lua_State* L);
	int global_svgClosePath(lua_State* L);
	int global_svgSetPathAsHole(lua_State* L);

	// Absolute Commands
	int global_svgMoveTo(lua_State* L);
	int global_svgLineTo(lua_State* L);
	int global_svgVtLineTo(lua_State* L);
	int global_svgHzLineTo(lua_State* L);
	int global_svgQuadTo(lua_State* L);
	int global_svgCubicTo(lua_State* L);
	int global_svgArcTo(lua_State* L);

	// Relative Commands
	int global_svgMoveToRel(lua_State* L);
	int global_svgLineToRel(lua_State* L);
	int global_svgVtLineToRel(lua_State* L);
	int global_svgHzLineToRel(lua_State* L);
	int global_svgQuadToRel(lua_State* L);
	int global_svgCubicToRel(lua_State* L);
	int global_svgArcToRel(lua_State* L);
	
	// ------- Exported libraries/shared library support -------
	int global_require(lua_State* L);
	int global_loadMathAnimLib(lua_State* L);
}

namespace MathAnim
{
	struct AnimationManagerData;
	struct AnimObject;

	namespace ScriptApi
	{
		void registerGlobalFunctions(lua_State* luaState, AnimationManagerData* am);

		void pushAnimObject(lua_State* L, const AnimObject& obj);
		void pushNewSvgObject(lua_State* L, const AnimObject& obj);
	}
}

#endif