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
	namespace CodeEditorPanel
	{
		// ------------- Internal Functions -------------
		static void renderTextCursor(CodeEditorPanelData& panel, ImVec2 const& textCursorDrawPosition, SizedFont const* const font);
		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber, SizedFont const* const font);
		static bool mouseInTextEditArea(CodeEditorPanelData const& panel);
		static ImVec2 addStringToDrawList(ImDrawList* drawList, SizedFont const* const font, std::string const& str, ImVec2 const& drawPos);
		static ImVec2 addCharToDrawList(ImDrawList* drawList, SizedFont const* const font, uint32 charToAdd, ImVec2 const& drawPos);
		static void addCodepointToBuffer(CodeEditorPanelData& panel, uint32 codepoint);

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

		static int32 getBeginningOfLineFrom(CodeEditorPanelData& panel, int32 index)
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

		static int32 getEndOfLineFrom(CodeEditorPanelData& panel, int32 index)
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

		static inline void setCursorBytePosition(CodeEditorPanelData& panel, int32 newBytePosition)
		{
			int32 newPos = glm::clamp(newBytePosition, 0, (int32)panel.visibleCharacterBufferSize);
			panel.cursorBytePosition = newPos;
			setCursorDistanceFromLineStart(panel);
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
			panel.timeSinceCursorLastBlinked += Application::getDeltaTime();

			// ---- Handle arrow key presses ----
			if (Input::keyRepeatedOrDown(GLFW_KEY_RIGHT))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				setCursorBytePosition(panel, panel.cursorBytePosition + 1);
				panel.firstByteInSelection = panel.cursorBytePosition;
				panel.lastByteInSelection = panel.cursorBytePosition;
				panel.mouseByteDragStart = panel.cursorBytePosition;
			}
			else if (Input::keyRepeatedOrDown(GLFW_KEY_LEFT))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				setCursorBytePosition(panel, panel.cursorBytePosition - 1);
				panel.firstByteInSelection = panel.cursorBytePosition;
				panel.lastByteInSelection = panel.cursorBytePosition;
				panel.mouseByteDragStart = panel.cursorBytePosition;
			}
			else if (Input::keyRepeatedOrDown(GLFW_KEY_UP))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				// Try to go up a line while maintaining the same distance from the beginning of the line
				int32 beginningOfCurrentLine = getBeginningOfLineFrom(panel, panel.cursorBytePosition);
				int32 beginningOfLineAbove = getBeginningOfLineFrom(panel, beginningOfCurrentLine - 1);

				// Only set the cursor to a new position if there is another line above the current line
				if (beginningOfLineAbove != -1)
				{
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

					panel.cursorBytePosition = newPos;
					panel.firstByteInSelection = newPos;
					panel.lastByteInSelection = newPos;
					panel.mouseByteDragStart = newPos;
				}
			}
			else if (Input::keyRepeatedOrDown(GLFW_KEY_DOWN))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				// Try to go up a line while maintaining the same distance from the beginning of the line
				int32 endOfCurrentLine = getEndOfLineFrom(panel, panel.cursorBytePosition);
				int32 beginningOfLineBelow = endOfCurrentLine + 1;

				// Only set the cursor to a new position if there is another line below the current line
				if (beginningOfLineBelow < panel.visibleCharacterBufferSize)
				{
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

					panel.cursorBytePosition = newPos;
					panel.firstByteInSelection = newPos;
					panel.lastByteInSelection = newPos;
					panel.mouseByteDragStart = newPos;
				}
			}

			if (Input::keyRepeatedOrDown(GLFW_KEY_RIGHT, KeyMods::Shift))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				int32 oldBytePos = panel.cursorBytePosition;
				setCursorBytePosition(panel, panel.cursorBytePosition + 1);

				if (oldBytePos >= panel.mouseByteDragStart)
				{
					panel.lastByteInSelection = panel.cursorBytePosition;
				}
				else
				{
					panel.firstByteInSelection = panel.cursorBytePosition;
				}
			}

			if (Input::keyRepeatedOrDown(GLFW_KEY_LEFT, KeyMods::Shift))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				int32 oldBytePos = panel.cursorBytePosition;
				setCursorBytePosition(panel, panel.cursorBytePosition - 1);

				if (oldBytePos > panel.mouseByteDragStart)
				{
					panel.lastByteInSelection = panel.cursorBytePosition;
				}
				else
				{
					panel.firstByteInSelection = panel.cursorBytePosition;
				}
			}

			// Handle backspace
			// TODO: Not all backspaces are handled for some reason
			if (Input::keyRepeatedOrDown(GLFW_KEY_BACKSPACE))
			{
				if (panel.cursorBytePosition != 0 || panel.firstByteInSelection != panel.lastByteInSelection)
				{
					size_t numBytesToRemove = panel.lastByteInSelection - panel.firstByteInSelection;
					size_t bytesToRemoveOffset = panel.firstByteInSelection;

					if (panel.firstByteInSelection == panel.lastByteInSelection)
					{
						// TODO: Find out how many bytes are behind the cursor
						numBytesToRemove = 1;
						bytesToRemoveOffset -= numBytesToRemove;
					}

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

					if (panel.lastByteInSelection == panel.firstByteInSelection || panel.cursorBytePosition == panel.lastByteInSelection)
					{
						setCursorBytePosition(panel, panel.cursorBytePosition - (int32)numBytesToRemove);
					}

					panel.mouseByteDragStart = panel.cursorBytePosition;
					panel.firstByteInSelection = panel.cursorBytePosition;
					panel.lastByteInSelection = panel.cursorBytePosition;

					fileHasBeenEdited = true;
				}
			}

			// Handle text-insertion
			if (uint32 codepoint = Input::getLastCharacterTyped(); codepoint != 0)
			{
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

			// ---- Handle Rendering/mouse clicking ----
			panel.drawStart = ImGui::GetCursorScreenPos();
			panel.drawEnd = panel.drawStart + ImGui::GetContentRegionAvail();

			ImGuiIO& io = ImGui::GetIO();
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

				bool isLastCharacter = cursor == panel.visibleCharacterBufferSize - 1;
				if (cursor == panel.cursorBytePosition)
				{
					ImVec2 textCursorDrawPosition = letterBoundsStart;
					renderTextCursor(panel, textCursorDrawPosition, codeFont);
				}
				else if (isLastCharacter && panel.cursorBytePosition == panel.visibleCharacterBufferSize)
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

				// If the mouse was clicked before any character, the closest byte 
				// is the first byte
				if (!passedFirstCharacter && (
					io.MousePos.y < letterBoundsStart.y || (
					io.MousePos.x <= letterBoundsStart.x && io.MousePos.y >= letterBoundsStart.y
					&& io.MousePos.y <= letterBoundsStart.y + letterBoundsSize.y
					)))
				{
					closestByteToMouseCursor = (int32)cursor;

				}
				// If the mouse clicked in between a line, then the closest byte is in 
				// this line
				else if (io.MousePos.y >= letterBoundsStart.y &&
					io.MousePos.y <= letterBoundsStart.y + letterBoundsSize.y)
				{
					// If we clicked in a letter bounds, then that's the closest byte to the mouse
					if (io.MousePos.x >= letterBoundsStart.x && io.MousePos.x < letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = (int32)cursor;
					}
					// If we clicked past the end of the line, the closest byte is the end of the line
					else if ((currentCodepoint == '\n' || cursor == panel.visibleCharacterBufferSize - 1)
						&& io.MousePos.x >= letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = (int32)cursor;
					}
					// If we clicked before any letters in the start of the line, the closest byte
					// is the first byte in this line
					else if (io.MousePos.x < letterBoundsStart.x && lastCharWasNewline)
					{
						closestByteToMouseCursor = (int32)cursor;
					}
				}
				// If the mouse clicked past the last visible character, then the closest
				// character is the last character on the screen
				else if (cursor == panel.visibleCharacterBufferSize - 1 &&
					io.MousePos.y >= letterBoundsStart.y + letterBoundsSize.y)
				{
					closestByteToMouseCursor = (int32)cursor;
				}

				if (panel.firstByteInSelection != panel.lastByteInSelection
					&& cursor >= panel.firstByteInSelection && cursor <= panel.lastByteInSelection)
				{
					if (!lineHighlightStartFound)
					{
						textHighlightRectStart = letterBoundsStart;
						lineHighlightStartFound = true;
					}

					if (cursor == panel.lastByteInSelection - 1)
					{
						// Draw highlight to last character selected
						ImVec2 highlightEnd = letterBoundsStart + letterBoundsSize;
						//drawList->AddRectFilled(textHighlightRectStart, highlightEnd, highlightTextColor);
						drawList->AddRectFilled(textHighlightRectStart, highlightEnd, ImColor(240, 123, 112, 128));
					}
					else if (currentCodepoint == (uint32)'\n')
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
				}

				if (io.MousePos.y > letterBoundsStart.y + letterBoundsSize.y && cursor == panel.visibleCharacterBufferSize - 1)
				{
					closestByteToMouseCursor = (int32)cursor;
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

			if (io.MouseDown[ImGuiMouseButton_Left])
			{
				panel.cursorBytePosition = closestByteToMouseCursor;
				setCursorDistanceFromLineStart(panel);
			}

			if (mouseInTextEditArea(panel))
			{
				const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				if (mouseClicked && !panel.mouseIsDragSelecting)
				{
					panel.mouseByteDragStart = (int32)panel.cursorBytePosition;
					panel.firstByteInSelection = (int32)panel.cursorBytePosition;
					panel.lastByteInSelection = (int32)panel.cursorBytePosition;
				}
			}

			{
				ImGui::Begin("Stats##Panel1");
				ImGui::Text("Selection Begin: %d", panel.firstByteInSelection);
				ImGui::Text("  Selection End: %d", panel.lastByteInSelection);
				ImGui::Text("     Drag Start: %d", panel.mouseByteDragStart);
				ImGui::Text("    Cursor Byte: %d", panel.cursorBytePosition);
				ImGui::Text("Line start dist: %d", panel.cursorDistanceToBeginningOfLine);
				char character = (char)panel.byteMap[panel.visibleCharacterBuffer[panel.cursorBytePosition]];
				if (character != '\r' && character != '\n' && character != '\t')
				{
					ImGui::Text("      Character: %c", character);
				}
				else
				{
					char escapedCharacter = character == '\r' ? 'r' :
						character == '\n' ? 'n' : 't';
					ImGui::Text("      Character: \\%c", escapedCharacter);
				}
				ImGui::End();
			}

			return fileHasBeenEdited;
		}

		// ------------- Internal Functions -------------
		static void renderTextCursor(CodeEditorPanelData& panel, ImVec2 const& drawPosition, SizedFont const* const font)
		{
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
	}
}