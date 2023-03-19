#include "editor/EditorCameraController.h"
#include "editor/EditorGui.h"
#include "editor/EditorSettings.h"
#include "renderer/Camera.h"
#include "core/Input.h"
#include "core/Profiling.h"
#include "math/CMath.h"

namespace MathAnim
{
	namespace EditorCameraController
	{
		static bool cameraIsDragging = false;
		static float mouseSensitivity = 0.1f;
		static float mouseScrollSensitivity = 0.035f;
		static Vec2 lastMouseWorldPos = Vec2{ 0, 0 };

		void update(Camera& camera)
		{
			MP_PROFILE_EVENT("EditorCameraController_UpdateOrtho");

			const EditorSettingsData& data = EditorSettings::getSettings();

			if (Input::mouseUp(MouseButton::Middle))
			{
				cameraIsDragging = false;
			}

			if (EditorGui::mouseHoveredEditorViewport() || cameraIsDragging)
			{
				if (Input::mouseDown(MouseButton::Middle))
				{
					Vec2 mouseWorldPos = EditorGui::mouseToNormalizedViewport();
					mouseWorldPos = camera.reverseProject(mouseWorldPos);
					if (cameraIsDragging)
					{
						Vec2 worldDelta = mouseWorldPos - lastMouseWorldPos;
						camera.position.x -= worldDelta.x * mouseSensitivity * data.mouseSensitivity;
						camera.position.y -= worldDelta.y * mouseSensitivity * data.mouseSensitivity;
					}

					lastMouseWorldPos = mouseWorldPos;
					cameraIsDragging = true;
				}
			}

			if (EditorGui::mouseHoveredEditorViewport())
			{
				if (Input::scrollY != 0.0f)
				{
					// Increase camera zoom logarithmically
					float cameraXStep = glm::log(camera.focusPlane);
					cameraXStep -= Input::scrollY * mouseScrollSensitivity * data.scrollSensitvity;
					camera.focusPlane = glm::clamp(glm::exp(cameraXStep), camera.nearFarRange.min / 2.0f, camera.nearFarRange.max * 2.0f);
				}
			}
		}
	}
}
