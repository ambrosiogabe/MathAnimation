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

		// Simple calculation functions
		static inline ImVec2 getTopLeftOfLine(CodeEditorPanelData const& panel, uint32 lineNumber, SizedFont const* const font)
		{
			const float fontHeight = font->unsizedFont->lineHeight * font->fontSizePixels;
			return panel.drawStart
				+ ImVec2(0.0f, fontHeight * (lineNumber - panel.lineNumberStart));
		}

		static ImVec2 addStringToDrawList(ImDrawList* drawList, SizedFont const* const font, std::string const& str, ImVec2 const& drawPos);
		static ImVec2 addCharToDrawList(ImDrawList* drawList, SizedFont const* const font, uint32 charToAdd, ImVec2 const& drawPos);

		static inline bool mouseIntersectsRect(ImVec2 rectStart, ImVec2 rectEnd)
		{
			ImGuiIO& io = ImGui::GetIO();
			return io.MousePos.x >= rectStart.x && io.MousePos.x <= rectEnd.x &&
				io.MousePos.y >= rectStart.y && io.MousePos.y <= rectEnd.y;
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

			// Take ownership of the data here
			auto parseInfo = Parser::makeParseInfo((char*)memory.data, fileSize);
			if (!parseInfo.hasValue())
			{
				g_logger_error("Could not open file '{}'. Had error '{}'.", filepath, parseInfo.error());
				return {};
			}

			res.visibleCharacterBuffer = parseInfo.value();
			res.lineNumberStart = 1;
			res.lineNumberByteStart = 0;

			return res;
		}

		void free(CodeEditorPanelData& panel)
		{
			g_memory_free((void*)panel.visibleCharacterBuffer.utf8String);
			panel = {};
		}

		void update(CodeEditorPanelData& panel)
		{
			SizedFont const* const codeFont = CodeEditorPanelManager::getCodeFont();
			panel.timeSinceCursorLastBlinked += Application::getDeltaTime();

			// ---- Handle arrow key presses ----
			if (Input::keyRepeatedOrDown(GLFW_KEY_RIGHT))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				panel.cursorBytePosition.byteIndex++;
				panel.firstByteInSelection = panel.cursorBytePosition.byteIndex;
				panel.lastByteInSelection = panel.cursorBytePosition.byteIndex;
				panel.mouseByteDragStart = panel.cursorBytePosition.byteIndex;
			}

			if (Input::keyRepeatedOrDown(GLFW_KEY_LEFT))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				panel.cursorBytePosition.byteIndex--;
				panel.firstByteInSelection = panel.cursorBytePosition.byteIndex;
				panel.lastByteInSelection = panel.cursorBytePosition.byteIndex;
				panel.mouseByteDragStart = panel.cursorBytePosition.byteIndex;
			}

			if (Input::keyRepeatedOrDown(GLFW_KEY_RIGHT, KeyMods::Shift))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				if (panel.cursorBytePosition.byteIndex != panel.lastByteInSelection)
				{
					panel.cursorBytePosition.byteIndex++;
				}

				if (panel.cursorBytePosition.byteIndex >= panel.mouseByteDragStart)
				{
					panel.lastByteInSelection = panel.cursorBytePosition.byteIndex + 1;
				}
				else
				{
					panel.firstByteInSelection++;
				}
			}

			if (Input::keyRepeatedOrDown(GLFW_KEY_LEFT, KeyMods::Shift))
			{
				panel.cursorIsBlinkedOn = true;
				panel.timeSinceCursorLastBlinked = 0.0f;

				panel.cursorBytePosition.byteIndex--;
				if (panel.cursorBytePosition.byteIndex > panel.mouseByteDragStart)
				{
					panel.lastByteInSelection--;
				}
				else
				{
					panel.firstByteInSelection--;
				}
			}

			// ---- Handle Rendering/mouse clicking ----
			panel.drawStart = ImGui::GetCursorScreenPos();
			panel.drawEnd = panel.drawStart + ImGui::GetContentRegionAvail();

			ImGuiIO& io = ImGui::GetIO();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(panel.drawStart, panel.drawEnd, ImColor(44, 44, 44));

			uint32 oldLineByteStart = panel.lineNumberByteStart;
			uint32 currentLine = panel.lineNumberStart;
			ImVec2 currentLetterDrawPos = renderNextLinePrefix(panel, currentLine, codeFont);

			ParseInfo& parser = panel.visibleCharacterBuffer;

			bool justStartedDragSelecting = false;
			if (mouseInTextEditArea(panel))
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
				const bool isDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left, io.MouseDragThreshold * 1.70f);
				const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				if (!panel.mouseIsDragSelecting && isDragging)
				{
					panel.mouseIsDragSelecting = true;
					justStartedDragSelecting = true;
					panel.mouseByteDragStart = 0;
					panel.firstByteInSelection = 0;
					panel.lastByteInSelection = 0;
				}

				if (mouseClicked && !panel.mouseIsDragSelecting)
				{
					panel.mouseByteDragStart = panel.cursorBytePosition.byteIndex;
					panel.firstByteInSelection = panel.cursorBytePosition.byteIndex;
					panel.lastByteInSelection = panel.cursorBytePosition.byteIndex;
				}
			}

			if (panel.mouseIsDragSelecting && !io.MouseDown[ImGuiMouseButton_Left])
			{
				panel.mouseIsDragSelecting = false;
			}

			bool lineHighlightStartFound = false;
			bool lastCharWasNewline = false;
			bool passedFirstCharacter = false;
			uint8 lastNumBytesParsed = 1;
			ImVec2 textHighlightRectStart = ImVec2();
			CharInfo closestByteToMouseCursor = { panel.lineNumberByteStart, 1 };

			while (parser.cursor < parser.numBytes)
			{
				uint8 numBytesParsed = 0;
				auto maybeCurrentCodepoint = Parser::parseCharacter(parser, &numBytesParsed);
				if (!maybeCurrentCodepoint.hasValue())
				{
					parser.cursor++;
					g_logger_warning("Malformed unicode encountered while rendering code editor.");
					continue;
				}

				uint32 currentCodepoint = maybeCurrentCodepoint.value();
				ImVec2 letterBoundsStart = currentLetterDrawPos;
				ImVec2 letterBoundsSize = addCharToDrawList(
					drawList,
					codeFont,
					currentCodepoint,
					letterBoundsStart
				);

				if (parser.cursor == panel.cursorBytePosition.byteIndex)
				{
					ImVec2 textCursorDrawPosition = letterBoundsStart;

					bool selectionIsLeading = panel.firstByteInSelection < panel.mouseByteDragStart || panel.firstByteInSelection == panel.lastByteInSelection;
					if (!selectionIsLeading)
					{
						textCursorDrawPosition = letterBoundsStart + ImVec2(letterBoundsSize.x, 0.0f);
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
					closestByteToMouseCursor.byteIndex = parser.cursor;
					closestByteToMouseCursor.numBytes = numBytesParsed;
				}
				// If the mouse clicked in between a line, then the closest byte is in 
				// this line
				else if (io.MousePos.y >= letterBoundsStart.y &&
					io.MousePos.y <= letterBoundsStart.y + letterBoundsSize.y)
				{
					// If we clicked in a letter bounds, then that's the closest byte to the mouse
					if (io.MousePos.x >= letterBoundsStart.x && io.MousePos.x < letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor.byteIndex = parser.cursor;
						closestByteToMouseCursor.numBytes = numBytesParsed;
					}
					// If we clicked past the end of the line, the closest byte is the end of the line
					else if ((currentCodepoint == '\n' || parser.cursor == parser.numBytes - 1)
						&& io.MousePos.x >= letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor.byteIndex = parser.cursor;
						closestByteToMouseCursor.numBytes = numBytesParsed;
					}
					// If we clicked before any letters in the start of the line, the closest byte
					// is the first byte in this line
					else if (io.MousePos.x < letterBoundsStart.x && lastCharWasNewline)
					{
						closestByteToMouseCursor.byteIndex = parser.cursor;
						closestByteToMouseCursor.numBytes = numBytesParsed;
					}
				}
				// If the mouse clicked past the last visible character, then the closest
				// character is the last character on the screen
				else if (parser.cursor == parser.numBytes - numBytesParsed &&
					io.MousePos.y >= letterBoundsStart.y + letterBoundsSize.y)
				{
					closestByteToMouseCursor.byteIndex = parser.cursor;
					closestByteToMouseCursor.numBytes = numBytesParsed;
				}

				if (panel.firstByteInSelection != panel.lastByteInSelection
					&& parser.cursor >= panel.firstByteInSelection && parser.cursor <= panel.lastByteInSelection)
				{
					if (!lineHighlightStartFound)
					{
						textHighlightRectStart = letterBoundsStart;
						lineHighlightStartFound = true;
					}

					if (parser.cursor == panel.lastByteInSelection - numBytesParsed)
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
							panel.drawStart.x + leftGutterWidth,
							letterBoundsStart.y + letterBoundsSize.y
						);
					}
				}

				if (io.MousePos.y > letterBoundsStart.y + letterBoundsSize.y &&
					parser.cursor == parser.numBytes - numBytesParsed)
				{
					closestByteToMouseCursor.byteIndex = parser.cursor;
					closestByteToMouseCursor.numBytes = numBytesParsed;
				}

				currentLetterDrawPos.x += letterBoundsSize.x;
				parser.cursor += numBytesParsed;
				lastCharWasNewline = currentCodepoint == (uint32)'\n';
				lastNumBytesParsed = numBytesParsed;
				passedFirstCharacter = true;
			}

			if (panel.mouseIsDragSelecting)
			{
				if (justStartedDragSelecting)
				{
					panel.mouseByteDragStart = closestByteToMouseCursor.byteIndex;
					panel.firstByteInSelection = closestByteToMouseCursor.byteIndex;
					panel.lastByteInSelection = closestByteToMouseCursor.byteIndex;
				}

				if (closestByteToMouseCursor.byteIndex >= panel.lastByteInSelection)
				{
					panel.lastByteInSelection = closestByteToMouseCursor.byteIndex + closestByteToMouseCursor.numBytes;
					panel.firstByteInSelection = panel.mouseByteDragStart;
				}
				else if (closestByteToMouseCursor.byteIndex <= panel.lastByteInSelection - closestByteToMouseCursor.numBytes &&
					closestByteToMouseCursor.byteIndex > panel.mouseByteDragStart)
				{
					panel.lastByteInSelection = closestByteToMouseCursor.byteIndex + closestByteToMouseCursor.numBytes;
					panel.firstByteInSelection = panel.mouseByteDragStart;
				}

				if (closestByteToMouseCursor.byteIndex < panel.firstByteInSelection)
				{
					panel.firstByteInSelection = closestByteToMouseCursor.byteIndex;
					panel.lastByteInSelection = panel.mouseByteDragStart;
				}
				else if (closestByteToMouseCursor.byteIndex > panel.firstByteInSelection &&
					closestByteToMouseCursor.byteIndex < panel.mouseByteDragStart)
				{
					panel.firstByteInSelection = closestByteToMouseCursor.byteIndex;
					panel.lastByteInSelection = panel.mouseByteDragStart;
				}
			}

			parser.cursor = oldLineByteStart;

			if (io.MouseDown[ImGuiMouseButton_Left])
			{
				panel.cursorBytePosition = closestByteToMouseCursor;
			}

			ImGui::Begin("Stats##Panel1");
			ImGui::Text("Selection Begin: %d", panel.firstByteInSelection);
			ImGui::Text("  Selection End: %d", panel.lastByteInSelection);
			ImGui::Text("     Drag Start: %d", panel.mouseByteDragStart);
			ImGui::Text("    Cursor Byte: %d", panel.cursorBytePosition.byteIndex);
			ImGui::End();
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
	}
}