#ifndef MATH_ANIM_PERSPECTIVE_CAMERA_H
#define MATH_ANIM_PERSPECTIVE_CAMERA_H

#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct [[deprecated("This is kept for legacy purposes. Use the Camera class instead for future work.")]]
    PerspectiveCamera
	{
		glm::vec3 position;
		glm::vec3 orientation;
		glm::vec3 forward;
		float fov;

		glm::mat4 calculateViewMatrix();
		glm::mat4 calculateProjectionMatrix() const;

		void serialize(nlohmann::json& j) const;
		static PerspectiveCamera deserialize(const nlohmann::json& j, uint32 version);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static PerspectiveCamera legacy_deserialize(RawMemory& memory, uint32 version);
	};
}

#endif