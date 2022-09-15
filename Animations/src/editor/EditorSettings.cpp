#include "editor/EditorSettings.h"

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
			data->mouseSensitivity = 5.0f;
			data->scrollSensitvity = 5.0f;
		}

		void imgui()
		{
			if (data)
			{
				ImGui::Begin("Editor Settings");

				ImGui::DragFloat(": Camera Pan Sensitivity", &data->mouseSensitivity, 0.2f, 1.0f, 20.0f);
				ImGui::DragFloat(": Camera Zoom Sensitivity", &data->scrollSensitvity, 0.2f, 1.0f, 20.0f);

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
