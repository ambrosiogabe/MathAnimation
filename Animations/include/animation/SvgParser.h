#ifndef MATH_ANIM_SVG_PARSER_H
#define MATH_ANIM_SVG_PARSER_H
#include "core.h"

namespace MathAnim
{
	struct SvgObject;

	namespace SvgParser
	{
		SvgObject* parseSvgDoc(const char* filepath);

		SvgObject* parseSvgPath(const char* pathText, size_t pathTextLength, const Vec4& viewBox);
	}
}

#endif 