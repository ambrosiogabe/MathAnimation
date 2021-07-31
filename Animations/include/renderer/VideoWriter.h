#ifndef MATH_ANIM_VIDEO_WRITER_H
#define MATH_ANIM_VIDEO_WRITER_H
#include "core.h"

namespace MathAnim
{
	struct Pixel
	{
		uint8 r;
		uint8 g;
		uint8 b;
	};

	namespace VideoWriter
	{
		void startEncodingFile(const char* outputFilename, int outputWidth, int outputHeight, int outputFramerate);

		void pushFrame(Pixel* pixels, int pixelsLength);

		void finishEncodingFile();
	}
}

#endif
