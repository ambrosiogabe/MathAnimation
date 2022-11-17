#include "svg/SvgCache.h"
#include "svg/Svg.h"
#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "renderer/Texture.h"
#include "renderer/Renderer.h"
#include "utils/CMath.h"

#include <nanovg.h>

namespace MathAnim
{
	Vec2 SvgCache::cachePadding = { 10.0f, 10.0f };

	void SvgCache::init()
	{
		constexpr int defaultWidth = 4096;
		this->generateDefaultFramebuffer(defaultWidth, defaultWidth);
	}

	void SvgCache::free()
	{
		framebuffer.destroy();
		this->cachedSvgs.clear();
	}

	bool SvgCache::exists(AnimationManagerData* am, SvgObject* svg, AnimObjId obj)
	{
		const AnimObject* animObj = AnimationManager::getObject(am, obj);
		return this->cachedSvgs.exists(hash(obj, animObj->percentCreated, animObj->svgScale));
	}

	SvgCacheEntry SvgCache::get(AnimationManagerData* am, SvgObject* svg, AnimObjId obj)
	{
		const AnimObject* animObj = AnimationManager::getObject(am, obj);
		auto entry = this->cachedSvgs.get(hash(obj, animObj->percentCreated, animObj->svgScale));
		if (entry.has_value())
		{
			return SvgCacheEntry{
				entry->texCoordsMin,
				entry->texCoordsMax,
				framebuffer.getColorAttachment(entry->colorAttachment)
			};
		}

		static Texture dummy = TextureBuilder().build();
		return SvgCacheEntry{ Vec2{0, 0}, Vec2{0, 0}, dummy };
	}

	SvgCacheEntry SvgCache::getOrCreateIfNotExist(NVGcontext* vg, AnimationManagerData* am, SvgObject* svg, AnimObjId obj, bool isSvgGroup)
	{
		const AnimObject* animObj = AnimationManager::getObject(am, obj);
		auto entry = this->cachedSvgs.get(hash(obj, animObj->percentCreated, animObj->svgScale));
		if (entry.has_value())
		{
			return SvgCacheEntry{
				entry->texCoordsMin,
				entry->texCoordsMax,
				framebuffer.getColorAttachment(entry->colorAttachment)
			};
		}

		// Doesn't exist, create it
		put(vg, animObj, svg, isSvgGroup);
		return get(am, svg, obj);
	}

