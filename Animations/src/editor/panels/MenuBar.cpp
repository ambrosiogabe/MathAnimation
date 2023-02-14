#include "editor/panels/MenuBar.h"
#include "editor/imgui/ImGuiLayer.h"
#include "editor/EditorLayout.h"
#include "core/Application.h"
#include "core/Profiling.h"

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
			MP_PROFILE_EVENT("MenuBar_Update");

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

					if (ImGui::MenuItem("Save Editor Layout"))
					{
						ImGuiLayer::saveEditorLayout();
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Edit"))
				{
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					if (ImGui::BeginMenu("Layouts"))
					{
						if (ImGui::MenuItem("Save Custom Layout"))
						{
							g_logger_warning("TODO: Implement Layout Saving");
						}

						ImGui::Separator();

						const std::vector<std::filesystem::path>& defaultLayouts = EditorLayout::getDefaultLayouts();
						for (const auto& layout : defaultLayouts)
						{
							if (ImGui::MenuItem(layout.stem().string().c_str()))
							{
								glm::vec2 windowSize = Application::getAppWindowSize();
								ImGuiLayer::loadEditorLayout(layout, Vec2{ windowSize.x, windowSize.y });
							}
						}

						const std::vector<std::filesystem::path>& customLayouts = EditorLayout::getCustomLayouts();
						if (customLayouts.size() > 0)
						{
							ImGui::Separator();
						}

						for (const auto& layout : customLayouts)
						{
							if (ImGui::MenuItem(layout.stem().string().c_str()))
							{
								glm::vec2 windowSize = Application::getAppWindowSize();
								ImGuiLayer::loadEditorLayout(layout, Vec2{ windowSize.x, windowSize.y });
							}
						}

						ImGui::EndMenu();
					}

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