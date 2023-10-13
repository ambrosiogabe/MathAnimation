#include "editor/panels/CodeEditorPanel.h"
#include "editor/imgui/ImGuiLayer.h"
#include "platform/Platform.h"
#include "core/Input.h"

using namespace CppUtils;

namespace MathAnim
{
	namespace CodeEditorPanel
	{
		// ------------- Internal Functions -------------
		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber);
		static bool mouseInTextEditArea(CodeEditorPanelData const& panel);

		// Simple calculation functions
		static inline ImVec2 getTopLeftOfLine(CodeEditorPanelData const& panel, uint32 lineNumber)
		{
			return panel.drawStart
				+ ImVec2(0.0f, ImGuiLayer::getMonoFont()->FontSize * (lineNumber - panel.lineNumberStart));
		}

		static inline bool mouseIntersectsRect(ImVec2 rectStart, ImVec2 rectEnd)
		{
			ImGuiIO& io = ImGui::GetIO();
			return io.MousePos.x >= rectStart.x && io.MousePos.x <= rectEnd.x &&
				io.MousePos.y >= rectStart.y && io.MousePos.y <= rectEnd.y;
		}

		// ------------- Internal Data -------------
		static float leftGutterWidth = 60;
		static float scrollBarWidth = 10;
		static ImColor lineNumberColor = ImColor(255, 255, 255);
		static ImColor highlightTextColor = ImColor(115, 163, 240, 128);

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
			panel.drawStart = ImGui::GetCursorScreenPos();
			panel.drawEnd = panel.drawStart + ImGui::GetContentRegionAvail();

			ImGuiIO& io = ImGui::GetIO();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(panel.drawStart, panel.drawEnd, ImColor(44, 44, 44));

			uint32 oldLineByteStart = panel.lineNumberByteStart;
			uint32 currentLine = panel.lineNumberStart;
			ImVec2 currentLetterDrawPos = renderNextLinePrefix(panel, currentLine);

			ParseInfo& parser = panel.visibleCharacterBuffer;

			bool justStartedDragSelecting = false;
			if (mouseInTextEditArea(panel))
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
				if (!panel.mouseIsDragSelecting && io.MouseDown[ImGuiMouseButton_Left])
				{
					panel.mouseIsDragSelecting = true;
					justStartedDragSelecting = true;
					panel.mouseByteDragStart = 0;
					panel.firstByteInSelection = 0;
					panel.lastByteInSelection = 0;
				}
				else if (panel.mouseIsDragSelecting && !io.MouseDown[ImGuiMouseButton_Left])
				{
					panel.mouseIsDragSelecting = false;
				}
			}

			bool lineHighlightStartFound = false;
			bool lastCharWasNewline = false;
			uint8 lastNumBytesParsed = 1;
			ImVec2 textHighlightRectStart = ImVec2();
			size_t closestByteToMouseCursor = panel.lineNumberByteStart;
			uint8 closestByteToMouseNumBytes = 1;

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
				ImVec2 letterBoundsSize = ImGui::CalcTextSize(
					(const char*)parser.utf8String + parser.cursor,
					(const char*)parser.utf8String + parser.cursor + numBytesParsed
				);

				if (currentCodepoint == (uint32)'\n')
				{
					currentLine++;
					currentLetterDrawPos = renderNextLinePrefix(panel, currentLine);
				}
				else
				{
					ImColor textColor = ImColor(255, 255, 255);
					drawList->AddText(
						currentLetterDrawPos,
						textColor,
						(const char*)parser.utf8String + parser.cursor,
						(const char*)parser.utf8String + parser.cursor + numBytesParsed
					);
				}

				ImVec2 mouseToLetterEnd = (io.MousePos - (letterBoundsStart + letterBoundsSize));
				if (io.MousePos.y >= letterBoundsStart.y &&
					io.MousePos.y <= letterBoundsStart.y + letterBoundsSize.y)
				{
					if (io.MousePos.x >= letterBoundsStart.x + letterBoundsSize.x)
					{
						closestByteToMouseCursor = parser.cursor;
						closestByteToMouseNumBytes = numBytesParsed;
					}
					else if (lastCharWasNewline)
					{
						closestByteToMouseCursor = parser.cursor - numBytesParsed;
						closestByteToMouseNumBytes = lastNumBytesParsed;
					}
				}

				if (parser.cursor >= panel.firstByteInSelection && parser.cursor <= panel.lastByteInSelection)
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
					closestByteToMouseCursor = parser.cursor;
					closestByteToMouseNumBytes = numBytesParsed;
				}

				currentLetterDrawPos.x += letterBoundsSize.x;
				parser.cursor += numBytesParsed;
				lastCharWasNewline = currentCodepoint == (uint32)'\n';
				lastNumBytesParsed = numBytesParsed;
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
					panel.lastByteInSelection = closestByteToMouseCursor + closestByteToMouseNumBytes;
					panel.firstByteInSelection = panel.mouseByteDragStart;
				}
				else if (closestByteToMouseCursor <= panel.lastByteInSelection - closestByteToMouseNumBytes &&
					closestByteToMouseCursor > panel.mouseByteDragStart)
				{
					panel.lastByteInSelection = closestByteToMouseCursor + closestByteToMouseNumBytes;
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

			parser.cursor = oldLineByteStart;

			ImGui::Begin("Stats##Panel1");
			ImGui::Text("Selection Begin: %d", panel.firstByteInSelection);
			ImGui::Text("  Selection End: %d", panel.lastByteInSelection);
			ImGui::Text("     Drag Start: %d", panel.mouseByteDragStart);
			ImGui::End();
		}

		// ------------- Internal Functions -------------
		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			std::string numberText = std::to_string(lineNumber);
			ImVec2 textSize = ImGui::CalcTextSize(numberText.c_str());

			ImVec2 lineStart = getTopLeftOfLine(panel, lineNumber);
			ImVec2 numberStart = lineStart + ImVec2(leftGutterWidth - textSize.x, 0.0f);
			drawList->AddText(numberStart, lineNumberColor, numberText.c_str());

			return lineStart + ImVec2(leftGutterWidth + style.FramePadding.x, 0.0f);
		}

		static bool mouseInTextEditArea(CodeEditorPanelData const& panel)
		{
			ImVec2 textAreaStart = panel.drawStart + ImVec2(leftGutterWidth, 0.0f);
			ImVec2 textAreaEnd = panel.drawEnd - ImVec2(scrollBarWidth, 0.0f);
			return mouseIntersectsRect(textAreaStart, textAreaEnd);
		}
	}
}