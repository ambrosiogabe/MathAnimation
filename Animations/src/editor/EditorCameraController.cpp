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

		static bool resettingToDefault = false;
		static constexpr float resetTimeLength = 0.5f;
		static float resetTimeCounter = 0.0f;
		static Camera targetCamera;

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
				resettingToDefault = true;
				targetCamera = Camera::createDefault();
				EditorGui::displayActionText("Editor Camera: Reset to Default '.'");
				resetTimeCounter = 0.0f;
			}

			if (resettingToDefault)
			{
				camera.orientation = glm::slerp(camera.orientation, targetCamera.orientation, resetTimeCounter / resetTimeLength);
				camera.position = CMath::convert(glm::lerp(CMath::convert(camera.position), CMath::convert(targetCamera.position), resetTimeCounter / resetTimeLength));
				camera.focalDistance = glm::lerp(camera.focalDistance, targetCamera.focalDistance, resetTimeCounter / resetTimeLength);
				camera.orthoZoomLevel = glm::lerp(camera.orthoZoomLevel, targetCamera.orthoZoomLevel, resetTimeCounter / resetTimeLength);
				if (resetTimeCounter >= resetTimeLength)
				{
					CameraMode oldMode = camera.mode;
					camera = targetCamera;
					camera.mode = oldMode;
					resettingToDefault = false;
				}
				resetTimeCounter += deltaTime;
				return;
			}

			if (EditorGui::mouseHoveredEditorViewport() && Input::keyPressed(GLFW_KEY_KP_5))
			{
				if (camera.mode == CameraMode::Perspective)
				{
					camera.mode = CameraMode::Orthographic;
					EditorGui::displayActionText("Editor Camera: Orthographic Mode '5'");
				}
				else
				{
					camera.mode = CameraMode::Perspective;
					EditorGui::displayActionText("Editor Camera: Perspective Mode '5'");
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
							camera.position -= worldDelta;
						}
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

						Vec3 focalPoint = camera.position + camera.forward * camera.focalDistance * camera.orthoZoomLevel;

						if (!CMath::compare(worldDelta.y, 0.0f, epsilon))
						{
							camera.orientation = glm::angleAxis(worldDelta.y * data.cameraRotateSensitivity, CMath::convert(camera.right)) * camera.orientation;
						}

						if (!CMath::compare(worldDelta.x, 0.0f, epsilon))
						{
							camera.orientation = glm::angleAxis(worldDelta.x * data.cameraRotateSensitivity, glm::vec3(0.0f, 1.0f, 0.0f)) * camera.orientation;
						}

						Vec3 newForward = CMath::normalize(CMath::convert(camera.orientation * glm::vec3(0, 0, 1)));
						camera.position = focalPoint - (newForward * camera.focalDistance * camera.orthoZoomLevel);
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
					camera.position -= camera.forward * zoomDelta * 20.0f;
				}
			}
		}
	}
}
