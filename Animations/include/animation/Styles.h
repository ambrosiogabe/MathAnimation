#ifndef MATH_ANIM_STYLES_H
#define MATH_ANIM_STYLES_H
#include "core.h"

namespace MathAnim
{
	struct Style
	{
		glm::vec3 color;
		float strokeWidth;
	};

	namespace Colors
	{
		extern glm::vec4 greenBrown;
		extern glm::vec4 offWhite;
		extern glm::vec4 darkGray;
		extern glm::vec4 red;
		extern glm::vec4 orange;
		extern glm::vec4 lightOrange;
		extern glm::vec4 yellow;
		extern glm::vec4 green;
		extern glm::vec4 blue;
		extern glm::vec4 purple;
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
