#ifndef MATH_ANIM_PLATFORM_H
#define MATH_ANIM_PLATFORM_H
#include "core.h"

// TODO: The real platform is in platform/Platform.h
// move this function over there
namespace MathAnim
{
	namespace Platform
	{
		const std::vector<std::string>& getAvailableFonts();
	}
}

#endif 