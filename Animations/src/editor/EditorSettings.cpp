#include "editor/EditorSettings.h"

namespace MathAnim
{
	namespace EditorSettings
	{
		static const char* viewModeStrings[(uint8)ViewMode::Length + 1] = {
			"Wire Mesh View",
			"Normal View",
			"Length"
		};

		static EditorSettingsData* data = nullptr;

		void init()
		{
			g_logger_assert(data == nullptr, "EditorSettings initialized twice.");
			data = (EditorSettingsData*)g_memory_allocate(sizeof(EditorSettingsData));

			// TODO: Load saved settings from file
			data->mouseSensitivity = 5.0f;
			data->scrollSensitvity = 5.0f;
			data->viewMode = ViewMode::Normal;
		}

		void imgui()
		{
			if (data)
			{
				ImGui::Begin("Editor Settings");

				ImGui::DragFloat(": Camera Pan Sensitivity", &data->mouseSensitivity, 0.2f, 1.0f, 20.0f);
				ImGui::DragFloat(": Camera Zoom Sensitivity", &data->scrollSensitvity, 0.2f, 1.0f, 20.0f);

				if (ImGui::BeginCombo("View Mode", viewModeStrings[(int)data->viewMode]))
				{
					for (int i = 0; i < (int)ViewMode::Length; i++)
					{
						if (ImGui::Selectable(viewModeStrings[i]))
						{
							data->viewMode = (ViewMode)i;
							switch (data->viewMode)
							{
							case ViewMode::WireMesh:
								glLineWidth(3.0f);
								glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
								break;
							case ViewMode::Normal:
								glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
								break;
							case ViewMode::Length:
								break;
							}
							ImGui::CloseCurrentPopup();
						}
					}
					ImGui::EndCombo();
				}

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

		const EditorSettingsData& getSettings()
		{
			return *data;
		}
	}
}
