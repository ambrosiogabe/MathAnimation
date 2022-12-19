#include "editor/EditorCameraController.h"
#include "editor/EditorGui.h"
#include "editor/EditorSettings.h"
#include "renderer/OrthoCamera.h"
#include "renderer/PerspectiveCamera.h"
#include "core/Input.h"

namespace MathAnim
{
	namespace EditorCameraController
	{
		static bool cameraIsDragging = false;
		static float mouseSensitivity = 0.1f;
		static float mouseScrollSensitivity = 0.035f;
		static Vec2 lastMouseWorldPos = Vec2{ 0, 0 };

		void updateOrtho(OrthoCamera& camera)
		{
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
					float cameraXStep = glm::log(camera.zoom);
					cameraXStep -= Input::scrollY * mouseScrollSensitivity * data.scrollSensitvity;
					camera.zoom = glm::clamp(glm::exp(cameraXStep), 0.01f, 100.0f);
				}
			}
		}

		void updatePerspective(PerspectiveCamera&)
		{

		}
	}
}
