#ifndef MATH_ANIM_SVG_PARSER_H
#define MATH_ANIM_SVG_PARSER_H
#include "core.h"

namespace MathAnim
{
	struct SvgObject;
	struct SvgGroup;

	namespace SvgParser
	{
		void init();

		SvgGroup* parseSvgDoc(const char* filepath);

		bool parseSvgPath(const char* pathText, size_t pathTextLength, SvgObject* output);

		const char* getLastError();
	}
}

#endif 