#include "editor/panels/MenuBar.h"
#include "editor/imgui/ImGuiLayer.h"
#include "editor/EditorLayout.h"
#include "core/Application.h"
#include "core/Profiling.h"
#include "renderer/Colors.h"

#include <imgui.h>

namespace MathAnim
{
	namespace MenuBar
	{
		// -------------- Internal Variables --------------
		static const char* creditsPopupId = "CREDITS_POPUP_WINDOW";
		static bool openCreditsPopup = false;
		static const char* saveEditorLayoutPopupId = "Save Layout##SAVE_EDITOR_LAYOUT_WINDOW";
		static bool openSaveEditorLayoutPopup = false;

		static const char* errorPopupId = "Error##MENU_BAR_ERROR_POPUP";
		static std::string errorMessage = "";
		static bool openErrorPopup = false;

		// -------------- Internal Functions --------------
		static void creditsWindow();
		static void saveEditorLayoutPopup();
		static void errorPopup();

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
						openSaveEditorLayoutPopup = true;
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

			if (openSaveEditorLayoutPopup)
			{
				ImGui::OpenPopup(saveEditorLayoutPopupId);
				openSaveEditorLayoutPopup = false;
			}

			if (openErrorPopup)
			{
				ImGui::OpenPopup(errorPopupId);
				openErrorPopup = false;
			}

			creditsWindow();
			saveEditorLayoutPopup();
			errorPopup();
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

		static void saveEditorLayoutPopup()
		{
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

			if (ImGui::BeginPopupModal(saveEditorLayoutPopupId))
			{
				constexpr size_t saveNameBufferSize = 256;
				static char saveNameBuffer[saveNameBufferSize];
				ImGui::InputText(": Template Name", saveNameBuffer, saveNameBufferSize);

				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();
				if (ImGui::Button("Save"))
				{
					SaveEditorLayoutError errorCode = ImGuiLayer::saveEditorLayout(saveNameBuffer);
					openErrorPopup = errorCode != SaveEditorLayoutError::None;

					switch (errorCode)
					{
					case SaveEditorLayoutError::ReservedLayoutName:
						errorMessage = "Failed to save editor layout. Name '" + std::string(saveNameBuffer) + "' is reserved.";
						g_logger_warning("Failed to save editor layout. Name '{}' is reserved.", saveNameBuffer);
						break;
					case SaveEditorLayoutError::FailedToSaveImGuiIni:
						errorMessage = "Failed to save editor layout. Failed to save imgui file for '" + std::string(saveNameBuffer) + "'";
						g_logger_warning("Failed to save editor layout. Failed to save imgui file for '{}'.", saveNameBuffer);
						break;
					case SaveEditorLayoutError::FailedToConvertIniToJson:
						errorMessage = "Failed to save editor layout. Failed to save convert imgui file to json for '" + std::string(saveNameBuffer) + "'";
						g_logger_warning("Failed to save editor layout. Failed to save convert imgui file to json for '{}'.", saveNameBuffer);
						break;
					case SaveEditorLayoutError::None:
						break;
					}

					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

		static void errorPopup()
		{
			if (ImGui::BeginPopupModal(errorPopupId, NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextColored(Colors::AccentRed[3], "Error:");
				ImGui::SameLine();
				ImGui::Text("%s", errorMessage.c_str());
				ImGui::NewLine();
				ImGui::Separator();

				if (ImGui::Button("OK", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();

				ImGui::EndPopup();
			}
		}
	}
}