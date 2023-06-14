#include "editor/EditorCameraController.h"
#include "editor/EditorGui.h"
#include "editor/EditorSettings.h"
#include "renderer/Camera.h"
#include "core/Input.h"
#include "core/Profiling.h"
#include "core/Serialization.hpp"
#include "math/CMath.h"
#include "renderer/Renderer.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	struct EditorCamera
	{
		Camera baseCamera;
		float pivotDistance;
	};

	namespace EditorCameraController
	{
		// NOTE: This should be slightly bigger than the camera z-distance in Camera::createDefault()
		static constexpr float defaultPivotDistance = 7.0f;

		static bool cameraIsPanning = false;
		static bool cameraIsRotating = false;
		static float mouseScrollSensitivity = 0.035f;
		static Vec3 lastMouseWorldPos = Vec3{ 0, 0, 0 };
		static Vec2 lastMouseScreenPos = Vec2{ 0, 0 };

		static bool resettingToDefault = false;
		static constexpr float resetTimeLength = 0.5f;
		static float resetTimeCounter = 0.0f;
		static Camera targetCamera;

		EditorCamera* init(const Camera& camera)
		{
			EditorCamera* res = (EditorCamera*)g_memory_allocate(sizeof(EditorCamera));
			res->baseCamera = camera;
			res->pivotDistance = defaultPivotDistance;
			return res;
		}

		void free(EditorCamera* editorCamera)
		{
			if (editorCamera != nullptr)
			{
				g_memory_free(editorCamera);
			}
		}

		void update(float deltaTime, EditorCamera* editorCamera)
		{
			MP_PROFILE_EVENT("EditorCameraController_UpdateOrtho");

			g_logger_assert(editorCamera != nullptr, "EditorCamera* cannot be nullptr.");
			Camera& camera = editorCamera->baseCamera;
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
					camera.calculateMatrices(true);
					editorCamera->pivotDistance = defaultPivotDistance;
					resettingToDefault = false;
				}
				resetTimeCounter += deltaTime;
				camera.calculateMatrices(true);
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
						Vec2 worldDelta = lastMouseScreenPos - mouseScreenPos;
						constexpr float epsilon = 0.001f;

						Vec3 focalPoint = camera.position + camera.forward * editorCamera->pivotDistance;

						if (!CMath::compare(worldDelta.y, 0.0f, epsilon))
						{
							camera.orientation = glm::angleAxis(worldDelta.y * data.cameraRotateSensitivity, CMath::convert(camera.right)) * camera.orientation;
						}

						if (!CMath::compare(worldDelta.x, 0.0f, epsilon))
						{
							camera.orientation = glm::angleAxis(worldDelta.x * data.cameraRotateSensitivity, glm::vec3(0.0f, 1.0f, 0.0f)) * camera.orientation;
						}

						Vec3 newForward = CMath::normalize(CMath::convert(camera.orientation * glm::vec3(0, 0, 1)));
						camera.position = focalPoint - (newForward * editorCamera->pivotDistance);
					}

					lastMouseScreenPos = mouseScreenPos;
					cameraIsRotating = true;
				}
			}

			if (EditorGui::mouseHoveredEditorViewport())
			{
				if (Input::scrollY != 0.0f)
				{
					// The steps we want to take are in linear space, but we want to zoom in exponential space.
					// So we figure out the delta by taking the log first, then step it linearly by the amount
					// we scrolled, then we take the exponential of the result to get it back to exponential space.

					float currentZoomLevel = camera.orthoZoomLevel;
					float cameraXStep = glm::log(currentZoomLevel);
					cameraXStep -= Input::scrollY * mouseScrollSensitivity * data.scrollSensitvity;
					float oldOrthoZoomLevel = camera.orthoZoomLevel;
					camera.orthoZoomLevel = glm::clamp(glm::exp(cameraXStep), camera.nearFarRange.min / 2.0f, camera.nearFarRange.max * 2.0f);
					float zoomDelta = camera.orthoZoomLevel - oldOrthoZoomLevel;

					// Change the camera position for 3D zoom effect
					// Move camera forwards/backwards
					if (!CMath::compare(zoomDelta, 0.0f))
					{
						camera.position -= camera.forward * zoomDelta * camera.focalDistance * 0.5f;
						editorCamera->pivotDistance += zoomDelta * camera.focalDistance * 0.5f;
					}
				}
			}

			camera.calculateMatrices(true);
		}

		void endFrame(EditorCamera* camera)
		{
			g_logger_assert(camera != nullptr, "EditorCamera* cannot be nullptr.");
			camera->baseCamera.endFrame();
		}

		const Camera& getCamera(const EditorCamera* editorCamera)
		{
			return editorCamera->baseCamera;
		}

		void serialize(const EditorCamera* camera, nlohmann::json& j)
		{
			SERIALIZE_OBJECT(j, camera, baseCamera);
			SERIALIZE_NON_NULL_PROP(j, camera, pivotDistance);
		}

		EditorCamera* deserialize(const nlohmann::json& j, uint32 version)
		{
			switch (version)
			{
			case 3:
			{
				EditorCamera* res = init(Camera::createDefault());
				DESERIALIZE_OBJECT(res, baseCamera, Camera, version, j);
				DESERIALIZE_PROP(res, pivotDistance, j, defaultPivotDistance);

				res->baseCamera.calculateMatrices(true);
				return res;
			}
			break;
			default:
				break;
			}

			g_logger_warning("EditorCamera serialized with unknown version: '{}'", version);
			return {};
		}
	}
}
