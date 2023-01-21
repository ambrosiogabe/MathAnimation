#include "video/Encoder.h"
#include "multithreading/GlobalThreadPool.h"
#include "core/Application.h"
#include "platform/Platform.h"

extern "C"
{
#include <EbSvtAv1Enc.h>
#include <EbSvtAv1.h>
}

// overall context
struct AV1Context
{
	MathAnim::VideoEncoder* videoEncoder;
	EbComponentType* svt_handle;
	size_t width;
	size_t height;
	size_t frame_count;
	size_t fps;
	FILE* fout;
};

// copied from FFmpeg

// copied these from aom
static void mem_put_le16(void* vmem, uint16_t val)
{
	uint8_t* mem = (uint8*)vmem;
	mem[0] = 0xff & (val >> 0);
	mem[1] = 0xff & (val >> 8);
}

static void mem_put_le32(void* vmem, uint32_t val)
{
	uint8_t* mem = (uint8*)vmem;
	mem[0] = 0xff & (val >> 0);
	mem[1] = 0xff & (val >> 8);
	mem[2] = 0xff & (val >> 16);
	mem[3] = 0xff & (val >> 24);
}

static void write_ivf_header(FILE* const fout, const size_t width, const size_t height,
	const size_t numerator, const size_t denominator,
	const size_t frame_count)
{
	unsigned char header[32] = { 'D', 'K', 'I', 'F', 0, 0, 32, 0, 'A', 'V', '0', '1' };
	mem_put_le16(header + 12, width);
	mem_put_le16(header + 14, height);
	mem_put_le32(header + 16, numerator);
	mem_put_le32(header + 20, denominator);
	mem_put_le32(header + 24, frame_count);
	fwrite(header, 32, 1, fout);
}

static void write_ivf_frame_size(FILE* const fout, const size_t size)
{
	unsigned char header[4];
	mem_put_le32(header, size);
	fwrite(header, 4, 1, fout);
}

static void write_ivf_frame_header(FILE* const fout, const size_t frame_size, const uint64_t pts)
{
	write_ivf_frame_size(fout, frame_size);
	unsigned char header[8];
	mem_put_le32(header + 0, pts & 0xFFFFFFFF);
	mem_put_le32(header + 4, pts >> 32);
	fwrite(header, 8, 1, fout);
}

// sends a single picture, tries to avoid the stack since the library already uses so much
static void send_frame(
	const EbSvtIOFormat* pic,
	const AV1Context* const c,
	const size_t index,
	const uint8* yChannel,
	size_t yChannelSize,
	const uint8* uChannel,
	size_t uChannelSize,
	const uint8* vChannel,
	size_t vChannelSize)
{
	EbBufferHeaderType* const send_buffer = (EbBufferHeaderType*)calloc(1, sizeof(EbBufferHeaderType));
	send_buffer->size = sizeof(EbBufferHeaderType);
	send_buffer->p_buffer = (uint8*)pic;
	send_buffer->pic_type = EB_AV1_INVALID_PICTURE;
	send_buffer->pts = index;
	// fill out the frame with a test source looking image
	const uint8_t* y = pic->luma;
	const uint8_t* u = pic->cb;
	const uint8_t* v = pic->cr;
	g_memory_copyMem((void*)y, (void*)yChannel, yChannelSize);
	g_memory_copyMem((void*)u, (void*)uChannel, uChannelSize);
	g_memory_copyMem((void*)v, (void*)vChannel, vChannelSize);
	
	// send the frame to the encoder
	svt_av1_enc_send_picture(c->svt_handle, send_buffer);
	free(send_buffer);
}

// receives the whole ivf to fout in it's own thread so we don't have to try to track alt refs
static void* write_ivf(void* p)
{
	const AV1Context* ctx = (AV1Context*)p;
	EbComponentType* svt_handle = ctx->svt_handle;
	const size_t width = ctx->width;
	const size_t height = ctx->height;
	const size_t frame_count = ctx->frame_count;
	const size_t fps = ctx->fps;
	FILE* fout = ctx->fout;

	fputs("starting ivf thread\n", stderr);

	EbBufferHeaderType* receive_buffer = NULL;
	write_ivf_header(fout, width, height, fps, 1, frame_count);
	bool eos = false;
	// setup some variables for handling non-visible frames, based on aom's code
	size_t frame_size = 0;
	off_t  ivf_header_position = 0;
	do
	{
		// retrieve the next ivf packet
		switch (svt_av1_enc_get_packet(svt_handle, &receive_buffer, 0))
		{
		case EB_ErrorMax: fprintf(stderr, "Error: EB_ErrorMax\n");// pthread_exit(NULL);
		case EB_NoErrorEmptyQueue: continue;
		default: break;
		}
		const uint32_t flags = receive_buffer->flags;
		const bool     alt_ref = flags & EB_BUFFERFLAG_IS_ALT_REF;
		if (!alt_ref)
		{
			// if this a visible frame, write out the header and store the position and size
			ivf_header_position = ftell(fout);
			frame_size = receive_buffer->n_filled_len;
			write_ivf_frame_header(fout, frame_size, receive_buffer->pts);
		}
		else
		{
			// aom seems to count all of the frames, visible or not inside the ivf frame header's size field
			// but it's probably not necessary since both encoders and decoders seem to be fine with files from
			// stdout, which wouldn't support fseek etc.
			frame_size += receive_buffer->n_filled_len;

			const off_t current_position = ftell(fout);
			if (!fseek(fout, ivf_header_position, SEEK_SET))
			{
				write_ivf_frame_size(fout, frame_size);
				fseek(fout, current_position, SEEK_SET);
			}
		}
		// write out the frame
		fwrite(receive_buffer->p_buffer, 1, receive_buffer->n_filled_len, fout);
		// just to make sure it's actually written in case the output is a buffered file
		fflush(fout);
		ctx->videoEncoder->setPercentComplete((float)receive_buffer->pts / (float)ctx->frame_count);
		// release back to the library
		svt_av1_enc_release_out_buffer(&receive_buffer);
		receive_buffer = NULL;
		eos = flags & EB_BUFFERFLAG_EOS;
	} while (!eos);

	ctx->videoEncoder->setPercentComplete(1.0f);

	return NULL;
}

