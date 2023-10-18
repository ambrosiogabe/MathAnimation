#include "editor/panels/CodeEditorPanel.h"
#include "editor/panels/CodeEditorPanelManager.h"
#include "editor/imgui/ImGuiLayer.h"
#include "platform/Platform.h"
#include "core/Application.h"
#include "core/Input.h"
#include "renderer/Fonts.h"

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
		static void moveTextCursor(CodeEditorPanelData& panel, KeyMoveDirection direction);
		static void moveTextCursorAndResetSelection(CodeEditorPanelData& panel, KeyMoveDirection direction);
		static void renderTextCursor(CodeEditorPanelData& panel, ImVec2 const& textCursorDrawPosition, SizedFont const* const font);
		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber, SizedFont const* const font);
		static bool mouseInTextEditArea(CodeEditorPanelData const& panel);
		static ImVec2 addStringToDrawList(ImDrawList* drawList, SizedFont const* const font, std::string const& str, ImVec2 const& drawPos);
		static ImVec2 addCharToDrawList(ImDrawList* drawList, SizedFont const* const font, uint32 charToAdd, ImVec2 const& drawPos);
		static void addCodepointToBuffer(CodeEditorPanelData& panel, uint32 codepoint);
		static bool removeSelectedTextWithBackspace(CodeEditorPanelData& panel);
		static bool removeSelectedTextWithDelete(CodeEditorPanelData& panel);
		static void removeSelectedText(CodeEditorPanelData& panel);
		static int32 getNewCursorPositionFromMove(CodeEditorPanelData const& panel, KeyMoveDirection direction);

		// Simple calculation functions
		static inline ImVec2 getTopLeftOfLine(CodeEditorPanelData const& panel, uint32 lineNumber, SizedFont const* const font)
		{
			const float fontHeight = font->unsizedFont->lineHeight * font->fontSizePixels;
			return panel.drawStart
				+ ImVec2(0.0f, fontHeight * (lineNumber - panel.lineNumberStart));
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

		// ------------- Internal Data -------------
		static float leftGutterWidth = 60;
		static float scrollBarWidth = 10;
		static float textCursorWidth = 3;
		static ImColor lineNumberColor = ImColor(255, 255, 255);
		static ImColor textCursorColor = ImColor(255, 255, 255);
		static ImColor highlightTextColor = ImColor(115, 163, 240, 128);

		// This is the number of seconds between the cursor blinking
		static float cursorBlinkTime = 0.5f;

		CodeEditorPanelData openFile(std::string const& filepath)
		{
			CodeEditorPanelData res = {};
			res.filepath = filepath;

			FILE* fp = fopen(filepath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			RawMemory memory;
			memory.init(fileSize);
			fread(memory.data, fileSize, 1, fp);
			fclose(fp);

			// Parse this file and map all codepoints to a valid uint8 value
			auto maybeParseInfo = Parser::makeParseInfo((char*)memory.data, fileSize);
			if (!maybeParseInfo.hasValue())
			{
				g_logger_error("Could not open file '{}'. Had error '{}'.", filepath, maybeParseInfo.error());
				return {};
			}

			size_t numberCharactersInFile = 0;
			auto parseInfo = maybeParseInfo.value();
			while (parseInfo.cursor < parseInfo.numBytes)
			{
				uint8 numBytesParsed = 1;
				auto codepoint = Parser::parseCharacter(parseInfo, &numBytesParsed);
				if (!codepoint.hasValue())
				{
					g_logger_error("File has malformed unicode. Skipping bad unicode data.");
					parseInfo.numBytes++;
					continue;
				}

				uint8 byteMapping = CodeEditorPanelManager::addCharToFont(codepoint.value());
				res.byteMap[byteMapping] = codepoint.value();
				numberCharactersInFile++;

				parseInfo.cursor += numBytesParsed;
			}

			// Take ownership of the data here
			res.visibleCharacterBuffer = (uint8*)g_memory_allocate(sizeof(uint8) * numberCharactersInFile);
			res.visibleCharacterBufferSize = numberCharactersInFile;
			res.lineNumberStart = 1;
			res.lineNumberByteStart = 0;

			// Parse file one more time and translate it into the byte mapping we now have.
			parseInfo.cursor = 0;
			size_t byteCursor = 0;
			while (parseInfo.cursor < parseInfo.numBytes)
			{
				uint8 numBytesParsed = 0;
				auto codepoint = Parser::parseCharacter(parseInfo, &numBytesParsed);
				if (!codepoint.hasValue())
				{
					parseInfo.cursor++;
					continue;
				}

				// Also, skip carriage returns 'cuz screw those
				if (codepoint.value() == (uint32)'\r')
				{
					parseInfo.cursor += numBytesParsed;
					continue;
				}

				uint8 byteMapping = CodeEditorPanelManager::addCharToFont(codepoint.value());
				res.visibleCharacterBuffer[byteCursor] = byteMapping;

				parseInfo.cursor += numBytesParsed;
				byteCursor++;
			}

			g_memory_free((void*)parseInfo.utf8String);

			res.visibleCharacterBufferSize = byteCursor;
			res.visibleCharacterBuffer = (uint8*)g_memory_realloc((void*)res.visibleCharacterBuffer, res.visibleCharacterBufferSize * sizeof(uint8));

			return res;
		}

		void saveFile(CodeEditorPanelData const& panel)
		{
			// Translate to valid UTF-8
			RawMemory memory{};
			memory.init(sizeof(uint8) * panel.visibleCharacterBufferSize);

			for (size_t i = 0; i < panel.visibleCharacterBufferSize; i++)
			{
				uint32 codepoint = panel.byteMap[panel.visibleCharacterBuffer[i]];

				// TODO: Figure out way to translate codepoint to valid u8 bytes
				uint8 numBytes = 1;
				uint8 utf8Bytes = (uint8)codepoint;

				#ifdef _WIN32
				g_logger_assert(codepoint != (uint32)'\r', "We should never get carriage returns in our edit buffers");

				// If we're on windows, translate newlines to carriage return + newlines when saving the files again
				if (codepoint == (uint32)'\n')
				{
					uint8 carriageReturn = '\r';
					memory.write(&carriageReturn);
				}
				#endif

				memory.writeDangerous(&utf8Bytes, numBytes * sizeof(uint8));
			}

			memory.shrinkToFit();

			// Dump UTF-8 to file
			FILE* fp = fopen(panel.filepath.string().c_str(), "wb");
			fwrite(memory.data, memory.size, 1, fp);
			fclose(fp);

			// Free memory
			memory.free();
		}

		void free(CodeEditorPanelData& panel)
		{
			g_memory_free((void*)panel.visibleCharacterBuffer);
			panel = {};
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
			if (windowIsFocused)
			{
				io.WantTextInput = true;

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

					addCodepointToBuffer(panel, codepoint);
					setCursorDistanceFromLineStart(panel);
					fileHasBeenEdited = true;
				}

				// Handle newline-insertion
				if (Input::keyRepeatedOrDown(GLFW_KEY_ENTER))
				{
					addCodepointToBuffer(panel, (uint32)'\n');
					setCursorDistanceFromLineStart(panel);
					fileHasBeenEdited = true;
				}
			}

			// ---- Handle Rendering/mouse clicking ----
			panel.drawStart = ImGui::GetCursorScreenPos();
			panel.drawEnd = panel.drawStart + ImGui::GetContentRegionAvail();

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(panel.drawStart, panel.drawEnd, ImColor(44, 44, 44));

			uint32 currentLine = panel.lineNumberStart;
			ImVec2 currentLetterDrawPos = renderNextLinePrefix(panel, currentLine, codeFont);

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

			for (size_t cursor = 0; cursor < panel.visibleCharacterBufferSize; cursor++)
			{
				uint32 currentCodepoint = panel.byteMap[panel.visibleCharacterBuffer[cursor]];
				ImVec2 letterBoundsStart = currentLetterDrawPos;
				ImVec2 letterBoundsSize = addCharToDrawList(
					drawList,
					codeFont,
					currentCodepoint,
					letterBoundsStart
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
							panel.drawEnd.x - scrollBarWidth,
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

			if (mouseInTextEditArea(panel))
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

		// ------------- Internal Functions -------------
		static void resetSelection(CodeEditorPanelData& panel)
		{
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;
			panel.mouseByteDragStart = panel.cursorBytePosition;
		}

		static void moveTextCursor(CodeEditorPanelData& panel, KeyMoveDirection direction)
		{
			panel.cursorIsBlinkedOn = true;
			panel.timeSinceCursorLastBlinked = 0.0f;

			panel.cursorBytePosition = getNewCursorPositionFromMove(panel, direction);

			bool recalculateDistanceFromLineStart = direction != KeyMoveDirection::Up && direction != KeyMoveDirection::Down;
			if (recalculateDistanceFromLineStart)
			{
				setCursorDistanceFromLineStart(panel);
			}
		}

		static void moveTextCursorAndResetSelection(CodeEditorPanelData& panel, KeyMoveDirection direction)
		{
			moveTextCursor(panel, direction);
			resetSelection(panel);
		}

		static void renderTextCursor(CodeEditorPanelData& panel, ImVec2 const& drawPosition, SizedFont const* const font)
		{
			if (!ImGui::IsWindowFocused())
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
			addStringToDrawList(drawList, font, numberText, numberStart);

			return lineStart + ImVec2(leftGutterWidth + style.FramePadding.x, 0.0f);
		}

		static bool mouseInTextEditArea(CodeEditorPanelData const& panel)
		{
			ImVec2 textAreaStart = panel.drawStart + ImVec2(leftGutterWidth, 0.0f);
			ImVec2 textAreaEnd = panel.drawEnd - ImVec2(scrollBarWidth, 0.0f);
			return mouseIntersectsRect(textAreaStart, textAreaEnd);
		}

		static ImVec2 addStringToDrawList(ImDrawList* drawList, SizedFont const* const sizedFont, std::string const& str, ImVec2 const& drawPos)
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

				ImVec2 charSize = addCharToDrawList(drawList, sizedFont, (uint32)str[i], cursorPos);
				cursorPos = cursorPos + ImVec2(charSize.x, 0.0f);
			}

			return cursorPos - drawPos;
		}

		static ImVec2 addCharToDrawList(ImDrawList* drawList, SizedFont const* const sizedFont, uint32 charToAdd, ImVec2 const& drawPos)
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
					uvMin + ImVec2(0.0f, uvSize.y)
				);
			}

			cursorPos = cursorPos + Vec2{ glyphOutline.advanceX * (float)sizedFont->fontSizePixels, 0.0f };

			const float totalFontHeight = sizedFont->fontSizePixels * font->lineHeight;
			return ImVec2((cursorPos - drawPos).x, totalFontHeight);
		}

		static void addCodepointToBuffer(CodeEditorPanelData& panel, uint32 codepoint)
		{
			uint8 byteMapping = CodeEditorPanelManager::addCharToFont(codepoint);
			panel.byteMap[byteMapping] = codepoint;

			size_t newCharBufferSize = sizeof(uint8) * (panel.visibleCharacterBufferSize + 1);
			uint8* newCharBuffer = (uint8*)g_memory_allocate(newCharBufferSize);

			g_memory_copyMem(newCharBuffer, newCharBufferSize, (void*)panel.visibleCharacterBuffer, panel.cursorBytePosition);
			newCharBufferSize -= panel.cursorBytePosition;
			g_memory_copyMem(newCharBuffer + panel.cursorBytePosition, newCharBufferSize, &byteMapping, sizeof(uint8));
			newCharBufferSize -= sizeof(uint8);
			g_memory_copyMem(
				newCharBuffer + panel.cursorBytePosition + sizeof(uint8),
				newCharBufferSize,
				(void*)(panel.visibleCharacterBuffer + panel.cursorBytePosition),
				panel.visibleCharacterBufferSize - panel.cursorBytePosition
			);

			g_memory_free((void*)panel.visibleCharacterBuffer);

			panel.visibleCharacterBuffer = newCharBuffer;
			panel.visibleCharacterBufferSize = sizeof(uint8) * (panel.visibleCharacterBufferSize + 1);

			panel.cursorBytePosition++;
			panel.mouseByteDragStart = panel.cursorBytePosition;
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;
		}

		static bool removeSelectedTextWithBackspace(CodeEditorPanelData& panel)
		{
			if (panel.cursorBytePosition == 0 && panel.firstByteInSelection == panel.lastByteInSelection)
			{
				// Cannot backspace if the cursor is at the beginning of the file and no text is selected
				return false;
			}

			// If we're pressing backspace and no text is selected, pretend that the character behind the cursor is selected
			// by pushing the selection window back one
			if (panel.firstByteInSelection == panel.lastByteInSelection)
			{
				panel.firstByteInSelection--;
			}

			int32 numBytesToRemove = panel.lastByteInSelection - panel.firstByteInSelection;
			removeSelectedText(panel);

			if (panel.lastByteInSelection == panel.firstByteInSelection || panel.cursorBytePosition == panel.lastByteInSelection)
			{
				panel.cursorBytePosition = glm::clamp(panel.cursorBytePosition - (int32)numBytesToRemove, 0, (int32)panel.visibleCharacterBufferSize);
			}

			panel.mouseByteDragStart = panel.cursorBytePosition;
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;

			return true;
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
			if (panel.firstByteInSelection == panel.lastByteInSelection)
			{
				panel.lastByteInSelection++;
			}

			int32 numBytesToRemove = panel.lastByteInSelection - panel.firstByteInSelection;
			removeSelectedText(panel);

			if (panel.cursorBytePosition == panel.lastByteInSelection)
			{
				panel.cursorBytePosition = glm::clamp(panel.cursorBytePosition - (int32)numBytesToRemove, 0, (int32)panel.visibleCharacterBufferSize);
			}

			panel.mouseByteDragStart = panel.cursorBytePosition;
			panel.firstByteInSelection = panel.cursorBytePosition;
			panel.lastByteInSelection = panel.cursorBytePosition;

			return true;
		}

		static void removeSelectedText(CodeEditorPanelData& panel)
		{
			// We should never call this function by itself. Instead, call the backspace or delete versions of this function
			g_logger_assert(panel.firstByteInSelection != panel.lastByteInSelection, "Cannot remove selected text if no text is selected.");

			// The + 1 is because we include the last byte in the selection, so this range is inclusive
			size_t numBytesToRemove = panel.lastByteInSelection - panel.firstByteInSelection;
			size_t bytesToRemoveOffset = panel.firstByteInSelection;

			size_t bytesToMoveOffset = bytesToRemoveOffset + numBytesToRemove;
			if (bytesToMoveOffset != panel.visibleCharacterBufferSize)
			{
				memmove(
					(void*)(panel.visibleCharacterBuffer + bytesToRemoveOffset),
					(void*)(panel.visibleCharacterBuffer + bytesToMoveOffset),
					panel.visibleCharacterBufferSize - bytesToMoveOffset
				);
			}

			panel.visibleCharacterBufferSize -= numBytesToRemove;
			panel.visibleCharacterBuffer = (uint8*)g_memory_realloc((void*)panel.visibleCharacterBuffer, panel.visibleCharacterBufferSize);
		}

		static inline bool isBoundaryCharacter(uint32 c)
		{
			if (c == ':' || c == ';' || c == '"' || c == '\'' || c == '.' || c == '(' || c == ')' || c == '{' || c == '}'
				|| c == '-' || c == '+' || c == '*' || c == '/' || c == ',' || c == '=')
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
				// Try to go up a line while maintaining the same distance from the beginning of the line
				int32 endOfCurrentLine = getEndOfLineFrom(panel, panel.cursorBytePosition);
				int32 beginningOfLineBelow = endOfCurrentLine + 1;

				// Only set the cursor to a new position if there is another line below the current line
				if (beginningOfLineBelow >= panel.visibleCharacterBufferSize)
				{
					return panel.cursorBytePosition;
				}

				int32 newPos = beginningOfLineBelow;
				for (int32 i = beginningOfLineBelow; i < panel.visibleCharacterBufferSize; i++)
				{
					if (panel.byteMap[panel.visibleCharacterBuffer[i]] == '\n')
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
	}
}