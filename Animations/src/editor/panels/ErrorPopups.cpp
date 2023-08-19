#include "editor/panels/ErrorPopups.h"
#include "renderer/Colors.h"
#include "svg/SvgParser.h"

namespace MathAnim
{
	namespace ErrorPopups
	{
		static bool svgErrorPopupOpen = false;
		static const char* ERROR_IMPORTING_SVG_POPUP = "Error Importing SVG##ERROR_IMPORTING_SVG";

		static bool missingFilePopupOpen = false;
		static const char* ERROR_MISSING_FILE_POPUP = "Error Missing File##ERROR_MISSING_FILE";
		static std::string missingFilename = "";
		static std::string missingFilenameAdditionalMsg = "";

		static bool textureLoadErrorPopupOpen = false;
		static const char* ERROR_LOADING_TEXTURE_POPUP = "Error Loading Texture##ERROR_LOADING_TEXTURE";
		static std::string textureLoadError = "";
		static std::string textureLoadName = "";

		// ------- Internal Funcs -------
		static void handleSvgErrorPopup();
		static void handleMissingFilePopup();
		static void handleTextureLoadErrorPopup();

		void update(AnimationManagerData*)
		{
			handleSvgErrorPopup();
			handleMissingFilePopup();
			handleTextureLoadErrorPopup();
		}

		void popupSvgImportError()
		{
			svgErrorPopupOpen = true;
		}

		static void handleSvgErrorPopup()
		{
			if (svgErrorPopupOpen)
			{
				ImGui::OpenPopup(ERROR_IMPORTING_SVG_POPUP);
				svgErrorPopupOpen = false;
			}

			// Always center this window when appearing
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

			if (ImGui::BeginPopupModal(ERROR_IMPORTING_SVG_POPUP, NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextColored(Colors::AccentRed[3], "Error:");
				ImGui::SameLine();
				ImGui::Text("%s", SvgParser::getLastError());
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

		void popupMissingFileError(const std::string& filename, const std::string& additionalMessage)
		{
			missingFilePopupOpen = true;
			missingFilename = filename;
			missingFilenameAdditionalMsg = additionalMessage;
		}

		static void handleMissingFilePopup()
		{
			if (missingFilePopupOpen)
			{
				ImGui::OpenPopup(ERROR_MISSING_FILE_POPUP);
				missingFilePopupOpen = false;
			}

			// Always center this window when appearing
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

			if (ImGui::BeginPopupModal(ERROR_MISSING_FILE_POPUP, NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (missingFilenameAdditionalMsg != "")
				{
					ImGui::Text("%s", missingFilenameAdditionalMsg.c_str());
				}
				ImGui::TextColored(Colors::AccentRed[3], "Error: ");
				ImGui::SameLine();
				ImGui::Text("File '%s' does not exist.", missingFilename.c_str());
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

		void popupTextureLoadError(const std::string& textureName, const std::string& errorMessage)
		{
			if (!textureLoadErrorPopupOpen)
			{
				textureLoadErrorPopupOpen = true;
				textureLoadError = errorMessage;
				textureLoadName = textureName;
			}
		}

		static void handleTextureLoadErrorPopup()
		{
			if (textureLoadErrorPopupOpen)
			{
				ImGui::OpenPopup(ERROR_LOADING_TEXTURE_POPUP);
				textureLoadErrorPopupOpen = false;
			}

			// Always center this window when appearing
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

			if (ImGui::BeginPopupModal(ERROR_LOADING_TEXTURE_POPUP, NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Failed to load texture '%s'.", textureLoadName.c_str());
				ImGui::TextColored(Colors::AccentRed[3], "Error: ");
				ImGui::SameLine();
				ImGui::Text("'%s'", textureLoadError.c_str());

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