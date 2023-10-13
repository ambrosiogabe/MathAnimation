#include "core.h"

#include <cppUtils/cppStrings.hpp>

namespace MathAnim
{
	struct CharInfo
	{
		size_t byteIndex;
		uint8 numBytes;
	};

	struct CodeEditorPanelData
	{
		std::filesystem::path filepath;

		uint32 lineNumberStart;
		uint32 lineNumberByteStart;
		ImVec2 drawStart;
		ImVec2 drawEnd;

		bool mouseIsDragSelecting;
		// The byte that was clicked when the user began clicking and dragging the mouse
		size_t mouseByteDragStart;
		size_t firstByteInSelection;
		size_t lastByteInSelection;
		CharInfo cursorBytePosition;

		CppUtils::ParseInfo visibleCharacterBuffer;
	};

	namespace CodeEditorPanel
	{
		CodeEditorPanelData openFile(std::string const& filepath);
		void free(CodeEditorPanelData& panel);

		void update(CodeEditorPanelData& panel);
	}
}