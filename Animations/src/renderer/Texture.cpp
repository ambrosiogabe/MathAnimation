#include "renderer/Texture.h"
#include "renderer/GLApi.h"
#include "core/Application.h"
#include "multithreading/GlobalThreadPool.h"
#include "editor/panels/ErrorPopups.h"

#include <stb/stb_image.h>

namespace MathAnim
{
	static const uint32 NULL_TEXTURE_ID = UINT32_MAX;

	struct LazyLoadData
	{
		Texture texture;
		TextureLoadedCallback callback;
		unsigned char* decodedPixels;
		char* errorMessage;
	};

	static void bindTextureParameters(const Texture& texture);

	// ========================================================
	// 	   Texture Builder
	// ========================================================
	TextureBuilder::TextureBuilder()
	{
		// Set default values for a texture
		texture.minFilter = FilterMode::Linear;
		texture.magFilter = FilterMode::Linear;
		texture.wrapS = WrapMode::None;
		texture.wrapT = WrapMode::None;
		texture.graphicsId = NULL_TEXTURE_ID;
		texture.width = 0;
		texture.height = 0;
		texture.format = ByteFormat::None;
		texture.path = std::filesystem::path();
		texture.swizzleFormat[0] = ColorChannel::Red;
		texture.swizzleFormat[1] = ColorChannel::Green;
		texture.swizzleFormat[2] = ColorChannel::Blue;
		texture.swizzleFormat[3] = ColorChannel::Alpha;
	}

	TextureBuilder& TextureBuilder::setMagFilter(FilterMode mode)
	{
		texture.magFilter = mode;
		return *this;
	}

	TextureBuilder& TextureBuilder::setMinFilter(FilterMode mode)
	{
		texture.minFilter = mode;
		return *this;
	}

	TextureBuilder& TextureBuilder::setWrapS(WrapMode mode)
	{
		texture.wrapS = mode;
		return *this;
	}

	TextureBuilder& TextureBuilder::setWrapT(WrapMode mode)
	{
		texture.wrapT = mode;
		return *this;
	}

	TextureBuilder& TextureBuilder::setFormat(ByteFormat format)
	{
		texture.format = format;
		return *this;
	}

	TextureBuilder& TextureBuilder::setFilepath(const char* filepath)
	{
		texture.path = filepath;
		return *this;
	}

	TextureBuilder& TextureBuilder::setWidth(uint32 width)
	{
		texture.width = width;
		return *this;
	}

	TextureBuilder& TextureBuilder::setHeight(uint32 height)
	{
		texture.height = height;
		return *this;
	}

	TextureBuilder& TextureBuilder::setSwizzle(std::initializer_list<ColorChannel> swizzleMask)
	{
		g_logger_assert(swizzleMask.size() == 4, "Must set swizzle mask to { R, G, B, A } format. Size must be 4.");
		texture.swizzleFormat[0] = *(swizzleMask.begin() + 0);
		texture.swizzleFormat[1] = *(swizzleMask.begin() + 1);
		texture.swizzleFormat[2] = *(swizzleMask.begin() + 2);
		texture.swizzleFormat[3] = *(swizzleMask.begin() + 3);
		return *this;
	}

	Texture TextureBuilder::generateEmpty()
	{
		TextureUtil::generateEmptyTexture(texture);
		return texture;
	}

	Texture TextureBuilder::generateFromFile()
	{
		TextureUtil::generateFromFile(texture);
		return texture;
	}

	Texture TextureBuilder::generateLazyFromFile(TextureLoadedCallback callback)
	{
		// This runs async
		TextureUtil::generateFromFileLazy(callback, texture);

		// Return a copy, the callback will be called once TextureUtil finishes processing it on a background thread
		Texture copy = texture;
		return copy;
	}

	Texture TextureBuilder::build()
	{
		return texture;
	}

	// ========================================================
	// 	   Texture Member Functions
	// ========================================================
	void Texture::bind(int textureSlot) const
	{
		GL::bindTexSlot(GL_TEXTURE_2D, graphicsId, textureSlot);
	}

	void Texture::unbind() const
	{
		GL::unbindTexture(GL_TEXTURE_2D);
	}

	void Texture::destroy()
	{
		GL::deleteTextures(1, &graphicsId);
		graphicsId = NULL_TEXTURE_ID;
	}

