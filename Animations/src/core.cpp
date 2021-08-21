#include "core.h"

int hexToInt(char hexCode)
{
	switch (hexCode)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return hexCode - '0';
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		return hexCode - 'A' + 10;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
		return hexCode - 'a' + 10;
	}

	g_logger_error("Invalid hex character '%c'", hexCode);
	return -1;
}

float hexToFloat(char hexCode)
{
	return (float)hexToInt(hexCode);
}

MathAnim::Vec4 operator""_hex(const char* rawHexColor, size_t inputLength)
{
	g_logger_assert(rawHexColor != nullptr, "Invalid hex color. Cannot be null.");

	const char* hexColor = rawHexColor[0] == '#' ? 
		rawHexColor + 1 :
		rawHexColor;
	size_t length = strlen(hexColor);
	g_logger_assert(length >= 6, "Invalid hex color '%s', hex color must have at least 6 digits.");
	float color1 = (hexToFloat(hexColor[0]) * 16 + hexToFloat(hexColor[1])) / 255.0f;
	float color2 = (hexToFloat(hexColor[2]) * 16 + hexToFloat(hexColor[3])) / 255.0f;
	float color3 = (hexToFloat(hexColor[4]) * 16 + hexToFloat(hexColor[5])) / 255.0f;
	
	if (length == 8)
	{
		float color4 = (hexToFloat(hexColor[6]) * 16 + hexToFloat(hexColor[7])) / 255.0f;
		return MathAnim::Vec4{
			color1, color2, color3, color4
		};
	}

	// Default alpha to 1
	return MathAnim::Vec4{
		color1, color2, color3, 1.0f
	};
}