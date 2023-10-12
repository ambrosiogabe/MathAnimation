#include "editor/panels/CodeEditorPanel.h"
#include "editor/imgui/ImGuiLayer.h"
#include "platform/Platform.h"

using namespace CppUtils;

namespace MathAnim
{
	namespace CodeEditorPanel
	{
		// ------------- Internal Functions -------------
		static ImVec2 renderNextLinePrefix(CodeEditorPanelData& panel, uint32 lineNumber);
		
		// Simple calculation functions
		static inline ImVec2 getTopLeftOfLine(CodeEditorPanelData& panel, uint32 lineNumber)
		{
			return panel.drawStart 
				+ ImVec2(0.0f, ImGuiLayer::getMonoFont()->FontSize * (lineNumber - panel.lineNumberStart));
		}

		// ------------- Internal Data -------------
		static float leftGutterWidth = 60;
		static ImColor lineNumberColor = ImColor(255, 255, 255);

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

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(panel.drawStart, panel.drawEnd, ImColor(44, 44, 44));

			uint32 oldLineByteStart = panel.lineNumberByteStart;
			uint32 currentLine = panel.lineNumberStart;
			ImVec2 currentLetterDrawPos = renderNextLinePrefix(panel, currentLine);

			ParseInfo& parser = panel.visibleCharacterBuffer;

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
				if (currentCodepoint == (uint32)'\n')
				{
					currentLine++;
					currentLetterDrawPos = renderNextLinePrefix(panel, currentLine);
				}

				ImColor textColor = ImColor(255, 255, 255);
				drawList->AddText(
					currentLetterDrawPos,
					textColor,
					(const char*)parser.utf8String + parser.cursor,
					(const char*)parser.utf8String + parser.cursor + numBytesParsed
				);

				currentLetterDrawPos.x += ImGui::CalcTextSize(
					(const char*)parser.utf8String + parser.cursor,
					(const char*)parser.utf8String + parser.cursor + numBytesParsed
				).x;

				parser.cursor += numBytesParsed;
			}

			parser.cursor = oldLineByteStart;
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
	}
}