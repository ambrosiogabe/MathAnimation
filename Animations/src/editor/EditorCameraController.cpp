#include "editor/EditorCameraController.h"
#include "editor/EditorGui.h"
#include "editor/EditorSettings.h"
#include "renderer/Camera.h"
#include "core/Input.h"
#include "core/Profiling.h"
#include "math/CMath.h"

#include "renderer/Renderer.h"

namespace MathAnim
{
	namespace EditorCameraController
	{
		static bool cameraIsPanning = false;
		static bool cameraIsRotating = false;
		static float mouseScrollSensitivity = 0.035f;
		static Vec3 lastMouseWorldPos = Vec3{ 0, 0, 0 };
		static Vec2 lastMouseScreenPos = Vec2{ 0, 0 };

		static bool orthoToPerspectiveTransition = false;
		static float lastFov = 0.0f;

		void update(float deltaTime, Camera& camera)
		{
			MP_PROFILE_EVENT("EditorCameraController_UpdateOrtho");

			const EditorSettingsData& data = EditorSettings::getSettings();

			if (Input::mouseUp(MouseButton::Middle))
			{
				cameraIsPanning = false;
				cameraIsRotating = false;
			}

			// Reset to default settings
			if (EditorGui::mouseHoveredEditorViewport() && Input::keyPressed(GLFW_KEY_KP_DECIMAL))
			{
				camera = Camera::createDefault();
			}

			if (EditorGui::mouseHoveredEditorViewport() && Input::keyPressed(GLFW_KEY_KP_5))
			{
				if (camera.mode == CameraMode::Perspective)
				{
					camera.mode = CameraMode::Orthographic;
					EditorGui::displayActionText("Camera Mode: Orthographic");
				}
				else
				{
					camera.mode = CameraMode::Perspective;
					EditorGui::displayActionText("Camera Mode: Perspective");
				}
			}

			if (orthoToPerspectiveTransition)
			{
				camera.fov = glm::lerp(camera.fov, 0.0f, deltaTime);
				if (camera.fov < 0.5f)
				{
					camera.fov = 0.0f;
					orthoToPerspectiveTransition = false;
				}
			}

			// Handle camera panning using middle mouse button + left shift
			if (EditorGui::mouseHoveredEditorViewport() || cameraIsPanning)
			{
				if (Input::mouseDown(MouseButton::Middle) && Input::keyDown(GLFW_KEY_LEFT_SHIFT))
				{
					Vec2 normalizedDeviceCoords = EditorGui::mouseToNormalizedViewport();
					Vec3 mouseWorldPos = camera.reverseProject(normalizedDeviceCoords);
					if (cameraIsPanning)
					{
						Vec3 worldDelta = mouseWorldPos - lastMouseWorldPos;
						if (!CMath::compare(worldDelta, Vec3{ 0, 0, 0 }, 0.01f))
						{
							camera.position -= worldDelta * data.cameraPanSensitivity;
						}
						g_logger_info("Delta: %2.3f %2.3f %2.3f", worldDelta.x, worldDelta.y, worldDelta.z);
					}

					camera.calculateMatrices(true);
					// NOTE: We have to re-reverseProject the device coordinates since the camera
					//       moved which would invalidate the old coordinates
					lastMouseWorldPos = camera.reverseProject(normalizedDeviceCoords);
					cameraIsPanning = true;
				}
			}

			// Handle camera rotating using middle mouse button
			if (EditorGui::mouseHoveredEditorViewport() || cameraIsRotating)
			{
				if (Input::mouseDown(MouseButton::Middle) && Input::keyUp(GLFW_KEY_LEFT_SHIFT))
				{
					Vec2 mouseScreenPos = EditorGui::mouseToNormalizedViewport();
					if (cameraIsRotating)
					{
						Vec2 worldDelta = mouseScreenPos - lastMouseScreenPos;
						constexpr float epsilon = 0.001f;

						float focalDistance = (camera.nearFarRange.max - camera.nearFarRange.min) / 2.0f;
						Vec3 focalPoint = camera.position + camera.forward * focalDistance;

						if (!CMath::compare(worldDelta.x, 0.0f, epsilon))
						{
							camera.orientation = glm::angleAxis(worldDelta.x * data.cameraRotateSensitivity, glm::vec3(0.0f, 1.0f, 0.0f)) * camera.orientation;
						}

						if (!CMath::compare(worldDelta.y, 0.0f, epsilon))
						{
							camera.orientation = glm::angleAxis(worldDelta.y * data.cameraRotateSensitivity, CMath::convert(camera.right)) * camera.orientation;
						}

						Vec3 newForward = CMath::normalize(CMath::convert(camera.orientation * glm::vec3(0, 0, 1)));
						camera.position = focalPoint - (newForward * focalDistance);
					}

					lastMouseScreenPos = mouseScreenPos;
					cameraIsRotating = true;
				}
			}

			if (EditorGui::mouseHoveredEditorViewport())
			{
				if (Input::scrollY != 0.0f)
				{
					float currentZoomLevel = camera.orthoZoomLevel;
					float cameraXStep = glm::log(currentZoomLevel);
					cameraXStep -= Input::scrollY * mouseScrollSensitivity * data.scrollSensitvity;
					float oldOrthoZoomLevel = camera.orthoZoomLevel;
					camera.orthoZoomLevel = glm::clamp(glm::exp(cameraXStep), camera.nearFarRange.min / 2.0f, camera.nearFarRange.max * 2.0f);
					float zoomDelta = camera.orthoZoomLevel - oldOrthoZoomLevel;

					// Change the camera position for 3D zoom effect
					// Move camera forwards/backwards
					camera.position -= camera.forward * zoomDelta;
				}
			}
		}
	}
}
