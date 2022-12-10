#include "renderer/Texture.h"

#include <stb/stb_image.h>

namespace MathAnim
{
	static const uint32 NULL_TEXTURE_ID = UINT32_MAX;

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

	Texture TextureBuilder::generate(bool generateFromFilepath)
	{
		if (generateFromFilepath)
		{
			TextureUtil::generateFromFile(texture);
		}
		else
		{
			TextureUtil::generateEmptyTexture(texture);
		}

		return texture;
	}

	Texture TextureBuilder::build()
	{
		return texture;
	}

	// ========================================================
	// 	   Texture Member Functions
	// ========================================================
	void Texture::bind() const
	{
		glBindTexture(GL_TEXTURE_2D, graphicsId);
	}

	void Texture::unbind() const
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::destroy()
	{
		glDeleteTextures(1, &graphicsId);
		graphicsId = NULL_TEXTURE_ID;
	}

	void Texture::copyTo(Texture& texture) const
	{
		g_logger_assert(this->width == texture.width && this->height == texture.height, "Cannot copy texture to a different sized texture.");

		glCopyImageSubData(
			this->graphicsId, GL_TEXTURE_2D, 0, 0, 0, 0,
			texture.graphicsId, GL_TEXTURE_2D, 0, 0, 0, 0,
			this->width, this->height, 0
		);
	}

	void Texture::uploadSubImage(int offsetX, int offsetY, int width, int height, uint8* buffer, size_t bufferLength, bool flipVertically) const
	{
		g_logger_assert(format != ByteFormat::None, "Cannot generate texture without color format.");
		g_logger_assert(offsetX + width <= this->width, "Sub-image out of range. OffsetX + width = %d which is greater than the texture width: %d", offsetX + width, this->width);
		g_logger_assert(offsetY + height <= this->height, "Sub-image out of range. OffsetY + height = %d which is greater than the texture height: %d", offsetY + height, this->height);
		g_logger_assert(offsetX >= 0, "Sub-image out of range. OffsetX is negative: %d", offsetX);
		g_logger_assert(offsetY >= 0, "Sub-image out of range. OffsetY is negative: %d", offsetY);
		g_logger_assert(width >= 0, "Sub-image out of range. Width is negative: %d", width);
		g_logger_assert(height >= 0, "Sub-image out of range. Height is negative: %d", height);

		uint32 externalFormat = TextureUtil::toGlExternalFormat(format);
		uint32 dataType = TextureUtil::toGlDataType(format);
		size_t componentsSize = TextureUtil::formatSize(format);

		g_logger_assert(componentsSize * width * height <= bufferLength, "Buffer overrun when trying to upload texture subimage to GPU.");

		if (flipVertically)
		{
			int stride = (int)(width * componentsSize);
			uint8* newBuffer = (uint8*)g_memory_allocate(stride * height);
			for (int i = 0; i < height; i++)
			{
				g_memory_copyMem(newBuffer + (i * stride), buffer + ((height - i - 1) * stride), stride);
			}
			buffer = newBuffer;
		}

		this->bind();
		glTexSubImage2D(GL_TEXTURE_2D, 0, offsetX, offsetY, width, height, externalFormat, dataType, buffer);

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
			case ByteFormat::RGB8_UI:
				return GL_RGB8;
			case ByteFormat::R32_UI:
				return GL_R32UI;
			case ByteFormat::RG32_UI:
				return GL_RG32UI;
			case ByteFormat::R8_UI:
				return GL_R8UI;
			case ByteFormat::R8_F:
				return GL_R8;
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
			case ByteFormat::RGB8_UI:
				return GL_RGB;
			case ByteFormat::R32_UI:
				return GL_RED_INTEGER;
			case ByteFormat::RG32_UI:
				return GL_RG_INTEGER;
			case ByteFormat::R8_UI:
				return GL_RED_INTEGER;
			case ByteFormat::R8_F:
				return GL_RED;
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
			case ByteFormat::RGB8_UI:
				return GL_UNSIGNED_BYTE;
			case ByteFormat::R32_UI:
				return GL_UNSIGNED_INT;
			case ByteFormat::RG32_UI:
				return GL_UNSIGNED_INT;
			case ByteFormat::R8_UI:
				return GL_UNSIGNED_BYTE;
			case ByteFormat::R8_F:
				return GL_FLOAT;
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
			case ByteFormat::RGB8_UI:
				return false;
			case ByteFormat::R32_UI:
				return true;
			case ByteFormat::RG32_UI:
				return false;
			case ByteFormat::R8_UI:
				return true;
			case ByteFormat::None:
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
			case ByteFormat::RGB8_UI:
				return true;
			case ByteFormat::R32_UI:
				return false;
			case ByteFormat::RG32_UI:
				return false;
			case ByteFormat::R8_UI:
				return false;
			case ByteFormat::None:
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
			case ByteFormat::RGB8_UI:
				return false;
			case ByteFormat::R32_UI:
				return false;
			case ByteFormat::RG32_UI:
				return true;
			case ByteFormat::R8_UI:
				return false;
			case ByteFormat::None:
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
			case ByteFormat::RGB8_UI:
				return sizeof(uint8) * 3;
			case ByteFormat::R32_UI:
				return sizeof(uint32);
			case ByteFormat::RG32_UI:
				return sizeof(uint32) * 2;
			case ByteFormat::R8_UI:
				return sizeof(uint8);
			case ByteFormat::R8_F:
				return sizeof(uint8);
			case ByteFormat::None:
				return 0;
			}

