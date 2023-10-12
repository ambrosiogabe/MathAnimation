#ifndef MATH_ANIMATIONS_TEXTURE_CACHE_H
#define MATH_ANIMATIONS_TEXTURE_CACHE_H
#include "core.h"
#include "renderer/Texture.h"

namespace MathAnim
{
	typedef uint64 TextureHandle;
	
	struct TextureLoadOptions
	{
		FilterMode magFilter;
		FilterMode minFilter;
		WrapMode wrapT;
		WrapMode wrapS;
	};

	namespace TextureCache
	{
		void init();

		// NOTE: Textures are ONLY cached according to filepath. The options
		//       may be different from load to load, but it will only use the
		//       options specified from the first load of that specific filepath.
		//       So, if you want one texture with two different sets of options,
		//       you will need to copy the image file and load a different filepath.
		TextureHandle loadTexture(
			const std::string& imageFilepath,
			const TextureLoadOptions& options = {
				FilterMode::Linear,
				FilterMode::Linear,
				WrapMode::None,
				WrapMode::None
			});
		void unloadTexture(TextureHandle handle);

		TextureHandle lazyLoadTexture(
			const std::string& imageFilepath,
			const TextureLoadOptions& options = {
					FilterMode::Linear,
					FilterMode::Linear,
					WrapMode::None,
					WrapMode::None
		});

		const Texture& getTexture(TextureHandle textureHandle);
		bool isTextureLoaded(TextureHandle textureHandle);

		void free();
	}
}

#endif 