#ifndef MATH_ANIM_PATHS_H
#define MATH_ANIM_PATHS_H
#include "core.h"

namespace MathAnim
{
	struct Path2DContext;

	namespace Paths
	{
		Path2DContext* createRectangle(const Vec2& size, const glm::mat4& transformation = glm::identity<glm::mat4>());
		Path2DContext* createCircle(float radius, const glm::mat4& transformation = glm::identity<glm::mat4>());
	}
}

#endif 