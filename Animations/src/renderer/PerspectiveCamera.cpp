#include "renderer/PerspectiveCamera.h"
#include "core/Application.h"
#include "utils/CMath.h"

namespace MathAnim
{
	// -------------- Internal Functions --------------
	static PerspectiveCamera deserializeCameraV1(RawMemory& memory);

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

	void PerspectiveCamera::serialize(RawMemory& memory) const
	{
		// position        -> Vec3
		// orientation     -> Vec3
		// forward         -> Vec3
		// fov             -> float
		CMath::serialize(memory, Vec3{ position.x, position.y, position.z });
		CMath::serialize(memory, Vec3{ orientation.x, orientation.y, orientation.z });
		CMath::serialize(memory, Vec3{ forward.x, forward.y, forward.z });
		memory.write<float>(&fov);
	}
	
	PerspectiveCamera PerspectiveCamera::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeCameraV1(memory);
		}

		PerspectiveCamera res = {};
		return res;
	}

	// -------------- Internal Functions --------------
	static PerspectiveCamera deserializeCameraV1(RawMemory& memory)
	{
		// position        -> Vec3
		// orientation     -> Vec3
		// forward         -> Vec3
		// fov             -> float
		PerspectiveCamera res;
		res.position = CMath::convert(CMath::deserializeVec3(memory));
		res.orientation = CMath::convert(CMath::deserializeVec3(memory));
		res.forward = CMath::convert(CMath::deserializeVec3(memory));
		memory.read<float>(&res.fov);

		return res;
	}
}