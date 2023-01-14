#ifndef MATH_ANIM_VIDEO_WRITER_H
#define MATH_ANIM_VIDEO_WRITER_H
#include "core.h"

struct AVOutputFormat;
struct AVFrame;
struct AVCodecContext;
struct SwsContext;
struct AVFormatContext;

namespace MathAnim
{
	struct Pixel
	{
		uint8 r;
		uint8 g;
		uint8 b;
	};

	struct VideoFrame
	{
		Pixel* pixels;
		size_t pixelsLength;
	};

	struct VideoEncoder
	{
		uint8* filename;
		size_t filenameLength;
		int width;
		int height;
		int framerate;
		int frameCounter;
		bool logProgress;

		// ffmpeg data
		const AVOutputFormat* outputFormat;
		AVFrame* videoFrame;
		AVCodecContext* codecContext;
		SwsContext* swsContext;
		AVFormatContext* formatContext;

		std::thread thread;
		std::queue<VideoFrame> queuedFrames;
		std::mutex encodeMtx;
		std::atomic_bool isEncoding;
	};

	typedef int32 Mbps;

	namespace VideoWriter
	{
		VideoEncoder* startEncodingFile(const char* outputFilename, int outputWidth, int outputHeight, int outputFramerate, Mbps bitrate = 60, bool logProgress = false);
		bool pushFrame(Pixel* pixels, size_t pixelsLength, VideoEncoder* encoder);

		bool finalizeEncodingFile(VideoEncoder* encoder);

	}
}

#endif
