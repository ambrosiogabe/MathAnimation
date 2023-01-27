#include "core.h"
#include "video/Encoder.h"

namespace MathAnim
{
	namespace VideoWriter
	{
		bool startEncodingFile(
			VideoEncoder* output,
			const char* outputFilename,
			int outputWidth,
			int outputHeight,
			int outputFramerate,
			Mbps bitrate,
			bool logProgress)
		{
			return false;
		}

		bool pushFrame(Pixel* pixels, int pixelsLength, VideoEncoder& encoder)
		{
			return false;
		}

		bool finalizeEncodingFile(VideoEncoder& encoder)
		{
			return false;
		}

		void freeEncoder(VideoEncoder& encoder)
		{
		}
	}
}