#ifndef MATH_ANIM_VIDEO_WRITER_H
#define MATH_ANIM_VIDEO_WRITER_H
#include "core.h"

extern "C" {
	struct AV1Context;
}

namespace MathAnim
{
	struct MemMappedFile;

	struct VideoFrame
	{
		uint8* pixels;
		size_t pixelsSize;
	};

	enum class VideoEncoderFlags : uint8
	{
		None = 0,
		LogProgress = 1,
		//Blah = LogProgress << 1
	};

	typedef int32 Mbps;

	class VideoEncoder
	{
	public:
		static VideoEncoder* startEncodingFile(const char* outputFilename, int outputWidth, int outputHeight, int outputFramerate, size_t totalNumFramesInVideo, Mbps bitrate = 20, VideoEncoderFlags flags = VideoEncoderFlags::None);
		static void finalizeEncodingFile(VideoEncoder* encoder);
		static void freeEncoder(VideoEncoder* encoder);

	public:
		VideoEncoder() = default;

		void pushYuvFrame(uint8* pixels, size_t pixelsSize);

		void setPercentComplete(float newVal);
		float getPercentComplete() const { return percentComplete.load(); }

		bool isEncodingVideo() const { return isEncoding.load(); }

		void destroy();

	private:
		void encodeThreadLoop();
		void threadSafeFinalize();

	private:
		// General data
		uint8* filename;
		size_t filenameLength;
		int width;
		int height;
		int framerate;
		int frameCounter;
		int totalFrames;
		bool logProgress;
		VideoEncoderFlags flags;
		FILE* outputFile;
		MemMappedFile* videoFrameCache;
		size_t numPushedFrames;

		// AV1 Data
		AV1Context* av1Context;

		// Threading data
		std::mutex encodeMtx;
		std::thread ivfFileWriteThread;
		std::thread thread;
		std::thread finalizeThread;
		std::queue<VideoFrame> queuedFrames;
		std::atomic_bool isEncoding;
		std::atomic<float> percentComplete;
		std::atomic<size_t> approxRamUsed;
	};
}

#endif
