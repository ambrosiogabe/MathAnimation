#ifndef MATH_ANIM_PERSPECTIVE_CAMERA_H
#define MATH_ANIM_PERSPECTIVE_CAMERA_H

#include "core.h"

namespace MathAnim
{
	struct PerspectiveCamera
	{
		glm::vec3 position;
		glm::vec3 orientation;
		glm::vec3 forward;
		float fov;

		glm::mat4 calculateViewMatrix();
		glm::mat4 calculateProjectionMatrix() const;

		void serialize(RawMemory& memory) const;
		static PerspectiveCamera deserialize(RawMemory& memory, uint32 version);
	};
}

#endif