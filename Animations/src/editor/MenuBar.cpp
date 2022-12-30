#include "editor/MenuBar.h"
#include "core/Application.h"

#include <imgui.h>

namespace MathAnim
{
	namespace MenuBar
	{
		// -------------- Internal Variables --------------
		static const char* creditsPopupId = "CREDITS_POPUP_WINDOW";
		static bool openCreditsPopup = false;

		// -------------- Internal Functions --------------
		static void creditsWindow();

		void update()
		{
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Save Project", "Ctrl+S"))
					{
						Application::saveProject();
					}

					if (ImGui::MenuItem("Open Project", "Ctrl+O"))
					{
						g_logger_warning("TODO: Implement open project menu bar item");
					}

					ImGui::Separator();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("About"))
				{
					if (ImGui::MenuItem("Credits"))
					{
						openCreditsPopup = true;
					}

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			if (openCreditsPopup)
			{
				ImGui::OpenPopup(creditsPopupId);
				openCreditsPopup = false;
			}

			creditsWindow();
		}

		// -------------- Internal Functions --------------
		static void creditsWindow()
		{
			if (ImGui::BeginPopupModal(creditsPopupId))
			{


				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemClicked())
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
	}
}