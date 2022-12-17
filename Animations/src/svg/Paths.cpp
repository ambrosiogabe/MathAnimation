#include "svg/Paths.h"
#include "svg/Svg.h"
#include "renderer/Renderer.h"

namespace MathAnim
{
	namespace Paths
	{
		Path2DContext* createRectangle(const Vec2& size, const glm::mat4& transformation)
		{
			Vec2 halfSize = size / 2.0f;
			Path2DContext* path = Renderer::beginPath(-1.0f * halfSize, transformation);
			Renderer::lineTo(path, Vec2{ -halfSize.x, halfSize.y });
			Renderer::lineTo(path, halfSize);
			Renderer::lineTo(path, Vec2{ halfSize.x, -halfSize.y });
			Renderer::lineTo(path, -1.0f * halfSize);

			return path;
		}

		Path2DContext* createCircle(float radius, const glm::mat4& transformation)
		{
			float equidistantControls = (float)((4.0 / 3.0) * glm::tan(glm::pi<double>() / 8.0) * (double)radius);

			Path2DContext* path = Renderer::beginPath(Vec2{ -radius, 0.0f }, transformation);
			Renderer::cubicTo(path,
				Vec2{ -radius, equidistantControls },
				Vec2{ -equidistantControls, radius },
				Vec2{ 0.0f, radius });
			Renderer::cubicTo(path,
				Vec2{ equidistantControls, radius },
				Vec2{ radius, equidistantControls },
				Vec2{ radius, 0.0f });
			Renderer::cubicTo(path,
				Vec2{ radius, -equidistantControls },
				Vec2{ equidistantControls, -radius },
				Vec2{ 0.0f, -radius });
			Renderer::cubicTo(path,
				Vec2{ -equidistantControls, -radius },
				Vec2{ -radius, -equidistantControls },
				Vec2{ -radius, 0.0f });

			return path;
		}
	}
}