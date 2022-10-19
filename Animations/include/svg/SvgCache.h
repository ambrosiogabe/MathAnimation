#ifndef MATH_ANIM_SVG_CACHE_H
#define MATH_ANIM_SVG_CACHE_H
#include "core.h"
#include "renderer/Framebuffer.h"

struct NVGcontext;

namespace MathAnim
{
	struct SvgObject;
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
		int colorAttachment;
	};

	class SvgCache
	{
	public:
		SvgCache() : 
			cachedSvgs(), 
			framebuffer(), 
			cacheCurrentPos(Vec2{0, 0}),
			cacheCurrentColorAttachment(0),
			cacheLineHeight(0.0f)
		{
		}

		void init();
		void free();

		bool exists(AnimationManagerData* am, SvgObject* svg, AnimObjId obj);
		SvgCacheEntry get(AnimationManagerData* am, SvgObject* svg, AnimObjId obj);
		SvgCacheEntry getOrCreateIfNotExist(NVGcontext* vg, AnimationManagerData* am, SvgObject* svg, AnimObjId obj, bool isSvgGroup);

		void put(NVGcontext* vg, const AnimObject* parent, SvgObject* svg, bool isSvgGroup);
		void clearAll();

		void render(NVGcontext* vg, AnimationManagerData* am, SvgObject* svg, AnimObjId obj, bool isSvgGroup = false);

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

		uint64 hash(AnimObjId obj, float percentCreated, float svgScale);

	private:
		// TODO: Add LRU-cache for long term storage
		// and temporary cache for objects that are currently
		// being animated
		std::unordered_map<uint64, _SvgCacheEntryInternal> cachedSvgs;
		Framebuffer framebuffer;
		Vec2 cacheCurrentPos;
		int cacheCurrentColorAttachment;
		float cacheLineHeight;
	};
}

#endif // MATH_ANIM_SVG_CACHE_H
