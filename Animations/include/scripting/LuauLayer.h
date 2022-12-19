#ifndef MATH_ANIM_LUAU_LAYER_H
#define MATH_ANIM_LUAU_LAYER_H

namespace MathAnim
{
	namespace LuauLayer
	{
		void init(const std::string& scriptDirectory);

		void update();

		bool compile(const std::string& filename);
		bool compile(const std::string& sourceCode, const std::string& scriptName);

		bool execute(const std::string& scriptName);

		void free();
	}
}

#endif 