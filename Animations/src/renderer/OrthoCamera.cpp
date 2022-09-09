#include "renderer/OrthoCamera.h"
#include "core/Application.h"

namespace MathAnim
{
	glm::mat4 OrthoCamera::calculateViewMatrix()
	{
		glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 localRight = glm::cross(forward, glm::vec3(0, 1, 0));
		glm::vec3 localUp = glm::cross(localRight, forward);
		glm::vec3 vec3Pos = glm::vec3(position.x, position.y, 0.0f);

		return glm::lookAt(
			vec3Pos,
			vec3Pos + forward,
			localUp
		);
	}

	glm::mat4 OrthoCamera::calculateProjectionMatrix() const
	{
		Vec2 halfSize = projectionSize / 2.0f;
		// Invert the y-coords so that the elements are laid out top->bottom
		return glm::ortho(-halfSize.x, halfSize.x, halfSize.y, -halfSize.y);
	};
}