	void SvgCache::put(NVGcontext* vg, const AnimObject* parent, SvgObject* svg, bool isSvgGroup)
	{
		uint64 hashValue = hash(parent->id, parent->percentCreated, parent->svgScale);

		// Only add the SVG if it hasn't already been added
		//auto iter = this->cachedSvgs.find(hashValue);
		//if (iter == this->cachedSvgs.end())
		if (!this->cachedSvgs.exists(hashValue))
		{
			// Setup the texture coords and everything 
			Vec2 svgTextureOffset = Vec2{
				(float)cacheCurrentPos.x + parent->strokeWidth * 0.5f,
				(float)cacheCurrentPos.y + parent->strokeWidth * 0.5f
			};

			// Check if the SVG cache needs to regenerate
			float svgTotalWidth = ((svg->bbox.max.x - svg->bbox.min.x) * parent->svgScale) + parent->strokeWidth;
			float svgTotalHeight = ((svg->bbox.max.y - svg->bbox.min.y) * parent->svgScale) + parent->strokeWidth;
			Vec2 allottedSize = Vec2{ svgTotalWidth, svgTotalHeight };
			{
				float newRightX = svgTextureOffset.x + svgTotalWidth;
				if (newRightX >= framebuffer.width)
				{
					// Move to the newline
					incrementCacheCurrentY();
				}

				float newBottomY = svgTextureOffset.y + svgTotalHeight;
				if (newBottomY >= framebuffer.height)
				{
					// Evict the first potential result from the LRU cache that
					// can contain the size of this SVG
					LRUCacheEntry<uint64, _SvgCacheEntryInternal>* oldest = this->cachedSvgs.getOldest();
					while (oldest != nullptr)
					{
						if (oldest->data.allottedSize.x >= svgTotalWidth && oldest->data.allottedSize.y >= svgTotalHeight)
						{
							allottedSize = oldest->data.allottedSize;

							// We found an entry that will fit this 
							// in the future we could evict this entry, 
							// repack the texture and then insert the new svg
							// but in practice this will probably be too slow
							// 
							// So for now I'll just "evict" then re-insert the new entry
							svgTextureOffset = Vec2{
								(float)oldest->data.textureOffset.x + parent->strokeWidth * 0.5f,
								(float)oldest->data.textureOffset.y + parent->strokeWidth * 0.5f
							};
							this->cachedSvgs.evict(oldest->key);
							// The svg will get reinserted below
							break;
						}

						// Try to find an entry that will fit this new SVG
						oldest = oldest->newerEntry;
					}

					if (oldest == nullptr)
					{
						// Didn't find room
						static bool displayMessage = true;
						if (displayMessage)
						{
							g_logger_error("Ran out of room in LRU-Cache.");
							displayMessage = false;
						}
						return;
					}
				}
				else
				{
					svgTextureOffset = Vec2{
						(float)cacheCurrentPos.x + parent->strokeWidth * 0.5f,
						(float)cacheCurrentPos.y + parent->strokeWidth * 0.5f
					};
				}
			}

			if (isSvgGroup)
			{
				//svgTextureOffset.x += offset.x * parent->svgScale;
				//svgTextureOffset.y += offset.y * parent->svgScale;
				static bool displayMessage = true;
				if (displayMessage)
				{
					g_logger_error("TODO: SVG Groups are not working at the moment, implement me. This message will now be suppressed.");
					displayMessage = false;
				}
			}

			svg->renderCreateAnimation(vg, parent->percentCreated, parent, svgTextureOffset, isSvgGroup);

			// Don't blit svg groups to a bunch of quads, they get rendered as one quad together
			if (isSvgGroup)
			{
				return;
			}

			// Subtract half stroke width to make sure it's getting the correct coords
			svgTextureOffset -= Vec2{ parent->strokeWidth * 0.5f, parent->strokeWidth * 0.5f };
			Vec2 cacheUvMin = Vec2{
				svgTextureOffset.x / framebuffer.width,
				1.0f - (svgTextureOffset.y / framebuffer.width) - (svgTotalHeight / framebuffer.height)
			};
			Vec2 cacheUvMax = cacheUvMin +
				Vec2{
					svgTotalWidth / framebuffer.width,
					svgTotalHeight / framebuffer.height
			};

			incrementCacheCurrentX(svgTotalWidth + cachePadding.x);
			checkLineHeight(svgTotalHeight);

			// Finally store the results
			_SvgCacheEntryInternal res;
			res.colorAttachment = cacheCurrentColorAttachment;
			res.texCoordsMin = cacheUvMin;
			res.texCoordsMax = cacheUvMax;
			res.svgSize = Vec2{ svgTotalWidth, svgTotalHeight };
			res.allottedSize = allottedSize;
			res.textureOffset = svgTextureOffset;
			this->cachedSvgs.insert(hashValue, res);
		}
	}

	void SvgCache::render(NVGcontext* vg, AnimationManagerData* am, SvgObject* svg, AnimObjId obj, bool isSvgGroup)
	{
		const AnimObject* parent = AnimationManager::getObject(am, obj);
		if (parent)
		{
			SvgCacheEntry metadata = getOrCreateIfNotExist(vg, am, svg, obj, isSvgGroup);
			// TODO: See if I can get rid of this duplication, see the function above
			float svgTotalWidth = ((svg->bbox.max.x - svg->bbox.min.x) * parent->svgScale) + parent->strokeWidth;
			float svgTotalHeight = ((svg->bbox.max.y - svg->bbox.min.y) * parent->svgScale) + parent->strokeWidth;

			if (parent->is3D)
			{
				Renderer::drawTexturedQuad3D(
					metadata.textureRef,
					Vec2{ svgTotalWidth / parent->svgScale, svgTotalHeight / parent->svgScale },
					metadata.texCoordsMin,
					metadata.texCoordsMax,
					parent->globalTransform,
					parent->isTransparent
				);
			}
			else
			{
				Renderer::drawTexturedQuad(
					metadata.textureRef,
					Vec2{ svgTotalWidth / parent->svgScale, svgTotalHeight / parent->svgScale },
					metadata.texCoordsMin,
					metadata.texCoordsMax,
					parent->id,
					parent->globalTransform
				);
			}
		}
	}

	const Framebuffer& SvgCache::getFramebuffer()
	{
		return framebuffer;
	}

	void SvgCache::clearAll()
	{
		cacheCurrentPos.x = 0;
		cacheCurrentPos.y = 0;
		cacheCurrentColorAttachment = 0;
		cacheLineHeight = 0;
		cachedSvgs.clear();

		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "SVG_Cache_Reset");

