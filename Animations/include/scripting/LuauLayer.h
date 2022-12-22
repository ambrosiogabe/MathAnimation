#ifndef MATH_ANIM_LUAU_LAYER_H
#define MATH_ANIM_LUAU_LAYER_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;

	namespace LuauLayer
	{
		void init(const std::string& scriptDirectory, AnimationManagerData* am);

		void update();

		bool compile(const std::string& filename);
		bool compile(const std::string& sourceCode, const std::string& scriptName);
		const std::string& getCurrentExecutingScriptFilepath();

		bool execute(const std::string& scriptName);
		bool executeOnAnimObj(const std::string& scriptName, const std::string& functionName, AnimationManagerData* am, AnimObjId obj);

		bool remove(const std::string& scriptName);

		void free();
	}
}

#endif 