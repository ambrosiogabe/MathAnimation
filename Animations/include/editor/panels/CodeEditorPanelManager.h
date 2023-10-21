#include "core.h"

#include "parsers/SyntaxHighlighter.h"

namespace MathAnim
{
	struct AnimationManagerData;
	struct SizedFont;
	class SyntaxHighlighter;
	struct SyntaxTheme;

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

		SyntaxHighlighter const& getHighlighter();
		SyntaxTheme const& getTheme();

		void setTheme(HighlighterTheme theme);
	}
}