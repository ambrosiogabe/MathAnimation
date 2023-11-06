#include "svg/Styles.h"

static const std::unordered_map<std::string, MathAnim::Vec4> cssColors = {
			{"aliceblue", "#f0f8ff"_hex},
			{"antiquewhite", "#faebd7"_hex},
			{"aqua", "#00ffff"_hex},
			{"aquamarine", "#7fffd4"_hex},
			{"azure", "#f0ffff"_hex},
			{"beige", "#f5f5dc"_hex},
			{"bisque", "#ffe4c4"_hex},
			{"black", "#000000"_hex},
			{"blanchedalmond", "#ffebcd"_hex},
			{"blue", "#0000ff"_hex},
			{"blueviolet", "#8a2be2"_hex},
			{"brown", "#a52a2a"_hex},
			{"burlywood", "#deb887"_hex},
			{"cadetblue", "#5f9ea0"_hex},
			{"chartreuse", "#7fff00"_hex},
			{"chocolate", "#d2691e"_hex},
			{"coral", "#ff7f50"_hex},
			{"cornflowerblue", "#6495ed"_hex},
			{"cornsilk", "#fff8dc"_hex},
			{"crimson", "#dc143c"_hex},
			{"cyan", "#00ffff"_hex},
			{"darkblue", "#00008b"_hex},
			{"darkcyan", "#008b8b"_hex},
			{"darkgoldenrod", "#b8860b"_hex},
			{"darkgray", "#a9a9a9"_hex},
			{"darkgreen", "#006400"_hex},
			{"darkgrey", "#a9a9a9"_hex},
			{"darkkhaki", "#bdb76b"_hex},
			{"darkmagenta", "#8b008b"_hex},
			{"darkolivegreen", "#556b2f"_hex},
			{"darkorange", "#ff8c00"_hex},
			{"darkorchid", "#9932cc"_hex},
			{"darkred", "#8b0000"_hex},
			{"darksalmon", "#e9967a"_hex},
			{"darkseagreen", "#8fbc8f"_hex},
			{"darkslateblue", "#483d8b"_hex},
			{"darkslategray", "#2f4f4f"_hex},
			{"darkslategrey", "#2f4f4f"_hex},
			{"darkturquoise", "#00ced1"_hex},
			{"darkviolet", "#9400d3"_hex},
			{"deeppink", "#ff1493"_hex},
			{"deepskyblue", "#00bfff"_hex},
			{"dimgray", "#696969"_hex},
			{"dimgrey", "#696969"_hex},
			{"dodgerblue", "#1e90ff"_hex},
			{"firebrick", "#b22222"_hex},
			{"floralwhite", "#fffaf0"_hex},
			{"forestgreen", "#228b22"_hex},
			{"fuchsia", "#ff00ff"_hex},
			{"gainsboro", "#dcdcdc"_hex},
			{"ghostwhite", "#f8f8ff"_hex},
			{"goldenrod", "#daa520"_hex},
			{"gold", "#ffd700"_hex},
			{"gray", "#808080"_hex},
			{"green", "#008000"_hex},
			{"greenyellow", "#adff2f"_hex},
			{"grey", "#808080"_hex},
			{"honeydew", "#f0fff0"_hex},
			{"hotpink", "#ff69b4"_hex},
			{"indianred", "#cd5c5c"_hex},
			{"indigo", "#4b0082"_hex},
			{"ivory", "#fffff0"_hex},
			{"khaki", "#f0e68c"_hex},
			{"lavenderblush", "#fff0f5"_hex},
			{"lavender", "#e6e6fa"_hex},
			{"lawngreen", "#7cfc00"_hex},
			{"lemonchiffon", "#fffacd"_hex},
			{"lightblue", "#add8e6"_hex},
			{"lightcoral", "#f08080"_hex},
			{"lightcyan", "#e0ffff"_hex},
			{"lightgoldenrodyellow", "#fafad2"_hex},
			{"lightgray", "#d3d3d3"_hex},
			{"lightgreen", "#90ee90"_hex},
			{"lightgrey", "#d3d3d3"_hex},
			{"lightpink", "#ffb6c1"_hex},
			{"lightsalmon", "#ffa07a"_hex},
			{"lightseagreen", "#20b2aa"_hex},
			{"lightskyblue", "#87cefa"_hex},
			{"lightslategray", "#778899"_hex},
			{"lightslategrey", "#778899"_hex},
			{"lightsteelblue", "#b0c4de"_hex},
			{"lightyellow", "#ffffe0"_hex},
			{"lime", "#00ff00"_hex},
			{"limegreen", "#32cd32"_hex},
			{"linen", "#faf0e6"_hex},
			{"magenta", "#ff00ff"_hex},
			{"maroon", "#800000"_hex},
			{"mediumaquamarine", "#66cdaa"_hex},
			{"mediumblue", "#0000cd"_hex},
			{"mediumorchid", "#ba55d3"_hex},
			{"mediumpurple", "#9370db"_hex},
			{"mediumseagreen", "#3cb371"_hex},
			{"mediumslateblue", "#7b68ee"_hex},
			{"mediumspringgreen", "#00fa9a"_hex},
			{"mediumturquoise", "#48d1cc"_hex},
			{"mediumvioletred", "#c71585"_hex},
			{"midnightblue", "#191970"_hex},
			{"mintcream", "#f5fffa"_hex},
			{"mistyrose", "#ffe4e1"_hex},
			{"moccasin", "#ffe4b5"_hex},
			{"navajowhite", "#ffdead"_hex},
			{"navy", "#000080"_hex},
			{"oldlace", "#fdf5e6"_hex},
			{"olive", "#808000"_hex},
			{"olivedrab", "#6b8e23"_hex},
			{"orange", "#ffa500"_hex},
			{"orangered", "#ff4500"_hex},
			{"orchid", "#da70d6"_hex},
			{"palegoldenrod", "#eee8aa"_hex},
			{"palegreen", "#98fb98"_hex},
			{"paleturquoise", "#afeeee"_hex},
			{"palevioletred", "#db7093"_hex},
			{"papayawhip", "#ffefd5"_hex},
			{"peachpuff", "#ffdab9"_hex},
			{"peru", "#cd853f"_hex},
			{"pink", "#ffc0cb"_hex},
			{"plum", "#dda0dd"_hex},
			{"powderblue", "#b0e0e6"_hex},
			{"purple", "#800080"_hex},
			{"rebeccapurple", "#663399"_hex},
			{"red", "#ff0000"_hex},
			{"rosybrown", "#bc8f8f"_hex},
			{"royalblue", "#4169e1"_hex},
			{"saddlebrown", "#8b4513"_hex},
			{"salmon", "#fa8072"_hex},
			{"sandybrown", "#f4a460"_hex},
			{"seagreen", "#2e8b57"_hex},
			{"seashell", "#fff5ee"_hex},
			{"sienna", "#a0522d"_hex},
			{"silver", "#c0c0c0"_hex},
			{"skyblue", "#87ceeb"_hex},
			{"slateblue", "#6a5acd"_hex},
			{"slategray", "#708090"_hex},
			{"slategrey", "#708090"_hex},
			{"snow", "#fffafa"_hex},
			{"springgreen", "#00ff7f"_hex},
			{"steelblue", "#4682b4"_hex},
			{"tan", "#d2b48c"_hex},
			{"teal", "#008080"_hex},
			{"thistle", "#d8bfd8"_hex},
			{"tomato", "#ff6347"_hex},
			{"turquoise", "#40e0d0"_hex},
			{"violet", "#ee82ee"_hex},
			{"wheat", "#f5deb3"_hex},
			{"white", "#ffffff"_hex},
			{"whitesmoke", "#f5f5f5"_hex},
			{"yellow", "#ffff00"_hex},
			{"yellowgreen", "#9acd32"_hex}
};

