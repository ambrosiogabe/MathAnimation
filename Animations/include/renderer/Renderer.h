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

		void drawLine(const glm::vec2& start, const glm::vec2& end, const Style& style);

		void drawTexture(const RenderableTexture& renderable, const glm::vec3& color);

		void drawString(const std::string& string, const Font& font, const glm::vec2& start, float scale, const glm::vec3& color);

		void flushBatch();

		void clearColor(const glm::vec4& color);
	}
}

#endif
