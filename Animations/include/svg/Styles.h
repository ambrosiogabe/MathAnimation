#ifndef MATH_ANIM_STYLES_H
#define MATH_ANIM_STYLES_H
#include "core.h"

namespace MathAnim
{
	enum class CssStyleType : uint8
	{
		Error = 0,
		Value,
		Inherit
	};

	struct CssColor
	{
		Vec4 color;
		CssStyleType styleType;
	};

	namespace Css
	{
		CssColor colorFromString(const char* cssColorStr, size_t strLength = 0);
		inline CssColor colorFromString(const std::string& cssColorStr) { return colorFromString(cssColorStr.c_str(), cssColorStr.length()); }
	}
}

#endif 