#ifndef MATH_ANIM_CORE_COLORS_H
#define MATH_ANIM_CORE_COLORS_H
#include "core.h"

namespace MathAnim
{
	enum class Color : uint8
	{
		White,
		GreenBrown,
		OffWhite,
		DarkGray,
		Red,
		Orange,
		LightOrange,
		Yellow,
		Green,
		Blue,
		Purple,
		Length
	};

	extern Vec4 colors[(uint8)Color::Length];
}

#endif 