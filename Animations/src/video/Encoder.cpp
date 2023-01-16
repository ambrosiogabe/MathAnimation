#include "video/Encoder.h"
#include "multithreading/GlobalThreadPool.h"
#include "core/Application.h"
#include "platform/Platform.h"

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
	// ------------------------ Internal Functions ------------------------
	static void waitForVideoEncodingToFinish(void* data, size_t dataSize);

	// Adapted from https://stackoverflow.com/questions/46444474/c-ffmpeg-create-mp4-file
	VideoEncoder* VideoEncoder::startEncodingFile(
		const char* outputFilename,
		int outputWidth,
		int outputHeight,
		int outputFramerate,
		Mbps bitrate,
		VideoEncoderFlags flags,
		size_t ramLimitGb)
	{
		VideoEncoder* output = (VideoEncoder*)g_memory_allocate(sizeof(VideoEncoder));
		new(output)VideoEncoder();

		output->width = outputWidth;
		output->height = outputHeight;
		output->framerate = outputFramerate;
		output->frameCounter = 0;
		output->logProgress = ((uint8)flags & (uint8)VideoEncoderFlags::LogProgress);
		output->ramLimitGb = ramLimitGb;

		output->codecContext = nullptr;
		output->formatContext = nullptr;
		output->swsContext = nullptr;
		output->videoFrame = nullptr;
		output->approxRamUsed = 0.0f;

		size_t filenameLength = std::strlen(outputFilename);
		output->filename = (uint8*)g_memory_allocate(sizeof(uint8) * (filenameLength + 1));
		output->filenameLength = filenameLength;
		g_memory_copyMem(output->filename, (void*)outputFilename, (filenameLength + 1) * sizeof(uint8));

		output->outputFormat = av_guess_format(nullptr, outputFilename, nullptr);
		if (!output->outputFormat)
		{
			output->outputFormat = av_guess_format("mov", NULL, NULL);
			g_logger_warning("Could not deduce format from file extension '%s'. Defaulting to mpeg.", outputFilename);
		}
		if (!output->outputFormat)
		{
			g_logger_error("Could not find suitable output format.");
			freeEncoder(output);
			return nullptr;
		}

		int err = avformat_alloc_output_context2(&output->formatContext, output->outputFormat, nullptr, outputFilename);
		if (err)
		{
			g_logger_error("Could not create output context.");
			output->printError(err);
			freeEncoder(output);
			return nullptr;
		}

		const AVCodec* codec = avcodec_find_encoder(output->outputFormat->video_codec);
		if (!codec)
		{
			g_logger_error("Failed to create codec %d.", output->outputFormat->video_codec);
			freeEncoder(output);
			return nullptr;
		}

		AVStream* stream = avformat_new_stream(output->formatContext, codec);
		if (!stream)
		{
			g_logger_error("Failed to create a new stream.");
			freeEncoder(output);
			return nullptr;
		}

		output->codecContext = avcodec_alloc_context3(codec);
		if (!output->codecContext)
		{
			g_logger_error("Failed to create codec context.");
			freeEncoder(output);
			return nullptr;
		}

		stream->codecpar->codec_id = output->outputFormat->video_codec;
		stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		stream->codecpar->width = output->width;
		stream->codecpar->height = output->height;
		stream->codecpar->format = AV_PIX_FMT_YUV420P;
		stream->codecpar->bit_rate = bitrate * (int64)10e4; // Bitrate * 10E4 should convert to Mbps
		stream->codecpar->sample_aspect_ratio = AVRational{ 1, 1 };
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
			output->printError(err);
			freeEncoder(output);
			return nullptr;
		}

		if (!(output->outputFormat->flags & AVFMT_NOFILE))
		{
			if ((err = avio_open(&output->formatContext->pb, outputFilename, AVIO_FLAG_WRITE)) < 0)
			{
				g_logger_error("Failed to open file.");
				output->printError(err);
				freeEncoder(output);
				return nullptr;
			}
		}

		if ((err = avformat_write_header(output->formatContext, NULL)) < 0)
		{
			g_logger_error("Failed to write file header.");
			output->printError(err);
			freeEncoder(output);
			return nullptr;
		}

		// TODO: Don't output this in release, instead display a progress bar in the UI
		av_dump_format(output->formatContext, 0, outputFilename, 1);

		output->isEncoding = true;
		output->thread = std::thread(&VideoEncoder::encodeThreadLoop, output);

		return output;
	}

	void VideoEncoder::finalizeEncodingFile(VideoEncoder* encoder)
	{
		if (!encoder)
		{
			return;
		}

		if (!encoder->finalizeThread.joinable())
		{
			encoder->finalizeThread = std::thread(&VideoEncoder::threadSafeFinalize, encoder);
		}
	}

	void VideoEncoder::freeEncoder(VideoEncoder* encoder)
	{
		if (encoder)
		{
			if (encoder->isEncodingVideo())
			{
				// This is kind of gross, we launch another thread using the thread pool to wait for it to finish
				// so the main window can finish closing then we finish encoding the video in the background
				// and clean up memory after everything finishes.
				Application::threadPool()->queueTask(waitForVideoEncodingToFinish, "WaitForVideoEncoderToFinish", (void*)encoder, sizeof(VideoEncoder));
			}
			else
			{
				encoder->destroy();
				g_memory_free(encoder);
			}
		}
	}

	void VideoEncoder::destroy()
	{
		if (this->finalizeThread.joinable())
		{
			this->finalizeThread.join();
		}
		else if (this->thread.joinable())
		{
			VideoEncoder::finalizeEncodingFile(this);
			this->finalizeThread.join();
		}

		// Free the data
		if (filename)
		{
			g_memory_free(filename);
		}

		filename = nullptr;
		frameCounter = 0;
		filenameLength = 0;
		width = 0;
		height = 0;
		framerate = 0;
		logProgress = false;
		outputFormat = nullptr;

		if (videoFrame)
		{
			av_frame_free(&videoFrame);
			videoFrame = nullptr;
		}

		if (codecContext)
		{
			avcodec_free_context(&codecContext);
			codecContext = nullptr;
		}

		if (formatContext)
		{
			avformat_free_context(formatContext);
			formatContext = nullptr;
		}

		if (swsContext)
		{
			sws_freeContext(swsContext);
			swsContext = nullptr;
		}

		this->~VideoEncoder();
	}

	void VideoEncoder::pushYuvFrame(uint8* pixels, size_t pixelsSize)
	{
		size_t singleChannelSize = width * height;
		size_t framePixelsSize = singleChannelSize * 3;
		g_logger_assert(pixelsSize == framePixelsSize, "Invalid pixel buffer for video encoding. Width and height do not match pixelsLength.");

		VideoFrame frame;
		frame.pixels = (uint8*)g_memory_allocate(framePixelsSize);
		g_memory_copyMem(frame.pixels, pixels, pixelsSize);
		frame.pixelsSize = pixelsSize;
		frame.cachedOnDisk = false;
		frame.filepath = "";

		// Check if we have enough room in RAM for more pixels, otherwise cache to the disk
		size_t ramUsedExpected = approxRamUsed.load();
		if (ramUsedExpected + frame.pixelsSize < ramLimitGb)
		{
			while (!approxRamUsed.compare_exchange_strong(ramUsedExpected, ramUsedExpected + frame.pixelsSize))
			{
				ramUsedExpected = approxRamUsed.load();
			}
		}
		else
		{
			// Cache to disk
			std::filesystem::path tmpDir = std::filesystem::path(Application::getCurrentProjectRoot()) / "tmp";
			Platform::createDirIfNotExists(tmpDir.string().c_str());
			std::string tmpFile = Platform::tmpFilename(tmpDir.string()) + ".bin";
			
			FILE* fp = fopen(tmpFile.c_str(), "wb");
			fwrite(pixels, pixelsSize, 1, fp);
			fclose(fp);

			frame.cachedOnDisk = true;
			frame.filepath = tmpFile;

			g_memory_free(frame.pixels);
			frame.pixels = nullptr;
			frame.pixelsSize = 0;
		}

		// Push frame onto queue
		{
			std::lock_guard<std::mutex> lock(encodeMtx);
			queuedFrames.push(frame);
			totalFrames++;
		}
	}

	// ---------------- Internal functions ----------------
	static void waitForVideoEncodingToFinish(void* data, size_t dataSize)
	{
		// Bad Data
		if (!data || dataSize != sizeof(VideoEncoder))
		{
			return;
		}

		VideoEncoder* encoder = (VideoEncoder*)data;
		encoder->destroy();
		g_memory_free(encoder);
	}

	void VideoEncoder::threadSafeFinalize()
	{
		// Stop the encoding loop
		{
			// Wait for queued frames to finish encoding
			while (true)
			{
				std::lock_guard<std::mutex> lock(encodeMtx);
				if (queuedFrames.size() == 0)
				{
					g_logger_info("Lock has been freed.");
					break;
				}
			}

			bool expected = isEncoding.load();
			while (!isEncoding.compare_exchange_strong(expected, false))
			{
				expected = isEncoding.load();
			}
		}

		if (thread.joinable())
		{
			thread.join();
		}

		//while (encodePacket())
		//{
		//	// NOP; Once encodePacket() returns false, the frames have been fully flushed
		//}

		// Delete any lingering temporary files
		std::filesystem::path tmpDir = std::filesystem::path(Application::getCurrentProjectRoot()) / "tmp";
		std::filesystem::remove_all(tmpDir);

		int err = av_write_trailer(formatContext);
		if (err < 0)
		{
			g_logger_error("Failed to write video trailer.");
			printError(err);
			return;
		}

		// If this stream was writing to a file, close it
		if (!(outputFormat->flags & AVFMT_NOFILE))
		{
			int err = avio_close(formatContext->pb);
			if (err < 0)
			{
				g_logger_error("Failed to close file.");
				printError(err);
			}
		}
	}

	void VideoEncoder::encodeThreadLoop()
	{
		while (isEncoding.load())
		{
			VideoFrame nextFrame = {};
			nextFrame.pixels = nullptr;
			{
				std::lock_guard<std::mutex> lock(encodeMtx);
				if (queuedFrames.size() == 0)
				{
					continue;
				}

				nextFrame = queuedFrames.front();
				queuedFrames.pop();
			}

			if (nextFrame.cachedOnDisk)
			{
				FILE* fp = fopen(nextFrame.filepath.c_str(), "rb");
				if (fp)
				{
					fseek(fp, 0, SEEK_END);
					size_t fileSize = ftell(fp);
					fseek(fp, 0, SEEK_SET);

					if (fileSize > 0)
					{
						nextFrame.pixelsSize = fileSize;
						nextFrame.pixels = (uint8*)g_memory_allocate(fileSize);
						fread(nextFrame.pixels, nextFrame.pixelsSize, 1, fp);

						size_t expectedRamUsed = approxRamUsed.load();
						while (!approxRamUsed.compare_exchange_strong(expectedRamUsed, expectedRamUsed + fileSize))
						{
							expectedRamUsed = approxRamUsed.load();
						}
					}

					fclose(fp);
				}

				Platform::deleteFile(nextFrame.filepath.c_str());
			}

			if (!nextFrame.pixels)
			{
				g_logger_warning("Queued video frame had nullptr. Skipping that frame.");
				continue;
			}

			int err;
			if (!videoFrame)
			{
				videoFrame = av_frame_alloc();
				if (!videoFrame)
				{
					g_logger_error("Failed to allocate video frame.");
					g_memory_free(nextFrame.pixels);
					continue;
				}

				videoFrame->format = AV_PIX_FMT_YUV444P;
				videoFrame->width = codecContext->width;
				videoFrame->height = codecContext->height;

				if ((err = av_frame_get_buffer(videoFrame, 24)) < 0)
				{
					g_logger_error("Failed to allocate video frame.");
					printError(err);
					g_memory_free(nextFrame.pixels);
					continue;
				}
			}

			av_frame_make_writable(videoFrame);

			size_t singleChannelSize = videoFrame->linesize[0] * videoFrame->height * sizeof(uint8);
			g_memory_copyMem(videoFrame->data[0], nextFrame.pixels, singleChannelSize);
			g_memory_copyMem(videoFrame->data[1], nextFrame.pixels + singleChannelSize, singleChannelSize);
			g_memory_copyMem(videoFrame->data[2], nextFrame.pixels + singleChannelSize * 2, singleChannelSize);
			videoFrame->pts = (int64)((1.0 / (double)framerate) * 90000.0 * (double)(frameCounter++));
			percentComplete.store((float)frameCounter / (float)totalFrames);

			if (logProgress && ((frameCounter % framerate) == 0))
			{
				g_logger_info("%d second(s) encoded.", (frameCounter / 60));
			}

			encodePacket();
			g_memory_free(nextFrame.pixels);

			size_t usedRamExpected = approxRamUsed.load();
			while (!approxRamUsed.compare_exchange_strong(usedRamExpected, usedRamExpected - nextFrame.pixelsSize))
			{
				usedRamExpected = approxRamUsed.load();
			}
		}

		g_logger_info("Video Encoding loop finished.");
	}

	bool VideoEncoder::encodePacket()
	{
		int err;
		if ((err = avcodec_send_frame(codecContext, videoFrame)) < 0)
		{
			g_logger_error("Failed to send frame '%d'", frameCounter);
			printError(err);
			return false;
		}

		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
		pkt.flags |= AV_PKT_FLAG_KEY;
		if ((err = avcodec_receive_packet(codecContext, &pkt)) < 0)
		{
			// The packet needs to be sent again, let the client know that by returning true
			if (err == AVERROR(EAGAIN))
			{
				return true;
			}

			if (err != AVERROR_EOF)
			{
				g_logger_error("Failed to recieve packet.");
				printError(err);
				av_packet_unref(&pkt);
			}
			return false;
		}

		uint8_t* size = ((uint8_t*)pkt.data);
		if ((err = av_interleaved_write_frame(formatContext, &pkt)) < 0)
		{
			g_logger_error("Failed to write frame: %d", frameCounter);
			printError(err);
			av_packet_unref(&pkt);
			return false;
		}

		av_packet_unref(&pkt);

		return true;
	}

	void VideoEncoder::printError(int errorNum) const
	{
		constexpr int errorBufferSize = 512;
		char* errorBuffer = (char*)g_memory_allocate(sizeof(char) * errorBufferSize);
		av_make_error_string(errorBuffer, errorBufferSize, errorNum);
		g_logger_error("AV Error: %s", errorBuffer);
		g_memory_free(errorBuffer);
	}
}