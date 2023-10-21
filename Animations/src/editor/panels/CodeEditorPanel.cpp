#include "editor/panels/CodeEditorPanel.h"
#include "editor/panels/CodeEditorPanelManager.h"
#include "editor/imgui/ImGuiLayer.h"
#include "platform/Platform.h"
#include "core/Application.h"
#include "core/Input.h"
#include "core/Window.h"
#include "renderer/Fonts.h"
#include "editor/TextEditUndo.h"
#include "parsers/SyntaxTheme.h"

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
		// ------------- Internal Functions -------------
		static void resetSelection(CodeEditorPanelData& panel);
		static void handleTypingUndo(CodeEditorPanelData& panel);
		static void handleBackspaceUndo(CodeEditorPanelData& panel, bool shouldSetTextSelectedOnUndo);
		static void handleDeleteUndo(CodeEditorPanelData& panel, bool shouldSetTextSelectedOnUndo);
		static void moveTextCursor(CodeEditorPanelData& panel, KeyMoveDirection direction);
		static void moveTextCursorAndResetSelection(CodeEditorPanelData& panel, KeyMoveDirection direction);
		static void renderTextCursor(CodeEditorPanelData& panel, ImVec2 const& textCursorDrawPosition, SizedFont const* const font);
		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber, SizedFont const* const font);
		static bool mouseInTextEditArea(CodeEditorPanelData const& panel);
		static ImVec2 addStringToDrawList(ImDrawList* drawList, SizedFont const* const font, std::string const& str, ImVec2 const& drawPos, ImColor const& color);
		static ImVec2 addCharToDrawList(ImDrawList* drawList, SizedFont const* const font, uint32 charToAdd, ImVec2 const& drawPos, ImColor const& color);
		static bool removeSelectedTextWithBackspace(CodeEditorPanelData& panel, bool addBackspaceToUndoHistory = true);
		static bool removeSelectedTextWithDelete(CodeEditorPanelData& panel);
		static bool removeText(CodeEditorPanelData& panel, int32 textToRemoveOffset, int32 textToRemoveNumBytes);
		static int32 getNewCursorPositionFromMove(CodeEditorPanelData const& panel, KeyMoveDirection direction);
		static void scrollCursorIntoViewIfNeeded(CodeEditorPanelData& panel);
		// TODO: Move this to <cppStrings.hpp>
		static uint8 codepointToUtf8Str(uint8* const buffer, uint32 codepoint);

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

		static int32 getBeginningOfLineFrom(CodeEditorPanelData const& panel, int32 index)
		{
			if (index == 0)
			{
				return 0;
			}

			int32 cursor = index;
			if (panel.byteMap[panel.visibleCharacterBuffer[cursor]] == '\n')
			{
				cursor--;
			}

			while (cursor > 0)
			{
				if (panel.byteMap[panel.visibleCharacterBuffer[cursor]] == '\n')
				{
					return cursor + 1;
				}
				cursor--;
			}

			return cursor;
		}

		static int32 getEndOfLineFrom(CodeEditorPanelData const& panel, int32 index)
		{
			int32 cursor = index;
			while (cursor < panel.visibleCharacterBufferSize)
			{
				if (panel.byteMap[panel.visibleCharacterBuffer[cursor]] == '\n')
				{
					return cursor;
				}
				cursor++;
			}

			return cursor - 1;
		}

		static void setCursorDistanceFromLineStart(CodeEditorPanelData& panel)
		{
			int32 beginningOfLine = getBeginningOfLineFrom(panel, panel.cursorBytePosition);
			panel.cursorDistanceToBeginningOfLine = (panel.cursorBytePosition - beginningOfLine);
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

				if (panel.byteMap[panel.visibleCharacterBuffer[i]] == '\n')
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

				if (panel.byteMap[panel.visibleCharacterBuffer[i]] == '\n')
				{
					currentLine++;
				}
			}

			return panel.totalNumberLines;
		}

		// ------------- Internal Data -------------
		static float leftGutterWidth = 60;
		static float scrollbarWidth = 10;
		static float textCursorWidth = 3;
		static uint32 numberBufferLines = 30;

		static ImColor lineNumberColor = ImColor(255, 255, 255);
		static ImColor textCursorColor = ImColor(255, 255, 255);
		static ImColor highlightTextColor = ImColor(115, 163, 240, 128);
		static ImColor backgroundColor = ImColor(44, 44, 44);
		static ImColor scrollbarColor = ImColor(95, 95, 95);

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
			fread(memory.data, fileSize + 1, 1, fp);
			fclose(fp);

			memory.data[fileSize + 1] = '\0';

			// Preprocess file into usable buffer
			res->visibleCharacterBuffer = nullptr;
			res->visibleCharacterBufferSize = 0;
			res->lineNumberStart = 1;
			res->lineNumberByteStart = 0;
			res->undoTypingStart = -1;
			res->totalNumberLines = 0;

			translateStringToLocalByteMapping(*res, (uint8*)memory.data, fileSize, &res->visibleCharacterBuffer, &res->visibleCharacterBufferSize, &res->totalNumberLines);
			// +1 for the extra line for EOF
			res->totalNumberLines++;

			reparseSyntax(*res);

			res->undoSystem = UndoSystem::createTextEditorUndoSystem(res, MAX_UNDO_HISTORY);

			g_memory_free(memory.data);

			return res;
		}

		void saveFile(CodeEditorPanelData const& panel)
		{
			uint8* utf8String = nullptr;
			size_t utf8StringNumBytes = 0;
			translateLocalByteMappingToString(panel, panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize, &utf8String, &utf8StringNumBytes);

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
			// TODO: This is very gross, instead of copying a million times, we should get rid of the local byte
			//       mapping garbage since it's not working well.
			uint8* noCarriageReturnStr = nullptr;
			size_t noCarriageReturnStrLength = 0;
			translateLocalByteMappingToString(
				panel,
				panel.visibleCharacterBuffer,
				panel.visibleCharacterBufferSize,
				&noCarriageReturnStr,
				&noCarriageReturnStrLength,
				false
			);

			// Parse the syntax
			// TODO: This will not work with UTF8, fix it.
			panel.syntaxHighlightTree = CodeEditorPanelManager::getHighlighter().parse(
				std::string((const char*)noCarriageReturnStr, noCarriageReturnStrLength),
				CodeEditorPanelManager::getTheme(),
				true
			);

			g_memory_free(noCarriageReturnStr);
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
					if (panel.undoTypingStart != -1)
					{
						handleTypingUndo(panel);
					}

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
					translateStringToLocalByteMapping(panel, (uint8*)utf8String, utf8StringNumBytes, &outStr, &numCharacters);

					// Free the new memory since we don't actually need the string
					g_memory_free(outStr);

					// Perform the paste and add an undo action
					int32 insertPosition = panel.cursorBytePosition;
					addUtf8StringToBuffer(panel, (uint8*)utf8String, utf8StringNumBytes, panel.cursorBytePosition);

					UndoSystem::insertTextAction(panel.undoSystem, utf8String, utf8StringNumBytes, insertPosition, numCharacters);
					scrollCursorIntoViewIfNeeded(panel);
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

						translateLocalByteMappingToString(
							panel,
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
					panel.cursorBytePosition = (int32)panel.visibleCharacterBufferSize;
					panel.mouseByteDragStart = 0;
				}

				// Handle undo/redo
				if (Input::keyRepeatedOrDown(GLFW_KEY_Z, KeyMods::Ctrl))
				{
					if (panel.undoTypingStart != -1)
					{
						handleTypingUndo(panel);
					}

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
					else if (Input::keyRepeatedOrDown(GLFW_KEY_UP))
					{
						moveTextCursorAndResetSelection(panel, KeyMoveDirection::Up);
					}
					else if (Input::keyRepeatedOrDown(GLFW_KEY_DOWN))
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

				// Handle key presses to move cursor + select modifier (Shift)
				{
					int32 oldBytePos = panel.cursorBytePosition;
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

					if (panel.cursorBytePosition != oldBytePos)
					{
						if (panel.cursorBytePosition == panel.mouseByteDragStart)
						{
							panel.firstByteInSelection = panel.cursorBytePosition;
							panel.lastByteInSelection = panel.cursorBytePosition;
						}
						else if (panel.cursorBytePosition > panel.mouseByteDragStart)
						{
							panel.lastByteInSelection = panel.cursorBytePosition;
						}
						else
						{
							panel.firstByteInSelection = panel.cursorBytePosition;
						}
					}
				}

				// Handle backspace
				// TODO: Not all backspaces are handled for some reason
				if (Input::keyRepeatedOrDown(GLFW_KEY_BACKSPACE))
				{
					if (removeSelectedTextWithBackspace(panel))
					{
						fileHasBeenEdited = true;
					}
				}

				// Handle delete
				if (Input::keyRepeatedOrDown(GLFW_KEY_DELETE))
				{
					if (removeSelectedTextWithDelete(panel))
					{
						fileHasBeenEdited = true;
					}
				}

				// Handle text-insertion
				if (uint32 codepoint = Input::getLastCharacterTyped(); codepoint != 0)
				{
					if (panel.firstByteInSelection != panel.lastByteInSelection)
					{
						removeSelectedTextWithBackspace(panel);
					}

					if (panel.undoTypingStart == -1)
					{
						panel.undoTypingStart = panel.cursorBytePosition;
					}

					addCodepointToBuffer(panel, codepoint, panel.cursorBytePosition);
					setCursorDistanceFromLineStart(panel);
					fileHasBeenEdited = true;
				}

				// Handle newline-insertion
				if (Input::keyRepeatedOrDown(GLFW_KEY_ENTER))
				{
					if (panel.firstByteInSelection != panel.lastByteInSelection)
					{
						removeSelectedTextWithBackspace(panel);
					}

					if (panel.undoTypingStart == -1)
					{
						panel.undoTypingStart = panel.cursorBytePosition;
					}

					addCodepointToBuffer(panel, (uint32)'\n', panel.cursorBytePosition);
					setCursorDistanceFromLineStart(panel);
					fileHasBeenEdited = true;
				}
			}

			// ---- Handle Rendering/mouse clicking ----
			auto syntaxTheme = CodeEditorPanelManager::getTheme();
			panel.drawStart = ImGui::GetCursorScreenPos();
			panel.drawEnd = panel.drawStart + ImGui::GetContentRegionAvail();

			panel.numberLinesCanFitOnScreen = (uint32)glm::floor((panel.drawEnd.y - panel.drawStart.y) / getLineHeight(codeFont));

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->PushClipRect(panel.drawStart, panel.drawEnd, true);
			drawList->AddRectFilled(panel.drawStart, panel.drawEnd, getColor(syntaxTheme.defaultBackground.color));

			uint32 currentLine = panel.lineNumberStart;
			ImVec2 currentLetterDrawPos = renderNextLinePrefix(panel, currentLine, codeFont);

			// Handle scroll bar render and logic
			if (windowIsHovered && panel.numberLinesCanFitOnScreen < panel.totalNumberLines + numberBufferLines)
			{
				const uint32 totalLinesIncludingBuffer = panel.totalNumberLines + numberBufferLines;
				float scrollbarHeight = ((float)panel.numberLinesCanFitOnScreen / (float)totalLinesIncludingBuffer) * (panel.drawEnd.y - panel.drawStart.y);
				float scrollYPos = ((float)(currentLine - 1) / (float)totalLinesIncludingBuffer) * (panel.drawEnd.y - panel.drawStart.y);

				ImVec2 scrollDrawStart = ImVec2(panel.drawEnd.x - scrollbarWidth, panel.drawStart.y + scrollYPos);
				ImVec2 scrollDrawEnd = scrollDrawStart + ImVec2(scrollbarWidth, scrollbarHeight);
				drawList->AddRectFilled(scrollDrawStart, scrollDrawEnd, scrollbarColor, 0.3f);

				if (Input::scrollY != 0)
				{
					int32 newLine = (int32)currentLine - ((int32)Input::scrollY * 3);
					newLine = (int32)glm::clamp(newLine, 1, (int32)(panel.totalNumberLines + numberBufferLines - panel.numberLinesCanFitOnScreen));

					panel.lineNumberStart = (uint32)newLine;
					currentLine = panel.lineNumberStart;
					panel.lineNumberByteStart = getLineNumberByteStartFrom(panel, (uint32)newLine);
				}
			}

			bool justStartedDragSelecting = false;
			if (mouseInTextEditArea(panel))
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

			bool lineHighlightStartFound = false;
			bool lastCharWasNewline = false;
			bool passedFirstCharacter = false;
			ImVec2 textHighlightRectStart = ImVec2();
			int32 closestByteToMouseCursor = (int32)panel.lineNumberByteStart;
			auto highlightIter = panel.syntaxHighlightTree.segments.begin();

			for (size_t cursor = panel.lineNumberByteStart; cursor < panel.visibleCharacterBufferSize; cursor++)
			{
				ImColor highlightedColor = syntaxTheme.defaultForeground.color;

				// Figure out what color this character should be
				if (highlightIter != panel.syntaxHighlightTree.segments.end() && cursor >= highlightIter->endPos)
				{
					highlightIter++;
				}

				if (highlightIter != panel.syntaxHighlightTree.segments.end() &&
					cursor >= highlightIter->startPos && cursor < highlightIter->endPos)
				{
					highlightedColor = getColor(highlightIter->color);
				}

				uint32 currentCodepoint = panel.byteMap[panel.visibleCharacterBuffer[cursor]];
				ImVec2 letterBoundsStart = currentLetterDrawPos;
				ImVec2 letterBoundsSize = addCharToDrawList(
					drawList,
					codeFont,
					currentCodepoint,
					letterBoundsStart,
					highlightedColor
				);

				if (cursor == panel.cursorBytePosition)
				{
					ImVec2 textCursorDrawPosition = letterBoundsStart;
					renderTextCursor(panel, textCursorDrawPosition, codeFont);
				}
				else if (cursor == panel.visibleCharacterBufferSize - 1 && panel.cursorBytePosition == panel.visibleCharacterBufferSize)
				{
					ImVec2 textCursorDrawPosition = letterBoundsStart + ImVec2(letterBoundsSize.x, 0.0f);
					if (currentCodepoint == '\n')
					{
						textCursorDrawPosition = getTopLeftOfLine(panel, currentLine, codeFont);

						const float lineHeight = codeFont->unsizedFont->lineHeight * codeFont->fontSizePixels;
						textCursorDrawPosition = textCursorDrawPosition + ImVec2(leftGutterWidth + style.FramePadding.x, lineHeight);
					}
					renderTextCursor(panel, textCursorDrawPosition, codeFont);
				}

				if (currentCodepoint == (uint32)'\n')
				{
					currentLine++;

					// Only render and handle logic for the visible characters
					if (currentLine > panel.lineNumberStart + panel.numberLinesCanFitOnScreen)
					{
						break;
					}

					currentLetterDrawPos = renderNextLinePrefix(panel, currentLine, codeFont);
				}

				// If the mouse was clicked before any character, the closest byte is the first byte
				if (!passedFirstCharacter && (
					io.MousePos.y < letterBoundsStart.y || (
					io.MousePos.x <= letterBoundsStart.x && io.MousePos.y >= letterBoundsStart.y
					&& io.MousePos.y <= letterBoundsStart.y + letterBoundsSize.y
					)))
				{
					closestByteToMouseCursor = (int32)cursor;

				}
				// If the mouse clicked in between a line, then the closest byte is in this line
				else if (io.MousePos.y >= letterBoundsStart.y && io.MousePos.y <= letterBoundsStart.y + letterBoundsSize.y)
				{
					// If we clicked in a letter bounds, then that's the closest byte to the mouse
					if (io.MousePos.x >= letterBoundsStart.x && io.MousePos.x < letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = (int32)cursor;
					}
					// If we clicked past the end of the line, the closest byte is the end of the line
					else if (currentCodepoint == '\n' && io.MousePos.x >= letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = (int32)cursor;
					}
					// If we clicked past the last character in the file, the closest byte is the end of the file
					else if (cursor == panel.visibleCharacterBufferSize - 1 && io.MousePos.x >= letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = (int32)panel.visibleCharacterBufferSize;
					}
					// If we clicked before any letters in the start of the line, the closest byte
					// is the first byte in this line
					else if (io.MousePos.x < letterBoundsStart.x && lastCharWasNewline)
					{
						closestByteToMouseCursor = (int32)cursor;
					}
				}
				// If the mouse clicked past the last visible character, then the closest byte is the end of the file
				else if (cursor == panel.visibleCharacterBufferSize - 1 && io.MousePos.y >= letterBoundsStart.y + letterBoundsSize.y)
				{
					closestByteToMouseCursor = (int32)panel.visibleCharacterBufferSize;
				}

				// Render selected text
				if (panel.firstByteInSelection != panel.lastByteInSelection &&
					cursor >= panel.firstByteInSelection && cursor <= panel.lastByteInSelection)
				{
					if (!lineHighlightStartFound)
					{
						textHighlightRectStart = letterBoundsStart;
						lineHighlightStartFound = true;
					}

					if (currentCodepoint == (uint32)'\n' && cursor != panel.lastByteInSelection)
					{
						// Draw highlight to the end of the line
						ImVec2 lineEndPos = ImVec2(
							panel.drawEnd.x - scrollbarWidth,
							letterBoundsStart.y + letterBoundsSize.y
						);
						drawList->AddRectFilled(textHighlightRectStart, lineEndPos, highlightTextColor);
						textHighlightRectStart = ImVec2(
							panel.drawStart.x + leftGutterWidth + style.FramePadding.x,
							letterBoundsStart.y + letterBoundsSize.y
						);
					}
					else if (cursor == panel.lastByteInSelection)
					{
						// Draw highlight to beginning of the last character selected
						ImVec2 highlightEnd = letterBoundsStart + ImVec2(0.0f, letterBoundsSize.y);
						drawList->AddRectFilled(textHighlightRectStart, highlightEnd, highlightTextColor);
					}
					else if (cursor == panel.visibleCharacterBufferSize - 1)
					{
						// NOTE: This is the special case of when you're highlighting the last character in the file, which we won't
						//       iterate past normally
						// Draw highlight to end of the last character selected
						ImVec2 highlightEnd = letterBoundsStart + letterBoundsSize;
						drawList->AddRectFilled(textHighlightRectStart, highlightEnd, highlightTextColor);
					}
				}

				currentLetterDrawPos.x += letterBoundsSize.x;
				lastCharWasNewline = currentCodepoint == (uint32)'\n';
				passedFirstCharacter = true;
			}

			drawList->PopClipRect();

			if (panel.mouseIsDragSelecting)
			{
				if (justStartedDragSelecting)
				{
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

			if (mouseInTextEditArea(panel) || panel.mouseIsDragSelecting)
			{
				if (io.MouseDown[ImGuiMouseButton_Left])
				{
					panel.cursorBytePosition = closestByteToMouseCursor;
					setCursorDistanceFromLineStart(panel);
				}

				const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				if (mouseClicked && !panel.mouseIsDragSelecting)
				{
					panel.mouseByteDragStart = (int32)panel.cursorBytePosition;
					panel.firstByteInSelection = (int32)panel.cursorBytePosition;
					panel.lastByteInSelection = (int32)panel.cursorBytePosition;
				}
			}

			// We'll render the text cursor at the start of the file if the file is empty
			if (panel.visibleCharacterBufferSize == 0)
			{
				renderTextCursor(panel, currentLetterDrawPos, codeFont);
			}

			{
				ImGui::Begin("Stats##Panel1");
				ImGui::Text("                 Selection Begin: %d", panel.firstByteInSelection);
				ImGui::Text("                   Selection End: %d", panel.lastByteInSelection);
				ImGui::Text("                      Drag Start: %d", panel.mouseByteDragStart);
				ImGui::Text("                     Cursor Byte: %d", panel.cursorBytePosition);
				ImGui::Text("                 Line start dist: %d", panel.cursorDistanceToBeginningOfLine);
				char character = panel.visibleCharacterBufferSize > 0
					? (char)panel.byteMap[panel.visibleCharacterBuffer[panel.cursorBytePosition]]
					: '\0';
				if (character != '\r' && character != '\n' && character != '\t' && character != '\0')
				{
					ImGui::Text("                       Character: %c", character);
				}
				else
				{
					char escapedCharacter = character == '\r' ? 'r' :
						character == '\n' ? 'n' :
						character == '\t' ? 't' : '0';
					ImGui::Text("                       Character: \\%c", escapedCharacter);
				}
				ImGui::End();
			}

			return fileHasBeenEdited;
		}

		void setCursorToLine(CodeEditorPanelData& panel, uint32 lineNumber)
		{
			g_logger_assert(lineNumber <= panel.totalNumberLines, "This shouldn't happen.");

			panel.cursorBytePosition = getLineNumberByteStartFrom(panel, lineNumber);
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;
			panel.mouseByteDragStart = panel.cursorBytePosition;

			scrollCursorIntoViewIfNeeded(panel);
		}

		void addUtf8StringToBuffer(CodeEditorPanelData& panel, uint8* utf8String, size_t stringNumBytes, size_t insertPosition)
		{
			g_logger_assert(insertPosition <= panel.visibleCharacterBufferSize, "Cannot insert string past end of buffer.");

			// Translate UTF8 string to local byte mapping
			uint8* byteMappedString = nullptr;
			size_t byteMappedStringLength = 0;
			uint32 numberLinesInString = 0;

			translateStringToLocalByteMapping(panel, utf8String, stringNumBytes, &byteMappedString, &byteMappedStringLength, &numberLinesInString);

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

			panel.cursorBytePosition = (int32)(insertPosition + byteMappedStringLength);
			panel.mouseByteDragStart = panel.cursorBytePosition;
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;

			panel.totalNumberLines += numberLinesInString;

			g_memory_free(byteMappedString);
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

			panel.cursorBytePosition = textToRemoveStart;
			panel.mouseByteDragStart = panel.cursorBytePosition;
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;

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

			panel.cursorBytePosition = textToRemoveStart;
			panel.mouseByteDragStart = panel.cursorBytePosition;
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;

			return true;
		}

		void translateStringToLocalByteMapping(CodeEditorPanelData& panel, uint8* utf8String, size_t stringNumBytes, uint8** outStr, size_t* outStrLength, uint32* numberLines)
		{
			*outStr = nullptr;
			*outStrLength = 0;
			if (numberLines != nullptr)
			{
				*numberLines = 0;
			}

			// Translate UTF8 string to local byte mapping
			auto maybeParseInfo = ::Parser::makeParseInfo((char*)utf8String, stringNumBytes);
			if (!maybeParseInfo.hasValue())
			{
				g_logger_error("Could not add UTF-8 string '{}' to editor. Had error '{}'.", utf8String, maybeParseInfo.error());
				return;
			}

			*outStr = (uint8*)g_memory_allocate(sizeof(uint8) * stringNumBytes);

			auto parseInfo = maybeParseInfo.value();
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
				}

				// Also, skip carriage returns 'cuz screw those
				if (codepoint.value() == (uint32)'\r')
				{
					parseInfo.cursor += numBytesParsed;
					continue;
				}

				uint8 byteMapping = CodeEditorPanelManager::addCharToFont(codepoint.value());
				panel.byteMap[byteMapping] = codepoint.value();

				(*outStr)[(*outStrLength)] = byteMapping;
				*outStrLength = *outStrLength + 1;

				parseInfo.cursor += numBytesParsed;
			}

			*outStr = (uint8*)g_memory_realloc((void*)(*outStr), (*outStrLength) * sizeof(uint8));
		}

		void translateLocalByteMappingToString(CodeEditorPanelData const& panel, uint8* byteMappedString, size_t byteMappedStringNumBytes, uint8** outUtf8String, size_t* outUtf8StringNumBytes, bool includeCarriageReturnsForWindows)
		{
			// Translate to valid UTF-8
			RawMemory memory{};
			memory.init(sizeof(uint8) * byteMappedStringNumBytes);

			for (size_t i = 0; i < byteMappedStringNumBytes; i++)
			{
				uint32 codepoint = panel.byteMap[byteMappedString[i]];

				uint8 utf8Bytes[4] = {};
				uint8 numBytes = codepointToUtf8Str(utf8Bytes, codepoint);

				#ifdef _WIN32
				if (includeCarriageReturnsForWindows)
				{
					g_logger_assert(codepoint != (uint32)'\r', "We should never get carriage returns in our edit buffers");

					// If we're on windows, translate newlines to carriage return + newlines when saving the files again
					if (codepoint == (uint32)'\n')
					{
						uint8 carriageReturn = '\r';
						memory.write(&carriageReturn);
					}
				}
				#endif

				memory.writeDangerous(utf8Bytes, numBytes * sizeof(uint8));
			}

			memory.shrinkToFit();

			*outUtf8String = memory.data;
			*outUtf8StringNumBytes = memory.size;
		}

		// ------------- Internal Functions -------------
		static void resetSelection(CodeEditorPanelData& panel)
		{
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;
			panel.mouseByteDragStart = panel.cursorBytePosition;
		}

		static void handleTypingUndo(CodeEditorPanelData& panel)
		{
			// Nothing to undo, just skip adding an undo operation
			if (panel.undoTypingStart == -1 || panel.undoTypingStart == panel.cursorBytePosition)
			{
				return;
			}

			int32 numBytesInUndo = (panel.cursorBytePosition - panel.undoTypingStart);
			if (panel.undoTypingStart > panel.cursorBytePosition || numBytesInUndo < 0 ||
				panel.undoTypingStart + numBytesInUndo > panel.visibleCharacterBufferSize)
			{
				g_logger_warning("Invalid undo typing cursor encountered.");
				panel.undoTypingStart = -1;
				return;
			}

			// We typed some stuff, and we want to add it as one big undo operation
			uint8* utf8String = nullptr;
			size_t utf8StringNumBytes = 0;

			translateLocalByteMappingToString(
				panel,
				panel.visibleCharacterBuffer + panel.undoTypingStart,
				numBytesInUndo,
				&utf8String,
				&utf8StringNumBytes
			);

			UndoSystem::insertTextAction(panel.undoSystem, utf8String, utf8StringNumBytes, panel.undoTypingStart, numBytesInUndo);

			g_memory_free(utf8String);

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

			// Translate the text to a utf8-string
			uint8* textAboutToBeDeleted = nullptr;
			size_t textAboutToBeDeletedLength = 0;
			translateLocalByteMappingToString(
				panel,
				panel.visibleCharacterBuffer + panel.firstByteInSelection,
				numBytesToDelete,
				&textAboutToBeDeleted,
				&textAboutToBeDeletedLength
			);

			UndoSystem::backspaceTextAction(
				panel.undoSystem,
				textAboutToBeDeleted,
				textAboutToBeDeletedLength,
				panel.firstByteInSelection,
				panel.firstByteInSelection + numBytesToDelete,
				panel.cursorBytePosition,
				shouldSetTextSelectedOnUndo
			);

			g_memory_free(textAboutToBeDeleted);
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

			// Translate the text to a utf8-string
			uint8* textAboutToBeDeleted = nullptr;
			size_t textAboutToBeDeletedLength = 0;
			translateLocalByteMappingToString(
				panel,
				panel.visibleCharacterBuffer + panel.firstByteInSelection,
				numBytesToDelete,
				&textAboutToBeDeleted,
				&textAboutToBeDeletedLength
			);

			UndoSystem::deleteTextAction(
				panel.undoSystem,
				textAboutToBeDeleted,
				textAboutToBeDeletedLength,
				panel.firstByteInSelection,
				panel.firstByteInSelection + numBytesToDelete,
				panel.cursorBytePosition,
				shouldSetTextSelectedOnUndo
			);

			g_memory_free(textAboutToBeDeleted);
		}

		static void moveTextCursor(CodeEditorPanelData& panel, KeyMoveDirection direction)
		{
			handleTypingUndo(panel);

			panel.cursorIsBlinkedOn = true;
			panel.timeSinceCursorLastBlinked = 0.0f;

			panel.cursorBytePosition = getNewCursorPositionFromMove(panel, direction);

			bool movedLines = direction == KeyMoveDirection::Up || direction == KeyMoveDirection::Down;
			if (!movedLines)
			{
				setCursorDistanceFromLineStart(panel);
			}

			scrollCursorIntoViewIfNeeded(panel);
		}

		static void moveTextCursorAndResetSelection(CodeEditorPanelData& panel, KeyMoveDirection direction)
		{
			moveTextCursor(panel, direction);
			resetSelection(panel);
		}

		static void renderTextCursor(CodeEditorPanelData& panel, ImVec2 const& drawPosition, SizedFont const* const font)
		{
			if (!ImGui::IsWindowFocused() && !panel.mouseIsDragSelecting)
			{
				return;
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
				drawList->AddRectFilled(drawPosition, drawPosition + ImVec2(textCursorWidth, currentLineHeight), textCursorColor);
			}
		}

		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber, SizedFont const* const font)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			std::string numberText = std::to_string(lineNumber);
			glm::vec2 textSize = font->getSizeOfString(numberText);

			ImVec2 lineStart = getTopLeftOfLine(panel, lineNumber, font);
			ImVec2 numberStart = lineStart + ImVec2(leftGutterWidth - textSize.x, 0.0f);
			addStringToDrawList(drawList, font, numberText, numberStart, ImColor(255, 255, 255, 255));

			return lineStart + ImVec2(leftGutterWidth + style.FramePadding.x, 0.0f);
		}

		static bool mouseInTextEditArea(CodeEditorPanelData const& panel)
		{
			ImVec2 textAreaStart = panel.drawStart + ImVec2(leftGutterWidth, 0.0f);
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

		static ImVec2 addCharToDrawList(ImDrawList* drawList, SizedFont const* const sizedFont, uint32 charToAdd, ImVec2 const& drawPos, ImColor const& color)
		{
			Font const* const font = sizedFont->unsizedFont;
			uint32 texId = sizedFont->texture.graphicsId;
			ImVec2 cursorPos = ImVec2(drawPos.x, drawPos.y + (font->lineHeight * sizedFont->fontSizePixels));

			if (charToAdd == (uint32)'\n' || charToAdd == (uint32)'\r')
			{
				return ImVec2(0.0f, (font->lineHeight * sizedFont->fontSizePixels));
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
			if (panel.cursorBytePosition == 0 && panel.firstByteInSelection == panel.lastByteInSelection)
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
				panel.firstByteInSelection--;
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
			if (panel.cursorBytePosition == panel.visibleCharacterBufferSize && panel.firstByteInSelection == panel.lastByteInSelection)
			{
				// Cannot delete if the cursor is at the end of the file and no text is selected
				return false;
			}

			// If we're pressing delete and no text is selected, pretend that the character in front of the cursor is selected
			// by pushing the selection window forward one
			bool shouldSetTextSelectedOnUndo = true;
			if (panel.firstByteInSelection == panel.lastByteInSelection)
			{
				panel.lastByteInSelection++;
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
			for (int32 i = textToRemoveOffset; i <= textToRemoveOffset + textToRemoveNumBytes; i++)
			{
				if (panel.byteMap[panel.visibleCharacterBuffer[i]] == '\n')
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
			uint32 oldTotalNumberLines = panel.totalNumberLines;
			panel.totalNumberLines -= numberLinesInString;

			// If removing the text changed the scroll height, shift the scroll up to fit on the screen
			if (panel.lineNumberStart + panel.numberLinesCanFitOnScreen > panel.totalNumberLines + numberBufferLines)
			{
				uint32 delta = (oldTotalNumberLines + numberBufferLines) - panel.lineNumberStart;
				int32 newLine = (int32)(panel.totalNumberLines + numberBufferLines) - (int32)delta;

				panel.lineNumberStart = (uint32)glm::clamp(newLine, 1, (int32)panel.totalNumberLines + (int32)numberBufferLines);
				panel.lineNumberByteStart = getLineNumberByteStartFrom(panel, panel.lineNumberStart);
			}

			return true;
		}

		static inline bool isBoundaryCharacter(uint32 c)
		{
			if (c == ':' || c == ';' || c == '"' || c == '\'' || c == '.' || c == '(' || c == ')' || c == '{' || c == '}'
				|| c == '-' || c == '+' || c == '*' || c == '/' || c == ',' || c == '=' || c == '!' || c == '`')
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
				return glm::clamp(panel.cursorBytePosition - 1, 0, (int32)panel.visibleCharacterBufferSize);
			}

			case KeyMoveDirection::Right:
			{
				return glm::clamp(panel.cursorBytePosition + 1, 0, (int32)panel.visibleCharacterBufferSize);
			}

			case KeyMoveDirection::Up:
			{
				// Try to go up a line while maintaining the same distance from the beginning of the line
				int32 beginningOfCurrentLine = getBeginningOfLineFrom(panel, panel.cursorBytePosition);
				int32 beginningOfLineAbove = getBeginningOfLineFrom(panel, beginningOfCurrentLine - 1);

				// Only set the cursor to a new position if there is another line above the current line
				if (beginningOfLineAbove == -1)
				{
					return panel.cursorBytePosition;
				}

				int32 newPos = beginningOfLineAbove;
				for (int32 i = beginningOfLineAbove; i < beginningOfCurrentLine; i++)
				{
					if (i - beginningOfLineAbove == panel.cursorDistanceToBeginningOfLine)
					{
						newPos = i;
						break;
					}
					else if (i == beginningOfCurrentLine - 1)
					{
						newPos = i;
					}
				}

				return glm::clamp(newPos, 0, (int32)panel.visibleCharacterBufferSize);
			}

			case KeyMoveDirection::Down:
			{
				// Try to go down a line while maintaining the same distance from the beginning of the line
				int32 endOfCurrentLine = getEndOfLineFrom(panel, panel.cursorBytePosition);
				int32 beginningOfLineBelow = endOfCurrentLine + 1;

				// Only set the cursor to a new position if there is another line below the current line
				if (beginningOfLineBelow > panel.visibleCharacterBufferSize)
				{
					return panel.cursorBytePosition;
				}

				int32 newPos = beginningOfLineBelow;
				for (int32 i = beginningOfLineBelow; i <= panel.visibleCharacterBufferSize; i++)
				{
					if (i == panel.visibleCharacterBufferSize || panel.byteMap[panel.visibleCharacterBuffer[i]] == '\n')
					{
						newPos = i;
						break;
					}
					else if (i - beginningOfLineBelow == panel.cursorDistanceToBeginningOfLine)
					{
						newPos = i;
						break;
					}
				}

				return glm::clamp(newPos, 0, (int32)panel.visibleCharacterBufferSize);
			}

			case KeyMoveDirection::LeftUntilBeginning:
			{
				int32 beginningOfCurrentLine = getBeginningOfLineFrom(panel, panel.cursorBytePosition);

				for (int32 i = beginningOfCurrentLine; i < panel.visibleCharacterBufferSize; i++)
				{
					uint32 c = panel.byteMap[panel.visibleCharacterBuffer[i]];
					if (c == ' ' || c == '\t')
					{
						continue;
					}

					// Return the first non-whitespace character from the beginning of the line if we're in front of the beginning
					// of the line, or at the beginning of the line
					if (panel.cursorBytePosition > i || panel.cursorBytePosition == beginningOfCurrentLine)
					{
						return i;
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
				int32 endOfCurrentLine = getEndOfLineFrom(panel, panel.cursorBytePosition);

				// If we're at the end of the file, move all the way to the end
				if (endOfCurrentLine == panel.visibleCharacterBufferSize - 1)
				{
					endOfCurrentLine++;
				}

				return endOfCurrentLine;
			}

			case KeyMoveDirection::LeftUntilBoundary:
			{
				int32 startPos = glm::clamp(panel.cursorBytePosition - 1, 0, (int32)panel.visibleCharacterBufferSize);

				uint32 c = panel.byteMap[panel.visibleCharacterBuffer[startPos]];
				bool startedOnSkippableWhitespace = c == ' ' || c == '\t';
				bool skippedAllWhitespace = false;
				bool hitBoundaryCharacterButSkipped = false;

				for (int32 i = panel.cursorBytePosition - 1; i >= 0; i--)
				{
					c = panel.byteMap[panel.visibleCharacterBuffer[i]];

					// Handle newlines
					if (c == '\n' && i != panel.cursorBytePosition - 1)
					{
						return glm::clamp(i + 1, 0, (int32)panel.visibleCharacterBufferSize);
					}

					// Handle boundary characters
					if (isBoundaryCharacter(c) && i != panel.cursorBytePosition - 1)
					{
						uint32 nextC = i + 1 < panel.visibleCharacterBufferSize ? panel.byteMap[panel.visibleCharacterBuffer[i + 1]] : '\0';
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
				uint32 c = panel.byteMap[panel.visibleCharacterBuffer[panel.cursorBytePosition]];
				bool startedOnSkippableWhitespace = c == ' ' || c == '\t';
				bool skippedAllWhitespace = false;
				bool hitBoundaryCharacterButSkipped = false;

				for (int32 i = panel.cursorBytePosition; i < panel.visibleCharacterBufferSize; i++)
				{
					c = panel.byteMap[panel.visibleCharacterBuffer[i]];

					// Handle newlines
					if (c == '\n' && i != panel.cursorBytePosition)
					{
						return i;
					}

					// Handle boundary characters
					if (isBoundaryCharacter(c) && i != panel.cursorBytePosition)
					{
						uint32 prevC = i - 1 >= 0 ? panel.byteMap[panel.visibleCharacterBuffer[i - 1]] : '\0';
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

			return panel.cursorBytePosition;
		}

		static void scrollCursorIntoViewIfNeeded(CodeEditorPanelData& panel)
		{
			// If cursor is off-screen, scroll it into view
			uint32 cursorLine = getLineNumberFromPosition(panel, panel.cursorBytePosition);
			if (cursorLine < panel.lineNumberStart || cursorLine >= panel.lineNumberStart + panel.numberLinesCanFitOnScreen)
			{
				if (cursorLine >= panel.lineNumberStart + panel.numberLinesCanFitOnScreen)
				{
					// Bring new line into view
					int32 newLineStart = (int32)cursorLine - (int32)panel.numberLinesCanFitOnScreen + 1;
					panel.lineNumberStart = (uint32)glm::clamp(newLineStart, 1, (int32)panel.totalNumberLines);
				}
				else if (cursorLine < panel.lineNumberStart)
				{
					panel.lineNumberStart = cursorLine;
				}

				panel.lineNumberByteStart = getLineNumberByteStartFrom(panel, panel.lineNumberStart);
			}
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
	}
}