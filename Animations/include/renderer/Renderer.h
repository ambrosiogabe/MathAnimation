#ifndef MATH_ANIM_RENDERER_H
#define MATH_ANIM_RENDERER_H
#include "core.h"

namespace MathAnim
{
	struct OrthoCamera;
	struct Shader;
	struct Style;
	struct Framebuffer;
	struct Texture;
	struct Font;

	struct RenderableTexture
	{
		const Texture* texture;
		Vec2 start;
		Vec2 size;
		Vec2 texCoordStart;
		Vec2 texCoordSize;
	};

	namespace Renderer
	{
		void init(OrthoCamera& sceneCamera);

		void render();

		void renderFramebuffer(const Framebuffer& framebuffer);

		void drawSquare(const Vec2& start, const Vec2& size, const Style& style);

		void drawFilledSquare(const Vec2& start, const Vec2& size, const Style& style);

		void drawLine(const Vec2& start, const Vec2& end, const Style& style);

		void drawTexture(const RenderableTexture& renderable, const Vec4& color);

		void drawString(const std::string& string, const Font& font, const Vec2& start, float scale, const Vec4& color);

		void drawFilledCircle(const Vec2& position, float radius, int numSegments, const Style& style);

		void drawFilledTriangle(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Style& style);

		void flushBatch();

		void clearColor(const Vec4& color);
	}
}

#endif
