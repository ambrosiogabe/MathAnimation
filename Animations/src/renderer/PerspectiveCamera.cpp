#include "renderer/PerspectiveCamera.h"
#include "core/Application.h"
#include "math/CMath.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	// -------------- Internal Functions --------------
	static PerspectiveCamera deserializeCameraV1(const nlohmann::json& j);

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
		CMath::serialize(memory, "Position", Vec3{position.x, position.y, position.z});
		CMath::serialize(memory, "Orientation", Vec3{orientation.x, orientation.y, orientation.z});
		CMath::serialize(memory, "Forward", Vec3{forward.x, forward.y, forward.z});
		memory["FieldOfView"] = fov;
	}
	
	PerspectiveCamera PerspectiveCamera::deserialize(const nlohmann::json& j, uint32 version)
	{
		if (version == 2)
		{
			return deserializeCameraV1(j);
		}

		g_logger_warning("Perspective camera serialized with unknown version: %d", version);
		PerspectiveCamera res = {};
		return res;
	}

	// -------------- Internal Functions --------------
	static PerspectiveCamera deserializeCameraV1(const nlohmann::json& j)
	{
		// position        -> Vec3
		// orientation     -> Vec3
		// forward         -> Vec3
		// fov             -> float
		PerspectiveCamera res;
		res.position = CMath::convert(CMath::deserializeVec3(j["Position"]));
		res.orientation = CMath::convert(CMath::deserializeVec3(j["Orientation"]));
		res.forward = CMath::convert(CMath::deserializeVec3(j["Forward"]));
		res.fov = j.contains("FieldOfView") ? j["FieldOfView"] : 75.0f;

		return res;
	}
}