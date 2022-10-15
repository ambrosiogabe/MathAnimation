#include "core.h"
#include "video/Encoder.h"

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
		//static const int bitrate = 2000;

		// ---------------- Internal functions ----------------
		static bool encodePacket(VideoEncoder& encoder);
		static void printError(int errorNum);

		// Adapted from https://stackoverflow.com/questions/46444474/c-ffmpeg-create-mp4-file
		bool startEncodingFile(
			VideoEncoder* output,
			const char* outputFilename,
			int outputWidth,
			int outputHeight,
			int outputFramerate,
			Mbps bitrate,
			bool logProgress)
		{
			g_logger_assert(output != nullptr, "Cannot start encoding with null output VideoEncoder.");

			output->width = outputWidth;
			output->height = outputHeight;
			output->framerate = outputFramerate;
			output->frameCounter = 0;
			output->logProgress = logProgress;

			output->codecContext = nullptr;
			output->formatContext = nullptr;
			output->swsContext = nullptr;
			output->videoFrame = nullptr;

			size_t filenameLength = std::strlen(outputFilename);
			output->filename = (uint8*)g_memory_allocate(sizeof(uint8) * (filenameLength + 1));
			output->filenameLength = filenameLength;
			g_memory_copyMem(output->filename, (void*)outputFilename, (filenameLength + 1) * sizeof(uint8));

			// av_register_all();
			// avcodec_register_all();

			output->outputFormat = av_guess_format(nullptr, outputFilename, nullptr);
			if (!output->outputFormat)
			{
				output->outputFormat = av_guess_format("mpeg", NULL, NULL);
				g_logger_warning("Could not deduce format from file extension '%s'. Defaulting to mpeg.", outputFilename);
			}
			if (!output->outputFormat)
			{
				g_logger_error("Could not find suitable output format.");
				freeEncoder(*output);
				return false;
			}

			int err = avformat_alloc_output_context2(&output->formatContext, output->outputFormat, nullptr, outputFilename);
			if (err)
			{
				g_logger_error("Could not create output context.");
				printError(err);
				freeEncoder(*output);
				return false;
			}

			const AVCodec* codec = avcodec_find_encoder(output->outputFormat->video_codec);
			if (!codec)
			{
				g_logger_error("Failed to create codec %d.", output->outputFormat->video_codec);
				freeEncoder(*output);
				return false;
			}

			AVStream* stream = avformat_new_stream(output->formatContext, codec);
			if (!stream)
			{
				g_logger_error("Failed to create a new stream.");
				freeEncoder(*output);
				return false;
			}

			output->codecContext = avcodec_alloc_context3(codec);
			if (!output->codecContext)
			{
				g_logger_error("Failed to create codec context.");
				freeEncoder(*output);
				return false;
			}

			stream->codecpar->codec_id = output->outputFormat->video_codec;
			stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
			stream->codecpar->width = output->width;
			stream->codecpar->height = output->height;
			stream->codecpar->format = AV_PIX_FMT_YUV420P;
			stream->codecpar->bit_rate = bitrate * (int64)10e4; // Bitrate * 10E4 should convert to Mbps
			avcodec_parameters_to_context(output->codecContext, stream->codecpar);
			output->codecContext->max_b_frames = 2;
			output->codecContext->gop_size = 12;
			output->codecContext->time_base = AVRational{ 1, output->framerate };
			output->codecContext->framerate = AVRational{ output->framerate, 1 };

			if (stream->codecpar->codec_id == AV_CODEC_ID_H264 || stream->codecpar->codec_id == AV_CODEC_ID_H265)
			{
				av_opt_set(output->codecContext, "preset", "ultrafast", 0);
			}

			avcodec_parameters_from_context(stream->codecpar, output->codecContext);
			if ((err = avcodec_open2(output->codecContext, codec, NULL)) < 0)
			{
				g_logger_error("Failed to open codec.");
				printError(err);
				freeEncoder(*output);
				return false;
			}

			if (!(output->outputFormat->flags & AVFMT_NOFILE))
			{
				if ((err = avio_open(&output->formatContext->pb, outputFilename, AVIO_FLAG_WRITE)) < 0)
				{
					g_logger_error("Failed to open file.");
					printError(err);
					freeEncoder(*output);
					return false;
				}
			}

			if ((err = avformat_write_header(output->formatContext, NULL)) < 0)
			{
				g_logger_error("Failed to write file header.");
				printError(err);
				freeEncoder(*output);
				return false;
			}

			av_dump_format(output->formatContext, 0, outputFilename, 1);
			return true;
		}

		bool pushFrame(Pixel* pixels, int pixelsLength, VideoEncoder& encoder)
		{
			g_logger_assert(pixelsLength == encoder.width * encoder.height, "Invalid pixel buffer for video encoding. Width and height do not match pixelsLength.");

			int err;
			if (!encoder.videoFrame)
			{
				encoder.videoFrame = av_frame_alloc();
				encoder.videoFrame->format = AV_PIX_FMT_YUV420P;
				encoder.videoFrame->width = encoder.codecContext->width;
				encoder.videoFrame->height = encoder.codecContext->height;

				if ((err = av_frame_get_buffer(encoder.videoFrame, 32)) < 0)
				{
					g_logger_error("Failed to allocate video frame.");
					printError(err);
					return false;
				}
			}

			if (!encoder.swsContext)
			{
				encoder.swsContext = sws_getContext(
					encoder.codecContext->width,
					encoder.codecContext->height,
					AV_PIX_FMT_RGB24,
					encoder.codecContext->width,
					encoder.codecContext->height,
					AV_PIX_FMT_YUV420P,
					SWS_BICUBIC,
					0,
					0,
					0
				);
			}

			int inLinesize[1] = { 3 * encoder.codecContext->width };

			// Convert from RGB to YUV
			// I believe this helps since we're encoding as H264
			sws_scale(
				encoder.swsContext,
				(const uint8* const*)&pixels,
				inLinesize,
				0,
				encoder.codecContext->height,
				encoder.videoFrame->data,
				encoder.videoFrame->linesize
			);

			encoder.videoFrame->pts = (int64)((1.0 / (double)encoder.framerate) * 90000.0 * (double)(encoder.frameCounter++));

			if (encoder.logProgress && ((encoder.frameCounter % encoder.framerate) == 0))
			{
				g_logger_info("%d second(s) encoded.", (encoder.frameCounter / 60));
			}

			return encodePacket(encoder);
		}

		bool finalizeEncodingFile(VideoEncoder& encoder)
		{
			for (;;)
			{
				if (!encodePacket(encoder))
				{
					break;
				}
			}

			int err = av_write_trailer(encoder.formatContext);
			if (err < 0)
			{
				g_logger_error("Failed to write video trailer.");
				printError(err);
				return false;
			}

			// If this stream was writing to a file, close it
			if (!(encoder.outputFormat->flags & AVFMT_NOFILE))
			{
				int err = avio_close(encoder.formatContext->pb);
				if (err < 0)
				{
					g_logger_error("Failed to close file.");
					printError(err);
				}
			}

			return true;
		}

		void freeEncoder(VideoEncoder& encoder)
		{
			if (encoder.filename)
			{
				g_memory_free(encoder.filename);
				encoder.filename = nullptr;
			}

			encoder.frameCounter = 0;
			encoder.filenameLength = 0;
			encoder.width = 0;
			encoder.height = 0;
			encoder.framerate = 0;
			encoder.logProgress = false;
			encoder.outputFormat = nullptr;

			if (encoder.videoFrame)
			{
				av_frame_free(&encoder.videoFrame);
				encoder.videoFrame = nullptr;
			}

			if (encoder.codecContext)
			{
				avcodec_free_context(&encoder.codecContext);
				encoder.codecContext = nullptr;
			}

			if (encoder.formatContext)
			{
				avformat_free_context(encoder.formatContext);
				encoder.formatContext = nullptr;
			}

			if (encoder.swsContext)
			{
				sws_freeContext(encoder.swsContext);
				encoder.swsContext = nullptr;
			}
		}

		// ---------------- Internal functions ----------------
		static bool encodePacket(VideoEncoder& encoder)
		{
			int err;
			if ((err = avcodec_send_frame(encoder.codecContext, encoder.videoFrame)) < 0)
			{
				g_logger_error("Failed to send frame '%d'", encoder.frameCounter);
				printError(err);
				return false;
			}

			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;
			pkt.flags |= AV_PKT_FLAG_KEY;
			if ((err = avcodec_receive_packet(encoder.codecContext, &pkt)) < 0)
			{
				g_logger_error("Failed to recieve packet.");
				printError(err);
				av_packet_unref(&pkt);
				return false;
			}

			uint8_t* size = ((uint8_t*)pkt.data);
			if ((err = av_interleaved_write_frame(encoder.formatContext, &pkt)) < 0)
			{
				g_logger_error("Failed to write frame: %d", encoder.frameCounter);
				printError(err);
				av_packet_unref(&pkt);
				return false;
			}

			av_packet_unref(&pkt);
			return true;
		}

		static void printError(int errorNum)
		{
			constexpr int errorBufferSize = 512;
			char* errorBuffer = (char*)g_memory_allocate(sizeof(char) * errorBufferSize);
			av_make_error_string(errorBuffer, errorBufferSize, errorNum);
			g_logger_error("AV Error: %s", errorBuffer);
			g_memory_free(errorBuffer);
		}
	}
}