#ifndef MATH_ANIM_PARSERS_COMMON_H
#define MATH_ANIM_PARSERS_COMMON_H
#include "core.h"

namespace MathAnim
{
	struct ParserInfo
	{
		const char* text;
		size_t textLength;
		size_t cursor;
	};

	struct ScopedName
	{
		std::vector<std::string> dotSeparatedScopes;
		std::string friendlyName;

		bool contains(const ScopedName& other, int* levelMatched) const;

		static ScopedName from(const std::string& str);
	};

	namespace Parsers
	{

	}
}

#endif 