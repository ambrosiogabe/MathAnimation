#ifndef MATH_ANIM_NVIDIA_ENCODER_H
#define MATH_ANIM_NVIDIA_ENCODER_H
#include "core.h"

namespace MathAnim
{
	struct Pixel;

	struct NVEncoder
	{
		uint8* filename;
		size_t filenameLength;
		int width;
		int height;
		int framerate;
		int frameCounter;
		bool logProgress;
	};

	typedef int32 Mbps;

	namespace NVEncode
	{
		bool startEncodingFile(NVEncoder* output, const char* outputFilename, int outputWidth, int outputHeight, int outputFramerate, Mbps bitrate = 60, bool logProgress = false);
		bool pushFrame(Pixel* pixels, int pixelsLength, NVEncoder& encoder);

		bool finalizeEncodingFile(NVEncoder& encoder);
		void freeEncoder(NVEncoder& encoder);
	}
}

#endif