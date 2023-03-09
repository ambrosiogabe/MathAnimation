#ifndef MATH_ANIM_PERSPECTIVE_CAMERA_H
#define MATH_ANIM_PERSPECTIVE_CAMERA_H

#include "core.h"

#include <nlohmann/json_fwd.hpp>

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

		void serialize(nlohmann::json& memory) const;
		static PerspectiveCamera deserialize(const nlohmann::json& j, uint32 version);
	};
}

#endif