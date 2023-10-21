#include "editor/panels/CodeEditorPanelManager.h"
#include "editor/panels/CodeEditorPanel.h"
#include "editor/TextEditUndo.h"
#include "animation/AnimationManager.h"
#include "core/Input.h"
#include "renderer/Fonts.h"
#include "parsers/SyntaxHighlighter.h"

namespace MathAnim
{
	struct CodeEditorMetadata
	{
		CodeEditorPanelData* panel;
		uint64_t uuid;
		std::string windowName;
		bool isEditedWithoutSave;
		bool setFocus;
	};

	namespace CodeEditorPanelManager
	{
		// ----------- Internal functinons -----------

		// ----------- Internal data -----------
		static std::unordered_map<std::string, size_t> fileMap;
		static std::vector<CodeEditorMetadata> openEditors;
		static uint64_t uuidCounter = 0;
		static std::string nextFileToAdd = "";
		static int fileToMakeActive = -1;
		static int lineNumberToGoTo = -1;
		static const char* luaGrammarJsonFile = "./assets/customGrammars/lua.grammar.json";
		static SyntaxHighlighter const* luaGrammar = nullptr;
		static SyntaxTheme const* syntaxTheme = nullptr;

		static const char* codeFontRegularFile = "./assets/fonts/fira/FiraCode-SemiBold.ttf";
		static SizedFont* codeFont = nullptr;
		static std::unordered_map<uint32, uint8> loadedCodepoints;
		static std::unordered_set<uint32> loadedCodepointsSet;
		static int fontSizePx = 28;

		void init()
		{
			codeFont = Fonts::loadSizedFont(codeFontRegularFile, fontSizePx, CharRange::Ascii, false);
			
			Highlighters::importGrammar(luaGrammarJsonFile);
			luaGrammar = Highlighters::getImportedHighlighter(luaGrammarJsonFile);
			syntaxTheme = Highlighters::getTheme(HighlighterTheme::MonokaiNight);

			for (uint32 i = CharRange::Ascii.firstCharCode; i <= CharRange::Ascii.lastCharCode; i++)
			{
				loadedCodepointsSet.insert(i);
				loadedCodepoints[i] = (uint8)(loadedCodepointsSet.size() - 1);
			}
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
				nextWindow.windowName = nextWindow.panel->filepath.filename().string() + "##" + std::to_string(nextWindow.uuid);
				openEditors.emplace_back(nextWindow);
				fileMap[nextFileToAdd] = openEditors.size() - 1;

				// Create window real quick so that it's opened docked to the correct place
				ImGui::Begin(nextWindow.windowName.c_str());
				ImGui::End();

				nextFileToAdd = "";

				if (lineNumberToGoTo != -1)
				{
					CodeEditorPanel::setCursorToLine(*nextWindow.panel, lineNumberToGoTo);
					lineNumberToGoTo = -1;
				}
			}

			if (lineNumberToGoTo != -1 && fileToMakeActive != -1)
			{
				g_logger_assert(fileToMakeActive >= 0 && fileToMakeActive < openEditors.size(), "How did this happen?");
				auto& editor = openEditors[fileToMakeActive];

				if (lineNumberToGoTo < 0 || lineNumberToGoTo >= (int)editor.panel->totalNumberLines)
				{
					lineNumberToGoTo = 0;
				}

				CodeEditorPanel::setCursorToLine(*editor.panel, lineNumberToGoTo);
				editor.setFocus = true;

				lineNumberToGoTo = -1;
				fileToMakeActive = -1;
			}

			for (auto editor = openEditors.begin(); editor != openEditors.end();)
			{
				if (editor->setFocus)
				{
					ImGui::SetNextWindowFocus();
					editor->setFocus = false;
				}

				bool open = true;
				int windowFlags = editor->isEditedWithoutSave ? ImGuiWindowFlags_UnsavedDocument : 0;
				bool windowIsActive = ImGui::Begin(editor->windowName.c_str(), &open, windowFlags);

				if (windowIsActive) 
				{
					bool hasBeenEdited = CodeEditorPanel::update(*editor->panel);
					editor->isEditedWithoutSave = editor->isEditedWithoutSave || hasBeenEdited;
				}

				bool windowIsFocused = ImGui::IsWindowFocused();
				ImGui::End();

				if (windowIsFocused && Input::keyPressed(GLFW_KEY_S, KeyMods::Ctrl))
				{
					CodeEditorPanel::saveFile(*editor->panel);
					editor->isEditedWithoutSave = false;
				}

				if (!open)
				{
					fileMap.erase(editor->panel->filepath.string());
					CodeEditorPanel::free(editor->panel);
					editor = openEditors.erase(editor);
				}
				else
				{
					editor++;
				}
			}
		}

		void imguiStats()
		{
			ImGui::Separator();

			ImGui::Text("Text Editor Undo Stats");

			for (auto const& editor : openEditors)
			{
				ImGui::NewLine();
				ImGui::Text("%s", editor.windowName.c_str());
				UndoSystem::imguiStats(editor.panel->undoSystem);
			}

			ImGui::Separator();
		}

		void openFile(std::string const& filename, uint32 lineNumber)
		{
			if (auto iter = fileMap.find(filename); iter == fileMap.end())
			{
				nextFileToAdd = filename;
				lineNumberToGoTo = lineNumber;
			}
			else
			{
				fileToMakeActive = (int)iter->second;
				lineNumberToGoTo = lineNumber;
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

		uint8 addCharToFont(uint32 codepoint)
		{
			if (auto iter = loadedCodepointsSet.find(codepoint); iter == loadedCodepointsSet.end())
			{
				// If we're trying to load a newline, just return a dummy value since newlines can't be rendered
				if (codepoint == '\n')
				{
					loadedCodepointsSet.insert(codepoint);
					loadedCodepoints[codepoint] = (uint8)(loadedCodepointsSet.size() - 1);
					return (uint8)loadedCodepoints.size() - 1;
				}

				loadedCodepointsSet.insert(codepoint);
				SizedFont* newFont = Fonts::loadSizedFont(codeFontRegularFile, fontSizePx, loadedCodepointsSet, false);
				g_logger_assert(newFont == codeFont, "We should have loaded this font in place. Weird...");
				Fonts::unloadSizedFont(codeFont);

				if (loadedCodepointsSet.size() > (1 << 8))
				{
					g_logger_error("Not good, we ran out of room for our codepoints. We have more than 255 unique characters in all files.");
				}
				
				loadedCodepoints[codepoint] = (uint8)(loadedCodepointsSet.size() - 1);
				return (uint8)loadedCodepoints.size() - 1;
			}
			else
			{
				auto codepointIter = loadedCodepoints.find(codepoint);
				g_logger_assert(codepointIter != loadedCodepoints.end(), "We shouldn't have mismatched codepoints in the set and map.");
				return codepointIter->second;
			}
		}

		SyntaxHighlighter const& getHighlighter()
		{
			g_logger_assert(luaGrammar != nullptr, "This shouldn't happen.");
			return *luaGrammar;
		}

		SyntaxTheme const& getTheme()
		{
			g_logger_assert(syntaxTheme != nullptr, "This shouldn't happen.");
			return *syntaxTheme;
		}

		// ----------- Internal functinons -----------
	}
}