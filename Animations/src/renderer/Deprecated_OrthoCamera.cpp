#include "renderer/Deprecated_OrthoCamera.h"
#include "core/Application.h"
#include "core/Serialization.hpp"
#include "math/CMath.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	glm::mat4 OrthoCamera::calculateViewMatrix() const
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
		Vec2 halfSize = projectionSize / 2.0f * zoom;
		// Invert the y-coords so that the elements are laid out top->bottom
		return glm::ortho(-halfSize.x, halfSize.x, halfSize.y, -halfSize.y);
	};

	Vec2 OrthoCamera::reverseProject(const Vec2& normalizedInput) const
	{
		Vec2 ndcCoords = normalizedInput * 2.0f - Vec2{ 1.0f, 1.0f };
		glm::mat4 inverseView = glm::inverse(calculateViewMatrix());
		glm::mat4 inverseProj = glm::inverse(calculateProjectionMatrix());
		glm::vec4 res = glm::vec4(ndcCoords.x, ndcCoords.y, 0.0f, 1.0f);
		res = inverseView * inverseProj * res;
		return Vec2{ res.x, res.y };
	}

	void OrthoCamera::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_VEC(memory, this, position);
		SERIALIZE_VEC(memory, this, projectionSize);
		SERIALIZE_NON_NULL_PROP(memory, this, zoom);
	}

	OrthoCamera OrthoCamera::deserialize(const nlohmann::json& j, uint32 version)
	{
		if (version == 2)
		{
			OrthoCamera res = {};
			DESERIALIZE_VEC2(&res, position, j, (Vec2{ 0, 0 }));
			DESERIALIZE_VEC2(&res, projectionSize, j, (Vec2{ 18, 9 }));
			DESERIALIZE_PROP(&res, zoom, j, 1.0f);
			return res;
		}

		g_logger_warning("Unknown camera version: '{}'", version);
		return {};
	}

	OrthoCamera OrthoCamera::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			// position        -> Vec2
			// projectionSize  -> Vec2
			// zoom            -> float
			OrthoCamera res = {};
			res.position = CMath::legacy_deserializeVec2(memory);
			res.projectionSize = CMath::legacy_deserializeVec2(memory);
			memory.read<float>(&res.zoom);

			return res;
		}

		OrthoCamera res = {};
		return res;
	}
}