#ifndef MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#define MATH_ANIM_SYNTAX_HIGHLIGHTER_H
#include "core.h"
#include "parsers/Common.h"
#include "parsers/Grammar.h"
#include "parsers/SyntaxTheme.h"

namespace MathAnim
{
	enum class HighlighterLanguage : uint8
	{
		None,
		Cpp,
		Glsl,
		Javascript,
		Custom,
		Length
	};

	constexpr auto _highlighterLanguageNames = fixedSizeArray<const char*, (size_t)HighlighterLanguage::Length>(
		"None",
		"C++",
		"Glsl",
		"JavaScript",
		"Undefined"
	);

	constexpr auto _highlighterLanguageFilenames = fixedSizeArray<const char*, (size_t)HighlighterLanguage::Length>(
		"None",
		"assets/grammars/cpp.tmLanguage.json",
		"assets/grammars/glsl.tmLanguage.json",
		"assets/grammars/javascript.json",
		"Undefined"
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

	struct CodeHighlightIter
	{
		std::vector<GrammarLineInfo>::iterator currentLineIter;
		std::vector<SourceSyntaxToken>::iterator currentTokenIter;
		SourceGrammarTree const* tree;

		CodeHighlightIter& next(size_t bytePos);

		inline Vec4 const& getForegroundColor(SyntaxTheme const& theme) 
		{
			if (this->currentLineIter == this->tree->sourceInfo.end())
			{
				return theme.getColor(theme.defaultForeground);
			}

			if (this->currentTokenIter == this->currentLineIter->tokens.end()) 
			{
				return theme.getColor(theme.defaultForeground);
			}

			return theme.getColor(this->currentTokenIter->style.getForegroundColor());
		}

		inline Vec4 const& getBackgroundColor(SyntaxTheme const& theme)
		{
			if (this->currentLineIter == this->tree->sourceInfo.end())
			{
				return theme.getColor(theme.defaultBackground);
			}

			if (this->currentTokenIter == this->currentLineIter->tokens.end())
			{
				return theme.getColor(theme.defaultBackground);
			}

			return theme.getColor(this->currentTokenIter->style.getBackgroundColor());
		}

		inline CssFontStyle getFontStyle()
		{
			if (this->currentLineIter == this->tree->sourceInfo.end())
			{
				return CssFontStyle::Normal;
			}

			if (this->currentTokenIter == this->currentLineIter->tokens.end())
			{
				return CssFontStyle::Normal;
			}

			return this->currentTokenIter->style.getFontStyle();
		}

		inline bool operator==(CodeHighlightIter const& other)
		{
			return this->currentLineIter == other.currentLineIter &&
				this->currentTokenIter == other.currentTokenIter;
		}

		inline bool operator!=(CodeHighlightIter const& other)
		{
			return !(*this == other);
		}
	};

	struct CodeHighlights
	{
		SyntaxTheme const* theme;
		SourceGrammarTree tree;
		std::string codeBlock;

		CodeHighlightIter begin(size_t bytePos = 0);
		CodeHighlightIter end();
	};

	struct CodeHighlightDebugInfo
	{
		std::vector<ScopedName> ancestors;
		DebugPackedSyntaxStyle settings;
		std::string matchText;
		bool usingDefaultSettings;
	};

	class SyntaxHighlighter
	{
	public:
		SyntaxHighlighter(const std::filesystem::path& grammar);

		CodeHighlightDebugInfo getAncestorsFor(SyntaxTheme const* theme, const std::string& code, size_t cursorPos) const;

		CodeHighlights parse(const std::string& code, const SyntaxTheme& theme, bool printDebugInfo = false) const;
		void reparseSection(CodeHighlights& codeHighlights, const std::string& newCode, size_t parseStart, size_t parseEnd, bool printDebugInfo = false) const;

		std::string getStringifiedParseTreeFor(const std::string& code, SyntaxTheme const& theme) const;

		void free();

	private:
		Grammar* grammar;
	};

	namespace Highlighters
	{
		void init();

		void importGrammar(const char* filename);
		const SyntaxHighlighter* getImportedHighlighter(const char* filename);

		const SyntaxHighlighter* getHighlighter(HighlighterLanguage language);
		const SyntaxTheme* getTheme(HighlighterTheme theme);

		void free();
	}
}

#endif 