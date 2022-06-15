#include "renderer/PerspectiveCamera.h"
#include "core/Application.h"

namespace MathAnim
{
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
}