	void Texture::uploadSubImage(int offsetX, int offsetY, int subWidth, int subHeight, uint8* buffer, size_t bufferLength, bool flipVertically) const
	{
		g_logger_assert(format != ByteFormat::None, "Cannot generate texture without color format.");
		g_logger_assert(offsetX + subWidth <= this->width, "Sub-image out of range. OffsetX + width = {} which is greater than the texture width: {}", offsetX + subWidth, this->width);
		g_logger_assert(offsetY + subHeight <= this->height, "Sub-image out of range. OffsetY + height = {} which is greater than the texture height: {}", offsetY + subHeight, this->height);
		g_logger_assert(offsetX >= 0, "Sub-image out of range. OffsetX is negative: {}", offsetX);
		g_logger_assert(offsetY >= 0, "Sub-image out of range. OffsetY is negative: {}", offsetY);
		g_logger_assert(subWidth >= 0, "Sub-image out of range. Width is negative: {}", subWidth);
		g_logger_assert(subHeight >= 0, "Sub-image out of range. Height is negative: {}", subHeight);

		uint32 externalFormat = TextureUtil::toGlExternalFormat(format);
		uint32 dataType = TextureUtil::toGlDataType(format);
		size_t componentsSize = TextureUtil::formatSize(format);

		g_logger_assert(componentsSize * subWidth * subHeight <= bufferLength, "Buffer overrun when trying to upload texture subimage to GPU.");

		if (flipVertically)
		{
			int stride = (int)(subWidth * componentsSize);
			size_t newBufferSize = stride * subHeight;
			uint8* newBuffer = (uint8*)g_memory_allocate(newBufferSize);
			for (int i = 0; i < subHeight; i++)
			{
				uint8* dst = newBuffer + (i * stride);
				size_t newBufferSizeLeft = newBufferSize - (dst - newBuffer);
				g_memory_copyMem(dst, newBufferSizeLeft, buffer + ((subHeight - i - 1) * stride), stride);
			}
			buffer = newBuffer;
		}

		GL::bindTexture(GL_TEXTURE_2D, this->graphicsId);
		GL::texSubImage2D(GL_TEXTURE_2D, 0, offsetX, offsetY, subWidth, subHeight, externalFormat, dataType, buffer);

		if (flipVertically)
		{
			g_memory_free(buffer);
		}
	}

	bool Texture::isNull() const
	{
		return graphicsId == NULL_TEXTURE_ID;
	}

	// ========================================================
	// 	   Texture Utilities
	// ========================================================
	namespace TextureUtil
	{
		uint32 toGl(WrapMode wrapMode)
		{
			switch (wrapMode)
			{
			case WrapMode::Repeat:
				return GL_REPEAT;
			case WrapMode::None:
				return GL_NONE;
			}

			return GL_NONE;
		}

		uint32 toGl(FilterMode filterMode)
		{
			switch (filterMode)
			{
			case FilterMode::Linear:
				return GL_LINEAR;
			case FilterMode::Nearest:
				return GL_NEAREST;
			case FilterMode::None:
				return GL_NONE;
			}

			return GL_NONE;
		}

		uint32 toGlSizedInternalFormat(ByteFormat format)
		{
			switch (format)
			{
			case ByteFormat::RGBA8_UI:
				return GL_RGBA8;
			case ByteFormat::RGBA16_F:
				return GL_RGBA16F;
			case ByteFormat::RGBA32_F:
				return GL_RGBA32F;
			case ByteFormat::RGB8_UI:
				return GL_RGB8;
			case ByteFormat::RGB32_F:
				return GL_RGB32F;
			case ByteFormat::RG32_F:
				return GL_RG32F;
			case ByteFormat::R32_UI:
				return GL_R32UI;
			case ByteFormat::RG32_UI:
				return GL_RG32UI;
			case ByteFormat::R8_UI:
				return GL_R8UI;
			case ByteFormat::R8_F:
				return GL_R8;
			case ByteFormat::DepthStencil:
				return GL_DEPTH24_STENCIL8;
			case ByteFormat::None:
				return GL_NONE;
			}

			return GL_NONE;
		}

