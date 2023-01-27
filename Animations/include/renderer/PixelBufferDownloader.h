#ifndef MATH_ANIM_PBO_DOWNLOADER_H
#define MATH_ANIM_PBO_DOWNLOADER_H
#include "core.h"

namespace MathAnim
{
	struct PixelBufferDownloadData;
	struct Texture;
	struct Framebuffer;

	struct Pixels
	{
		uint8* yColorBuffer;
		uint8* uColorBuffer;
		uint8* vColorBuffer;
		size_t dataSize;
	};

	class PixelBufferDownload
	{
	public:
		PixelBufferDownload() 
			: data(nullptr), currentOutputPixels({nullptr, nullptr, nullptr, 0})
		{ 
			reset();
		}

		void create(uint32 width, uint32 height, uint8 numOfBuffers = 3);

		void PixelBufferDownload::queueDownloadFrom(const Framebuffer& yFramebuffer, const Framebuffer& uvFramebuffer);
		const Pixels& getPixels();

		void reset()
		{
			pixelsAreReady = false;
			downloadQueueIndex = 0;
			writeQueueIndex = 0;
			totalNumQueuedItems = 0;
			numItemsInQueue = 0;
		}

		void free();

	public:
		bool pixelsAreReady;
		uint32 numItemsInQueue;

	private:
		PixelBufferDownloadData* data;
		Pixels currentOutputPixels;
		uint8 downloadQueueIndex;
		uint8 writeQueueIndex;
		uint32 totalNumQueuedItems;
	};
}

#endif 