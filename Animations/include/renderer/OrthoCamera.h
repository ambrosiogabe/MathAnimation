#ifndef MATH_ANIM_ORTHO_CAMERA_H
#define MATH_ANIM_ORTHO_CAMERA_H

#include "core.h"

namespace MathAnim
{
	struct OrthoCamera
	{
		Vec2 position;
		Vec2 projectionSize;

		glm::mat4 calculateViewMatrix() const;
		glm::mat4 calculateProjectionMatrix() const;
		Vec2 reverseProject(const Vec2& normalizedInput) const;
	};
}

#endif