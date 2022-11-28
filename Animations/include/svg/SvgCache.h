#ifndef MATH_ANIM_SVG_CACHE_H
#define MATH_ANIM_SVG_CACHE_H
#include "core.h"
#include "renderer/Framebuffer.h"
#include "utils/LRUCache.hpp"

namespace MathAnim
{
	struct SvgObject;
	struct SvgGroup;
	struct Texture;
	struct AnimationManagerData;
	struct AnimObject;

	struct SvgCacheEntry
	{
		Vec2 texCoordsMin;
		Vec2 texCoordsMax;
		const Texture& textureRef;
	};

	struct _SvgCacheEntryInternal
	{
		Vec2 texCoordsMin;
		Vec2 texCoordsMax;
		Vec2 svgSize;
		Vec2 allottedSize;
		Vec2 textureOffset;
		int colorAttachment;
	};

	class SvgCache
	{
	public:
		SvgCache() :
			cachedSvgs(),
			framebuffer(),
			cacheCurrentPos(Vec2{ 0, 0 }),
			cacheCurrentColorAttachment(0),
			cacheLineHeight(0.0f)
		{
		}

		void init();
		void free();

		bool exists(AnimationManagerData* am, AnimObjId obj);

		SvgCacheEntry get(AnimationManagerData* am, AnimObjId obj);
		SvgCacheEntry getOrCreateIfNotExist(AnimationManagerData* am, SvgObject* svg, AnimObjId obj);
		SvgCacheEntry getOrCreateIfNotExist(AnimationManagerData* am, SvgGroup* group, AnimObjId obj);

		void put(const AnimObject* parent, SvgObject* svg);
		void put(const AnimObject* parent, SvgGroup* group);
		void clearAll();

		void render(AnimationManagerData* am, SvgObject* svg, AnimObjId obj);
		void render(AnimationManagerData* am, SvgGroup* group, AnimObjId obj);

		const Framebuffer& getFramebuffer();

	public:
		static Vec2 cachePadding;

	private:
		void incrementCacheCurrentY();
		void incrementCacheCurrentX(float distance);
		//const Vec2& getCacheCurrentPos();
		void checkLineHeight(float newLineHeight);
		void growCache();

		void generateDefaultFramebuffer(uint32 width, uint32 height);
		void addColorAttachment();

		uint64 hash(AnimObjId obj, float svgScale);

	private:
		// TODO: Add LRU-cache for long term storage
		// and temporary cache for objects that are currently
		// being animated
		//std::unordered_map<uint64, _SvgCacheEntryInternal> cachedSvgs;
		LRUCache<uint64, _SvgCacheEntryInternal> cachedSvgs;
		Framebuffer framebuffer;
	public:
		Vec2 cacheCurrentPos;
	private:
		int cacheCurrentColorAttachment;
		float cacheLineHeight;
	};
}

#endif // MATH_ANIM_SVG_CACHE_H
