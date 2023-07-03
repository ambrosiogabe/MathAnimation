#ifndef MATH_ANIM_BASE64_H
#define MATH_ANIM_BASE64_H
#include "core.h"

namespace MathAnim
{
	namespace Base64
	{
		RawMemory encode(const uint8* data, size_t numBytes);

		RawMemory decode(const char* b64String, size_t length);
	}
}

#endif 