#ifndef MATH_ANIM_ORTHO_CAMERA_H
#define MATH_ANIM_ORTHO_CAMERA_H

#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct OrthoCamera
	{
		Vec2 position;
		Vec2 projectionSize;
		float zoom;

		glm::mat4 calculateViewMatrix() const;
		glm::mat4 calculateProjectionMatrix() const;
		Vec2 reverseProject(const Vec2& normalizedInput) const;

		void serialize(nlohmann::json& j) const;
		static OrthoCamera deserialize(const nlohmann::json& j, uint32 version);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static OrthoCamera legacy_deserialize(RawMemory& memory, uint32 version);
	};
}

#endif