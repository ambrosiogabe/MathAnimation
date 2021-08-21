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
		glm::vec2 start;
		glm::vec2 size;
		glm::vec2 texCoordStart;
		glm::vec2 texCoordSize;
	};

	namespace Renderer
	{
		void init(OrthoCamera& sceneCamera);

		void render();

		void renderFramebuffer(const Framebuffer& framebuffer);

		void drawSquare(const glm::vec2& start, const glm::vec2& size, const Style& style);

		void drawFilledSquare(const glm::vec2& start, const glm::vec2& size, const Style& style);

		void drawLine(const glm::vec2& start, const glm::vec2& end, const Style& style);

		void drawTexture(const RenderableTexture& renderable, const glm::vec4& color);

		void drawString(const std::string& string, const Font& font, const glm::vec2& start, float scale, const glm::vec4& color);

		void drawFilledCircle(const glm::vec2& position, float radius, int numSegments, const Style& style);

		void drawFilledTriangle(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const Style& style);

		void flushBatch();

		void clearColor(const glm::vec4& color);
	}
}

#endif
