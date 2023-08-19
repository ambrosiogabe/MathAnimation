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

		// NOTE: Binary format spec is listed below
		bool parseBinSvgPath(const uint8* bin, size_t numBytes, SvgObject* output);
		bool parseB64BinSvgPath(const std::string b64String, SvgObject* output);

		const char* getLastError();
	}
}

// Binary SVG Path Format
// ----------------------
// 
// The Binary SVG Path Format an SVG path encoded as binary data. The binary data is then
// converted to a base64 string to make it JSON safe. The following are the specs.
// 
// Consists of the following commands from the SVG spec. All the commands are always in Absolute form:
// 
//  * MoveTo  = 0
//  * LineTo  = 1
//  * CurveTo = 2
//  * QuadTo  = 3
//
// A path always starts with a MoveTo command and never ends with a close command.
// 
// Any numbers that follow a command are the coordinate pairs as specified by the SVG spec. Each
// number is a normalized floating point value. The normalized value is calculated by doing:
// 
//   (int)(val * 1e6)
// 
// to truncate the value at 6 decimal places as an integer. This means any values outside the range
// approximately [-2,147 -> 2,147] will be truncated into this range. It's recommended to normalize all
// values before converting to a binary path string.
// 
// Commands cannot contain any implicit commands by specifying extra coordinate pairs. This means you cannot
// do stuff like "M x1 y1 x2 y2", and instead it will be converted to something like "M x1 y1 M x2 y2".
// 
// Commands are formatted as the following, where the command name is replaced with the command value
// listed above:
// 
// [  MoveTo, int32, int32 ]
// [  LineTo, int32, int32 ]
// [ CurveTo, int32, int32, int32, int32, int32, int32 ]
// [  QuadTo, int32, int32, int32, int32 ]
//
// The meaning of each command follows the SVG path spec:
// 
// [  MoveTo, x1, y1 ]
// [  LineTo, x1, y1 ]
// [ CurveTo, x1, y1, x2, y2, x, y ]
// [  QuadTo, x1, y1, x, y ]

#endif 