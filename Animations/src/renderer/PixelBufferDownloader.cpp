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
		size_t yChannelSize = width * height * sizeof(uint8);
		size_t uChannelSize = width / 2 * height / 2 * sizeof(uint8);
		size_t vChannelSize = uChannelSize;
		this->data->pboSize = yChannelSize + uChannelSize + vChannelSize;
		
		this->currentOutputPixels.dataSize = this->data->pboSize;
		this->currentOutputPixels.yColorBuffer = (uint8*)g_memory_allocate(this->data->pboSize);
		this->currentOutputPixels.uColorBuffer = this->currentOutputPixels.yColorBuffer + yChannelSize;
		this->currentOutputPixels.vColorBuffer = this->currentOutputPixels.yColorBuffer + yChannelSize + uChannelSize;

		GL::genBuffers(this->data->numPbos, this->data->pboIds);
		for (uint8 i = 0; i < this->data->numPbos; i++)
		{
			GL::bindBuffer(GL_PIXEL_PACK_BUFFER, this->data->pboIds[i]);
			GL::bufferData(GL_PIXEL_PACK_BUFFER, this->data->pboSize, NULL, GL_STREAM_READ);
		}

		// Unbind pbos since this won't be called often
		GL::bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}

	void PixelBufferDownload::queueDownloadFrom(const Framebuffer& yFramebuffer, const Framebuffer& uvFramebuffer)
	{
		if (!this->data)
		{
			return;
		}

		size_t yChannelSize = yFramebuffer.width * yFramebuffer.height * TextureUtil::formatSize(yFramebuffer.colorAttachments.at(0).format);
		size_t uChannelSize = uvFramebuffer.width * uvFramebuffer.height * TextureUtil::formatSize(uvFramebuffer.colorAttachments.at(0).format);
		size_t vChannelSize = uChannelSize;
		size_t totalTextureSpaceAvailable = yChannelSize + uChannelSize + vChannelSize;
		g_logger_assert(totalTextureSpaceAvailable >= this->data->pboSize, "Texture is too small, can't transfer PBO data to this texture.");

		// Adapted from https://www.roxlu.com/2014/048/fast-pixel-transfers-with-pixel-buffer-objects
		yFramebuffer.bind();
		GL::bindBuffer(GL_PIXEL_PACK_BUFFER, this->data->pboIds[this->writeQueueIndex]);
		GL::readBuffer(GL_COLOR_ATTACHMENT0);
		GL::readPixels(
			0, 0, 
			yFramebuffer.width, yFramebuffer.height,
			GL_RED_INTEGER,
			GL_UNSIGNED_BYTE,
			0 // Read into pbo[0]
		);
		uvFramebuffer.bind();
		GL::readBuffer(GL_COLOR_ATTACHMENT0);
		GL::readPixels(
			0, 0,
			uvFramebuffer.width, uvFramebuffer.height,
			GL_RED_INTEGER,
			GL_UNSIGNED_BYTE,
			(void*)(yChannelSize) // Read into pbo[yTextureSpaceAvailable]
		);
		GL::readBuffer(GL_COLOR_ATTACHMENT1);
		GL::readPixels(
			0, 0,
			uvFramebuffer.width, uvFramebuffer.height,
			GL_RED_INTEGER,
			GL_UNSIGNED_BYTE,
			(void*)(yChannelSize + uChannelSize) // Read into pbo[yTextureSpaceAvailable + uTexAvail]
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

		// Unbind pixel pack unpack buffer so we don't accidentally transfer pixels here
		GL::bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
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

		// Unbind pixel pack unpack buffer so we don't accidentally transfer pixels here
		GL::bindBuffer(GL_PIXEL_PACK_BUFFER, 0);

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
