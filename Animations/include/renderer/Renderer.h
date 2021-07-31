#ifndef MATH_ANIM_RENDERER_H
#define MATH_ANIM_RENDERER_H
#include "core.h"

namespace MathAnim
{
	struct OrthoCamera;
	struct Shader;
	struct Style;
	struct Framebuffer;

	namespace Renderer
	{
		void init(OrthoCamera& sceneCamera);

		void render();

		void renderFramebuffer(const Framebuffer& framebuffer);

		void drawLine(const glm::vec2& start, const glm::vec2& end, const Style& style);

		void flushBatch();

		void clearColor(const glm::vec4& color);
	}
}

#endif
