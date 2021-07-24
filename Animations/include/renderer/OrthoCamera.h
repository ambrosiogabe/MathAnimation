#ifndef MATH_ANIM_ORTHO_CAMERA_H
#define MATH_ANIM_ORTHO_CAMERA_H

#include "core.h"

namespace MathAnim
{
	struct OrthoCamera
	{
		glm::vec2 position;
		glm::vec2 projectionSize;

		glm::mat4 calculateViewMatrix();
		glm::mat4 calculateProjectionMatrix() const;
	};
}

#endif