		uint32 toGlExternalFormat(ByteFormat format)
		{
			switch (format)
			{
			case ByteFormat::RGBA8_UI:
				return GL_RGBA;
			case ByteFormat::RGBA16_F:
				return GL_RGBA;
			case ByteFormat::RGBA32_F:
				return GL_RGBA;
			case ByteFormat::RGB8_UI:
				return GL_RGB;
			case ByteFormat::RGB32_F:
				return GL_RGB;
			case ByteFormat::RG32_F:
				return GL_RG;
			case ByteFormat::R32_UI:
				return GL_RED_INTEGER;
			case ByteFormat::RG32_UI:
				return GL_RG_INTEGER;
			case ByteFormat::R8_UI:
				return GL_RED_INTEGER;
			case ByteFormat::R8_F:
				return GL_RED;
			case ByteFormat::DepthStencil:
				return GL_DEPTH_STENCIL;
			case ByteFormat::None:
				return GL_NONE;
			}

			return GL_NONE;
		}

		uint32 toGlDataType(ByteFormat format)
		{
			switch (format)
			{
			case ByteFormat::RGBA8_UI:
				return GL_UNSIGNED_BYTE;
			case ByteFormat::RGBA16_F:
				return GL_HALF_FLOAT;
			case ByteFormat::RGBA32_F:
				return GL_FLOAT;
			case ByteFormat::RGB8_UI:
				return GL_UNSIGNED_BYTE;
			case ByteFormat::RGB32_F:
				return GL_FLOAT;
			case ByteFormat::RG32_F:
				return GL_FLOAT;
			case ByteFormat::R32_UI:
				return GL_UNSIGNED_INT;
			case ByteFormat::RG32_UI:
				return GL_UNSIGNED_INT;
			case ByteFormat::R8_UI:
				return GL_UNSIGNED_BYTE;
			case ByteFormat::R8_F:
				return GL_FLOAT;
			case ByteFormat::DepthStencil:
				return GL_UNSIGNED_INT_24_8;
			case ByteFormat::None:
				return GL_NONE;
			}

			return GL_NONE;
		}

		bool byteFormatIsInt(const Texture& texture)
		{
			switch (texture.format)
			{
			case ByteFormat::RGBA8_UI:
				return false;
			case ByteFormat::RGBA16_F:
				return false;
			case ByteFormat::RGBA32_F:
				return false;
			case ByteFormat::RGB8_UI:
				return false;
			case ByteFormat::RGB32_F:
				return false;
			case ByteFormat::RG32_F:
				return false;
			case ByteFormat::R32_UI:
				return true;
			case ByteFormat::RG32_UI:
				return false;
			case ByteFormat::R8_UI:
				return true;
			case ByteFormat::None:
				return false;
			case ByteFormat::DepthStencil:
				return false;
			case ByteFormat::R8_F:
				return false;
			}

			return false;

		}

		bool byteFormatIsRgb(const Texture& texture)
		{
			switch (texture.format)
			{
			case ByteFormat::RGBA8_UI:
				return true;
			case ByteFormat::RGBA16_F:
				return true;
			case ByteFormat::RGBA32_F:
				return true;
			case ByteFormat::RGB8_UI:
				return true;
			case ByteFormat::RGB32_F:
				return true;
			case ByteFormat::RG32_F:
				return false;
			case ByteFormat::R32_UI:
				return false;
			case ByteFormat::RG32_UI:
				return false;
			case ByteFormat::R8_UI:
				return false;
			case ByteFormat::None:
				return false;
			case ByteFormat::DepthStencil:
				return false;
			case ByteFormat::R8_F:
				return false;
			}

			return false;
		}

		bool byteFormatIsUint64(const Texture& texture)
		{
			switch (texture.format)
			{
			case ByteFormat::RGBA8_UI:
				return false;
			case ByteFormat::RGBA16_F:
				return false;
			case ByteFormat::RGBA32_F:
				return false;
			case ByteFormat::RGB8_UI:
				return false;
			case ByteFormat::RGB32_F:
				return false;
			case ByteFormat::RG32_F:
				return false;
			case ByteFormat::R32_UI:
				return false;
			case ByteFormat::RG32_UI:
				return true;
			case ByteFormat::R8_UI:
				return false;
			case ByteFormat::None:
				return false;
			case ByteFormat::DepthStencil:
				return false;
			case ByteFormat::R8_F:
				return false;
			}

			return false;
		}

		int32 toGlSwizzle(ColorChannel colorChannel)
		{
			switch (colorChannel)
			{
			case ColorChannel::Blue:
				return GL_BLUE;
			case ColorChannel::Green:
				return GL_GREEN;
			case ColorChannel::Red:
				return GL_RED;
			case ColorChannel::One:
				return GL_ONE;
			case ColorChannel::Zero:
				return GL_ZERO;
			case ColorChannel::Alpha:
				return GL_ALPHA;
			case ColorChannel::None:
				return GL_NONE;
			}

			return GL_ONE;
		}

