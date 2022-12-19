#include "svg/TexturePacker.h"

namespace MathAnim
{
	void TexturePacker::init(const Vec2& cacheSize, const Vec2& texturePadding)
	{
		this->cacheSize = cacheSize;
		this->cachePadding = texturePadding;

		textureMap.clear();
		textureConstraints.clear();

		if (framebuffer.fbo != UINT32_MAX)
		{
			// Should destroy all textures associated with
			// this framebuffer
			framebuffer.destroy();
		}

		Texture initialTexture = TextureBuilder()
			.setFormat(ByteFormat::RGBA8_UI)
			.setMinFilter(FilterMode::Linear)
			.setMagFilter(FilterMode::Linear)
			.setWidth((uint32)cacheSize.x)
			.setHeight((uint32)cacheSize.y)
			.build();
		TextureConstraint initialTextureConstraint;
		initialTextureConstraint.pos = {0, 0};
		initialTextureConstraint.lineHeight = 0;
		textureConstraints.push_back(initialTextureConstraint);

		framebuffer = FramebufferBuilder((uint32)cacheSize.x, (uint32)cacheSize.y)
			.addColorAttachment(initialTexture)
			.includeDepthStencil()
			.generate();
	}

	bool TexturePacker::insert(uint64 quadKey, const Vec2& quadSize)
	{
		// Check if this attachment fits in any of the textures
		bool textureFits = false;
		for (int i = 0; i < textureConstraints.size(); i++)
		{
			TextureConstraint constraintCopy = textureConstraints[i];
			float newRightX = constraintCopy.pos.x + quadSize.x;
			if (newRightX >= cacheSize.x)
			{
				// Move to the newline
				constraintCopy.pos.y += constraintCopy.lineHeight + cachePadding.y;
				constraintCopy.lineHeight= 0;
				constraintCopy.pos.x = 0;
			}

			float newBottomY = constraintCopy.pos.y + quadSize.y;
			if (newBottomY >= cacheSize.y)
			{
				// Texture does not fit in here, so don't modify this constraint
				continue;
			}

			// Texture does fit in here, so insert the quad and modify the constraint
			PackedTexture packedTexture;
			packedTexture.colorAttachmentIndex = i;
			packedTexture.quadSize = quadSize;
			packedTexture.uvMin = Vec2{
				textureConstraints[i].pos.x / cacheSize.x,
				1.0f - ((textureConstraints[i].pos.y - quadSize.y) / cacheSize.y)
			};
			packedTexture.uvMax = packedTexture.uvMin +
				Vec2{
				quadSize.x / cacheSize.x,
				quadSize.y / cacheSize.y
			};
			textureMap.insert({quadKey, packedTexture});

			// Update the constraint
			constraintCopy.lineHeight = glm::max(constraintCopy.lineHeight, quadSize.y);
			textureConstraints[i] = constraintCopy;
			textureFits = true;
		}

		if (!textureFits)
		{
			// If the quad is bigger than the cache size, there's no way it could ever fit
			if (quadSize.x >= cacheSize.x || quadSize.y >= cacheSize.y)
			{
				return false;
			}

			addAttachment();

			// Texture should fit in here, so insert the quad and modify the constraint
			int constraintIndex = (int)textureConstraints.size() - 1;
			PackedTexture packedTexture;
			packedTexture.colorAttachmentIndex = constraintIndex;
			packedTexture.quadSize = quadSize;
			packedTexture.uvMin = Vec2{
				textureConstraints[constraintIndex].pos.x / cacheSize.x,
				1.0f - ((textureConstraints[constraintIndex].pos.y - quadSize.y) / cacheSize.y)
			};
			packedTexture.uvMax = packedTexture.uvMin +
				Vec2{
				quadSize.x / cacheSize.x,
				quadSize.y / cacheSize.y
			};
			textureMap.insert({ quadKey, packedTexture });

			// Update the constraint
			textureConstraints[constraintIndex].pos.x += quadSize.x;
			textureConstraints[constraintIndex].lineHeight = glm::max(textureConstraints[constraintIndex].lineHeight, quadSize.y);
		}

		return true;
	}

	bool TexturePacker::getUvMin(uint64 quadKey, Vec2* outPosition)
	{
		auto iter = textureMap.find(quadKey);
		if (iter != textureMap.end())
		{
			*outPosition = iter->second.uvMin;
			return true;
		}

		return false;
	}

	bool TexturePacker::getUvMax(uint64 quadKey, Vec2* outPosition)
	{
		auto iter = textureMap.find(quadKey);
		if (iter != textureMap.end())
		{
			*outPosition = iter->second.uvMax;
			return true;
		}

		return false;
	}

	bool TexturePacker::getTextureId(uint64 quadKey, uint32* outTextureId)
	{
		auto iter = textureMap.find(quadKey);
		if (iter != textureMap.end())
		{
			*outTextureId = framebuffer.getColorAttachment(iter->second.colorAttachmentIndex).graphicsId;
			return true;
		}

		return false;
	}

	void TexturePacker::clearAllQuads()
	{
		textureMap.clear();
		for (int i = 0; i < textureConstraints.size(); i++)
		{
			textureConstraints[i].lineHeight = 0.0f;
			textureConstraints[i].pos = Vec2{ 0, 0 };
		}
	}

	void TexturePacker::addAttachment()
	{
		// Generate a new framebuffer with 1 extra color attachment
		FramebufferBuilder fbBuilder = FramebufferBuilder((uint32)cacheSize.x, (uint32)cacheSize.y);
		for (int i = 0; i < framebuffer.colorAttachments.size() + 1; i++)
		{
			Texture texture = TextureBuilder()
				.setFormat(ByteFormat::RGBA8_UI)
				.setMinFilter(FilterMode::Linear)
				.setMagFilter(FilterMode::Linear)
				.setWidth((uint32)cacheSize.x)
				.setHeight((uint32)cacheSize.y)
				.build();

			if (i >= textureConstraints.size())
			{
				TextureConstraint initialTextureConstraint;
				initialTextureConstraint.pos = {0, 0};
				initialTextureConstraint.lineHeight = 0;
				textureConstraints.push_back(initialTextureConstraint);
			}
			fbBuilder.addColorAttachment(texture);
		}
		fbBuilder.includeDepthStencil();
		Framebuffer newFramebuffer = fbBuilder.generate();

		// Copy all the textures over to the new framebuffer
		for (int i = 0; i < framebuffer.colorAttachments.size(); i++)
		{
			framebuffer.colorAttachments[i].copyTo(newFramebuffer.colorAttachments[i]);
		}

		// And destroy old framebuffer 
		framebuffer.destroy();

		// And replace
		framebuffer = newFramebuffer;
	}
}