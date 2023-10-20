#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;
	struct SizedFont;

	namespace CodeEditorPanelManager
	{
		void init();
		void free();

		void update(AnimationManagerData const* am, ImGuiID parentDockId);

		void openFile(std::string const& filename, uint32 lineNumber);
		void openFile(std::string const& filename);
		void closeFile(std::string const& filename);

		SizedFont const* const getCodeFont();

		int getCodeFontSizePx();
		void setCodeFontSizePx(int fontSizePx);

		uint8 addCharToFont(uint32 codepoint);

		void imguiStats();
	}
}