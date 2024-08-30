#ifndef MATH_ANIM_SCRIPT_GLOBALS_H
#define MATH_ANIM_SCRIPT_GLOBALS_H
#include "core.h"

namespace MathAnim
{
	namespace MathAnimGlobals
	{
		std::string_view getAnimGuiModule();
		std::string_view getAnimCoreModule();
		std::string_view getAnimMathModule();
		std::string_view getBuiltinDefinitionSource();
	}
}

#endif 