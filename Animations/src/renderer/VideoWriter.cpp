#include "core.h"
#include "renderer/VideoWriter.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

namespace MathAnim
{
	namespace VideoWriter
	{
		// Member variables
		static AVFrame* videoFrame = nullptr;
		static AVCodecContext* cctx = nullptr;
		static SwsContext* swsCtx = nullptr;
		static int frameCounter = 0;
		static AVFormatContext* ofctx = nullptr;
		static AVOutputFormat* oformat = nullptr;

		// Default values
		static int fps = 60;
		static int width = 1920;
		static int height = 1080;
		static const int bitrate = 2000;

		// Internal functions
		static void finish();
		static void free();

		// Adapted from https://stackoverflow.com/questions/46444474/c-ffmpeg-create-mp4-file
		void startEncodingFile(const char* outputFilename, int outputWidth, int outputHeight, int outputFramerate)
		{
			width = outputWidth;
			height = outputHeight;
			fps = outputFramerate;

			av_register_all();
			avcodec_register_all();

			oformat = av_guess_format(nullptr, outputFilename, nullptr);
			if (!oformat)
			{
				Logger::Error("can't create output format");
				return;
			}

			int err = avformat_alloc_output_context2(&ofctx, oformat, nullptr, outputFilename);
			if (err)
			{
				Logger::Error("can't create output context");
				return;
			}

			AVCodec* codec = avcodec_find_encoder(oformat->video_codec);
			if (!codec)
			{
				Logger::Error("can't create codec");
				return;
			}

			AVStream* stream = avformat_new_stream(ofctx, codec);
			if (!stream)
			{
				Logger::Error("can't find format");
				return;
			}

			cctx = avcodec_alloc_context3(codec);
			if (!cctx)
			{
				Logger::Error("can't create codec context");
				return;
			}

			stream->codecpar->codec_id = oformat->video_codec;
			stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
			stream->codecpar->width = width;
			stream->codecpar->height = height;
			stream->codecpar->format = AV_PIX_FMT_YUV420P;
			stream->codecpar->bit_rate = bitrate * 1000;
			avcodec_parameters_to_context(cctx, stream->codecpar);
			cctx->max_b_frames = 2;
			cctx->gop_size = 12;
			cctx->time_base = AVRational{ 1, fps };
			cctx->framerate = AVRational{ fps, 1 };

			if (stream->codecpar->codec_id == AV_CODEC_ID_H264)
			{
				av_opt_set(cctx, "preset", "ultrafast", 0);
			}
			else if (stream->codecpar->codec_id == AV_CODEC_ID_H265)
			{
				av_opt_set(cctx, "preset", "ultrafast", 0);
			}

			avcodec_parameters_from_context(stream->codecpar, cctx);
			if ((err = avcodec_open2(cctx, codec, NULL)) < 0)
			{
				Logger::Error("Failed to open codec %d", err);
				return;
			}

			if (!(oformat->flags & AVFMT_NOFILE))
			{
				if ((err = avio_open(&ofctx->pb, outputFilename, AVIO_FLAG_WRITE)) < 0)
				{
					Logger::Error("Failed to open file %d", err);
					return;
				}
			}

			if ((err = avformat_write_header(ofctx, NULL)) < 0)
			{
				Logger::Error("Failed to write header %d", err);
				return;
			}

			av_dump_format(ofctx, 0, outputFilename, 1);
		}

		void pushFrame(Pixel* pixels, int pixelsLength)
		{
			Logger::Assert(pixelsLength == width * height, "Invalid pixel buffer for video encoding. Width and height do not match pixelsLength.");

			int err;
			if (!videoFrame)
			{
				videoFrame = av_frame_alloc();
				videoFrame->format = AV_PIX_FMT_YUV420P;
				videoFrame->width = cctx->width;
				videoFrame->height = cctx->height;

				if ((err = av_frame_get_buffer(videoFrame, 32)) < 0)
				{
					Logger::Error("Failed to allocate video frame %d", err);
					return;
				}
			}

			if (!swsCtx)
			{
				swsCtx = sws_getContext(
					cctx->width,
					cctx->height,
					AV_PIX_FMT_RGB24,
					cctx->width,
					cctx->height,
					AV_PIX_FMT_YUV420P,
					SWS_BICUBIC,
					0,
					0,
					0
				);
			}

			int inLinesize[1] = { 3 * cctx->width };

			// From RGB to YUV
			sws_scale(swsCtx, (const uint8* const*)&pixels, inLinesize, 0, cctx->height, videoFrame->data, videoFrame->linesize);

			videoFrame->pts = (1.0 / fps) * 90000 * (frameCounter++);

			if (frameCounter % 60 == 0)
			{
				Logger::Info("%d second(s) encoded.", (frameCounter / 60));
			}

			if ((err = avcodec_send_frame(cctx, videoFrame)) < 0)
			{
				Logger::Error("Failed to send frame %d", err);
				return;
			}

			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;
			pkt.flags |= AV_PKT_FLAG_KEY;
			if (avcodec_receive_packet(cctx, &pkt) == 0)
			{
				uint8_t* size = ((uint8_t*)pkt.data);
				av_interleaved_write_frame(ofctx, &pkt);
				av_packet_unref(&pkt);
			}
		}

		void finishEncodingFile()
		{
			finish();
			free();
		}

		static void finish()
		{
			//DELAYED FRAMES
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;

			for (;;)
			{
				avcodec_send_frame(cctx, NULL);
				if (avcodec_receive_packet(cctx, &pkt) == 0)
				{
					av_interleaved_write_frame(ofctx, &pkt);
					av_packet_unref(&pkt);
				}
				else
				{
					break;
				}
			}

			av_write_trailer(ofctx);
			if (!(oformat->flags & AVFMT_NOFILE))
			{
				int err = avio_close(ofctx->pb);
				if (err < 0)
				{
					Logger::Error("Failed to close file %d", err);
				}
			}
		}

		static void free()
		{
			if (videoFrame)
			{
				av_frame_free(&videoFrame);
			}
			if (cctx)
			{
				avcodec_free_context(&cctx);
			}
			if (ofctx)
			{
				avformat_free_context(ofctx);
			}
			if (swsCtx)
			{
				sws_freeContext(swsCtx);
			}
		}
	}
}