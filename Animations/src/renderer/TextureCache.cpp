#include "renderer/TextureCache.h"

namespace MathAnim
{
	struct CachedTexture
	{
		Texture texture;
		uint32 refCount;
		std::filesystem::path absPath;
	};

	namespace TextureCache
	{
		static std::unordered_map<std::filesystem::path, TextureHandle> cachedTexturePaths = {};
		static std::unordered_map<TextureHandle, CachedTexture> cachedTextures = {};

		static std::unordered_map<TextureHandle, std::filesystem::path> deadTextures = {};
		static uint64 textureHandleCounter;

		// -------------------- Internal Functions --------------------
		static inline std::filesystem::path stringToAbsPath(const std::string& path) { return std::filesystem::absolute(path).make_preferred(); }
		static TextureHandle getCachedTextureFor(const std::filesystem::path& absPath);
		static TextureHandle cacheTexture(const std::filesystem::path& absolutePath, const Texture& texture);

		void init()
		{
			cachedTexturePaths = {};
			cachedTextures = {};
			textureHandleCounter = 0;
		}

		TextureHandle loadTexture(const std::string& imageFilepath, const TextureLoadOptions& options)
		{
			std::filesystem::path absolutePath = stringToAbsPath(imageFilepath);
			TextureHandle cachedHandle = getCachedTextureFor(absolutePath);
			if (cachedHandle != NULL_TEXTURE_HANDLE)
			{
				return cachedHandle;
			}

			g_logger_info("Caching texture '{}'", absolutePath);

			// Load the texture since it does not exist
			Texture texture = TextureBuilder()
				.setFilepath(imageFilepath.c_str())
				.setMagFilter(options.magFilter)
				.setMinFilter(options.minFilter)
				.setWrapS(options.wrapS)
				.setWrapT(options.wrapT)
				.generateFromFile();

			return cacheTexture(absolutePath, texture);
		}

		static void finishLoadingTexture(const Texture& texture)
		{
			for (auto& [key, value] : cachedTextures)
			{
				if (value.texture.path == texture.path)
				{
					value.texture.graphicsId = texture.graphicsId;
					return;
				}
			}
		}

		TextureHandle lazyLoadTexture(const std::string& imageFilepath, const TextureLoadOptions& options)
		{
			std::filesystem::path absolutePath = stringToAbsPath(imageFilepath);
			TextureHandle cachedHandle = getCachedTextureFor(absolutePath);
			if (cachedHandle != NULL_TEXTURE_HANDLE)
			{
				return cachedHandle;
			}

			g_logger_info("Lazy loading texture '{}'", absolutePath);

			Texture texture = TextureBuilder()
				.setFilepath(imageFilepath.c_str())
				.setMagFilter(options.magFilter)
				.setMinFilter(options.minFilter)
				.setWrapS(options.wrapS)
				.setWrapT(options.wrapT)
				.generateLazyFromFile(finishLoadingTexture);

			return cacheTexture(absolutePath, texture);
		}

		void unloadTexture(TextureHandle handle)
		{
			if (isNull(handle))
			{
				return;
			}

			auto textureIter = cachedTextures.find(handle);
			if (textureIter != cachedTextures.end())
			{
				// NOTE: This should never be hit because of the next block of statements
				//       but it's good to have the assertion documented in code since this
				//       isn't in a critical path
				if (textureIter->second.refCount == 0)
				{
					g_logger_error("Attempted to unload a texture that has already been unloaded. Texture: '{}'", textureIter->second.absPath);
					return;
				}

				textureIter->second.refCount--;
				if (textureIter->second.refCount == 0)
				{
					g_logger_info("Unloading cached texture '{}'", textureIter->second.absPath);

					// Delete the filepath -> handle mapping
					auto handleIter = cachedTexturePaths.find(textureIter->second.absPath);
					if (handleIter != cachedTexturePaths.end())
					{
						cachedTexturePaths.erase(handleIter);
					}

					// Store the dead texture filepath for recall when debugging
					deadTextures[handle] = textureIter->second.absPath;

					// Delete the actual GPU texture and the handle -> texture mapping
					textureIter->second.texture.destroy();
					cachedTextures.erase(textureIter);
				}
			}
			else
			{
				auto deadTextureHandle = deadTextures.find(handle);
				if (deadTextureHandle != deadTextures.end())
				{
					g_logger_warning("Tried to unload a dead texture. The texture has already been unloaded '{}'.", deadTextureHandle->second);
				}
				else
				{
					g_logger_warning("Tried to unload unknown texture handle '{}'", handle);
				}
			}
		}

		const Texture& getTexture(TextureHandle textureHandle)
		{
			auto textureHandleIter = cachedTextures.find(textureHandle);
			if (textureHandleIter != cachedTextures.end())
			{
				return textureHandleIter->second.texture;
			}

			if (!isNull(textureHandle))
			{
				g_logger_error("Texture with handle '{}' not cached!", textureHandle);
			}
			static Texture dummy = {};
			return dummy;
		}

		void free()
		{
			cachedTexturePaths.clear();
			cachedTexturePaths = {};
			deadTextures.clear();
			deadTextures = {};

			// Destroy all GPU resources
			for (auto& [key, value] : cachedTextures)
			{
				value.texture.destroy();
			}
			cachedTextures.clear();
			cachedTextures = {};
		}

		// -------------------- Internal Functions --------------------
		static TextureHandle getCachedTextureFor(const std::filesystem::path& absolutePath)
		{
			auto textureHandleIter = cachedTexturePaths.find(absolutePath);
			if (textureHandleIter != cachedTexturePaths.end())
			{
				// Texture is already loaded: Increase ref count then return
				auto textureIter = cachedTextures.find(textureHandleIter->second);
				g_logger_assert(textureIter != cachedTextures.end(), "Somehow a TextureHandle was cached but the corresponding Texture data was not cached. This should never be hit.");
				textureIter->second.refCount++;
				return textureHandleIter->second;
			}

			return NULL_TEXTURE_HANDLE;
		}

		static TextureHandle cacheTexture(const std::filesystem::path& absolutePath, const Texture& texture)
		{
			CachedTexture newEntry;
			newEntry.texture = texture;

			TextureHandle handle = textureHandleCounter++;

			// Cache the path
			cachedTexturePaths[absolutePath] = handle;

			// Cache the texture data
			newEntry.absPath = absolutePath;
			newEntry.refCount = 1;
			cachedTextures[handle] = newEntry;

			return handle;
		}
	}
}