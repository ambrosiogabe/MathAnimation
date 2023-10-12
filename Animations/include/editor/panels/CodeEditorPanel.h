#include "core.h"

#include <cppUtils/cppStrings.hpp>

namespace MathAnim
{
	struct CodeEditorPanelData
	{
		std::filesystem::path filepath;

		uint32 lineNumberStart;
		uint32 lineNumberByteStart;
		ImVec2 drawStart;
		ImVec2 drawEnd;

		CppUtils::ParseInfo visibleCharacterBuffer;
	};

	namespace CodeEditorPanel
	{
		CodeEditorPanelData openFile(std::string const& filepath);
		void free(CodeEditorPanelData& panel);

		void update(CodeEditorPanelData& panel);
	}
}