			return 0;
		}

		void generateFromFile(Texture& texture)
		{
			g_logger_assert(texture.path != "", "Cannot generate texture from file without a filepath provided.");
			int channels;

			unsigned char* pixels = stbi_load(texture.path.string().c_str(), &texture.width, &texture.height, &channels, 0);
			g_logger_assert((pixels != nullptr), "STB failed to load image: %s\n-> STB Failure Reason: %s", texture.path.string().c_str(), stbi_failure_reason());

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
				g_logger_warning("Unknown number of channels '%d' in image '%s'.", texture.path.string().c_str(), channels);
				return;
			}

			glGenTextures(1, &texture.graphicsId);
			glBindTexture(GL_TEXTURE_2D, texture.graphicsId);

			bindTextureParameters(texture);

			uint32 internalFormat = TextureUtil::toGlSizedInternalFormat(texture.format);
			uint32 externalFormat = TextureUtil::toGlExternalFormat(texture.format);
			g_logger_assert(internalFormat != GL_NONE && externalFormat != GL_NONE, "Tried to load image from file, but failed to identify internal format for image '%s'", texture.path.string().c_str());
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texture.width, texture.height, 0, externalFormat, GL_UNSIGNED_BYTE, pixels);

			stbi_image_free(pixels);
		}

		void generateEmptyTexture(Texture& texture)
		{
			g_logger_assert(texture.format != ByteFormat::None, "Cannot generate texture without color format.");
			glGenTextures(1, &texture.graphicsId);
			glBindTexture(GL_TEXTURE_2D, texture.graphicsId);

			bindTextureParameters(texture);

			uint32 internalFormat = TextureUtil::toGlSizedInternalFormat(texture.format);
			uint32 externalFormat = TextureUtil::toGlExternalFormat(texture.format);
			uint32 dataType = TextureUtil::toGlDataType(texture.format);

			// Here the GL_UNSIGNED_BYTE does nothing since we are just allocating space
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texture.width, texture.height, 0, externalFormat, dataType, nullptr);
		}
	}

	// ========================================================
	// 	   Internal helper functions
	// ========================================================
	static void bindTextureParameters(const Texture& texture)
	{
		if (texture.wrapS != WrapMode::None)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TextureUtil::toGl(texture.wrapS));
		}
		if (texture.wrapT != WrapMode::None)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TextureUtil::toGl(texture.wrapT));
		}
		if (texture.minFilter != FilterMode::None)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureUtil::toGl(texture.minFilter));
		}
		if (texture.magFilter != FilterMode::None)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TextureUtil::toGl(texture.magFilter));
		}

		GLint swizzleMask[4] = { 
			TextureUtil::toGlSwizzle(texture.swizzleFormat[0]),
			TextureUtil::toGlSwizzle(texture.swizzleFormat[1]), 
			TextureUtil::toGlSwizzle(texture.swizzleFormat[2]), 
			TextureUtil::toGlSwizzle(texture.swizzleFormat[3]) 
		};
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	}
}