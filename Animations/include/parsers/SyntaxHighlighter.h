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

		CodeHighlightDebugInfo getAncestorsFor(SyntaxTheme const* theme, CodeHighlights const& highlights, size_t cursorPos) const;

		CodeHighlights parse(const char* code, size_t codeLength, const SyntaxTheme& theme) const;

		/**
		* @brief This function checksand updates any lines starting from `lineToCheckFrom`and ending at
		*        `lineToCheckFrom + maxLinesToUpdate`. It will resume updates at the last line if it exited early and
		*         you run this function again with `lineToCheckFrom = lineToCheckFrom + maxLinesToUpdate`.
		* 
		* @param highlights The highlights object that will be modified if any updates are found
		* @param lineToCheckFrom The first line that this method will begin checking from
		* @param maxLinesToUpdate The maximum number of lines that can be updated before this function exits early
		* @returns A span that indicates the first line updated and the last line updated
		*/
		Vec2i checkForUpdatesFrom(CodeHighlights& highlights, size_t lineToCheckFrom, size_t maxLinesToUpdate = DEFAULT_MAX_LINES_TO_UPDATE) const;

		/**
		* @brief This function should be run if you've inserted text and would like to modify the current `highlights` object. It will
		*        check to see if there's been any changes between [insertStart, insertEnd) and update any lines as applicable. If the
		*        changes exceed the range of [insertStart, insertEnd), then it will continue to update until it hits `maxLinesToUpdate`,
		*        at which point you can resume parsing using `checkForUpdatesFrom` at a later time.
		* 
		* @param highlights The highlights object that will be modified if any updates are found
		* @param newCodeBlock A stable pointer representing the new code block with the insertion already in the text
		* @param newCodeBlockLength Length of the new code block
		* @param insertStart Where the insertion started
		* @param insertEnd Where the insertion ended. NOTE: This is NOT inclusive of insertEnd.
		* @param maxLinesToUpdate The maximum number of lines that can be updated before this function exits early
		* @returns A span that indicates the first line updated and the last line updated
		*/
		Vec2i insertText(CodeHighlights& highlights, const char* newCodeBlock, size_t newCodeBlockLength, size_t insertStart, size_t insertEnd, size_t maxLinesToUpdate = DEFAULT_MAX_LINES_TO_UPDATE) const;

		/**
		* @brief This function will update the `highlights` object passed in and check the lines surrounding the area removed.
		*        It will update the lines around it if needed, and continue updating as far as needed. If it reaches the
		*        `maxNumLinesToUpdate` then it will exit early and you can resume parsing using `checkForUpdatesFrom` at a later time.
		* 
		* @param highlights The highlights object that will be modified if any updates are found
		* @param newCodeBlock A stable pointer representing the new code block with the text already removed
		* @param newCodeBlockLength Length of the new code block
		* @param removeStart Where the removal started
		* @param removeEnd Where the removal ended
		* @param maxLinesToUpdate The maximum number of lines that can be updated before this function exits early
		* @returns A span that indicates the first line updated and the last line updated
		*/
		Vec2i removeText(CodeHighlights& highlights, const char* newCodeBlock, size_t newCodeBlockLength, size_t removeStart, size_t removeEnd, size_t maxLinesToUpdate = DEFAULT_MAX_LINES_TO_UPDATE) const;

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