#include "editor/EditorSettings.h"
#include "editor/panels/CodeEditorPanel.h"
#include "editor/panels/CodeEditorPanelManager.h"
#include "editor/imgui/ImGuiLayer.h"
#include "editor/TextEditUndo.h"
#include "platform/Platform.h"
#include "core/Application.h"
#include "core/Input.h"
#include "core/Window.h"
#include "math/CMath.h"
#include "renderer/Fonts.h"
#include "parsers/SyntaxTheme.h"
#include "scripting/LuauLayer.h"
#include "scripting/ScriptAnalyzer.h"

#include <cppUtils/cppStrings.hpp>

using namespace CppUtils;

namespace MathAnim
{
	enum class KeyMoveDirection : uint8
	{
		Left,
		Right,
		Up,
		Down,
		LeftUntilBoundary,
		RightUntilBoundary,
		LeftUntilBeginning,
		RightUntilEnd
	};

	namespace CodeEditorPanel
	{
		// ------------- Internal Variables -------------
		static bool inspectCodeStyles = false;
		static bool visualizeSyntaxUpdates = false;
		static uint32 numMsToShowLineUpdates = 500;
		static Vec4 flashColor = "#36d174"_hex;

		static float maxTimeToInterpolateCursor = 0.1f;

		static float minIntellisensePanelWidth = 400.0f;
		static float intellisensePanelBorderWidth = 2.0f;
		static float intellisenseSuggestionSpacing = 1.0f;
		static float intellisenseFrameRounding = 4.0f;
		static uint32 maxIntellisenseSuggestions = 12;

		// ------------- Internal Functions -------------
		static void resetSelection(CodeEditorPanelData& panel);
		static void handleTypingUndo(CodeEditorPanelData& panel);
		static void handleBackspaceUndo(CodeEditorPanelData& panel, bool shouldSetTextSelectedOnUndo);
		static void handleDeleteUndo(CodeEditorPanelData& panel, bool shouldSetTextSelectedOnUndo);
		static void moveTextCursor(CodeEditorPanelData& panel, KeyMoveDirection direction);
		static void moveTextCursorAndResetSelection(CodeEditorPanelData& panel, KeyMoveDirection direction);
		static void renderTextCursor(CodeEditorPanelData& panel, ImVec2 textCursorDrawPosition, SizedFont const* const font);
		static void renderIntellisensePanel(CodeEditorPanelData& panel, SizedFont const* const font);
		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber, SizedFont const* const font);
		static bool mouseInTextEditArea(CodeEditorPanelData const& panel);
		static ImVec2 addStringToDrawList(ImDrawList* drawList, SizedFont const* const font, std::string const& str, ImVec2 const& drawPos, ImColor const& color);
		static ImVec2 getGlyphSize(SizedFont const* const sizedFont, uint32 charToAdd);
		static ImVec2 addCharToDrawList(ImDrawList* drawList, SizedFont const* const font, uint32 charToAdd, ImVec2 const& drawPos, ImColor const& color);
		static bool removeSelectedTextWithBackspace(CodeEditorPanelData& panel, bool addBackspaceToUndoHistory = true);
		static bool removeSelectedTextWithDelete(CodeEditorPanelData& panel);
		static bool removeText(CodeEditorPanelData& panel, int32 textToRemoveOffset, int32 textToRemoveNumBytes);
		static int32 getNewCursorPositionFromMove(CodeEditorPanelData const& panel, KeyMoveDirection direction);
		static void scrollCursorIntoViewIfNeeded(CodeEditorPanelData& panel);
		static void openIntellisensePanelAtCursor(CodeEditorPanelData& panel);
		// TODO: Move this to <cppStrings.hpp>
		static uint8 codepointToUtf8Str(uint8* const buffer, uint32 codepoint);

		static int32 getBeginningOfLineFrom(CodeEditorPanelData const& panel, int32 index);
		static int32 getNumCharsFromBeginningOfLine(CodeEditorPanelData const& panel, int32 index);
		static int32 getEndOfLineFrom(CodeEditorPanelData const& panel, int32 index);
		static void setCursorDistanceFromLineStart(CodeEditorPanelData& panel);
		static uint32 getLineNumberByteStartFrom(CodeEditorPanelData const& panel, uint32 lineNumber);
		static uint32 getLineNumberFromPosition(CodeEditorPanelData const& panel, uint32 bytePos);
		static float calculateLeftGutterWidth(CodeEditorPanelData const& panel);

		// Simple calculation functions
		static inline ImColor getColor(Vec4 const& color)
		{
			return ImColor(color.r, color.g, color.b, color.a);
		}

		static inline float getLineHeight(SizedFont const* const font)
		{
			return font->unsizedFont->lineHeight * font->fontSizePixels;
		}

		static inline ImVec2 getTopLeftOfLine(CodeEditorPanelData const& panel, uint32 lineNumber, SizedFont const* const font)
		{
			return panel.drawStart
				+ ImVec2(0.0f, getLineHeight(font) * (lineNumber - panel.lineNumberStart));
		}

		static inline float getDocumentHeight(CodeEditorPanelData const& panel, SizedFont const* const font)
		{
			return getLineHeight(font) * panel.totalNumberLines;
		}

		static inline bool mouseIntersectsRect(ImVec2 rectStart, ImVec2 rectEnd)
		{
			ImGuiIO& io = ImGui::GetIO();
			return io.MousePos.x >= rectStart.x && io.MousePos.x <= rectEnd.x &&
				io.MousePos.y >= rectStart.y && io.MousePos.y <= rectEnd.y;
		}

		// ------------- Internal Data -------------
		static float baseLeftGutterWidth = 130;
		static float additionalLeftGutterWidth = 25;
		static float scrollbarWidth = 10;
		static float textCursorWidth = 3;
		static uint32 numberBufferLines = 30;
		static float baseNewlineCharacterWidth = 18.0f;

		static ImColor backgroundColor = ImColor(44, 44, 44);

		static ImColor editorSelectionBackground = ImColor(115, 163, 240, 128);
		static ImColor editorCursorForeground = ImColor(255, 255, 255);

		static ImColor editorLineNumberForeground = ImColor(230, 230, 230);
		static ImColor editorLineNumberActiveForeground = ImColor(255, 255, 255);

		static ImColor scrollbarSliderBackground = ImColor(95, 95, 95);
		static ImColor scrollbarSliderActiveBackground = scrollbarSliderBackground;
		static ImColor scrollbarSliderHoverBackground = scrollbarSliderBackground;

		// TODO: Make this configurable
		static int MAX_UNDO_HISTORY = 1024;

		// This is the number of seconds between the cursor blinking
		static float cursorBlinkTime = 0.5f;