		size_t formatSize(ByteFormat format)
		{
			switch (format)
			{
			case ByteFormat::RGBA8_UI:
				return sizeof(uint8) * 4;
			case ByteFormat::RGBA16_F:
				return sizeof(uint16) * 4;
			case ByteFormat::RGBA32_F:
				return sizeof(float) * 4;
			case ByteFormat::RGB8_UI:
				return sizeof(uint8) * 3;
			case ByteFormat::RGB32_F:
				return sizeof(float) * 3;
			case ByteFormat::RG32_F:
				return sizeof(float) * 2;
			case ByteFormat::R32_UI:
				return sizeof(uint32);
			case ByteFormat::RG32_UI:
				return sizeof(uint32) * 2;
			case ByteFormat::R8_UI:
				return sizeof(uint8);
			case ByteFormat::R8_F:
				return sizeof(uint8);
			case ByteFormat::None:
			case ByteFormat::DepthStencil:
				return 0;
			}

			return 0;
		}

		void generateFromFile(Texture& texture)
		{
			g_logger_assert(texture.path != "", "Cannot generate texture from file without a filepath provided.");
			int channels;

			stbi_set_flip_vertically_on_load(true);
			unsigned char* pixels = stbi_load(texture.path.string().c_str(), &texture.width, &texture.height, &channels, 0);
			
			if (pixels == nullptr)
			{
				g_logger_error("STB failed to load image: '{}'\n-> STB Failure Reason: '{}'", texture.path, stbi_failure_reason());
			}

			int bytesPerPixel = channels;
			if (bytesPerPixel == 4)
			{
				texture.format = ByteFormat::RGBA8_UI;
			}
			else if (bytesPerPixel == 3)
			{
				texture.format = ByteFormat::RGB8_UI;
			}
			else
			{
				g_logger_warning("Unknown number of channels '{}' in image '{}'.", texture.path, channels);
				stbi_image_free(pixels);
				return;
			}

			GL::genTextures(1, &texture.graphicsId);
			GL::bindTexture(GL_TEXTURE_2D, texture.graphicsId);

			bindTextureParameters(texture);

			uint32 internalFormat = TextureUtil::toGlSizedInternalFormat(texture.format);
			uint32 externalFormat = TextureUtil::toGlExternalFormat(texture.format);
			g_logger_assert(internalFormat != GL_NONE && externalFormat != GL_NONE, "Tried to load image from file, but failed to identify internal format for image '{}'", texture.path);
			GL::pixelStorei(GL_UNPACK_ALIGNMENT, 1);
			GL::texImage2D(GL_TEXTURE_2D, 0, internalFormat, texture.width, texture.height, 0, externalFormat, GL_UNSIGNED_BYTE, pixels);

			stbi_image_free(pixels);
		}

		// Called async
		static void async_generateFromFileLazy(void* data, size_t dataSize)
		{
			g_logger_assert(dataSize == sizeof(LazyLoadData), "Invalid data passed to callback.");

			LazyLoadData& lazyLoad = (*(LazyLoadData*)data);
			Texture& texture = lazyLoad.texture;

			g_logger_assert(texture.path != "", "Cannot generate texture from file without a filepath provided.");
			int channels;

			stbi_set_flip_vertically_on_load(true);
			lazyLoad.decodedPixels = stbi_load(texture.path.string().c_str(), &texture.width, &texture.height, &channels, 0);

			if (lazyLoad.decodedPixels == nullptr)
			{
				g_logger_error("STB failed to load image: '{}'\n-> STB Failure Reason: '{}'", texture.path, stbi_failure_reason());
				const char* errorMessage = stbi_failure_reason();
				size_t errorMessageLength = std::strlen(errorMessage);
				lazyLoad.errorMessage = (char*)g_memory_allocate(sizeof(char) * (errorMessageLength + 1));
				g_memory_copyMem(lazyLoad.errorMessage, sizeof(char) * (errorMessageLength + 1), (void*)errorMessage, sizeof(char) * errorMessageLength);
				lazyLoad.errorMessage[errorMessageLength] = '\0';
				return;
			}

			int bytesPerPixel = channels;
			if (bytesPerPixel == 4)
			{
				texture.format = ByteFormat::RGBA8_UI;
			}
			else if (bytesPerPixel == 3)
			{
				texture.format = ByteFormat::RGB8_UI;
			}
			else
			{
				stbi_image_free(lazyLoad.decodedPixels);
				texture.format = ByteFormat::None;
				g_logger_error("Unknown number of channels '{}' in image '{}'.", texture.path, channels);
				return;
			}
		}

