#include "editor/panels/CodeEditorPanelManager.h"
#include "editor/panels/CodeEditorPanel.h"
#include "animation/AnimationManager.h"
#include "core/Input.h"
#include "renderer/Fonts.h"

namespace MathAnim
{
	struct CodeEditorMetadata
	{
		CodeEditorPanelData panel;
		uint64_t uuid;
		std::string windowName;
	};

	namespace CodeEditorPanelManager
	{
		// ----------- Internal data -----------
		static std::unordered_map<std::string, size_t> fileMap;
		static std::vector<CodeEditorMetadata> openEditors;
		static uint64_t uuidCounter = 0;
		static std::string nextFileToAdd = "";

		static const char* codeFontRegularFile = "./assets/fonts/fira/FiraCode-SemiBold.ttf";
		static SizedFont* codeFont = nullptr;
		static int fontSizePx = 28;

		void init()
		{
			codeFont = Fonts::loadSizedFont(codeFontRegularFile, fontSizePx, CharRange::Ascii, false);
		}

		void free()
		{
			Fonts::unloadSizedFont(codeFont);

			for (auto& editor : openEditors)
			{
				CodeEditorPanel::free(editor.panel);
			}
		}

		void update(AnimationManagerData const*, ImGuiID parentDockId)
		{
			if (nextFileToAdd != "")
			{
				ImGui::SetNextWindowDockID(parentDockId, ImGuiCond_Appearing);

				CodeEditorMetadata nextWindow = {};
				nextWindow.panel = CodeEditorPanel::openFile(nextFileToAdd);
				nextWindow.uuid = uuidCounter++;
				nextWindow.windowName = nextWindow.panel.filepath.filename().string() + "##" + std::to_string(nextWindow.uuid);
				openEditors.emplace_back(nextWindow);
				fileMap[nextFileToAdd] = openEditors.size() - 1;

				// Create window real quick so that it's opened docked to the correct place
				ImGui::Begin(nextWindow.windowName.c_str());
				ImGui::End();

				nextFileToAdd = "";
			}

			for (auto editor = openEditors.begin(); editor != openEditors.end();)
			{
				bool open = true;
				ImGui::Begin(editor->windowName.c_str(), &open);

				CodeEditorPanel::update(editor->panel);

				ImGui::End();

				if (!open)
				{
					fileMap.erase(editor->panel.filepath.string());
					CodeEditorPanel::free(editor->panel);
					editor = openEditors.erase(editor);
				}
				else
				{
					editor++;
				}
			}
		}

		void openFile(std::string const& filename)
		{
			if (fileMap.find(filename) == fileMap.end())
			{
				nextFileToAdd = filename;
			}
		}

		void closeFile(std::string const& file)
		{
			if (auto iter = fileMap.find(file); iter != fileMap.end())
			{
				size_t index = iter->second;
				if (index < openEditors.size())
				{
					CodeEditorPanel::free(openEditors[index].panel);
					openEditors.erase(openEditors.begin() + index);
					fileMap.erase(iter);
				}
				else
				{
					g_logger_warning("Tried to close editor with invalid index '{}'. Max size '{}'.", index, openEditors.size());
				}
			}
		}

		SizedFont const* const getCodeFont()
		{
			return codeFont;
		}

		int getCodeFontSizePx()
		{
			return fontSizePx;
		}

		void setCodeFontSizePx(int inFontSizePx)
		{
			Fonts::unloadSizedFont(codeFont);
			fontSizePx = inFontSizePx;
			codeFont = Fonts::loadSizedFont(codeFontRegularFile, fontSizePx, CharRange::Ascii, false);
		}
	}
}