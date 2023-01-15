#include "renderer/PixelBufferDownloader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/GLApi.h"

namespace MathAnim
{
	struct PixelBufferDownloadData
	{
		uint32* pboIds;
		size_t pboSize;
		uint8 numPbos;
		ByteFormat formatType;
	};

	void PixelBufferDownload::create(uint32 width, uint32 height, uint8 numOfBuffers)
	{
		g_logger_assert(this->data == nullptr, "Tried to create PixelBufferDownloader twice. Data was not null.");

		this->data = (PixelBufferDownloadData*)g_memory_allocate(sizeof(PixelBufferDownloadData));

		this->data->numPbos = numOfBuffers;
		this->data->pboIds = (uint32*)g_memory_allocate(sizeof(uint32) * this->data->numPbos);
		this->data->formatType = ByteFormat::R8_UI;
		// We'll store 3 color channels in 1 pbo like YYYY...UUUU...VVVV
		size_t singleColorChannelSize = width * height * sizeof(uint8);
		this->data->pboSize = singleColorChannelSize * 3;
		
		this->currentOutputPixels.dataSize = this->data->pboSize;
		this->currentOutputPixels.yColorBuffer = (uint8*)g_memory_allocate(this->data->pboSize);
		this->currentOutputPixels.uColorBuffer = this->currentOutputPixels.yColorBuffer + (singleColorChannelSize * 1);
		this->currentOutputPixels.vColorBuffer = this->currentOutputPixels.yColorBuffer + (singleColorChannelSize * 2);

		GL::genBuffers(this->data->numPbos, this->data->pboIds);
		for (uint8 i = 0; i < this->data->numPbos; i++)
		{
			GL::bindBuffer(GL_PIXEL_PACK_BUFFER, this->data->pboIds[i]);
			GL::bufferData(GL_PIXEL_PACK_BUFFER, this->data->pboSize, NULL, GL_STREAM_READ);
		}

		// Unbind pbos since this won't be called often
		GL::bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}

	void PixelBufferDownload::queueDownloadFrom(const Texture& yTexture, const Texture& uTexture, const Texture& vTexture)
	{
		if (!this->data)
		{
			return;
		}

		size_t yTextureSpaceAvailable = yTexture.width * yTexture.height * TextureUtil::formatSize(yTexture.format);
		size_t uTextureSpaceAvailable = uTexture.width * uTexture.height * TextureUtil::formatSize(uTexture.format);
		size_t vTextureSpaceAvailable = vTexture.width * vTexture.height * TextureUtil::formatSize(vTexture.format);
		size_t totalTextureSpaceAvailable = yTextureSpaceAvailable + uTextureSpaceAvailable + vTextureSpaceAvailable;
		g_logger_assert(totalTextureSpaceAvailable >= this->data->pboSize, "Texture is too small, can't transfer PBO data to this texture.");

		// Adapted from https://www.roxlu.com/2014/048/fast-pixel-transfers-with-pixel-buffer-objects
		GL::bindBuffer(GL_PIXEL_PACK_BUFFER, this->data->pboIds[this->writeQueueIndex]);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		GL::readPixels(
			0, 0, 
			yTexture.width, yTexture.height,
			GL_RED_INTEGER,
			GL_UNSIGNED_BYTE,
			0
		);
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		GL::readPixels(
			0, 0,
			uTexture.width, uTexture.height,
			GL_RED_INTEGER,
			GL_UNSIGNED_BYTE,
			(void*)(yTextureSpaceAvailable)
		);
		glReadBuffer(GL_COLOR_ATTACHMENT2);
		GL::readPixels(
			0, 0,
			vTexture.width, vTexture.height,
			GL_RED_INTEGER,
			GL_UNSIGNED_BYTE,
			(void*)(yTextureSpaceAvailable + uTextureSpaceAvailable)
		);
		this->totalNumQueuedItems++;
		this->numItemsInQueue++;
		if (this->totalNumQueuedItems >= UINT32_MAX)
		{
			this->totalNumQueuedItems = this->data->numPbos;
		}

		if (this->totalNumQueuedItems >= this->data->numPbos)
		{
			this->pixelsAreReady = true;
		}

		this->writeQueueIndex = (this->writeQueueIndex + 1) % this->data->numPbos;
	}

	const Pixels& PixelBufferDownload::getPixels()
	{
		GL::bindBuffer(GL_PIXEL_PACK_BUFFER, this->data->pboIds[this->downloadQueueIndex]);
		uint8* gpuPixelData = (uint8*)GL::mapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if (gpuPixelData)
		{
			g_memory_copyMem(this->currentOutputPixels.yColorBuffer, gpuPixelData, this->data->pboSize);
		}
		else
		{
			g_logger_error("Failed to map the pixel buffer object.");
		}

		GL::unmapBuffer(GL_PIXEL_PACK_BUFFER);

		this->numItemsInQueue--;
		if (this->numItemsInQueue <= 0)
		{
			this->pixelsAreReady = false;
		}
		this->downloadQueueIndex = (this->downloadQueueIndex + 1) % this->data->numPbos;

		return currentOutputPixels;
	}

	void PixelBufferDownload::free()
	{
		if (this->data)
		{
			if (this->data->pboIds)
			{
				GL::deleteBuffers(this->data->numPbos, this->data->pboIds);
				g_memory_free(this->data->pboIds);
			}

			g_memory_free(this->data);
		}

		if (this->currentOutputPixels.yColorBuffer)
		{
			g_memory_free(this->currentOutputPixels.yColorBuffer);
		}

		this->data = nullptr;
		this->currentOutputPixels.yColorBuffer = nullptr;
		this->currentOutputPixels.uColorBuffer = nullptr;
		this->currentOutputPixels.vColorBuffer = nullptr;
		this->currentOutputPixels.dataSize = 0;
		this->pixelsAreReady = false;
	}
}
