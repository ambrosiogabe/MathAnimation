#include "renderer/Deprecated_PerspectiveCamera.h"
#include "core/Application.h"
#include "core/Serialization.hpp"
#include "math/CMath.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	// TODO: Cache these values and make this const by separating
	// calculations from getting the matrices
	glm::mat4 PerspectiveCamera::calculateViewMatrix()
	{
		glm::vec3 direction;
		direction.x = cos(glm::radians(orientation.y)) * cos(glm::radians(orientation.x));
		direction.y = sin(glm::radians(orientation.x));
		direction.z = sin(glm::radians(orientation.y)) * cos(glm::radians(orientation.x));
		forward = glm::normalize(direction);
		// 0 1 0
		glm::vec3 localRight = glm::cross(forward, glm::vec3(0, 1, 0));
		glm::vec3 localUp = glm::cross(localRight, forward);

		return glm::lookAt(
			position,
			position + forward,
			localUp
		);
	}

	glm::mat4 PerspectiveCamera::calculateProjectionMatrix() const
	{
		return glm::perspective(
			fov,
			Application::getOutputTargetAspectRatio(),
			0.1f,
			100.0f
		);
	};

	void PerspectiveCamera::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_GLM_VEC3(memory, this, position);
		SERIALIZE_GLM_VEC3(memory, this, orientation);
		SERIALIZE_GLM_VEC3(memory, this, forward);
		SERIALIZE_NON_NULL_PROP(memory, this, fov);
	}

	PerspectiveCamera PerspectiveCamera::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
		{
			PerspectiveCamera res;
			DESERIALIZE_GLM_VEC3(&res, position, j, (Vec3{0, 0, -2}));
			DESERIALIZE_GLM_VEC3(&res, orientation, j, (Vec3{0, 0, 0}));
			DESERIALIZE_GLM_VEC3(&res, forward, j, (Vec3{0, 0, 1}));
			DESERIALIZE_PROP(&res, fov, j, 75.0f);
			return res;
		}
		break;
		default:
			break;
		}

		g_logger_warning("PerspectiveCamera serialized with unknown version: %d", version);
		return {};
	}

	PerspectiveCamera PerspectiveCamera::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			// position        -> Vec3
			// orientation     -> Vec3
			// forward         -> Vec3
			// fov             -> float
			PerspectiveCamera res;
			res.position = CMath::convert(CMath::legacy_deserializeVec3(memory));
			res.orientation = CMath::convert(CMath::legacy_deserializeVec3(memory));
			res.forward = CMath::convert(CMath::legacy_deserializeVec3(memory));
			memory.read<float>(&res.fov);

			return res;
		}

		PerspectiveCamera res = {};
		return res;
	}
}