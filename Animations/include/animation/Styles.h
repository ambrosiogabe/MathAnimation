#ifndef MATH_ANIM_STYLES_H
#define MATH_ANIM_STYLES_H
#include "core.h"

namespace MathAnim
{
	enum class CapType
	{
		Flat,
		Arrow
	};

	struct Style
	{
		Vec4 color;
		float strokeWidth;
		CapType lineEnding;
	};

	namespace Colors
	{
		extern Vec4 greenBrown;
		extern Vec4 offWhite;
		extern Vec4 darkGray;
		extern Vec4 red;
		extern Vec4 orange;
		extern Vec4 lightOrange;
		extern Vec4 yellow;
		extern Vec4 green;
		extern Vec4 blue;
		extern Vec4 purple;
	}

	namespace Styles
	{
		extern Style defaultStyle;
		extern Style gridStyle;
		extern Style verticalAxisStyle;
		extern Style horizontalAxisStyle;
	}
}

#endif
