#ifndef MATH_ANIM_TEXTURE_PACKER_H
#define MATH_ANIM_TEXTURE_PACKER_H
#include "core.h"
#include "renderer/Texture.h"
#include "renderer/Framebuffer.h"

namespace MathAnim
{
	struct PackedTexture
	{
		Vec2 quadSize;
		Vec2 uvMin;
		Vec2 uvMax;
		uint32 colorAttachmentIndex;
	};

	struct TextureConstraint
	{
		Vec2 pos;
		float lineHeight;
	};

	class TexturePacker
	{
	public:
		void init(const Vec2& cacheSize, const Vec2& texturePadding);

		bool insert(uint64 quadKey, const Vec2& quadSize);
		bool getUvMin(uint64 quadKey, Vec2* outPosition);
		bool getUvMax(uint64 quadKey, Vec2* outPosition);

		bool getTextureId(uint64 quadKey, uint32* outTextureId);

		void clearAllQuads();

	private:
		void addAttachment();

	private:
		std::unordered_map<uint64, PackedTexture> textureMap;
		std::vector<TextureConstraint> textureConstraints;
		Vec2 cacheSize;
		Vec2 cachePadding;
		Framebuffer framebuffer;
	};
}

#endif 