		framebuffer.bind();
		glViewport(0, 0, framebuffer.width, framebuffer.height);
		for (int i = 0; i < framebuffer.colorAttachments.size(); i++)
		{
			//svgCache.clearColorAttachmentRgba(0, "#fc03ecFF"_hex);
			framebuffer.clearColorAttachmentRgba(i, "#00000000"_hex);
		}
		framebuffer.clearDepthStencil();

		glPopDebugGroup();
	}

	void SvgCache::flushCacheToFramebuffer(NVGcontext* vg)
	{
		// First render to the cache
		framebuffer.bind();
		glViewport(0, 0, framebuffer.width, framebuffer.height);

		// Reset the draw buffers to draw to FB_attachment_0
		GLenum compositeDrawBuffers[] = {
			GL_COLOR_ATTACHMENT0 + this->cacheCurrentColorAttachment,
			GL_NONE,
			GL_NONE,
			GL_COLOR_ATTACHMENT3
		};
		glDrawBuffers(4, compositeDrawBuffers);

		constexpr size_t bufferLength = 256;
		char buffer[bufferLength];
		snprintf(buffer, bufferLength, "NanoVG_RenderSVG[%d]\0", this->cacheCurrentColorAttachment);
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, buffer);
		nvgEndFrame(vg);
		glPopDebugGroup();
	}

	// --------------------- Private ---------------------
	void SvgCache::incrementCacheCurrentY()
	{
		cacheCurrentPos.y += cacheLineHeight + cachePadding.y;
		cacheLineHeight = 0;
		cacheCurrentPos.x = 0;
	}

	void SvgCache::incrementCacheCurrentX(float distance)
	{
		cacheCurrentPos.x += distance;
	}

	//const Vec2& SvgCache::getCacheCurrentPos()
	//{
	//	return cacheCurrentPos;
	//}

	void SvgCache::checkLineHeight(float newLineHeight)
	{
		cacheLineHeight = glm::max(cacheLineHeight, newLineHeight);
	}

	void SvgCache::growCache()
	{
		// TODO: This should just add a new color attachment
		// Svg::generateSvgCache(svgCache.width * 2, svgCache.height * 2);
		static bool messageDisplayed = false;
		if (!messageDisplayed)
		{
			g_logger_error("TODO: SvgCache::growCache() not implemented yet. This message will be suppressed.");
			messageDisplayed = true;
		}
	}

	void SvgCache::generateDefaultFramebuffer(uint32 width, uint32 height)
	{
		if (width > 4096 || height > 4096)
		{
			g_logger_error("SVG cache cannot be bigger than 4096x4096 pixels. The SVG will be truncated.");
			width = 4096;
			height = 4096;
		}

		// Default the svg framebuffer cache to 1024x1024 and resize if necessary
		Texture cacheTexture = TextureBuilder()
			.setFormat(ByteFormat::RGBA8_UI)
			.setMinFilter(FilterMode::Linear)
			.setMagFilter(FilterMode::Linear)
			.setWidth(width)
			.setHeight(height)
			.build();
		framebuffer = FramebufferBuilder(width, height)
			.addColorAttachment(cacheTexture)
			.includeDepthStencil()
			.generate();
	}

	void SvgCache::addColorAttachment()
	{
		// TODO: Generate new framebuffer, copy textures to the new framebuffer
		// then delete the old framebuffer
		//Texture cacheTexture = TextureBuilder()
		//	.setFormat(ByteFormat::RGBA8_UI)
		//	.setMinFilter(FilterMode::Linear)
		//	.setMagFilter(FilterMode::Linear)
		//	.setWidth(framebuffer.width)
		//	.setHeight(framebuffer.height)
		//	.build();
		//framebuffer = FramebufferBuilder(framebuffer.width, framebuffer.height)
		//	.addColorAttachment(cacheTexture)
		//	.includeDepthStencil()
		//	.generate();
	}

	uint64 SvgCache::hash(AnimObjId obj, float percentCreated, float svgScale)
	{
		uint64 hash = obj;
		// Only hash floating point numbers to 3 decimal places
		// This allows up to 16 seconds unique frames in an animation
		int roundedPercentCreated = (int)(percentCreated * 1000.0f);
		hash = CMath::combineHash<int>(roundedPercentCreated, hash);
		int roundedSvgScale = (int)(svgScale * 1000.0f);
		hash = CMath::combineHash<int>(roundedSvgScale, hash);
		return hash;
	}
}