		CodeEditorPanelData* openFile(std::string const& filepath)
		{
			CodeEditorPanelData* res = g_memory_new CodeEditorPanelData();
			res->filepath = filepath;

			FILE* fp = fopen(filepath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			RawMemory memory;
			memory.init(fileSize);
			fread(memory.data, fileSize, 1, fp);
			fclose(fp);

			// Preprocess file into usable buffer
			res->visibleCharacterBuffer = nullptr;
			res->visibleCharacterBufferSize = 0;
			res->lineNumberStart = 1;
			res->lineNumberByteStart = 0;
			res->undoTypingStart = -1;
			res->totalNumberLines = 0;
			res->hzCharacterOffset = 0;
			res->maxLineLength = 0;

			res->intellisensePanelOpen = false;
			res->intellisenseScrollOffset = 0;
			res->selectedIntellisenseSuggestion = 0;

			preprocessText((uint8*)memory.data, fileSize, &res->visibleCharacterBuffer, &res->visibleCharacterBufferSize, &res->totalNumberLines, &res->maxLineLength);
			// +1 for the extra line for EOF
			res->totalNumberLines++;
			res->cursor = CppUtils::String::makeIter(res->visibleCharacterBuffer, res->visibleCharacterBufferSize);

			// Parse the syntax
			res->syntaxHighlightTree = CodeEditorPanelManager::getHighlighter().parse(
				(const char*)res->visibleCharacterBuffer,
				res->visibleCharacterBufferSize,
				CodeEditorPanelManager::getTheme()
			);

			res->undoSystem = UndoSystem::createTextEditorUndoSystem(res, MAX_UNDO_HISTORY);

			g_memory_free(memory.data);

			return res;
		}

		void saveFile(CodeEditorPanelData const& panel)
		{
			uint8* utf8String = nullptr;
			size_t utf8StringNumBytes = 0;
			postprocessText(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, &utf8String, &utf8StringNumBytes);

			// Dump UTF-8 to file
			FILE* fp = fopen(panel.filepath.string().c_str(), "wb");
			fwrite(utf8String, utf8StringNumBytes, 1, fp);
			fclose(fp);

			// Free memory
			g_memory_free(utf8String);
		}

		void free(CodeEditorPanelData* panel)
		{
			UndoSystem::free(panel->undoSystem);
			g_memory_free((void*)panel->visibleCharacterBuffer);
			g_memory_delete(panel);
		}

		void reparseSyntax(CodeEditorPanelData& panel)
		{
			// Parse the syntax
			panel.syntaxHighlightTree = CodeEditorPanelManager::getHighlighter().parse(
				(const char*)panel.visibleCharacterBuffer,
				panel.visibleCharacterBufferSize,
				CodeEditorPanelManager::getTheme()
			);

		}

		bool update(CodeEditorPanelData& panel)
		{
			bool fileHasBeenEdited = false;

			SizedFont const* const codeFont = CodeEditorPanelManager::getCodeFont();
			ImGuiStyle& style = ImGui::GetStyle();
			ImGuiIO& io = ImGui::GetIO();
			panel.timeSinceCursorLastBlinked += Application::getDeltaTime();

			// ---- Handle arrow key presses ----

			bool windowIsFocused = ImGui::IsWindowFocused();
			bool windowIsHovered = ImGui::IsWindowHovered();
			if (windowIsFocused)
			{
				io.WantTextInput = true;

				// Handle copy/paste
				if (Input::keyRepeatedOrDown(GLFW_KEY_V, KeyMods::Ctrl))
				{
					handleTypingUndo(panel);

					if (panel.firstByteInSelection != panel.lastByteInSelection)
					{
						// TODO: In the future, we should treat paste over text as one undo action
						//       so you don't have to undo twice to get back to the original state
						removeSelectedTextWithBackspace(panel);
					}

					// Get clipboard string
					const uint8* utf8String = Application::getWindow().getClipboardString();
					size_t utf8StringNumBytes = strlen((const char*)utf8String);

					// Figure out how many characters exist in this string
					uint8* outStr = nullptr;
					size_t numCharacters;
					preprocessText((uint8*)utf8String, utf8StringNumBytes, &outStr, &numCharacters);

					// Free the new memory since we don't actually need the string
					g_memory_free(outStr);

					// Perform the paste and add an undo action
					int32 insertPosition = (int32)panel.cursor.bytePos;
					addUtf8StringToBuffer(panel, (uint8*)utf8String, utf8StringNumBytes, panel.cursor.bytePos);

					UndoSystem::insertTextAction(panel.undoSystem, utf8String, utf8StringNumBytes, insertPosition, numCharacters);
				}
				else if (Input::keyRepeatedOrDown(GLFW_KEY_C, KeyMods::Ctrl))
				{
					// Only copy if we have text selected
					if (panel.lastByteInSelection != panel.firstByteInSelection)
					{
						g_logger_assert(panel.lastByteInSelection > panel.firstByteInSelection, "This shouldn't happen.");
						g_logger_assert(panel.lastByteInSelection <= panel.visibleCharacterBufferSize, "This shouldn't happen either.");

						uint8* utf8String = nullptr;
						size_t utf8StringSize = 0;

						postprocessText(
							panel.visibleCharacterBuffer + panel.firstByteInSelection,
							(panel.lastByteInSelection - panel.firstByteInSelection),
							&utf8String,
							&utf8StringSize
						);

						// Add null-byte for GLFW
						utf8String = (uint8*)g_memory_realloc(utf8String, utf8StringSize + 1);
						utf8String[utf8StringSize] = '\0';

						Application::getWindow().setClipboardString((const char*)utf8String);

						// Free temporary allocation
						g_memory_free(utf8String);
					}
				}

				// Special hotkeys like Ctrl+A
				if (Input::keyRepeatedOrDown(GLFW_KEY_A, KeyMods::Ctrl))
				{
					panel.firstByteInSelection = 0;
					panel.lastByteInSelection = (int32)panel.visibleCharacterBufferSize;
					panel.cursor.bytePos = (int32)panel.visibleCharacterBufferSize;
					panel.mouseByteDragStart = 0;
				}

				// Handle undo/redo
				if (Input::keyRepeatedOrDown(GLFW_KEY_Z, KeyMods::Ctrl))
				{
					handleTypingUndo(panel);
					UndoSystem::undo(panel.undoSystem);
				}
				else if (Input::keyRepeatedOrDown(GLFW_KEY_Z, KeyMods::Ctrl | KeyMods::Shift) || Input::keyRepeatedOrDown(GLFW_KEY_Y, KeyMods::Ctrl))
				{
					UndoSystem::redo(panel.undoSystem);
				}

				// Handle key presses to move cursor without the select modifier (Shift) being pressed
				{
					if (Input::keyRepeatedOrDown(GLFW_KEY_RIGHT))
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::Right);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_LEFT))
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::Left);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_UP) && !panel.intellisensePanelOpen)
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::Up);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_DOWN) && !panel.intellisensePanelOpen)
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::Down);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_RIGHT, KeyMods::Ctrl))
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::RightUntilBoundary);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_LEFT, KeyMods::Ctrl))
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::LeftUntilBoundary);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_HOME))
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::LeftUntilBeginning);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_END))
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::RightUntilEnd);
					}
				}

				// Handle intellisense key combos
				{
					if (Input::keyPressed(GLFW_KEY_SPACE, KeyMods::Ctrl))
					{
						openIntellisensePanelAtCursor(panel);
					}

					if (Input::keyPressed(GLFW_KEY_ESCAPE))
					{
						panel.intellisensePanelOpen = false;
					}

					if (panel.intellisensePanelOpen)
					{
						if (Input::keyRepeatedOrDown(GLFW_KEY_DOWN))
						{
							panel.selectedIntellisenseSuggestion++;
							if (panel.selectedIntellisenseSuggestion >= (uint32)panel.intellisenseSuggestions.size())
							{
								panel.selectedIntellisenseSuggestion = 0;
								panel.intellisenseScrollOffset = 0;
							}

							if ((panel.selectedIntellisenseSuggestion - panel.intellisenseScrollOffset) >= maxIntellisenseSuggestions)
							{
								panel.intellisenseScrollOffset++;
							}
						}
						else if (Input::keyRepeatedOrDown(GLFW_KEY_UP))
						{
							if (panel.selectedIntellisenseSuggestion == 0)
							{
								if (panel.intellisenseSuggestions.size() > 0)
								{
									panel.selectedIntellisenseSuggestion = (uint32)panel.intellisenseSuggestions.size() - 1;
								}

								if (panel.intellisenseSuggestions.size() > maxIntellisenseSuggestions)
								{
									panel.intellisenseScrollOffset = (uint32)panel.intellisenseSuggestions.size() - maxIntellisenseSuggestions;
								}
							}
							else
							{
								panel.selectedIntellisenseSuggestion--;

								if (panel.selectedIntellisenseSuggestion < panel.intellisenseScrollOffset)
								{
									panel.intellisenseScrollOffset = panel.selectedIntellisenseSuggestion;
								}
							}
						}

						// Blit the current intellisense suggestion into the buffer at the cursor
						if (Input::keyPressed(GLFW_KEY_TAB) || Input::keyPressed(GLFW_KEY_ENTER))
						{
							auto const& suggestion = panel.intellisenseSuggestions[panel.selectedIntellisenseSuggestion];
							addUtf8StringToBuffer(panel, (uint8*)suggestion.c_str(), suggestion.size(), panel.cursor.bytePos);
							panel.intellisensePanelOpen = false;
							fileHasBeenEdited = true;
						}
					}
				}

				// Handle key presses to move cursor + select modifier (Shift)
				{
					int32 oldBytePos = (int32)panel.cursor.bytePos;
					if (Input::keyRepeatedOrDown(GLFW_KEY_RIGHT, KeyMods::Shift))
					{
						moveTextCursor(panel, KeyMoveDirection::Right);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_LEFT, KeyMods::Shift))
					{
						moveTextCursor(panel, KeyMoveDirection::Left);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_UP, KeyMods::Shift))
					{
						moveTextCursor(panel, KeyMoveDirection::Up);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_DOWN, KeyMods::Shift))
					{
						moveTextCursor(panel, KeyMoveDirection::Down);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_RIGHT, KeyMods::Shift | KeyMods::Ctrl))
					{
						moveTextCursor(panel, KeyMoveDirection::RightUntilBoundary);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_LEFT, KeyMods::Shift | KeyMods::Ctrl))
					{
						moveTextCursor(panel, KeyMoveDirection::LeftUntilBoundary);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_HOME, KeyMods::Shift))
					{
						moveTextCursor(panel, KeyMoveDirection::LeftUntilBeginning);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_END, KeyMods::Shift))
					{
						moveTextCursor(panel, KeyMoveDirection::RightUntilEnd);
					}

					if (panel.cursor.bytePos != oldBytePos)
					{
						if (panel.cursor.bytePos == panel.mouseByteDragStart)
						{
							panel.firstByteInSelection = (int32)panel.cursor.bytePos;
							panel.lastByteInSelection = (int32)panel.cursor.bytePos;
						}
						else if (panel.cursor.bytePos > panel.mouseByteDragStart)
						{
							panel.lastByteInSelection = (int32)panel.cursor.bytePos;
						}
						else
						{
							panel.firstByteInSelection = (int32)panel.cursor.bytePos;
						}
					}
				}

				// Handle backspace
				// TODO: Not all backspaces are handled for some reason
				if (Input::keyRepeatedOrDown(GLFW_KEY_BACKSPACE))
				{
					panel.cursorTimeSpentInterpolating = 0.0f;
					panel.cursorIsBlinkedOn = true;
					panel.timeSinceCursorLastBlinked = 0.0f;
					panel.intellisensePanelOpen = false;

					handleTypingUndo(panel);

					if (removeSelectedTextWithBackspace(panel))
					{
						fileHasBeenEdited = true;
					}
				}

				// Handle delete
				if (Input::keyRepeatedOrDown(GLFW_KEY_DELETE))
				{
					panel.cursorIsBlinkedOn = true;
					panel.timeSinceCursorLastBlinked = 0.0f;
					panel.intellisensePanelOpen = false;

					handleTypingUndo(panel);

					if (removeSelectedTextWithDelete(panel))
					{
						fileHasBeenEdited = true;
					}
				}

				// Handle text-insertion
				if (uint32 codepoint = Input::getLastCharacterTyped(); codepoint != 0)
				{
					if (!(codepoint == ' ' && Input::keyDown(GLFW_KEY_SPACE, KeyMods::Ctrl)))
					{
						panel.cursorTimeSpentInterpolating = 0.0f;
						panel.cursorIsBlinkedOn = true;
						panel.timeSinceCursorLastBlinked = 0.0f;
						panel.intellisensePanelOpen = false;

						if (panel.firstByteInSelection != panel.lastByteInSelection)
						{
							removeSelectedTextWithBackspace(panel);
						}

						if (panel.undoTypingStart == -1)
						{
							panel.undoTypingStart = (int32)panel.cursor.bytePos;
						}

						addCodepointToBuffer(panel, codepoint, (int32)panel.cursor.bytePos);
						fileHasBeenEdited = true;
					}

					// Check to see if we should open intellisense
					if (codepoint == '.' || codepoint == ':')
					{
						// If the previous character was not also the same character we'll open intellisense
						auto prevChar = panel.cursor;
						// We have to go back two characters because the cursor is already one character ahead of the last character typed
						--prevChar;
						--prevChar;
						auto maybePrevChar = *prevChar;

						if ((maybePrevChar.hasValue() && maybePrevChar.value() != codepoint) || prevChar.bytePos == panel.cursor.bytePos)
						{
							openIntellisensePanelAtCursor(panel);
						}
					}
				}

				// Handle tab key
				if (Input::keyRepeatedOrDown(GLFW_KEY_TAB) && !panel.intellisensePanelOpen)
				{
					panel.cursorTimeSpentInterpolating = 0.0f;
					panel.cursorIsBlinkedOn = true;
					panel.timeSinceCursorLastBlinked = 0.0f;
					panel.intellisensePanelOpen = false;

					if (panel.firstByteInSelection != panel.lastByteInSelection)
					{
						removeSelectedTextWithBackspace(panel);
					}

					if (panel.undoTypingStart == -1)
					{
						panel.undoTypingStart = (int32)panel.cursor.bytePos;
					}

					addCodepointToBuffer(panel, (uint32)'\t', panel.cursor.bytePos);
					fileHasBeenEdited = true;
				}

				// Handle newline-insertion
				if (Input::keyRepeatedOrDown(GLFW_KEY_ENTER) && !panel.intellisensePanelOpen)
				{
					panel.cursorTimeSpentInterpolating = 0.0f;
					panel.cursorIsBlinkedOn = true;
					panel.timeSinceCursorLastBlinked = 0.0f;
					panel.intellisensePanelOpen = false;

					if (panel.firstByteInSelection != panel.lastByteInSelection)
					{
						removeSelectedTextWithBackspace(panel);
					}

					if (panel.undoTypingStart == -1)
					{
						panel.undoTypingStart = (int32)panel.cursor.bytePos;
					}

					addCodepointToBuffer(panel, (uint32)'\n', panel.cursor.bytePos);
					fileHasBeenEdited = true;
				}
			}

			// ---- Handle Rendering/mouse clicking ----
			SyntaxTheme const& syntaxTheme = CodeEditorPanelManager::getTheme();
			panel.drawStart = ImGui::GetCursorScreenPos();
			panel.drawEnd = panel.drawStart + ImGui::GetContentRegionAvail();
			panel.leftGutterWidth = calculateLeftGutterWidth(panel);
			panel.cursorCurrentLine = getLineNumberFromPosition(panel, (uint32)panel.cursor.bytePos);
			panel.numberLinesCanFitOnScreen = (uint32)glm::floor((panel.drawEnd.y - panel.drawStart.y) / getLineHeight(codeFont));

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(panel.drawStart, panel.drawEnd, getColor(syntaxTheme.getColor(syntaxTheme.defaultBackground)));

			uint32 currentLine = panel.lineNumberStart;
			ImVec2 currentLetterDrawPos = renderNextLinePrefix(panel, currentLine, codeFont);

			// Visualize syntax updates by flashing a green color behind any text that's been updated
			if (visualizeSyntaxUpdates)
			{
				auto now = std::chrono::high_resolution_clock::now();
				for (size_t i = 0; i < panel.debugData.linesUpdated.size(); i++)
				{
					auto const& linesUpdated = panel.debugData.linesUpdated[i];
					auto lineStartTime = panel.debugData.ageOfLinesUpdated[i];
					auto lineAge = std::chrono::duration_cast<std::chrono::milliseconds>(now - lineStartTime);

					// Lines over a certain number of milliseconds stop showing
					if (lineAge.count() > numMsToShowLineUpdates)
					{
						panel.debugData.linesUpdated.erase(panel.debugData.linesUpdated.begin() + i);
						panel.debugData.ageOfLinesUpdated.erase(panel.debugData.ageOfLinesUpdated.begin() + i);
						i--;
						continue;
					}

					float lineAlpha = 1.0f - ((float)lineAge.count() / (float)numMsToShowLineUpdates);
					Vec4 lineColor = flashColor;
					lineColor.a = lineAlpha;

					ImVec2 topLeftOfLine = getTopLeftOfLine(panel, linesUpdated.min, codeFont);
					ImVec2 bottomRightOfArea = getTopLeftOfLine(panel, linesUpdated.max, codeFont);
					bottomRightOfArea.x = panel.drawEnd.x - scrollbarWidth;

					// NOTE: We don't highlight below the last line because the last line is not inclusive in the updates

					drawList->AddRectFilled(topLeftOfLine, bottomRightOfArea, ImColor(lineColor));
				}
			}

			// Handle scroll bar render and logic
			if (windowIsHovered && panel.numberLinesCanFitOnScreen < panel.totalNumberLines + numberBufferLines)
			{
				const uint32 totalLinesIncludingBuffer = panel.totalNumberLines + numberBufferLines;
				float scrollbarHeight = ((float)panel.numberLinesCanFitOnScreen / (float)totalLinesIncludingBuffer) * (panel.drawEnd.y - panel.drawStart.y);
				float scrollYPos = ((float)(currentLine - 1) / (float)totalLinesIncludingBuffer) * (panel.drawEnd.y - panel.drawStart.y);

				ImVec2 scrollDrawStart = ImVec2(panel.drawEnd.x - scrollbarWidth, panel.drawStart.y + scrollYPos);
				ImVec2 scrollDrawEnd = scrollDrawStart + ImVec2(scrollbarWidth, scrollbarHeight);

				ImColor scrollbarBgColor = scrollbarSliderBackground;
				if (syntaxTheme.scrollbarSliderBackground)
				{
					scrollbarBgColor = ImColor(syntaxTheme.getColor(syntaxTheme.scrollbarSliderBackground));
				}
				drawList->AddRectFilled(scrollDrawStart, scrollDrawEnd, scrollbarBgColor, 0.3f);

				if (Input::scrollY != 0)
				{
					int32 newLine = (int32)currentLine - ((int32)Input::scrollY * 3);
					newLine = (int32)glm::clamp(newLine, 1, (int32)(panel.totalNumberLines + numberBufferLines - panel.numberLinesCanFitOnScreen));

					if (newLine > (int32)panel.lineNumberStart)
					{
						Vec2i linesUpdated = CodeEditorPanelManager::getHighlighter().checkForUpdatesFrom(
							panel.syntaxHighlightTree,
							panel.lineNumberStart + panel.numberLinesCanFitOnScreen,
							newLine - panel.lineNumberStart
						);

						if (linesUpdated.min <= (int32)panel.totalNumberLines)
						{
							panel.debugData.linesUpdated.emplace_back(linesUpdated);
							panel.debugData.ageOfLinesUpdated.emplace_back(std::chrono::high_resolution_clock::now());
						}
					}

					panel.lineNumberStart = (uint32)newLine;
					currentLine = panel.lineNumberStart;
					panel.lineNumberByteStart = getLineNumberByteStartFrom(panel, (uint32)newLine);
				}
			}

			// Push the clip rect after we've rendered the scrollbar
			drawList->PushClipRect(panel.drawStart, ImVec2(panel.drawEnd.x - scrollbarWidth, panel.drawEnd.y), true);

			bool justStartedDragSelecting = false;
			if (windowIsHovered && mouseInTextEditArea(panel))
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
				const bool isDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left, io.MouseDragThreshold * 1.70f);
				if (!panel.mouseIsDragSelecting && isDragging)
				{
					panel.mouseIsDragSelecting = true;
					justStartedDragSelecting = true;
					panel.mouseByteDragStart = 0;
					panel.firstByteInSelection = 0;
					panel.lastByteInSelection = 0;
				}
			}

			if (panel.mouseIsDragSelecting && !io.MouseDown[ImGuiMouseButton_Left])
			{
				panel.mouseIsDragSelecting = false;
			}

			bool lastCharWasNewline = false;
			bool passedFirstCharacter = false;
			int32 closestByteToMouseCursor = (int32)panel.lineNumberByteStart;

			// Render the text
			auto highlightIter = panel.syntaxHighlightTree.begin(panel.lineNumberByteStart);
			for (auto cursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, panel.lineNumberByteStart);
				cursor.bytePos < panel.visibleCharacterBufferSize;
				++cursor)
			{
				// Figure out what color this character should be
				highlightIter = highlightIter.next(cursor.bytePos);
				ImColor highlightedColor = highlightIter.getForegroundColor(syntaxTheme);

				auto maybeCurrentCodepoint = *cursor;
				g_logger_assert(maybeCurrentCodepoint.hasValue(), "We shouldn't have any invalid UTF8 in our edit buffer.");
				uint32 currentCodepoint = maybeCurrentCodepoint.value();
				ImVec2 letterBoundsStart = currentLetterDrawPos;

				// Before drawing the letter, draw the highlighted background if applicable
				// Render selected text background color
				if (panel.firstByteInSelection != panel.lastByteInSelection &&
					cursor.bytePos >= panel.firstByteInSelection && cursor.bytePos < panel.lastByteInSelection)
				{
					ImVec2 glyphSize = getGlyphSize(codeFont, currentCodepoint);
					ImVec2 highlightEnd = letterBoundsStart + glyphSize;

					ImColor highlightTextColor = editorSelectionBackground;
					if (syntaxTheme.editorSelectionBackground)
					{
						highlightTextColor = ImColor(syntaxTheme.getColor(syntaxTheme.editorSelectionBackground));
					}
					drawList->AddRectFilled(letterBoundsStart, highlightEnd, highlightTextColor);
				}

				// Render the letter and get the glyph size back
				ImVec2 letterBoundsSize = addCharToDrawList(
					drawList,
					codeFont,
					currentCodepoint,
					letterBoundsStart,
					highlightedColor
				);

				// Render the text cursor
				if (cursor.bytePos == panel.cursor.bytePos)
				{
					renderTextCursor(panel, letterBoundsStart, codeFont);
				}
				else if (cursor.bytePos == panel.visibleCharacterBufferSize - 1 && panel.cursor.bytePos == panel.visibleCharacterBufferSize)
				{
					ImVec2 textCursorDrawPosition = letterBoundsStart + ImVec2(letterBoundsSize.x, 0.0f);
					if (currentCodepoint == '\n')
					{
						textCursorDrawPosition = getTopLeftOfLine(panel, currentLine, codeFont);

						const float lineHeight = codeFont->unsizedFont->lineHeight * codeFont->fontSizePixels;
						textCursorDrawPosition = textCursorDrawPosition + ImVec2(panel.leftGutterWidth + style.FramePadding.x, lineHeight);
					}
					renderTextCursor(panel, textCursorDrawPosition, codeFont);
				}

				// Handle newlines
				if (currentCodepoint == (uint32)'\n')
				{
					currentLine++;

					// Only render and handle logic for the visible characters
					if (currentLine > panel.lineNumberStart + panel.numberLinesCanFitOnScreen)
					{
						break;
					}

					currentLetterDrawPos = renderNextLinePrefix(panel, currentLine, codeFont);

					// Render the current line highlighted color if applicable
					// We don't render the highlight background if we're selecting text, I guess it's because it makes it confusing
					// but it's mainly to match what VsCode does here
					if (currentLine == panel.cursorCurrentLine && panel.firstByteInSelection == panel.lastByteInSelection)
					{
						if (syntaxTheme.editorLineHighlightBackground)
						{
							ImVec2 currentLineDrawEnd = ImVec2(
								panel.drawEnd.x - scrollbarWidth,
								currentLetterDrawPos.y + getLineHeight(codeFont)
							);
							drawList->AddRectFilled(
								currentLetterDrawPos,
								currentLineDrawEnd,
								ImColor(syntaxTheme.getColor(syntaxTheme.editorLineHighlightBackground))
							);
						}
					}
				}

				// If the mouse was clicked before any character, the closest byte is the first byte
				if (!passedFirstCharacter && (
					io.MousePos.y < letterBoundsStart.y || (
					io.MousePos.x <= letterBoundsStart.x && io.MousePos.y >= letterBoundsStart.y
					&& io.MousePos.y <= letterBoundsStart.y + letterBoundsSize.y
					)))
				{
					closestByteToMouseCursor = (int32)cursor.bytePos;

				}
				// If the mouse clicked in between a line, then the closest byte is in this line
				else if (io.MousePos.y >= letterBoundsStart.y && io.MousePos.y <= letterBoundsStart.y + letterBoundsSize.y)
				{
					// If we clicked in a letter bounds, then that's the closest byte to the mouse
					if (io.MousePos.x >= letterBoundsStart.x && io.MousePos.x < letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = (int32)cursor.bytePos;
					}
					// If we clicked past the end of the line, the closest byte is the end of the line
					else if (currentCodepoint == '\n' && io.MousePos.x >= letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = (int32)cursor.bytePos;
					}
					// If we clicked past the last character in the file, the closest byte is the end of the file
					else if (cursor.bytePos == panel.visibleCharacterBufferSize - 1 && io.MousePos.x >= letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = (int32)panel.visibleCharacterBufferSize;
					}
					// If we clicked before any letters in the start of the line, the closest byte
					// is the first byte in this line
					else if (io.MousePos.x < letterBoundsStart.x && lastCharWasNewline)
					{
						closestByteToMouseCursor = (int32)cursor.bytePos;
					}
				}
				// If the mouse clicked past the last visible character, then the closest byte is the end of the file
				else if (cursor.bytePos == panel.visibleCharacterBufferSize - 1 && io.MousePos.y >= letterBoundsStart.y + letterBoundsSize.y)
				{
					closestByteToMouseCursor = (int32)panel.visibleCharacterBufferSize;
				}

				// Don't increment the letter draw pos for newlines
				if (currentCodepoint != '\n')
				{
					currentLetterDrawPos.x += letterBoundsSize.x;
				}
				lastCharWasNewline = currentCodepoint == (uint32)'\n';
				passedFirstCharacter = true;
			}

			drawList->PopClipRect();

			if (panel.mouseIsDragSelecting)
			{
				if (justStartedDragSelecting)
				{
					handleTypingUndo(panel);

					panel.mouseByteDragStart = closestByteToMouseCursor;
					panel.firstByteInSelection = closestByteToMouseCursor;
					panel.lastByteInSelection = closestByteToMouseCursor;
				}

				if (closestByteToMouseCursor >= panel.lastByteInSelection)
				{
					panel.lastByteInSelection = closestByteToMouseCursor;
					panel.firstByteInSelection = panel.mouseByteDragStart;
				}
				else if (closestByteToMouseCursor <= panel.lastByteInSelection - 1 &&
					closestByteToMouseCursor >= panel.mouseByteDragStart)
				{
					panel.lastByteInSelection = closestByteToMouseCursor;
					panel.firstByteInSelection = panel.mouseByteDragStart;
				}

				if (closestByteToMouseCursor < panel.firstByteInSelection)
				{
					panel.firstByteInSelection = closestByteToMouseCursor;
					panel.lastByteInSelection = panel.mouseByteDragStart;
				}
				else if (closestByteToMouseCursor > panel.firstByteInSelection &&
					closestByteToMouseCursor < panel.mouseByteDragStart)
				{
					panel.firstByteInSelection = closestByteToMouseCursor;
					panel.lastByteInSelection = panel.mouseByteDragStart;
				}
			}

			if ((windowIsHovered && mouseInTextEditArea(panel)) || panel.mouseIsDragSelecting)
			{
				if (io.MouseDown[ImGuiMouseButton_Left])
				{
					handleTypingUndo(panel);

					panel.cursor.bytePos = closestByteToMouseCursor;
					setCursorDistanceFromLineStart(panel);
				}

				const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				if (mouseClicked && !panel.mouseIsDragSelecting)
				{
					panel.mouseByteDragStart = (int32)panel.cursor.bytePos;
					panel.firstByteInSelection = (int32)panel.cursor.bytePos;
					panel.lastByteInSelection = (int32)panel.cursor.bytePos;
					panel.cursorTimeSpentInterpolating = 0.0f;
				}
			}

			// We'll render the text cursor at the start of the file if the file is empty
			if (panel.visibleCharacterBufferSize == 0)
			{
				renderTextCursor(panel, currentLetterDrawPos, codeFont);
			}

			if (panel.intellisensePanelOpen)
			{
				renderIntellisensePanel(panel, codeFont);
			}

			static bool inspectorOn = false;
			if (windowIsFocused && Input::keyPressed(GLFW_KEY_I, KeyMods::Ctrl | KeyMods::Shift))
			{
				inspectorOn = true;
			}

			if (inspectorOn)
			{
				ImGui::Begin("Stats##Panel1", &inspectorOn);

				if (ImGui::BeginTable("Editor Stats", 2))
				{
					ImGui::TableNextColumn(); ImGui::Text("Panel Line Number Start");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.lineNumberStart);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Selection Begin");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.firstByteInSelection);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Selection End");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.lastByteInSelection);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Drag Start");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.mouseByteDragStart);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Cursor Byte");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.cursor.bytePos);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Cursor current line");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.cursorCurrentLine);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Line start dist (Chars)");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.numOfCharsFromBeginningOfLine);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Beginning of current line");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.beginningOfCurrentLineByte);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Character");
					ImGui::TableNextColumn();
					char character = panel.visibleCharacterBufferSize > 0 && panel.cursor.bytePos < panel.visibleCharacterBufferSize
						? (char)panel.visibleCharacterBuffer[panel.cursor.bytePos]
						: '\0';
					if (character != '\r' && character != '\n' && character != '\t' && character != '\0')
					{
						ImGui::Text("%c", character);
					}
					else
					{
						char escapedCharacter = character == '\r' ? 'r' :
							character == '\n' ? 'n' :
							character == '\t' ? 't' : '0';
						ImGui::Text("\\%c", escapedCharacter);
					}
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Separator();
					ImGui::TableNextColumn(); ImGui::Separator();
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Max Line Length");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.maxLineLength);
					ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("Hz Char Offset");
					ImGui::TableNextColumn(); ImGui::Text("%d", panel.hzCharacterOffset);
					ImGui::TableNextRow();

					ImGui::EndTable();
				}

				ImGui::Checkbox(": Visualize Syntax Updates", &visualizeSyntaxUpdates);
				ImGui::Checkbox(": Inspect Syntax Styles", &inspectCodeStyles);
				if (inspectCodeStyles)
				{
					auto const& highlighter = CodeEditorPanelManager::getHighlighter();
					auto const& theme = CodeEditorPanelManager::getTheme();
					auto parseInfo = highlighter.getAncestorsFor(&theme, panel.syntaxHighlightTree, panel.cursor.bytePos);

					showInspectorGui(theme, parseInfo);
				}

				ImGui::End();
			}

			return fileHasBeenEdited;
		}

		void setCursorToLine(CodeEditorPanelData& panel, uint32 lineNumber)
		{
			g_logger_assert(lineNumber <= panel.totalNumberLines, "This shouldn't happen.");

			panel.cursor.bytePos = getLineNumberByteStartFrom(panel, lineNumber);
			panel.firstByteInSelection = (int32)panel.cursor.bytePos;
			panel.lastByteInSelection = (int32)panel.cursor.bytePos;
			panel.mouseByteDragStart = (int32)panel.cursor.bytePos;

			scrollCursorIntoViewIfNeeded(panel);
		}

		void addUtf8StringToBuffer(CodeEditorPanelData& panel, uint8* utf8String, size_t stringNumBytes, size_t insertPosition)
		{
			g_logger_assert(insertPosition <= panel.visibleCharacterBufferSize, "Cannot insert string past end of buffer.");

			// Translate UTF8 string to local byte mapping
			uint8* byteMappedString = nullptr;
			size_t byteMappedStringLength = 0;
			uint32 numberLinesInString = 0;

			preprocessText(utf8String, stringNumBytes, &byteMappedString, &byteMappedStringLength, &numberLinesInString);

			size_t newCharBufferSize = sizeof(uint8) * (panel.visibleCharacterBufferSize + byteMappedStringLength);
			panel.visibleCharacterBuffer = (uint8*)g_memory_realloc(panel.visibleCharacterBuffer, newCharBufferSize);

			// Move the right half of the string over
			size_t placeToMoveRightHalfToOffset = byteMappedStringLength + insertPosition;
			memmove(
				panel.visibleCharacterBuffer + placeToMoveRightHalfToOffset,
				panel.visibleCharacterBuffer + insertPosition,
				panel.visibleCharacterBufferSize - insertPosition
			);

			// Copy the mapped text into the middle portion
			g_memory_copyMem(
				panel.visibleCharacterBuffer + insertPosition,
				newCharBufferSize - insertPosition,
				byteMappedString,
				byteMappedStringLength * sizeof(uint8)
			);

			panel.visibleCharacterBufferSize = newCharBufferSize;

			panel.cursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, insertPosition + byteMappedStringLength);
			panel.mouseByteDragStart = (int32)panel.cursor.bytePos;
			panel.firstByteInSelection = (int32)panel.cursor.bytePos;
			panel.lastByteInSelection = (int32)panel.cursor.bytePos;

			panel.totalNumberLines += numberLinesInString;

			g_memory_free(byteMappedString);

			size_t numberLinesToUpdate = panel.numberLinesCanFitOnScreen;
			Vec2i linesUpdated = CodeEditorPanelManager::getHighlighter().insertText(
				panel.syntaxHighlightTree,
				(const char*)panel.visibleCharacterBuffer,
				panel.visibleCharacterBufferSize,
				insertPosition,
				insertPosition + stringNumBytes,
				numberLinesToUpdate
			);

			if (linesUpdated.min <= (int32)panel.totalNumberLines)
			{
				panel.debugData.linesUpdated.emplace_back(linesUpdated);
				panel.debugData.ageOfLinesUpdated.emplace_back(std::chrono::high_resolution_clock::now());
			}

			scrollCursorIntoViewIfNeeded(panel);
			setCursorDistanceFromLineStart(panel);
		}

		void addCodepointToBuffer(CodeEditorPanelData& panel, uint32 codepoint, size_t insertPosition)
		{
			uint8 bytes[4] = {};
			uint8 numBytes = codepointToUtf8Str(bytes, codepoint);
			addUtf8StringToBuffer(panel, bytes, numBytes, insertPosition);
		}

		bool removeTextWithBackspace(CodeEditorPanelData& panel, int32 textToRemoveStart, int32 textToRemoveLength)
		{
			if (textToRemoveStart < 0 || textToRemoveStart + textToRemoveLength > panel.visibleCharacterBufferSize)
			{
				return false;
			}

			if (textToRemoveLength == 0)
			{
				return false;
			}

			removeText(panel, textToRemoveStart, textToRemoveLength);

			panel.cursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, textToRemoveStart);
			panel.mouseByteDragStart = (int32)panel.cursor.bytePos;
			panel.firstByteInSelection = (int32)panel.cursor.bytePos;
			panel.lastByteInSelection = (int32)panel.cursor.bytePos;

			scrollCursorIntoViewIfNeeded(panel);
			setCursorDistanceFromLineStart(panel);

			return true;
		}

		bool removeTextWithDelete(CodeEditorPanelData& panel, int32 textToRemoveStart, int32 textToRemoveLength)
		{
			if (textToRemoveStart < 0 || textToRemoveStart + textToRemoveLength > panel.visibleCharacterBufferSize)
			{
				return false;
			}

			if (textToRemoveLength == 0)
			{
				return false;
			}

			removeText(panel, textToRemoveStart, textToRemoveLength);

			panel.cursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, textToRemoveStart);
			panel.mouseByteDragStart = (int32)panel.cursor.bytePos;
			panel.firstByteInSelection = (int32)panel.cursor.bytePos;
			panel.lastByteInSelection = (int32)panel.cursor.bytePos;

			setCursorDistanceFromLineStart(panel);

			return true;
		}

		void preprocessText(uint8* utf8String, size_t stringNumBytes, uint8** outStr, size_t* outStrLength, uint32* numberLines, uint32* maxLineLength)
		{
			*outStr = nullptr;
			*outStrLength = 0;
			if (numberLines != nullptr)
			{
				*numberLines = 0;
			}

			// Strip any carriage returns and any invalid UTF8
			auto maybeParseInfo = ::Parser::makeParseInfo((char*)utf8String, stringNumBytes);
			if (!maybeParseInfo.hasValue())
			{
				g_logger_error("Could not add UTF-8 string '{}' to editor. Had error '{}'.", utf8String, maybeParseInfo.error());
				return;
			}

			*outStr = (uint8*)g_memory_allocate(sizeof(uint8) * stringNumBytes);

			auto parseInfo = maybeParseInfo.value();
			uint32 charsInCurrentLine = 1;
			while (parseInfo.cursor < parseInfo.numBytes)
			{
				uint8 numBytesParsed = 1;
				auto codepoint = ::Parser::parseCharacter(parseInfo, &numBytesParsed);
				if (!codepoint.hasValue())
				{
					g_logger_error("File has malformed unicode. Skipping bad unicode data.");
					parseInfo.numBytes++;
					continue;
				}

				if (numberLines != nullptr && codepoint.value() == '\n')
				{
					*numberLines += 1;

					// Keep track of max line length
					if (maxLineLength != nullptr && charsInCurrentLine > *maxLineLength)
					{
						*maxLineLength = charsInCurrentLine;
					}
					charsInCurrentLine = 0;
				}

				// Also, skip carriage returns 'cuz screw those
				if (codepoint.value() == (uint32)'\r')
				{
					parseInfo.cursor += numBytesParsed;
					continue;
				}

				CodeEditorPanelManager::addCharToFont(codepoint.value());

				// Copy all UTF8 bytes over to the new string
				for (size_t tmpCursor = parseInfo.cursor;
					tmpCursor < parseInfo.cursor + numBytesParsed;
					tmpCursor++)
				{
					(*outStr)[(*outStrLength)] = parseInfo.utf8String[tmpCursor];
					*outStrLength = *outStrLength + 1;
				}

				parseInfo.cursor += numBytesParsed;
				charsInCurrentLine++;
			}

			if (maxLineLength != nullptr && charsInCurrentLine > *maxLineLength)
			{
				*maxLineLength = charsInCurrentLine;
			}

			*outStr = (uint8*)g_memory_realloc((void*)(*outStr), (*outStrLength) * sizeof(uint8));
		}

		void postprocessText(uint8* byteMappedString, size_t byteMappedStringNumBytes, uint8** outUtf8String, size_t* outUtf8StringNumBytes, bool includeCarriageReturnsForWindows)
		{
			// Add carriage returns as necessary
			RawMemory memory{};
			memory.init(sizeof(uint8) * byteMappedStringNumBytes);

			for (size_t i = 0; i < byteMappedStringNumBytes; i++)
			{
				uint8 c = byteMappedString[i];

				#ifdef _WIN32
				if (includeCarriageReturnsForWindows)
				{
					g_logger_assert(c != (uint32)'\r', "We should never get carriage returns in our edit buffers");

					// If we're on windows, translate newlines to carriage return + newlines when saving the files again
					if (c == (uint32)'\n')
					{
						uint8 carriageReturn = '\r';
						memory.write(&carriageReturn);
					}
				}
				#endif

				memory.write(&c);
			}

			memory.shrinkToFit();

			*outUtf8String = memory.data;
			*outUtf8StringNumBytes = memory.size;
		}

		void showInspectorGui(SyntaxTheme const& theme, CodeHighlightDebugInfo const& parseInfo)
		{
			ImGui::BeginChild("Code Ancestors", ImVec2(0, 0), true);

			ImGui::PushFont(ImGuiLayer::getMediumFont());
			if (parseInfo.matchText.length() > 30)
			{
				std::string firstPart = parseInfo.matchText.substr(0, 15);
				std::string lastPart = parseInfo.matchText.substr(parseInfo.matchText.length() - 15);
				ImGui::Text("%s...%s", firstPart.c_str(), lastPart.c_str());
			}
			else
			{
				ImGui::Text(parseInfo.matchText.c_str());
			}
			ImGui::PopFont();

			if (ImGui::BeginTable("textmate scopes", 2))
			{
				ImGui::TableNextColumn(); ImGui::Text("language");
				ImGui::TableNextColumn(); ImGui::Text("%d", parseInfo.settings.style.getLanguageId());

				ImGui::TableNextColumn(); ImGui::Text("standard token type");
				ImGui::TableNextColumn(); ImGui::Text("%d", parseInfo.settings.style.getStandardTokenType());

				ImGui::TableNextColumn(); ImGui::Text("foreground");
				Vec4 foregroundColor = theme.getColor(parseInfo.settings.style.getForegroundColor());
				std::string foregroundStr = toHexString(foregroundColor);
				ImGui::TableNextColumn(); ImGui::Text("%s", foregroundStr.c_str()); ImGui::SameLine();
				ImGui::ColorButton("##ForegroundInspector_InspectorPanel", foregroundColor,
					ImGuiColorEditFlags_NoLabel
				);

				ImGui::TableNextColumn(); ImGui::Text("background");
				Vec4 styleBackgroundColor = theme.getColor(parseInfo.settings.style.getBackgroundColor());
				std::string backgroundStr = toHexString(styleBackgroundColor);
				ImGui::TableNextColumn(); ImGui::Text("%s", backgroundStr.c_str()); ImGui::SameLine();
				ImGui::ColorButton("##ForegroundInspector_InspectorPanel", styleBackgroundColor,
					ImGuiColorEditFlags_NoLabel
				);

				ImGui::TableNextColumn(); ImGui::Text("font style");
				ImGui::TableNextColumn(); ImGui::Text("%s", Css::toString(parseInfo.settings.style.getFontStyle()).c_str());

				ImGui::Separator();

				ImGui::TableNextColumn(); ImGui::Text("textmate scopes");
				for (size_t i = 0; i < parseInfo.ancestors.size(); i++)
				{
					size_t backwardsIndex = parseInfo.ancestors.size() - i - 1;
					std::string friendlyName = parseInfo.ancestors[backwardsIndex].getFriendlyName();
					ImGui::TableNextColumn(); ImGui::Text(friendlyName.c_str());

					if (i < parseInfo.ancestors.size() - 1)
					{
						ImGui::TableNextRow(); ImGui::TableNextColumn();
					}
				}

				ImGui::TableNextRow(); ImGui::TableNextRow();

				ImGui::TableNextColumn(); ImGui::Text("foreground");
				bool foundThemeSelector = false;

				if (parseInfo.usingDefaultSettings)
				{
					ImGui::TableNextColumn(); ImGui::Text("No theme selector");
				}
				else
				{
					Vec4 const& foregroundCssColor = theme.getColor(parseInfo.settings.style.getForegroundColor());
					std::string foregroundColorStr = toHexString(foregroundCssColor);
					if (parseInfo.settings.style.isForegroundInherited())
					{
						foregroundColorStr = "inherit";
					}

					ImGui::TableNextColumn(); ImGui::Text(parseInfo.settings.styleMatched.c_str());
					ImGui::TableNextRow(); ImGui::TableNextColumn();
					ImGui::TableNextColumn(); ImGui::Text("{ \"foreground\": \"%s\"", foregroundColorStr.c_str());
					foundThemeSelector = true;
				}

				ImGui::EndTable();
			}

			ImGui::EndChild();
		}

		// ------------- Internal Functions -------------
		static void resetSelection(CodeEditorPanelData& panel)
		{
			panel.firstByteInSelection = (int32)panel.cursor.bytePos;
			panel.lastByteInSelection = (int32)panel.cursor.bytePos;
			panel.mouseByteDragStart = (int32)panel.cursor.bytePos;
		}

		static void handleTypingUndo(CodeEditorPanelData& panel)
		{
			// Nothing to undo, just skip adding an undo operation
			if (panel.undoTypingStart == -1 || panel.undoTypingStart == panel.cursor.bytePos)
			{
				return;
			}

			int32 numBytesInUndo = ((int32)panel.cursor.bytePos - panel.undoTypingStart);
			if (panel.undoTypingStart > panel.cursor.bytePos || numBytesInUndo < 0 ||
				panel.undoTypingStart + numBytesInUndo > panel.visibleCharacterBufferSize)
			{
				g_logger_warning("Invalid undo typing cursor encountered.");
				panel.undoTypingStart = -1;
				return;
			}

			// We typed some stuff, and we want to add it as one big undo operation
			UndoSystem::insertTextAction(
				panel.undoSystem,
				panel.visibleCharacterBuffer + panel.undoTypingStart,
				numBytesInUndo,
				panel.undoTypingStart,
				numBytesInUndo);

			panel.undoTypingStart = -1;
		}

		static void handleBackspaceUndo(CodeEditorPanelData& panel, bool shouldSetTextSelectedOnUndo)
		{
			// Figure out what text we're about to delete
			int32 numBytesToDelete = panel.lastByteInSelection - panel.firstByteInSelection;
			if (numBytesToDelete < 0 || panel.firstByteInSelection + numBytesToDelete > panel.visibleCharacterBufferSize)
			{
				g_logger_error("This shouldn't happen. We are trying to add a backspace operation that deletes an invalid amount of text.");
				return;
			}

			UndoSystem::backspaceTextAction(
				panel.undoSystem,
				panel.visibleCharacterBuffer + panel.firstByteInSelection,
				numBytesToDelete,
				panel.firstByteInSelection,
				panel.firstByteInSelection + numBytesToDelete,
				panel.cursor.bytePos,
				shouldSetTextSelectedOnUndo
			);
		}

		static void handleDeleteUndo(CodeEditorPanelData& panel, bool shouldSetTextSelectedOnUndo)
		{
			// Figure out what text we're about to delete
			int32 numBytesToDelete = panel.lastByteInSelection - panel.firstByteInSelection;
			if (numBytesToDelete < 0 || panel.firstByteInSelection + numBytesToDelete > panel.visibleCharacterBufferSize)
			{
				g_logger_error("This shouldn't happen. We are trying to add a backspace operation that deletes an invalid amount of text.");
				return;
			}

			UndoSystem::deleteTextAction(
				panel.undoSystem,
				panel.visibleCharacterBuffer + panel.firstByteInSelection,
				numBytesToDelete,
				panel.firstByteInSelection,
				panel.firstByteInSelection + numBytesToDelete,
				panel.cursor.bytePos,
				shouldSetTextSelectedOnUndo
			);
		}

		static void moveTextCursor(CodeEditorPanelData& panel, KeyMoveDirection direction)
		{
			handleTypingUndo(panel);

			panel.cursorIsBlinkedOn = true;
			panel.timeSinceCursorLastBlinked = 0.0f;
			panel.cursorTimeSpentInterpolating = 0.0f;
			panel.intellisensePanelOpen = false;

			panel.cursor.bytePos = getNewCursorPositionFromMove(panel, direction);

			bool movedLines = direction == KeyMoveDirection::Up || direction == KeyMoveDirection::Down;
			if (!movedLines)
			{
				setCursorDistanceFromLineStart(panel);
			}
			else
			{
				// Don't change the number of characters if we move up or down since we want the cursor to try
				// to keep that position if it can
				panel.beginningOfCurrentLineByte = getBeginningOfLineFrom(panel, (int32)panel.cursor.bytePos);
			}

			scrollCursorIntoViewIfNeeded(panel);
		}

		static void moveTextCursorAndResetSelection(CodeEditorPanelData& panel, KeyMoveDirection direction)
		{
			moveTextCursor(panel, direction);
			resetSelection(panel);
		}

		static void renderTextCursor(CodeEditorPanelData& panel, ImVec2 drawPosition, SizedFont const* const font)
		{
			if (!ImGui::IsWindowFocused() && !panel.mouseIsDragSelecting)
			{
				return;
			}

			SyntaxTheme const& syntaxTheme = CodeEditorPanelManager::getTheme();

			// Smooth the cursor movement if needed
			EditorSettingsData const& editorSettings = EditorSettings::getSettings();
			if (editorSettings.smoothCursor && panel.cursorTimeSpentInterpolating < maxTimeToInterpolateCursor)
			{
				panel.timeSinceCursorLastBlinked = 0.0f;
				panel.cursorIsBlinkedOn = true;
				panel.cursorTimeSpentInterpolating += Application::getDeltaTime();

				float t = panel.cursorTimeSpentInterpolating / maxTimeToInterpolateCursor;
				t = CMath::ease(t, EaseType::Linear, EaseDirection::In);

				panel.lastCursorPosition = CMath::interpolate(t, panel.lastCursorPosition, drawPosition);
				drawPosition = panel.lastCursorPosition;
			}
			else
			{
				panel.lastCursorPosition = drawPosition;
			}

			if (panel.timeSinceCursorLastBlinked >= cursorBlinkTime)
			{
				panel.cursorIsBlinkedOn = !panel.cursorIsBlinkedOn;
				panel.timeSinceCursorLastBlinked = 0.0f;
			}

			if (panel.cursorIsBlinkedOn || panel.mouseIsDragSelecting)
			{
				float currentLineHeight = font->fontSizePixels * font->unsizedFont->lineHeight;

				ImDrawList* drawList = ImGui::GetWindowDrawList();

				ImColor textCursorColor = editorCursorForeground;
				if (syntaxTheme.editorCursorForeground)
				{
					textCursorColor = ImColor(syntaxTheme.getColor(syntaxTheme.editorCursorForeground));
				}
				drawList->AddRectFilled(drawPosition, drawPosition + ImVec2(textCursorWidth, currentLineHeight), textCursorColor);
			}
		}

		static void renderIntellisensePanel(CodeEditorPanelData& panel, SizedFont const* const font)
		{
			if (!ImGui::IsWindowFocused() || panel.intellisenseSuggestions.size() == 0)
			{
				return;
			}

			SyntaxTheme const& syntaxTheme = CodeEditorPanelManager::getTheme();
			ImGuiStyle& style = ImGui::GetStyle();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			Vec4 const& borderColor = syntaxTheme.getColor(syntaxTheme.editorSuggestWidgetBorder);
			Vec4 const& normalBgColor = syntaxTheme.getColor(syntaxTheme.editorSuggestWidgetBackground);
			Vec4 const& normalFgColor = syntaxTheme.getColor(syntaxTheme.defaultForeground);
			Vec4 const& selectedBgColor = syntaxTheme.getColor(syntaxTheme.editorSuggestWidgetSelectedBackground);
			Vec4 const& selectedFgColor = "#FFF"_hex;

			float maxSuggestionStrLength = minIntellisensePanelWidth;
			for (auto& suggestion : panel.intellisenseSuggestions)
			{
				float strLength = ImGui::CalcTextSize(suggestion.c_str()).x + intellisenseFrameRounding * 2.0f;
				maxSuggestionStrLength = glm::max(maxSuggestionStrLength, strLength);
			}

			float lineHeight = getLineHeight(font);
			float selectionHeight = lineHeight + intellisenseFrameRounding * 2.0f;

			size_t numItemsToShow = panel.intellisenseSuggestions.size() > maxIntellisenseSuggestions
				? maxIntellisenseSuggestions
				: panel.intellisenseSuggestions.size();
			ImVec2 panelBgDrawStart = panel.lastCursorPosition + ImVec2(0.0f, lineHeight);
			ImVec2 panelBgDrawEnd = panelBgDrawStart
				+ ImVec2(maxSuggestionStrLength, numItemsToShow * selectionHeight)
				+ (style.FramePadding * 2.0f);

			ImVec2 borderSize = ImVec2(intellisensePanelBorderWidth, intellisensePanelBorderWidth);
			drawList->AddRectFilled(panelBgDrawStart - borderSize, panelBgDrawEnd + borderSize, ImColor(borderColor), intellisenseFrameRounding);
			drawList->AddRectFilled(panelBgDrawStart, panelBgDrawEnd, ImColor(normalBgColor), intellisenseFrameRounding);

			ImVec2 cursor = panelBgDrawStart + style.FramePadding;
			uint32 endIndex = panel.intellisenseSuggestions.size() >= maxIntellisenseSuggestions
				? maxIntellisenseSuggestions
				: (uint32)panel.intellisenseSuggestions.size();
			for (uint32 index = panel.intellisenseScrollOffset; (index - panel.intellisenseScrollOffset) < endIndex; index++)
			{
				auto const& suggestion = panel.intellisenseSuggestions[index];

				ImColor fgColor = normalFgColor;

				if (index == panel.selectedIntellisenseSuggestion)
				{
					fgColor = ImColor(selectedFgColor);

					drawList->AddRectFilled(
						ImVec2(panelBgDrawStart.x, cursor.y),
						ImVec2(panelBgDrawEnd.x, cursor.y + selectionHeight),
						ImColor(selectedBgColor)
					);
				}

				drawList->AddText(cursor + ImVec2(intellisenseFrameRounding, intellisenseFrameRounding), fgColor, suggestion.c_str());
				cursor = cursor + ImVec2(0.0f, selectionHeight);
			}

			// Render scrollbar
			if (panel.intellisenseSuggestions.size() > maxIntellisenseSuggestions)
			{
				float bgHeight = panelBgDrawEnd.y - panelBgDrawStart.y;
				float scrollbarHeight = ((float)maxIntellisenseSuggestions / (float)panel.intellisenseSuggestions.size()) * bgHeight;
				float scrollbarStartY = ((float)panel.intellisenseScrollOffset / (float)panel.intellisenseSuggestions.size()) * bgHeight;

				ImVec2 scrollbarStart = ImVec2(panelBgDrawEnd.x, scrollbarStartY + panelBgDrawStart.y);
				ImVec2 scrollbarEnd = scrollbarStart + ImVec2(-scrollbarWidth, scrollbarHeight);
				ImColor scrollbarColor = syntaxTheme.getColor(syntaxTheme.scrollbarSliderBackground);

				drawList->AddRectFilled(scrollbarStart, scrollbarEnd, scrollbarColor);
			}
		}

		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber, SizedFont const* const font)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			SyntaxTheme const& theme = CodeEditorPanelManager::getTheme();

			std::string largestNumberText = std::to_string(panel.totalNumberLines);
			glm::vec2 largestNumberSize = font->getSizeOfString(largestNumberText);

			// Center the number text according to the largest sized number, but make sure the numbers are all aligned still
			ImVec2 lineStart = getTopLeftOfLine(panel, lineNumber, font);
			ImVec2 rightSideOfLargestNumberSize = lineStart + ImVec2((panel.leftGutterWidth / 2.0f) + (largestNumberSize.x / 2.0f), 0.0f);

			std::string numberText = std::to_string(lineNumber);
			glm::vec2 textSize = font->getSizeOfString(numberText);

			ImVec2 numberStart = rightSideOfLargestNumberSize - ImVec2(textSize.x, 0.0f);

			// Figure out what color we should draw the line number as
			ImColor numberColor = editorLineNumberForeground;
			if (panel.cursorCurrentLine == lineNumber)
			{
				numberColor = editorLineNumberActiveForeground;
				if (theme.editorLineNumberActiveForeground)
				{
					numberColor = ImColor(theme.getColor(theme.editorLineNumberActiveForeground));
				}
			}
			else if (theme.editorLineNumberForeground)
			{
				numberColor = ImColor(theme.getColor(theme.editorLineNumberForeground));
			}

			// Draw the line number
			addStringToDrawList(drawList, font, numberText, numberStart, numberColor);

			// Return the new position
			return lineStart + ImVec2(panel.leftGutterWidth + style.FramePadding.x, 0.0f);
		}

		static bool mouseInTextEditArea(CodeEditorPanelData const& panel)
		{
			ImVec2 textAreaStart = panel.drawStart + ImVec2(panel.leftGutterWidth, 0.0f);
			ImVec2 textAreaEnd = panel.drawEnd - ImVec2(scrollbarWidth, 0.0f);
			return mouseIntersectsRect(textAreaStart, textAreaEnd);
		}

		static ImVec2 addStringToDrawList(ImDrawList* drawList, SizedFont const* const sizedFont, std::string const& str, ImVec2 const& drawPos, ImColor const& color)
		{
			Font const* const font = sizedFont->unsizedFont;
			ImVec2 cursorPos = drawPos;

			for (int i = 0; i < str.length(); i++)
			{
				if (str[i] == '\n')
				{
					cursorPos = Vec2{ drawPos.x, cursorPos.y + font->lineHeight * sizedFont->fontSizePixels };
					continue;
				}

				ImVec2 charSize = addCharToDrawList(drawList, sizedFont, (uint32)str[i], cursorPos, color);
				cursorPos = cursorPos + ImVec2(charSize.x, 0.0f);
			}

			return cursorPos - drawPos;
		}

		static ImVec2 getGlyphSize(SizedFont const* const sizedFont, uint32 charToAdd)
		{
			Font const* const font = sizedFont->unsizedFont;

			if (charToAdd == (uint32)'\n' || charToAdd == (uint32)'\r')
			{
				return ImVec2(baseNewlineCharacterWidth, (font->lineHeight * sizedFont->fontSizePixels));
			}

			const GlyphOutline& glyphOutline = font->getGlyphInfo(charToAdd);
			if (!glyphOutline.svg)
			{
				return ImVec2(0.0f, (font->lineHeight * sizedFont->fontSizePixels));
			}

			const float totalFontHeight = sizedFont->fontSizePixels * font->lineHeight;
			return Vec2{ glyphOutline.advanceX * (float)sizedFont->fontSizePixels, totalFontHeight };
		}

		static ImVec2 addCharToDrawList(ImDrawList* drawList, SizedFont const* const sizedFont, uint32 charToAdd, ImVec2 const& drawPos, ImColor const& color)
		{
			Font const* const font = sizedFont->unsizedFont;
			uint32 texId = sizedFont->texture.graphicsId;
			ImVec2 cursorPos = ImVec2(drawPos.x, drawPos.y + (font->lineHeight * sizedFont->fontSizePixels));

			if (charToAdd == (uint32)'\n' || charToAdd == (uint32)'\r')
			{
				return ImVec2(baseNewlineCharacterWidth, (font->lineHeight * sizedFont->fontSizePixels));
			}

			const GlyphOutline& glyphOutline = font->getGlyphInfo(charToAdd);
			const GlyphTexture& glyphTexture = sizedFont->getGlyphTexture(charToAdd);
			if (!glyphOutline.svg)
			{
				return ImVec2(0.0f, (font->lineHeight * sizedFont->fontSizePixels));
			}

			ImVec2 offset = ImVec2(
				glyphOutline.bearingX,
				font->maxDescentY + glyphOutline.descentY - glyphOutline.glyphHeight
			) * (float)sizedFont->fontSizePixels;

			ImVec2 finalPos = cursorPos + offset;
			ImVec2 glyphSize = ImVec2(glyphOutline.glyphWidth, glyphOutline.glyphHeight) * (float)sizedFont->fontSizePixels;

			ImVec2 uvMin = ImVec2(glyphTexture.uvMin.x, glyphTexture.uvMin.y);
			ImVec2 uvSize = ImVec2(glyphTexture.uvMax.x - glyphTexture.uvMin.x, glyphTexture.uvMax.y - glyphTexture.uvMin.y);

			if (charToAdd != (uint32)' ' && charToAdd != (uint32)'\t')
			{
				Vec2 finalOffset = offset + cursorPos;
				drawList->AddImageQuad(
					(void*)(uintptr_t)texId,
					finalPos,
					finalPos + ImVec2(glyphSize.x, 0.0f),
					finalPos + glyphSize,
					finalPos + ImVec2(0.0f, glyphSize.y),
					uvMin,
					uvMin + ImVec2(uvSize.x, 0.0f),
					uvMin + uvSize,
					uvMin + ImVec2(0.0f, uvSize.y),
					color
				);
			}

			cursorPos = cursorPos + Vec2{ glyphOutline.advanceX * (float)sizedFont->fontSizePixels, 0.0f };

			const float totalFontHeight = sizedFont->fontSizePixels * font->lineHeight;
			return ImVec2((cursorPos - drawPos).x, totalFontHeight);
		}

		static bool removeSelectedTextWithBackspace(CodeEditorPanelData& panel, bool addBackspaceToUndoHistory)
		{
			if (panel.cursor.bytePos == 0 && panel.firstByteInSelection == panel.lastByteInSelection)
			{
				// Cannot backspace if the cursor is at the beginning of the file and no text is selected
				return false;
			}

			// If we're pressing backspace and no text is selected, pretend that the character behind the cursor is selected
			// by pushing the selection window back one
			bool shouldSetTextSelectedOnUndo = true;
			if (panel.firstByteInSelection == panel.lastByteInSelection)
			{
				shouldSetTextSelectedOnUndo = false;
				auto tmpCursor = panel.cursor;
				--tmpCursor;
				panel.firstByteInSelection = (int32)tmpCursor.bytePos;
			}

			if (addBackspaceToUndoHistory)
			{
				// Add an undo command if needed
				handleBackspaceUndo(panel, shouldSetTextSelectedOnUndo);
			}

			return removeTextWithBackspace(panel, panel.firstByteInSelection, panel.lastByteInSelection - panel.firstByteInSelection);
		}

		static bool removeSelectedTextWithDelete(CodeEditorPanelData& panel)
		{
			if (panel.cursor.bytePos == panel.visibleCharacterBufferSize && panel.firstByteInSelection == panel.lastByteInSelection)
			{
				// Cannot delete if the cursor is at the end of the file and no text is selected
				return false;
			}

			// If we're pressing delete and no text is selected, pretend that the character in front of the cursor is selected
			// by pushing the selection window forward one
			bool shouldSetTextSelectedOnUndo = true;
			if (panel.firstByteInSelection == panel.lastByteInSelection)
			{
				auto tmpCursor = panel.cursor;
				++tmpCursor;
				panel.lastByteInSelection = (int32)tmpCursor.bytePos;
				shouldSetTextSelectedOnUndo = false;
			}

			// Add an undo command if needed
			handleDeleteUndo(panel, shouldSetTextSelectedOnUndo);

			return removeTextWithDelete(panel, panel.firstByteInSelection, panel.lastByteInSelection - panel.firstByteInSelection);
		}

		static bool removeText(CodeEditorPanelData& panel, int32 textToRemoveOffset, int32 textToRemoveNumBytes)
		{
			// No text to remove
			if (textToRemoveNumBytes == 0)
			{
				return false;
			}

			// Count the number of lines in the text we're about to remove
			uint32 numberLinesInString = 0;
			for (int32 i = textToRemoveOffset; i < textToRemoveOffset + textToRemoveNumBytes; i++)
			{
				if (panel.visibleCharacterBuffer[i] == '\n')
				{
					numberLinesInString += 1;
				}
			}

			size_t bytesToMoveOffset = textToRemoveOffset + textToRemoveNumBytes;
			if (bytesToMoveOffset != panel.visibleCharacterBufferSize)
			{
				memmove(
					(void*)(panel.visibleCharacterBuffer + textToRemoveOffset),
					(void*)(panel.visibleCharacterBuffer + bytesToMoveOffset),
					panel.visibleCharacterBufferSize - bytesToMoveOffset
				);
			}

			panel.visibleCharacterBufferSize -= textToRemoveNumBytes;
			panel.visibleCharacterBuffer = (uint8*)g_memory_realloc((void*)panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize);

			// Update total number of lines
			panel.totalNumberLines -= numberLinesInString;

			// If removing the text changed the scroll height, shift the scroll up to fit on the screen
			if (panel.lineNumberStart + panel.numberLinesCanFitOnScreen > panel.totalNumberLines + numberBufferLines)
			{
				int32 delta = (int32)(panel.lineNumberStart + panel.numberLinesCanFitOnScreen) - (int32)(panel.totalNumberLines + numberBufferLines);
				int32 newLine = (int32)panel.lineNumberStart - delta;

				panel.lineNumberStart = (uint32)glm::clamp(newLine, 1, (int32)panel.totalNumberLines + (int32)numberBufferLines);
				panel.lineNumberByteStart = getLineNumberByteStartFrom(panel, panel.lineNumberStart);
			}

			// Update the syntax highlighting
			size_t numberLinesToUpdate = panel.numberLinesCanFitOnScreen;
			Vec2i linesUpdated = CodeEditorPanelManager::getHighlighter().removeText(
				panel.syntaxHighlightTree,
				(const char*)panel.visibleCharacterBuffer,
				panel.visibleCharacterBufferSize,
				textToRemoveOffset,
				numberLinesInString,
				numberLinesToUpdate
			);

			if (linesUpdated.min <= (int32)panel.totalNumberLines)
			{
				panel.debugData.linesUpdated.emplace_back(linesUpdated);
				panel.debugData.ageOfLinesUpdated.emplace_back(std::chrono::high_resolution_clock::now());
			}

			return true;
		}

		static inline bool isBoundaryCharacter(uint32 c)
		{
			if (c == ':' || c == ';' || c == '"' || c == '\'' || c == '.' || c == '(' || c == ')' || c == '{' || c == '}'
				|| c == '-' || c == '+' || c == '*' || c == '/' || c == ',' || c == '=' || c == '!' || c == '`' || c == '<' || c == '>')
			{
				return true;
			}

			return false;
		}

		static int32 getNewCursorPositionFromMove(CodeEditorPanelData const& panel, KeyMoveDirection direction)
		{
			switch (direction)
			{
			case KeyMoveDirection::Left:
			{
				// TODO: Add operator - to this iterator
				auto newCursorPos = panel.cursor;
				--newCursorPos;
				return glm::clamp((int32)newCursorPos.bytePos, 0, (int32)panel.visibleCharacterBufferSize);
			}

			case KeyMoveDirection::Right:
			{
				// TODO: Add operator + to this iterator
				auto newCursorPos = panel.cursor;
				++newCursorPos;
				return glm::clamp((int32)newCursorPos.bytePos, 0, (int32)panel.visibleCharacterBufferSize);
			}

			case KeyMoveDirection::Up:
			{
				// Try to go up a line while maintaining the same distance from the beginning of the line
				int32 beginningOfCurrentLine = panel.beginningOfCurrentLineByte;
				int32 beginningOfLineAbove = getBeginningOfLineFrom(panel, beginningOfCurrentLine - 1);

				// Only set the cursor to a new position if there is another line above the current line
				if (beginningOfCurrentLine == 0)
				{
					return (int32)panel.cursor.bytePos;
				}

				int32 newPos = beginningOfLineAbove;
				int32 numCharsCounted = 1;
				for (auto tmpCursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, beginningOfLineAbove);
					tmpCursor.bytePos < beginningOfCurrentLine;
					++tmpCursor)
				{
					if (numCharsCounted == panel.numOfCharsFromBeginningOfLine)
					{
						newPos = (int32)tmpCursor.bytePos;
						break;
					}
					else if (tmpCursor.bytePos == beginningOfCurrentLine - 1)
					{
						newPos = (int32)tmpCursor.bytePos;
					}

					numCharsCounted++;
				}

				return glm::clamp(newPos, 0, (int32)panel.visibleCharacterBufferSize);
			}

			case KeyMoveDirection::Down:
			{
				// Try to go down a line while maintaining the same distance from the beginning of the line
				int32 endOfCurrentLine = getEndOfLineFrom(panel, (int32)panel.cursor.bytePos);
				int32 beginningOfLineBelow = endOfCurrentLine + 1;

				// Only set the cursor to a new position if there is another line below the current line
				if (beginningOfLineBelow > panel.visibleCharacterBufferSize)
				{
					return (int32)panel.cursor.bytePos;
				}

				int32 numCharsCounted = 1;
				int32 newPos = beginningOfLineBelow;
				for (auto tmpCursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, beginningOfLineBelow);
					tmpCursor.bytePos <= panel.visibleCharacterBufferSize;
					++tmpCursor)
				{
					if (tmpCursor.bytePos == panel.visibleCharacterBufferSize || (*tmpCursor).value() == '\n')
					{
						newPos = (int32)tmpCursor.bytePos;
						break;
					}
					else if (numCharsCounted == panel.numOfCharsFromBeginningOfLine)
					{
						newPos = (int32)tmpCursor.bytePos;
						break;
					}

					numCharsCounted++;
				}

				return glm::clamp(newPos, 0, (int32)panel.visibleCharacterBufferSize);
			}

			case KeyMoveDirection::LeftUntilBeginning:
			{
				int32 beginningOfCurrentLine = getBeginningOfLineFrom(panel, (int32)panel.cursor.bytePos);

				for (auto tmpCursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, beginningOfCurrentLine);
					tmpCursor.bytePos < panel.visibleCharacterBufferSize;
					++tmpCursor)
				{
					uint32 c = (*tmpCursor).value();
					if (c == ' ' || c == '\t')
					{
						continue;
					}

					// Return the first non-whitespace character from the beginning of the line if we're in front of the beginning
					// of the line, or at the beginning of the line
					if (panel.cursor.bytePos > tmpCursor.bytePos || panel.cursor.bytePos == beginningOfCurrentLine)
					{
						return (int32)tmpCursor.bytePos;
					}
					// Otherwise return the beginning of the line
					else
					{
						return beginningOfCurrentLine;
					}
				}
				break;
			}

			case KeyMoveDirection::RightUntilEnd:
			{
				int32 endOfCurrentLine = getEndOfLineFrom(panel, (int32)panel.cursor.bytePos);

				// If we're at the end of the file, move all the way to the end if it's not a newline
				if (endOfCurrentLine == panel.visibleCharacterBufferSize - 1 && panel.visibleCharacterBuffer[panel.visibleCharacterBufferSize - 1] != '\n')
				{
					endOfCurrentLine++;
				}

				return endOfCurrentLine;
			}

			// NOTE: We can ignore UTF8 characters in the LeftUntilBoundary and RightUntilBoundary move types because
			//       the result would be the same regardless of whether we hit any UTF8 characters or not.

			case KeyMoveDirection::LeftUntilBoundary:
			{
				int32 startPos = glm::clamp((int32)panel.cursor.bytePos - 1, 0, (int32)panel.visibleCharacterBufferSize);

				uint8 c = panel.visibleCharacterBuffer[startPos];
				bool startedOnSkippableWhitespace = c == ' ' || c == '\t';
				bool skippedAllWhitespace = false;
				bool hitBoundaryCharacterButSkipped = false;

				for (int32 i = (int32)panel.cursor.bytePos - 1; i >= 0; i--)
				{
					c = panel.visibleCharacterBuffer[i];

					// Handle newlines
					if (c == '\n' && i != panel.cursor.bytePos - 1)
					{
						return glm::clamp(i + 1, 0, (int32)panel.visibleCharacterBufferSize);
					}

					// Handle boundary characters
					if (isBoundaryCharacter(c) && i != panel.cursor.bytePos - 1)
					{
						uint32 nextC = i + 1 < panel.visibleCharacterBufferSize ? panel.visibleCharacterBuffer[i + 1] : '\0';
						if (isBoundaryCharacter(nextC) || nextC == ' ' || nextC == '\t' || hitBoundaryCharacterButSkipped)
						{
							hitBoundaryCharacterButSkipped = true;
							continue;
						}
						else
						{
							return glm::clamp(i + 1, 0, (int32)panel.visibleCharacterBufferSize);
						}
					}
					else if (!isBoundaryCharacter(c) && hitBoundaryCharacterButSkipped)
					{
						return glm::clamp(i + 1, 0, (int32)panel.visibleCharacterBufferSize);
					}

					// Handle skippable whitespace
					if (startedOnSkippableWhitespace && c != ' ' && c != '\t')
					{
						skippedAllWhitespace = true;
						continue;
					}
					else if (startedOnSkippableWhitespace && skippedAllWhitespace && (c == ' ' || c == '\t'))
					{
						return glm::clamp(i + 1, 0, (int32)panel.visibleCharacterBufferSize);
					}
					else if (!startedOnSkippableWhitespace && (c == ' ' || c == '\t'))
					{
						return glm::clamp(i + 1, 0, (int32)panel.visibleCharacterBufferSize);
					}
				}

				return 0;
			}

			case KeyMoveDirection::RightUntilBoundary:
			{
				uint8 c = panel.visibleCharacterBuffer[panel.cursor.bytePos];
				bool startedOnSkippableWhitespace = c == ' ' || c == '\t';
				bool skippedAllWhitespace = false;
				bool hitBoundaryCharacterButSkipped = false;

				for (int32 i = (int32)panel.cursor.bytePos; i < panel.visibleCharacterBufferSize; i++)
				{
					c = panel.visibleCharacterBuffer[i];

					// Handle newlines
					if (c == '\n' && i != panel.cursor.bytePos)
					{
						return i;
					}

					// Handle boundary characters
					if (isBoundaryCharacter(c) && i != panel.cursor.bytePos)
					{
						uint32 prevC = i - 1 >= 0 ? panel.visibleCharacterBuffer[i - 1] : '\0';
						if (isBoundaryCharacter(prevC) || prevC == ' ' || prevC == '\t' || hitBoundaryCharacterButSkipped)
						{
							hitBoundaryCharacterButSkipped = true;
							continue;
						}
						else
						{
							return i;
						}
					}
					else if (!isBoundaryCharacter(c) && hitBoundaryCharacterButSkipped)
					{
						return i;
					}

					// Handle skippable whitespace
					if (startedOnSkippableWhitespace && c != ' ' && c != '\t')
					{
						skippedAllWhitespace = true;
						continue;
					}
					else if (startedOnSkippableWhitespace && skippedAllWhitespace && (c == ' ' || c == '\t'))
					{
						return i;
					}
					else if (!startedOnSkippableWhitespace && (c == ' ' || c == '\t'))
					{
						return i;
					}
				}

				return (int32)panel.visibleCharacterBufferSize;
			}

			}

			return (int32)panel.cursor.bytePos;
		}

		static void scrollCursorIntoViewIfNeeded(CodeEditorPanelData& panel)
		{
			// If cursor is off-screen, scroll it into view
			uint32 cursorLine = getLineNumberFromPosition(panel, (uint32)panel.cursor.bytePos);
			if (cursorLine < panel.lineNumberStart || cursorLine >= panel.lineNumberStart + panel.numberLinesCanFitOnScreen)
			{
				if (cursorLine >= panel.lineNumberStart + panel.numberLinesCanFitOnScreen)
				{
					uint32 oldLineStart = panel.lineNumberStart;

					// Bring new line into view
					int32 newLineStart = (int32)cursorLine - (int32)panel.numberLinesCanFitOnScreen + 1;
					panel.lineNumberStart = (uint32)glm::clamp(newLineStart, 1, (int32)panel.totalNumberLines);

					// Parse up to the new end of our view if we scrolled down so the syntax highlighting is up to date
					Vec2i linesUpdated = CodeEditorPanelManager::getHighlighter().checkForUpdatesFrom(
						panel.syntaxHighlightTree,
						oldLineStart + panel.numberLinesCanFitOnScreen,
						newLineStart - oldLineStart
					);

					// Update bookkeeping for visualizing these syntax updates
					if (linesUpdated.min < (int32)panel.totalNumberLines)
					{
						panel.debugData.linesUpdated.emplace_back(linesUpdated);
						panel.debugData.ageOfLinesUpdated.emplace_back(std::chrono::high_resolution_clock::now());
					}
				}
				else if (cursorLine < panel.lineNumberStart)
				{
					panel.lineNumberStart = cursorLine;
				}

				panel.lineNumberByteStart = getLineNumberByteStartFrom(panel, panel.lineNumberStart);
			}
		}

		static void openIntellisensePanelAtCursor(CodeEditorPanelData& panel)
		{
			auto& analyzer = LuauLayer::getScriptAnalyzer();
			panel.intellisenseSuggestions = analyzer.sandbox(
				std::string((const char*)panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize),
				"code-being-edited",
				panel.cursorCurrentLine,
				panel.numOfCharsFromBeginningOfLine
			);
			panel.intellisensePanelOpen = true;
			panel.intellisenseScrollOffset = 0;
			panel.selectedIntellisenseSuggestion = 0;
		}

		static uint8 codepointToUtf8Str(uint8* const outBuffer, uint32 code)
		{
			if (code <= 0x7F)
			{
				outBuffer[0] = (uint8)code;
				return 1;
			}

			if (code <= 0x7FF)
			{
				outBuffer[0] = (uint8)(0xC0 | (code >> 6));   /* 110xxxxx */
				outBuffer[1] = (uint8)(0x80 | (code & 0x3F)); /* 10xxxxxx */
				return 2;
			}

			if (code <= 0xFFFF)
			{
				outBuffer[0] = (uint8)(0xE0 | (code >> 12));         /* 1110xxxx */
				outBuffer[1] = (uint8)(0x80 | ((code >> 6) & 0x3F)); /* 10xxxxxx */
				outBuffer[2] = (uint8)(0x80 | (code & 0x3F));        /* 10xxxxxx */
				return 3;
			}

			if (code <= 0x10FFFF)
			{
				outBuffer[0] = (uint8)(0xF0 | (code >> 18));          /* 11110xxx */
				outBuffer[1] = (uint8)(0x80 | ((code >> 12) & 0x3F)); /* 10xxxxxx */
				outBuffer[2] = (uint8)(0x80 | ((code >> 6) & 0x3F));  /* 10xxxxxx */
				outBuffer[3] = (uint8)(0x80 | (code & 0x3F));         /* 10xxxxxx */
				return 4;
			}

			return 0;
		}

		static int32 getBeginningOfLineFrom(CodeEditorPanelData const& panel, int32 index)
		{
			if (index <= 0)
			{
				return 0;
			}

			auto tmpCursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, index);
			if (tmpCursor.bytePos == panel.visibleCharacterBufferSize || (*tmpCursor).value() == (uint32)'\n')
			{
				--tmpCursor;
			}

			while (tmpCursor.bytePos != 0)
			{
				if ((*tmpCursor).value() == '\n')
				{
					++tmpCursor;
					return (int32)tmpCursor.bytePos;
				}
				--tmpCursor;
			}

			return (int32)tmpCursor.bytePos;
		}

		static int32 getNumCharsFromBeginningOfLine(CodeEditorPanelData const& panel, int32 index)
		{
			if (index <= 0)
			{
				return 0;
			}

			int32 numChars = 0;
			auto tmpCursor = CppUtils::String::makeIterFromBytePos(panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, index);
			if (tmpCursor.bytePos == panel.visibleCharacterBufferSize || (*tmpCursor).value() == (uint32)'\n')
			{
				--tmpCursor;
				numChars++;
			}

			while (tmpCursor.bytePos != 0)
			{
				if ((*tmpCursor).value() == '\n')
				{
					return numChars;
				}

				--tmpCursor;
				numChars++;
			}

			return numChars;
		}

		static int32 getEndOfLineFrom(CodeEditorPanelData const& panel, int32 index)
		{
			int32 cursor = index;
			while (cursor < panel.visibleCharacterBufferSize)
			{
				if (panel.visibleCharacterBuffer[cursor] == '\n')
				{
					return cursor;
				}
				cursor++;
			}

			return cursor - 1;
		}

		static void setCursorDistanceFromLineStart(CodeEditorPanelData& panel)
		{
			panel.beginningOfCurrentLineByte = getBeginningOfLineFrom(panel, (int32)panel.cursor.bytePos);
			panel.numOfCharsFromBeginningOfLine = getNumCharsFromBeginningOfLine(panel, (int32)panel.cursor.bytePos);
		}

		static uint32 getLineNumberByteStartFrom(CodeEditorPanelData const& panel, uint32 lineNumber)
		{
			uint32 currentLine = 1;
			for (uint32 i = 0; i < (uint32)panel.visibleCharacterBufferSize; i++)
			{
				if (currentLine >= lineNumber)
				{
					return i;
				}

				if (panel.visibleCharacterBuffer[i] == '\n')
				{
					currentLine++;
				}
			}

			return (uint32)panel.visibleCharacterBufferSize;
		}

		static uint32 getLineNumberFromPosition(CodeEditorPanelData const& panel, uint32 bytePos)
		{
			uint32 currentLine = 1;
			for (uint32 i = 0; i < (uint32)panel.visibleCharacterBufferSize; i++)
			{
				if (i >= bytePos)
				{
					return currentLine;
				}

				if (panel.visibleCharacterBuffer[i] == '\n')
				{
					currentLine++;
				}
			}

			return panel.totalNumberLines;
		}

		static float calculateLeftGutterWidth(CodeEditorPanelData const& panel)
		{
			if (panel.totalNumberLines >= 1'000)
			{
				int numAdditionalSpace = 0;
				uint32 numLines = panel.totalNumberLines;
				while (numLines >= 1'000)
				{
					numLines /= 10;
					numAdditionalSpace++;
				}

				return baseLeftGutterWidth + (numAdditionalSpace * additionalLeftGutterWidth);
			}

			return baseLeftGutterWidth;
		}
	}
}