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
	struct VideoFrame
	{
		uint8* pixels;
		size_t pixelsSize;
		std::string filepath;
		bool cachedOnDisk;
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
		static VideoEncoder* startEncodingFile(const char* outputFilename, int outputWidth, int outputHeight, int outputFramerate, Mbps bitrate = 60, VideoEncoderFlags flags = VideoEncoderFlags::None, size_t ramLimitBytes = GB(48));
		static void finalizeEncodingFile(VideoEncoder* encoder);
		static void freeEncoder(VideoEncoder* encoder);

	public:
		VideoEncoder() = default;
		
		void pushYuvFrame(uint8* pixels, size_t pixelsSize);

		float getPercentComplete() const { return percentComplete.load(); }
		bool isEncodingVideo() const { return isEncoding.load(); }
		size_t getAmountOfRamUsed() const { return approxRamUsed.load(); }

		void destroy();

	private:
		void encodeThreadLoop();
		void threadSafeFinalize();

		bool encodePacket();
		void printError(int errorNum) const;

	private:
		uint8* filename;
		size_t filenameLength;
		int width;
		int height;
		int framerate;
		int frameCounter;
		int totalFrames;
		size_t ramLimitGb;
		bool logProgress;

		// ffmpeg data
		const AVOutputFormat* outputFormat;
		AVFrame* videoFrame;
		AVCodecContext* codecContext;
		SwsContext* swsContext;
		AVFormatContext* formatContext;
		VideoEncoderFlags flags;

		std::mutex encodeMtx;
		std::thread thread;
		std::thread finalizeThread;
		std::queue<VideoFrame> queuedFrames;
		std::atomic_bool isEncoding;
		std::atomic<float> percentComplete;
		std::atomic<size_t> approxRamUsed;
	};
}

#endif