static EbSvtIOFormat* allocate_io_format(const size_t width, const size_t height)
{
	EbSvtIOFormat* pic = (EbSvtIOFormat*)calloc(1, sizeof(EbSvtIOFormat));
	pic->y_stride = width;
	pic->cr_stride = width / 2;
	pic->cb_stride = width / 2;
	pic->width = width;
	pic->height = height;
	pic->color_fmt = EB_YUV420;
	pic->bit_depth = EB_EIGHT_BIT;
	pic->luma = (uint8*)calloc(height * pic->y_stride, sizeof(uint8_t));
	pic->cb = (uint8*)calloc(height * pic->cb_stride, sizeof(uint8_t));
	pic->cr = (uint8*)calloc(height * pic->cr_stride, sizeof(uint8_t));
	return pic;
}

static void free_io_format(EbSvtIOFormat* const pic)
{
	free(pic->luma);
	free(pic->cb);
	free(pic->cr);
	free(pic);
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
		size_t totalNumFramesInVideo,
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
		output->isEncoding = true;

		size_t outputFilenameLength = std::strlen(outputFilename);
		output->filename = (uint8*)g_memory_allocate(sizeof(uint8) * (outputFilenameLength + 1));
		output->filenameLength = outputFilenameLength;
		g_memory_copyMem(output->filename, (void*)outputFilename, (output->filenameLength + 1) * sizeof(uint8));
		output->filename[output->filenameLength] = '\0';

		output->outputFile = fopen((const char*)output->filename, "wb");
		if (!output->outputFile)
		{
			// TODO: Clean up memory and stuff
			return nullptr;
		}

		const size_t video_width = output->width;
		const size_t video_height = output->height;
		const size_t video_frames = totalNumFramesInVideo;
		const size_t video_fps = output->framerate;
		FILE* const output_file = output->outputFile;

		// setup our base handle
		EbComponentType* svt_handle = NULL;
		{
			EbSvtAv1EncConfiguration* enc_params = (EbSvtAv1EncConfiguration*)calloc(1, sizeof(*enc_params));
			// initlize the handle and get the default configuration
			if (EB_ErrorNone != svt_av1_enc_init_handle(&svt_handle, NULL, enc_params))
			{
				g_logger_error("Uh oh.");
				return nullptr;
			}
			// set individual parameters before sending them to the encoder
			svt_av1_enc_parse_parameter(enc_params, "width", std::to_string(video_width).c_str());
			svt_av1_enc_parse_parameter(enc_params, "height", std::to_string(video_height).c_str());
			svt_av1_enc_parse_parameter(enc_params, "input-depth", "8");
			svt_av1_enc_parse_parameter(enc_params, "color-format", "420");
			svt_av1_enc_parse_parameter(enc_params, "fps-num", std::to_string(video_fps).c_str());
			svt_av1_enc_parse_parameter(enc_params, "fps-denom", "1");
			svt_av1_enc_parse_parameter(enc_params, "irefresh-type", "kf");
			svt_av1_enc_parse_parameter(enc_params, "preset", "12");
			svt_av1_enc_parse_parameter(enc_params, "rc", "crf");
			svt_av1_enc_parse_parameter(enc_params, "crf", "28");
			svt_av1_enc_parse_parameter(enc_params, "lp", "2");

			// send the parameters to the encoder, and then initialize the encoder
			if (EB_ErrorNone != svt_av1_enc_set_parameter(svt_handle, enc_params) ||
				EB_ErrorNone != svt_av1_enc_init(svt_handle))
			{
				g_logger_error("Uh oh");
				return nullptr;
			}


			// we no longer need enc_params past this point
			free(enc_params);
		}

		AV1Context* p = (AV1Context*)g_memory_allocate(sizeof(AV1Context));
		p->svt_handle = svt_handle;
		p->width = video_width;
		p->height = video_height;
		p->frame_count = video_frames;
		p->fps = video_fps;
		p->fout = output_file;
		p->videoEncoder = output;
		output->av1Context = p;

		// start the thread to receive frames from the encoder
		output->ivfFileWriteThread = std::thread(write_ivf, p);
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

	void VideoEncoder::setPercentComplete(float newVal) 
	{
		float expected = percentComplete.load();
		while (!percentComplete.compare_exchange_strong(expected, newVal))
		{
			expected = percentComplete.load();
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

		if (av1Context)
		{
			g_memory_free(av1Context);
		}

		av1Context = nullptr;
		filename = nullptr;
		frameCounter = 0;
		filenameLength = 0;
		width = 0;
		height = 0;
		framerate = 0;
		logProgress = false;

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

		// send the EOS frame to the encoder
		EbBufferHeaderType eofFlags = { 0 };
		eofFlags.flags = EB_BUFFERFLAG_EOS;
		svt_av1_enc_send_picture(av1Context->svt_handle, &eofFlags);
		g_logger_info("Sent all frames", stderr);

		// wait for all of the frames to finish writing out
		//pthread_join(receive_threads, NULL);
		if (ivfFileWriteThread.joinable())
		{
			ivfFileWriteThread.join();
		}

		if (outputFile)
		{
			fclose(outputFile);
			g_logger_info("Encoding done");
		}

		// clean up the encoder
		svt_av1_enc_deinit(av1Context->svt_handle);
		svt_av1_enc_deinit_handle(av1Context->svt_handle);
		// pthread_exit here just in case a thread inside the library managed to not die
		//pthread_exit(NULL);

		// Delete any lingering temporary files
		std::filesystem::path tmpDir = std::filesystem::path(Application::getCurrentProjectRoot()) / "tmp";
		std::filesystem::remove_all(tmpDir);
	}

	void VideoEncoder::encodeThreadLoop()
	{
		EbSvtIOFormat* pic = allocate_io_format(width, height);
		size_t frameIndex = 0;

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

			// send the individual frames to the encoder
			size_t yChannelSize = width * height * sizeof(uint8);
			size_t uChannelSize = width / 2 * height / 2 * sizeof(uint8);
			size_t vChannelSize = width / 2 * height / 2 * sizeof(uint8);
			send_frame(
				pic,
				av1Context,
				frameIndex,
				nextFrame.pixels,
				yChannelSize,
				nextFrame.pixels + yChannelSize,
				uChannelSize,
				nextFrame.pixels + yChannelSize + uChannelSize,
				vChannelSize
			);
			frameIndex++;
			g_memory_free(nextFrame.pixels);

			if (logProgress && ((frameCounter % framerate) == 0))
			{
				g_logger_info("%d second(s) encoded.", (frameCounter / 60));
			}

			size_t usedRamExpected = approxRamUsed.load();
			while (!approxRamUsed.compare_exchange_strong(usedRamExpected, usedRamExpected - nextFrame.pixelsSize))
			{
				usedRamExpected = approxRamUsed.load();
			}
		}

		free_io_format(pic);
		g_logger_info("Video Encoding loop finished.");
	}

	bool VideoEncoder::encodePacket()
	{
		int err;
		//if ((err = avcodec_send_frame(codecContext, videoFrame)) < 0)
		//{
		//	g_logger_error("Failed to send frame '%d'", frameCounter);
		//	printError(err);
		//	return false;
		//}

		//AVPacket pkt;
		//av_init_packet(&pkt);
		//pkt.data = NULL;
		//pkt.size = 0;
		//pkt.flags |= AV_PKT_FLAG_KEY;
		//if ((err = avcodec_receive_packet(codecContext, &pkt)) < 0)
		//{
		//	// The packet needs to be sent again, let the client know that by returning true
		//	if (err == AVERROR(EAGAIN))
		//	{
		//		return true;
		//	}

		//	if (err != AVERROR_EOF)
		//	{
		//		g_logger_error("Failed to recieve packet.");
		//		printError(err);
		//		av_packet_unref(&pkt);
		//	}
		//	return false;
		//}

		//if ((err = av_interleaved_write_frame(formatContext, &pkt)) < 0)
		//{
		//	g_logger_error("Failed to write frame: %d", frameCounter);
		//	printError(err);
		//	av_packet_unref(&pkt);
		//	return false;
		//}

		//av_packet_unref(&pkt);

		return true;
	}

	void VideoEncoder::printError(int errorNum) const
	{
		/*constexpr int errorBufferSize = 512;
		char* errorBuffer = (char*)g_memory_allocate(sizeof(char) * errorBufferSize);
		av_make_error_string(errorBuffer, errorBufferSize, errorNum);
		g_logger_error("AV Error: %s", errorBuffer);
		g_memory_free(errorBuffer);*/
	}
}