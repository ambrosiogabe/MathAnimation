#include "editor/EditorSettings.h"
#include "renderer/GLApi.h"
#include "animation/AnimationManager.h"

namespace MathAnim
{
	namespace EditorSettings
	{
		static EditorSettingsData* data = nullptr;

		void init()
		{
			g_logger_assert(data == nullptr, "EditorSettings initialized twice.");
			data = (EditorSettingsData*)g_memory_allocate(sizeof(EditorSettingsData));

			// TODO: Load saved settings from file
			data->cameraRotateSensitivity = 5.0f;
			data->scrollSensitvity = 5.0f;
			data->previewFidelity = PreviewSvgFidelity::Medium;
			data->svgTargetScale = _previewFidelityValues[(int)data->previewFidelity];
			data->viewMode = ViewMode::Normal;
			data->activeObjectOutlineWidth = 9.0f;
			data->activeObjectHighlightColor = "#FF9E28"_hex;
			data->smoothCursor = false;
		}

		void imgui(AnimationManagerData* am)
		{
			if (data)
			{
				ImGui::Begin("Editor Settings");

				ImGui::DragFloat(": Camera Rotate Sensitivity", &data->cameraRotateSensitivity, 0.2f, 1.0f, 20.0f);
				ImGui::DragFloat(": Camera Zoom Sensitivity", &data->scrollSensitvity, 0.2f, 1.0f, 20.0f);

				ImGui::ColorEdit4(": Selection Highlight Color", &data->activeObjectHighlightColor.r);
				ImGui::DragFloat(": Selection Highlight Width", &data->activeObjectOutlineWidth, 0.2f, 1.0f, 50.0f);

				if (ImGui::BeginCombo("Preview Fidelity", _previewFidelityEnumNames[(int)data->previewFidelity]))
				{
					for (int i = 0; i < (int)PreviewSvgFidelity::Length; i++)
					{
						if (ImGui::Selectable(_previewFidelityEnumNames[i]))
						{
							data->previewFidelity = (PreviewSvgFidelity)i;
							data->svgTargetScale = _previewFidelityValues[i];
							AnimationManager::retargetSvgScales(am);
							ImGui::CloseCurrentPopup();
						}
					}
					ImGui::EndCombo();
				}

				if (ImGui::BeginCombo("View Mode", _viewModeEnumNames[(int)data->viewMode]))
				{
					for (int i = 0; i < (int)ViewMode::Length; i++)
					{
						if (ImGui::Selectable(_viewModeEnumNames[i]))
						{
							data->viewMode = (ViewMode)i;
							switch (data->viewMode)
							{
							case ViewMode::WireMesh:
								GL::lineWidth(3.0f);
								GL::polygonMode(GL_FRONT_AND_BACK, GL_LINE);
								break;
							case ViewMode::Normal:
								GL::polygonMode(GL_FRONT_AND_BACK, GL_FILL);
								break;
							case ViewMode::Length:
								break;
							}
							ImGui::CloseCurrentPopup();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::Checkbox(": Smooth Cursor", &data->smoothCursor);

				ImGui::End();
			}
		}

		void free()
		{
			if (data)
			{
				g_memory_free(data);
				data = nullptr;
			}
		}

		void setFidelity(PreviewSvgFidelity fidelity)
		{
			data->previewFidelity = fidelity;
			data->svgTargetScale = _previewFidelityValues[(int)fidelity];
		}

		const EditorSettingsData& getSettings()
		{
			return *data;
		}
	}
}
