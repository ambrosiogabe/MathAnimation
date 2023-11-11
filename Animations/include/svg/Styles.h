#ifndef MATH_ANIM_STYLES_H
#define MATH_ANIM_STYLES_H
#include "core.h"

#include <cppUtils/cppPrint.hpp>
#include <cppUtils/cppUtils.hpp>

namespace MathAnim
{
	enum class CssStyleType : uint8
	{
		Error = 0,
		Value,
		Inherit
	};

	enum class CssFontStyle : uint8
	{
		None = 0,
		Normal,
		Italic,
		Bold,
		Inherit,
		Length
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

		CssFontStyle fontStyleFromString(std::string const& str);
	}
}

// Print functions
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::CssFontStyle& style);
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::CssStyleType& style);

#endif 