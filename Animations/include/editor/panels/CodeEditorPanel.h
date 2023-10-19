#include "core.h"

#include <cppUtils/cppStrings.hpp>

namespace MathAnim
{
	struct TextEditorUndoSystem;

	struct CodeEditorPanelData
	{
		TextEditorUndoSystem* undoSystem;
		std::filesystem::path filepath;

		uint32 lineNumberStart;
		uint32 lineNumberByteStart;
		ImVec2 drawStart;
		ImVec2 drawEnd;

		bool mouseIsDragSelecting;
		// The byte that was clicked when the user began clicking and dragging the mouse
		int32 mouseByteDragStart;
		int32 firstByteInSelection;
		int32 lastByteInSelection;
		int32 cursorBytePosition;
		int32 cursorDistanceToBeginningOfLine;
		float timeSinceCursorLastBlinked;
		bool cursorIsBlinkedOn;

		uint8* visibleCharacterBuffer;
		size_t visibleCharacterBufferSize;

		// A map that contains this files byte->codepoint mapping
		uint32 byteMap[1 << 8];
	};

	namespace CodeEditorPanel
	{
		CodeEditorPanelData* openFile(std::string const& filepath);
		void saveFile(CodeEditorPanelData const& panel);
		void free(CodeEditorPanelData* panel);

		bool update(CodeEditorPanelData& panel);

		void addUtf8StringToBuffer(CodeEditorPanelData& panel, uint8* utf8String, size_t stringNumBytes, size_t insertPosition);
		void addCodepointToBuffer(CodeEditorPanelData& panel, uint32 codepoint, size_t insertPosition);
	}
}