#ifndef MATH_ANIM_TEXTURE_H
#define MATH_ANIM_TEXTURE_H
#include "core.h"

namespace MathAnim
{
	enum class FilterMode
	{
		None = 0,
		Linear,
		Nearest
	};

	enum class WrapMode
	{
		None = 0,
		Repeat
	};

	enum class ByteFormat
	{
		None = 0,
		RGBA8_UI,
		RGBA16_F,
		RGBA32_F,

		RGB8_UI,
		RGB32_F,
		RG32_F,

		R32_UI,
		RG32_UI,
		R8_UI,
		R8_F,

		DepthStencil,
	};

	enum class ColorChannel
	{
		None = 0,
		Red,
		Green,
		Blue,
		Alpha,
		Zero,
		One
	};

	struct Texture
	{
		uint32 graphicsId;
		int32 width;
		int32 height;

		// Texture attributes
		FilterMode magFilter;
		FilterMode minFilter;
		WrapMode wrapS;
		WrapMode wrapT;
		ByteFormat format;
		ColorChannel swizzleFormat[4];

		std::filesystem::path path;

		void bind(int textureSlot) const;
		void unbind() const;
		void destroy();

		void uploadSubImage(int offsetX, int offsetY, int width, int height, uint8* buffer, size_t bufferLength, bool flipVertically = false) const;

		bool isNull() const;
	};

	typedef void (*TextureLoadedCallback)(const Texture& loadedTexture);

	class TextureBuilder
	{
	public:
		TextureBuilder();

		TextureBuilder& setMagFilter(FilterMode mode);
		TextureBuilder& setMinFilter(FilterMode mode);
		TextureBuilder& setWrapS(WrapMode mode);
		TextureBuilder& setWrapT(WrapMode mode);
		TextureBuilder& setFormat(ByteFormat format);
		TextureBuilder& setFilepath(const char* filepath);
		TextureBuilder& setWidth(uint32 width);
		TextureBuilder& setHeight(uint32 height);
		TextureBuilder& setSwizzle(std::initializer_list<ColorChannel> swizzleMask);

		Texture generateEmpty();
		Texture generateFromFile();

		/**
		 * @brief Generates a texture in a background thread. The callback is called
		 * on the main thread once the texture is finally generated and it passes
		 * the finalized texture details.
		*/
		Texture generateLazyFromFile(TextureLoadedCallback callback);

		Texture build();

	private:
		Texture texture;
		TextureLoadedCallback textureLoadedCallback = nullptr;
	};

	namespace TextureUtil
	{
		uint32 toGlSizedInternalFormat(ByteFormat format);
		uint32 toGlExternalFormat(ByteFormat format);
		uint32 toGl(WrapMode wrapMode);
		uint32 toGl(FilterMode filterMode);
		uint32 toGlDataType(ByteFormat format);
		int32 toGlSwizzle(ColorChannel colorChannel);
		size_t formatSize(ByteFormat format);

		bool byteFormatIsInt(const Texture& texture);
		bool byteFormatIsRgb(const Texture& texture);
		bool byteFormatIsUint64(const Texture& texture);

		void generateFromFile(Texture& texture);
		void generateFromFileLazy(TextureLoadedCallback callback, const Texture& texture);
		void generateEmptyTexture(Texture& texture);
	}
}

#endif
