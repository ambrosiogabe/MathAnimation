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
		glm::mat4 projectionMatrix;
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
		Vec2i aspectRatioFraction;
		Vec2 nearFarRange;
		float fov;
		float orthoZoomLevel;
		float focalDistance;
		Vec4 fillColor;

		void calculateMatrices(bool ignoreCache = false);
		void endFrame();
		Vec3 reverseProject(const Vec2& normalizedInput, float zDepth) const;
		inline Vec3 reverseProject(const Vec2& normalizedInput) const { return reverseProject(normalizedInput, nearFarRange.max); }
		Vec4 getLeftRightBottomTop() const;

		void serialize(nlohmann::json& j) const;
		static Camera deserialize(const nlohmann::json& j, uint32 version);
		static Camera createDefault();
		static Camera upgrade(const CameraObject& legacy_camera);
	};
}

#endif 