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
		RGBA,
		RGBA8,

		RGB,
		RGB8,

		R32UI,
		RED_INTEGER,

		// Depth/Stencil formats
		DEPTH24_STENCIL8
	};

	struct Texture
	{
		uint32 graphicsId = (uint32)-1;
		int32 width = 0;
		int32 height = 0;

		// Texture attributes
		FilterMode magFilter = FilterMode::None;
		FilterMode minFilter = FilterMode::None;
		WrapMode wrapS = WrapMode::None;
		WrapMode wrapT = WrapMode::None;
		ByteFormat internalFormat = ByteFormat::None;
		ByteFormat externalFormat = ByteFormat::None;

		std::filesystem::path path = std::filesystem::path();
		bool isDefault = false;
	};

	namespace TextureUtil
	{
		// Namespace variables
		// NOTE: To make sure this variable is visible to other translation units, declare it as extern
		extern const Texture NULL_TEXTURE;

		void bind(const Texture& texture);
		void unbind(const Texture& texture);
		void destroy(Texture& texture);

		// Loads a texture using stb library and generates a texutre using the filter/wrap modes and automatically detects
		// internal/external format, width, height, and alpha channel
		void Generate(Texture& texture, const std::filesystem::path& filepath);

		// Allocates memory space on the GPU according to the texture specifications listed here
		void generate(Texture& texture);

		bool isNull(const Texture& texture);

		uint32 toGl(ByteFormat format);
		uint32 toGl(WrapMode wrapMode);
		uint32 toGl(FilterMode filterMode);
		uint32 toGlDataType(ByteFormat format);
		bool byteFormatIsInt(ByteFormat format);
		bool byteFormatIsRgb(ByteFormat format);
	}
}

#endif
