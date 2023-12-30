#include "core.h"
#include "parsers/SyntaxHighlighter.h"

#include <cppUtils/cppStrings.hpp>

namespace MathAnim
{
	struct TextEditorUndoSystem;

	struct CodeEditorPanelDebugData
	{
		std::vector<Vec2i> linesUpdated;
		std::vector<std::chrono::steady_clock::time_point> ageOfLinesUpdated;
	};

	struct CodeEditorPanelData
	{
		TextEditorUndoSystem* undoSystem;
		std::filesystem::path filepath;

		uint32 numberLinesCanFitOnScreen;
		uint32 totalNumberLines;
		uint32 lineNumberStart;
		uint32 lineNumberByteStart;
		ImVec2 drawStart;
		ImVec2 drawEnd;
		float leftGutterWidth;

		int32 undoTypingStart;

		bool mouseIsDragSelecting;
		// The byte that was clicked when the user began clicking and dragging the mouse
		int32 mouseByteDragStart;
		int32 firstByteInSelection;
		int32 lastByteInSelection;
		CppUtils::BasicUtf8StringIter cursor;
		uint32 cursorCurrentLine;
		int32 numOfCharsFromBeginningOfLine;
		int32 beginningOfCurrentLineByte;
		float timeSinceCursorLastBlinked;
		bool cursorIsBlinkedOn;

		uint8* visibleCharacterBuffer;
		size_t visibleCharacterBufferSize;

		CodeHighlights syntaxHighlightTree;
		CodeEditorPanelDebugData debugData;
	};

	namespace CodeEditorPanel
	{
		CodeEditorPanelData* openFile(std::string const& filepath);
		void saveFile(CodeEditorPanelData const& panel);
		void free(CodeEditorPanelData* panel);

		void reparseSyntax(CodeEditorPanelData& panel);

		bool update(CodeEditorPanelData& panel);

		void setCursorToLine(CodeEditorPanelData& panel, uint32 lineNumber);

		void addUtf8StringToBuffer(CodeEditorPanelData& panel, uint8* utf8String, size_t stringNumBytes, size_t insertPosition);
		void addCodepointToBuffer(CodeEditorPanelData& panel, uint32 codepoint, size_t insertPosition);

		bool removeTextWithBackspace(CodeEditorPanelData& panel, int32 textToRemoveStart, int32 textToRemoveLength);
		bool removeTextWithDelete(CodeEditorPanelData& panel, int32 textToRemoveStart, int32 textToRemoveLength);

		// Strip any carriage returns and invalid UTF8
		void preprocessText(uint8* utf8String, size_t stringNumBytes, uint8** outStr, size_t* outStrLength, uint32* numberLines = nullptr);

		// Adds carriage returns as necessary
		void postprocessText(uint8* byteMappedString, size_t byteMappedStringNumBytes, uint8** outUtf8String, size_t* outUtf8StringNumBytes, bool includeCarriageReturnsForWindows = true);

		void showInspectorGui(SyntaxTheme const& theme, CodeHighlightDebugInfo const& parseInfo);
	}
}