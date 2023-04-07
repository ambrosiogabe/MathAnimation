#include "renderer/Camera.h"
#include "core/Serialization.hpp"
#include "math/CMath.h"

#include "renderer/Deprecated_OrthoCamera.h"
#include "renderer/Deprecated_PerspectiveCamera.h"
#include "animation/Animation.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	// ------------- Internal Variables -------------
	static constexpr glm::vec3 GLOBAL_UP = glm::vec3(0, 1, 0);
	static constexpr glm::vec3 GLOBAL_RIGHT = glm::vec3(1, 0, 0);
	static constexpr glm::vec3 GLOBAL_FORWARD = glm::vec3(0, 0, 1);

	void Camera::calculateMatrices(bool ignoreCache)
	{
		if (matricesAreCached && !ignoreCache)
		{
			return;
		}

		aspectRatio = (float)aspectRatioFraction.x / (float)aspectRatioFraction.y;

		forward = CMath::normalize(CMath::convert(orientation * GLOBAL_FORWARD));
		right = CMath::normalize(CMath::convert(orientation * GLOBAL_RIGHT));
		up = CMath::normalize(CMath::convert(orientation * GLOBAL_UP));

		fov = glm::clamp(fov, 1.0f, 270.0f);
		if (mode == CameraMode::Perspective)
		{
			projectionMatrix = glm::perspective(fov, aspectRatio, nearFarRange.min, nearFarRange.max);
		}
		else
		{
			Vec4 orthoLeftRightBottomTop = getLeftRightBottomTop();
			projectionMatrix = glm::ortho(
				orthoLeftRightBottomTop.values[0],
				orthoLeftRightBottomTop.values[1],
				orthoLeftRightBottomTop.values[2],
				orthoLeftRightBottomTop.values[3],
				nearFarRange.min,
				nearFarRange.max
			);
		}

		viewMatrix = glm::lookAt(
			CMath::convert(position),
			CMath::convert(position + forward),
			CMath::convert(up)
		);

		matricesAreCached = true;
	}

	void Camera::endFrame()
	{
		matricesAreCached = false;
	}

	Vec3 Camera::reverseProject(const Vec2& normalizedInput) const
	{
		Vec2 ndcCoords = normalizedInput * 2.0f - Vec2{ 1.0f, 1.0f };
		glm::mat4 inverseView = glm::inverse(viewMatrix);
		glm::mat4 inverseProj = glm::inverse(projectionMatrix);
		float zDepth = CMath::mapRange(nearFarRange, Vec2{ 0.0f, 1.0f }, focalDistance);
		glm::vec4 res = glm::vec4(ndcCoords.x, ndcCoords.y, zDepth, 1.0f);
		res = inverseView * inverseProj * res;
		return Vec3{ res.x, res.y, res.z };
	}

	Vec4 Camera::getLeftRightBottomTop() const
	{
		float sizeX = focalDistance * glm::tan(glm::radians(fov / 2.0f));
		float sizeY = sizeX / aspectRatio;

		float halfSizeX = -sizeX * 0.5f;
		float halfSizeY = -sizeY * 0.5f;

		return Vec4{
			-halfSizeX * orthoZoomLevel,
			halfSizeX * orthoZoomLevel,
			-halfSizeY * orthoZoomLevel,
			halfSizeY * orthoZoomLevel
		};
	}

	void Camera::serialize(nlohmann::json& j) const
	{
		SERIALIZE_GLM_QUAT(j, this, orientation);
		SERIALIZE_VEC(j, this, position);
		SERIALIZE_ENUM(j, this, mode, _cameraModeNames);
		SERIALIZE_NON_NULL_PROP(j, this, fov);
		SERIALIZE_VEC(j, this, aspectRatioFraction);
		SERIALIZE_VEC(j, this, nearFarRange);
		SERIALIZE_VEC(j, this, fillColor);
		SERIALIZE_NON_NULL_PROP(j, this, orthoZoomLevel);
		SERIALIZE_NON_NULL_PROP(j, this, focalDistance);
	}

	Camera Camera::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 3:
		{
			Camera res = createDefault();
			DESERIALIZE_GLM_QUAT(&res, orientation, j, res.orientation);
			DESERIALIZE_VEC3(&res, position, j, res.position);
			DESERIALIZE_ENUM(&res, mode, _cameraModeNames, CameraMode, j);
			DESERIALIZE_PROP(&res, fov, j, res.fov);
			DESERIALIZE_VEC2i(&res, aspectRatioFraction, j, res.aspectRatioFraction);
			DESERIALIZE_VEC2(&res, nearFarRange, j, res.nearFarRange);
			DESERIALIZE_VEC4(&res, fillColor, j, res.fillColor);
			DESERIALIZE_PROP(&res, orthoZoomLevel, j, res.orthoZoomLevel);
			DESERIALIZE_PROP(&res, focalDistance, j, res.focalDistance);

			res.matricesAreCached = false;
			res.calculateMatrices();

			return res;
		}
		break;
		default:
			break;
		}

		g_logger_warning("PerspectiveCamera serialized with unknown version: '{}'", version);
		return {};
	}

	Camera Camera::createDefault()
	{
		Camera res = {};

		// Quaternion from euler angles
		res.orientation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
		res.position = Vec3{ 9.0f, 4.5f, -20.0f };
		res.mode = CameraMode::Orthographic;
		res.fov = 75.0f;
		res.aspectRatioFraction.x = 1920;
		res.aspectRatioFraction.y = 1080;
		res.nearFarRange = Vec2{ 1.0f, 100.0f };
		const Vec4 greenBrown = "#272822FF"_hex;
		res.fillColor = greenBrown;
		res.orthoZoomLevel = 1.0f;
		res.focalDistance = 20.0f;

		res.calculateMatrices();

		return res;
	}

	Camera Camera::upgrade(const CameraObject& legacy_camera)
	{
		Camera res = createDefault();

		if (legacy_camera.is2D)
		{
			res.position = CMath::vector3From2(legacy_camera.camera2D.position);
			res.position.z = -2.0f;
			res.aspectRatioFraction.x = (int)legacy_camera.camera2D.projectionSize.x;
			res.aspectRatioFraction.y = (int)legacy_camera.camera2D.projectionSize.y;
			res.mode = CameraMode::Orthographic;
		}
		else
		{
			res.fov = legacy_camera.camera3D.fov;
			res.orientation = glm::quat(legacy_camera.camera3D.orientation);
			res.position = CMath::convert(legacy_camera.camera3D.position);
			res.mode = CameraMode::Perspective;
		}

		res.matricesAreCached = false;
		res.calculateMatrices();

		return res;
	}
}