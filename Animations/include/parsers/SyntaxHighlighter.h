#ifndef MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#define MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#include "core.h"
#include "parsers/Common.h"

namespace MathAnim
{
	struct SyntaxTheme;
	struct Grammar;

	enum class HighlighterLanguage : uint8
	{
		None,
		Cpp,
		Glsl,
		Javascript,
		Length
	};

	constexpr auto _highlighterLanguageNames = fixedSizeArray<const char*, (size_t)HighlighterLanguage::Length>(
		"None",
		"C++",
		"Glsl",
		"JavaScript"
	);

	constexpr auto _highlighterLanguageFilenames = fixedSizeArray<const char*, (size_t)HighlighterLanguage::Length>(
		"None",
		"assets/grammars/cpp.tmLanguage.json",
		"assets/grammars/glsl.tmLanguage.json",
		"assets/grammars/javascript.json"
		);

	enum class HighlighterTheme : uint8
	{
		None,
		Gruvbox,
		MonokaiNight, 
		OneDark,
		OneMonokai,
		Palenight,
		Panda,
		Length
	};

	constexpr auto _highlighterThemeNames = fixedSizeArray<const char*, (size_t)HighlighterTheme::Length>(
		"None",
		"Gruvbox",
		"Monokai Night",
		"Atom One Dark",
		"One Monokai",
		"Palenight",
		"Panda"
	);

	constexpr auto _highlighterThemeFilenames = fixedSizeArray<const char*, (size_t)HighlighterTheme::Length>(
		"None",
		"assets/themes/gruvbox-dark-soft.json",
		"assets/themes/default.json",
		"assets/themes/oneDark.json",
		"assets/themes/OneMonokai-color-theme.json",
		"assets/themes/palenight.json",
		"assets/themes/Panda.json"
		);

	// Styles a text segment from
	//   startPos <= text < endPos
	struct HighlightSegment
	{
		size_t startPos;
		size_t endPos;
		Vec4 color;
	};

	struct CodeHighlights
	{
		std::vector<HighlightSegment> segments;
		std::string codeBlock;
	};

	class SyntaxHighlighter
	{
	public:
		SyntaxHighlighter(const std::filesystem::path& grammar);

		std::vector<ScopedName> getAncestorsFor(const std::string& code, size_t cursorPos) const;

		CodeHighlights parse(const std::string& code, const SyntaxTheme& theme, bool printDebugInfo = false) const;

		std::string getStringifiedParseTreeFor(const std::string& code) const;

		void free();

	private:
		Grammar* grammar;
	};

	namespace Highlighters
	{
		void init();

		const SyntaxHighlighter* getHighlighter(HighlighterLanguage language);
		const SyntaxTheme* getTheme(HighlighterTheme theme);

		void free();
	}
}

#endif 