namespace MathAnim
{
	namespace Css
	{
		CssColor colorFromString(const char* cssColorStr, size_t strLength)
		{
			if (strLength == 0)
			{
				strLength = std::strlen(cssColorStr);
			}

			if (cssColorStr[0] == '#')
			{
				return {
					toHex(cssColorStr),
					CssStyleType::Value
				};
			}

			std::string lowerCaseString = cssColorStr;
			for (size_t i = 0; i < lowerCaseString.length(); i++)
			{
				if ((uint8)lowerCaseString[i] >= (uint8)'A' && (uint8)lowerCaseString[i] <= (uint8)'Z')
				{
					lowerCaseString[i] = (char)(((uint8)lowerCaseString[i] - (uint8)'A') + (uint8)'a');
				}
			}

			auto iter = cssColors.find(lowerCaseString);
			if (iter != cssColors.end())
			{
				return {
					iter->second,
					CssStyleType::Value
				};
			}

			if (lowerCaseString == "inherit")
			{
				return {
					cssColors.at("white"),
					CssStyleType::Inherit
				};
			}

			// Return ugly color by default just in case
			return {
				cssColors.at("magenta"),
				CssStyleType::Error
			};
		}

		CssFontStyle fontStyleFromString(std::string const& str)
		{
			std::string strLowerCase = std::string(str);
			for (size_t i = 0; i < strLowerCase.length(); i++)
			{
				if ((uint8)strLowerCase[i] >= (uint8)'A' && (uint8)strLowerCase[i] <= (uint8)'Z')
				{
					strLowerCase[i] = (char)(((uint8)strLowerCase[i] - (uint8)'A') + (uint8)'a');
				}
			}

			if (strLowerCase == "bold")
			{
				return CssFontStyle::Bold;
			}

			if (strLowerCase == "italic")
			{
				return CssFontStyle::Italic;
			}

			if (strLowerCase == "normal")
			{
				return CssFontStyle::Normal;
			}

			if (strLowerCase == "inherit")
			{
				return CssFontStyle::Inherit;
			}

			return CssFontStyle::None;
		}
	}
}

CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::CssFontStyle& style)
{
	switch (style)
	{
	case MathAnim::CssFontStyle::Bold:
		ostream << "<CssFontStyle:Bold>";
		break;
	case MathAnim::CssFontStyle::Inherit:
		ostream << "<CssFontStyle:Inherit>";
		break;
	case MathAnim::CssFontStyle::Italic:
		ostream << "<CssFontStyle:Italic>";
		break;
	case MathAnim::CssFontStyle::None:
		ostream << "<CssFontStyle:None>";
		break;
	case MathAnim::CssFontStyle::Normal:
		ostream << "<CssFontStyle:Normal>";
		break;
	};

	return ostream;
}

CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::CssStyleType& style)
{
	switch (style)
	{
	case MathAnim::CssStyleType::Error:
		ostream << "<CssStyleType:error>";
		break;
	case MathAnim::CssStyleType::Inherit:
		ostream << "<CssStyleType:inherit>";
		break;
	case MathAnim::CssStyleType::Value:
		ostream << "<CssStyleType:value>";
		break;
	};

	return ostream;
}