#ifndef MATH_ANIMATIONS_CAMERA_H
#define MATH_ANIMATIONS_CAMERA_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct CameraObject;

	enum class CameraMode : uint8
	{
		Orthographic,
		Perspective,
		Length
	};

	constexpr auto _cameraModeNames = fixedSizeArray<const char*, (size_t)CameraMode::Length>(
		"Orthographic",
		"Perspective"
	);

	struct Camera
	{
		// Cached variables
		glm::mat4 perspProjectionMatrix;
		glm::mat4 orthoProjectionMatrix;
		glm::mat4 viewMatrix;
		Vec3 forward;
		Vec3 right;
		float aspectRatio;
		Vec3 up;
		bool matricesAreCached;

		// Parameters
		glm::quat orientation;
		Vec3 position;
		CameraMode mode;
		float fov;
		Vec2i aspectRatioFraction;
		Vec2 nearFarRange;
		Vec4 fillColor;

		void calculateMatrices();
		void endFrame();
		Vec2 reverseProject(const Vec2& normalizedInput) const;
		Vec4 getLeftRightBottomTop() const;

		inline const glm::mat4& getProjectionMatrix() const
		{
			static glm::mat4 dummy(1.0f);
			return mode == CameraMode::Orthographic
				? orthoProjectionMatrix
				: mode == CameraMode::Perspective
				? perspProjectionMatrix
				: dummy;
		}

		void serialize(nlohmann::json& j) const;
		static Camera deserialize(const nlohmann::json& j, uint32 version);
		static Camera createDefault();
		static Camera upgrade(const CameraObject& legacy_camera);
	};
}

#endif 