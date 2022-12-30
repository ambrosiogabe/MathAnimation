#include "svg/SvgCache.h"
#include "svg/Svg.h"
#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "renderer/Texture.h"
#include "renderer/Renderer.h"
#include "utils/CMath.h"

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
		for (size_t i = 0; i < cachedSvgs.size(); i++)
		{
			cachedSvgs[i].clear();
		}
		this->cachedSvgs.clear();
	}

	bool SvgCache::exists(AnimationManagerData* am, AnimObjId obj)
	{
		const AnimObject* animObj = AnimationManager::getObject(am, obj);
		return existsInternal(hash(obj, animObj->svgScale, animObj->percentReplacementTransformed));
	}

	SvgCacheEntry SvgCache::get(AnimationManagerData* am, AnimObjId obj)
	{
		const AnimObject* animObj = AnimationManager::getObject(am, obj);
		auto entry = getInternal(hash(obj, animObj->svgScale, animObj->percentReplacementTransformed));
		if (entry.has_value())
		{
			return SvgCacheEntry{
				entry->texCoordsMin,
				entry->texCoordsMax,
				framebuffer.getColorAttachment(entry->colorAttachment)
			};
		}

		static Texture dummy = TextureBuilder()
			.setWidth(1)
			.setHeight(1)
			.setFormat(ByteFormat::RGB8_UI)
			.setMagFilter(FilterMode::Linear)
			.setMinFilter(FilterMode::Linear)
			.generate();
		return SvgCacheEntry{ Vec2{0, 0}, Vec2{1, 1}, dummy };
	}

	SvgCacheEntry SvgCache::getOrCreateIfNotExist(AnimationManagerData* am, SvgObject* svg, AnimObjId obj)
	{
		const AnimObject* animObj = AnimationManager::getObject(am, obj);
		auto entry = getInternal(hash(obj, animObj->svgScale, animObj->percentReplacementTransformed));
		if (entry.has_value())
		{
			return SvgCacheEntry{
				entry->texCoordsMin,
				entry->texCoordsMax,
				framebuffer.getColorAttachment(entry->colorAttachment)
			};
		}

		put(animObj, svg);
		return get(am, obj);
	}

	void SvgCache::put(const AnimObject* parent, SvgObject* svg)
	{
		uint64 hashValue = hash(parent->id, parent->svgScale, parent->percentReplacementTransformed);

		// Only add the SVG if it hasn't already been added
		if (!existsInternal(hashValue))
		{
			// Setup the texture coords and everything 
			const Vec2& svgTextureOffset = cacheCurrentPos;

			// Check if the SVG cache needs to regenerate
			float svgTotalWidth = ((svg->bbox.max.x - svg->bbox.min.x) * parent->svgScale);
			float svgTotalHeight = ((svg->bbox.max.y - svg->bbox.min.y) * parent->svgScale);
			if (svgTotalWidth <= 0.0f || svgTotalHeight <= 0.0f)
			{
				return;
			}

			Vec2 allottedSize = Vec2{ svgTotalWidth, svgTotalHeight };
			{
				int newRightX = (int)(svgTextureOffset.x + svgTotalWidth + cachePadding.x);
				if (newRightX >= framebuffer.width)
				{
					// Move to the newline
					incrementCacheCurrentY();
				}

				float newBottomY = svgTextureOffset.y + svgTotalHeight + cachePadding.y;
				if (newBottomY >= framebuffer.height)
				{
					growCache();
					// TODO: FIXME
					// Evict the first potential result from the LRU cache that
					// can contain the size of this SVG
					//LRUCacheEntry<uint64, _SvgCacheEntryInternal>* oldest = this->cachedSvgs[this->cacheCurrentColorAttachment].getOldest();
					//while (oldest != nullptr)
					//{
					//	if (oldest->data.allottedSize.x >= svgTotalWidth && oldest->data.allottedSize.y >= svgTotalHeight)
					//	{
					//		allottedSize = oldest->data.allottedSize;

					//		// We found an entry that will fit this 
					//		// in the future we could evict this entry, 
					//		// repack the texture and then insert the new svg
					//		// but in practice this will probably be too slow
					//		// 
					//		// So for now I'll just "evict" then re-insert the new entry
					//		svgTextureOffset = oldest->data.textureOffset;
					//		if (!this->cachedSvgs[this->cacheCurrentColorAttachment].evict(oldest->key))
					//		{
					//			g_logger_error("Eviction failed: 0x%8x", oldest->key);
					//		}
					//		// The svg will get reinserted below
					//		break;
					//	}

					//	// Try to find an entry that will fit this new SVG
					//	oldest = oldest->next;
					//}

					//if (oldest == nullptr)
					//{
					//	// Didn't find room
					//	static bool displayMessage = true;
					//	if (displayMessage)
					//	{
					//		g_logger_error("Ran out of room in LRU-Cache.");
					//		displayMessage = false;
					//	}
					//	return;
					//}

					//uint32 clearColor[] = { 0, 0, 0, 0 };
					//glClearTexSubImage(
					//	framebuffer.colorAttachments[this->cacheCurrentColorAttachment].graphicsId,
					//	0,
					//	svgTextureOffset.x, svgTextureOffset.y, 0,
					//	allottedSize.x, allottedSize.y, 0,
					//	GL_RGBA, GL_UNSIGNED_INT,
					//	clearColor
					//);
				}
			}

			svg->render(
				parent,
				framebuffer.colorAttachments[this->cacheCurrentColorAttachment],
				svgTextureOffset
			);

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
			this->cachedSvgs[this->cacheCurrentColorAttachment].insert(hashValue, res);
		}
	}

	void SvgCache::render(AnimationManagerData* am, SvgObject* svg, AnimObjId obj)
	{
		const AnimObject* parent = AnimationManager::getObject(am, obj);
		if (parent)
		{
			SvgCacheEntry metadata = getOrCreateIfNotExist(am, svg, obj);
			// TODO: See if I can get rid of this duplication, see the function above
			float svgTotalWidth = ((svg->bbox.max.x - svg->bbox.min.x) * parent->svgScale);
			float svgTotalHeight = ((svg->bbox.max.y - svg->bbox.min.y) * parent->svgScale);

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
					Vec4{
						(float)parent->fillColor.r / 255.0f,
						(float)parent->fillColor.g / 255.0f,
						(float)parent->fillColor.b / 255.0f,
						(float)parent->fillColor.a / 255.0f
					},
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
		for (size_t i = 0; i < cachedSvgs.size(); i++)
		{
			cachedSvgs[i].clear();
		}

		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "SVG_Cache_Reset");

		framebuffer.bind();
		glViewport(0, 0, framebuffer.width, framebuffer.height);
		for (int i = 0; i < framebuffer.colorAttachments.size(); i++)
		{
			framebuffer.clearColorAttachmentRgba(i, "#00000000"_hex);
		}
		framebuffer.clearDepthStencil();

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

	void SvgCache::checkLineHeight(float newLineHeight)
	{
		cacheLineHeight = glm::max(cacheLineHeight, newLineHeight);
	}

	void SvgCache::growCache()
	{
		// TODO: This should just add a new color attachment
		cacheCurrentColorAttachment = (cacheCurrentColorAttachment + 1) % framebuffer.colorAttachments.size();
		this->cachedSvgs[cacheCurrentColorAttachment].clear();
		this->cacheCurrentPos = Vec2{ 0, 0 };
	}

	std::optional<_SvgCacheEntryInternal> SvgCache::getInternal(uint64 hash)
	{
		for (size_t i = 0; i < cachedSvgs.size(); i++)
		{
			const auto& res = cachedSvgs[(cacheCurrentColorAttachment + i) % cachedSvgs.size()].get(hash);
			if (res.has_value())
			{
				return res;
			}
		}

		return std::nullopt;
	}

	bool SvgCache::existsInternal(uint64 hash)
	{
		for (size_t i = 0; i < cachedSvgs.size(); i++)
		{
			const auto& res = cachedSvgs[(cacheCurrentColorAttachment + i) % cachedSvgs.size()].get(hash);
			if (res.has_value())
			{
				return true;
			}
		}

		return false;
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
		// Add four cached textures and increment through them
		// for more space when "growing" the svg cache
		framebuffer = FramebufferBuilder(width, height)
			.addColorAttachment(cacheTexture)
			.addColorAttachment(cacheTexture)
			.addColorAttachment(cacheTexture)
			.addColorAttachment(cacheTexture)
			.includeDepthStencil()
			.generate();
		// Push back four caches since we have four color attachments
		cachedSvgs.push_back({});
		cachedSvgs.push_back({});
		cachedSvgs.push_back({});
		cachedSvgs.push_back({});
	}

	uint64 SvgCache::hash(AnimObjId obj, float svgScale, float replacementTransform)
	{
		uint64 hash = obj;
		// Only hash floating point numbers to 3 decimal places
		int roundedSvgScale = (int)(svgScale * 1000.0f);
		hash = CMath::combineHash<int>(roundedSvgScale, hash);
		int roundedTransform = (int)(replacementTransform * 100.0f);
		hash = CMath::combineHash<int>(roundedTransform, hash);
		return hash;
	}
}
