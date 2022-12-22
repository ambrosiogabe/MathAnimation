#ifndef MATH_ANIM_GLOBAL_API_H
#define MATH_ANIM_GLOBAL_API_H

struct lua_State;

extern "C"
{
	int global_printStackVar(lua_State* L, int index);
	int global_printWrapper(lua_State* L);
	int global_logWrite(lua_State* L);
	int global_logInfo(lua_State* L);
	int global_logWarning(lua_State* L);
	int global_logError(lua_State* L);
	int global_createAnimObjectFn(lua_State* L);

	int global_setAnimObjName(lua_State* L);
	int global_setAnimObjPosVec3(lua_State* L);
	int global_setAnimObjPosFloats(lua_State* L);
	
	// Exported libraries/shared library support
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
	}
}

#endif