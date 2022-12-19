#ifndef MATH_ANIM_GLOBAL_API_H
#define MATH_ANIM_GLOBAL_API_H

struct lua_State;

extern "C"
{
	int global_printWrapper(lua_State* L);
}

namespace MathAnim
{
	namespace ScriptApi
	{
		void registerGlobalFunctions(lua_State* luaState);
	}
}

#endif