		// Called from main thread
		static void sync_generateFromFileLazyFinished(void* data, size_t dataSize)
		{
			g_logger_assert(dataSize == sizeof(LazyLoadData), "Invalid data passed to callback.");

			LazyLoadData& lazyLoad = (*(LazyLoadData*)data);
			Texture& texture = lazyLoad.texture;

			if (texture.format == ByteFormat::None || lazyLoad.decodedPixels == nullptr)
			{
				ErrorPopups::popupTextureLoadError(lazyLoad.texture.path.string(), lazyLoad.errorMessage);
				g_memory_free(lazyLoad.errorMessage);
				lazyLoad.errorMessage = nullptr;

				g_memory_delete((LazyLoadData*)data);
				return;
			}

			GL::genTextures(1, &texture.graphicsId);
			GL::bindTexture(GL_TEXTURE_2D, texture.graphicsId);

			bindTextureParameters(texture);

			uint32 internalFormat = TextureUtil::toGlSizedInternalFormat(texture.format);
			uint32 externalFormat = TextureUtil::toGlExternalFormat(texture.format);
			g_logger_assert(internalFormat != GL_NONE && externalFormat != GL_NONE, "Tried to load image from file, but failed to identify internal format for image '{}'", texture.path);
			GL::pixelStorei(GL_UNPACK_ALIGNMENT, 1);
			GL::texImage2D(GL_TEXTURE_2D, 0, internalFormat, texture.width, texture.height, 0, externalFormat, GL_UNSIGNED_BYTE, lazyLoad.decodedPixels);

			stbi_image_free(lazyLoad.decodedPixels);

			lazyLoad.callback(texture);

			g_memory_delete((LazyLoadData*)data);
		}

		void generateFromFileLazy(TextureLoadedCallback textureLoadedCallback, const Texture& texture)
		{
			LazyLoadData* lazyLoad = g_memory_new LazyLoadData();

			lazyLoad->callback = textureLoadedCallback;
			lazyLoad->texture = texture;
			Application::threadPool()->queueTask(
				async_generateFromFileLazy,
				"async_generateFromFileLazy",
				lazyLoad,
				sizeof(LazyLoadData),
				Priority::Medium,
				sync_generateFromFileLazyFinished
			);
		}

		void generateEmptyTexture(Texture& texture)
		{
			g_logger_assert(texture.format != ByteFormat::None, "Cannot generate texture without color format.");
			GL::genTextures(1, &texture.graphicsId);
			GL::bindTexture(GL_TEXTURE_2D, texture.graphicsId);

			bindTextureParameters(texture);

			uint32 internalFormat = TextureUtil::toGlSizedInternalFormat(texture.format);
			uint32 externalFormat = TextureUtil::toGlExternalFormat(texture.format);
			uint32 dataType = TextureUtil::toGlDataType(texture.format);

			// Here the GL_UNSIGNED_BYTE does nothing since we are just allocating space
			GL::texImage2D(GL_TEXTURE_2D, 0, internalFormat, texture.width, texture.height, 0, externalFormat, dataType, nullptr);
		}
	}

	// ========================================================
	// 	   Internal helper functions
	// ========================================================
	static void bindTextureParameters(const Texture& texture)
	{
		if (texture.wrapS != WrapMode::None)
		{
			GL::texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TextureUtil::toGl(texture.wrapS));
		}
		if (texture.wrapT != WrapMode::None)
		{
			GL::texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TextureUtil::toGl(texture.wrapT));
		}
		if (texture.minFilter != FilterMode::None)
		{
			GL::texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureUtil::toGl(texture.minFilter));
		}
		if (texture.magFilter != FilterMode::None)
		{
			GL::texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TextureUtil::toGl(texture.magFilter));
		}

		GLint swizzleMask[4] = {
			TextureUtil::toGlSwizzle(texture.swizzleFormat[0]),
			TextureUtil::toGlSwizzle(texture.swizzleFormat[1]),
			TextureUtil::toGlSwizzle(texture.swizzleFormat[2]),
			TextureUtil::toGlSwizzle(texture.swizzleFormat[3])
		};
		GL::texParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	}
}