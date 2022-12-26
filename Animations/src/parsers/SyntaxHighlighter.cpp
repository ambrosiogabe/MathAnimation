#include "parsers/SyntaxHighlighter.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"

namespace MathAnim
{
	// --------------- Internal Functions ---------------
	size_t applyTheme(const GrammarMatch& match, size_t highlightCursor, const SyntaxTheme& theme, CodeHighlights& out, const TokenRule* parentRule);
	static HighlightSegment getSegmentFrom(size_t startIndex, size_t endIndex, const TokenRule& setting);
	static int printMatches(char* bufferPtr, size_t bufferSizeLeft, const GrammarMatch& match, const std::string& code, int level = 1);

	SyntaxHighlighter::SyntaxHighlighter(const std::filesystem::path& grammar)
	{
		this->grammar = Grammar::importGrammar(grammar.string().c_str());
	}

	CodeHighlights SyntaxHighlighter::parse(const std::string& code, const SyntaxTheme& theme, bool printDebugInfo) const
	{
		if (!this->grammar)
		{
			return {};
		}

		std::vector<GrammarMatch> matches = {};
		while (grammar->getNextMatch(code, &matches))
		{
		}

		if (printDebugInfo)
		{
			constexpr size_t bufferSize = 1024 * 10;
			char buffer[bufferSize] = "\0";
			char* bufferPtr = buffer;
			size_t bufferSizeLeft = bufferSize;
			for (auto match : matches)
			{
				int numBytesWritten = printMatches(bufferPtr, bufferSizeLeft, match, code);
				if (numBytesWritten > 0 && numBytesWritten < bufferSizeLeft)
				{
					bufferSizeLeft -= numBytesWritten;
					bufferPtr += numBytesWritten;
				}
				else
				{
					break;
				}
			}

			g_logger_info("Matches:\n%s", buffer);
		}

		CodeHighlights res = {};
		res.codeBlock = code;

		// Attempt to break up the matches into one set of well-defined highlight segments
		size_t highlightCursor = 0;
		for (size_t i = 0; i < matches.size(); i++)
		{
			bool usedDefaultTheme;
			highlightCursor = applyTheme(matches[i], highlightCursor, theme, res, nullptr);
		}

		if (highlightCursor < code.length())
		{
			HighlightSegment finalSegment = getSegmentFrom(highlightCursor, code.length(), theme.defaultRule);
			res.segments.emplace_back(finalSegment);
		}

		return res;
	}

	void SyntaxHighlighter::free()
	{
		if (grammar)
		{
			Grammar::free(grammar);
		}

		grammar = nullptr;
	}

	namespace Highlighters
	{
		static std::unordered_map<HighlighterLanguage, SyntaxHighlighter*> grammars = {};
		static std::unordered_map<HighlighterTheme, SyntaxTheme*> themes = {};

		void init()
		{
			for (int i = 0; i < (int)HighlighterLanguage::Length; i++)
			{
				if (Platform::fileExists(_highlighterLanguageFilenames[i]))
				{
					SyntaxHighlighter* highlighter = (SyntaxHighlighter*)g_memory_allocate(sizeof(SyntaxHighlighter));
					new(highlighter)SyntaxHighlighter(_highlighterLanguageFilenames[i]);
					grammars[(HighlighterLanguage)i] = highlighter;
				}
			}

			for (int i = 0; i < (int)HighlighterTheme::Length; i++)
			{
				if (Platform::fileExists(_highlighterThemeFilenames[i]))
				{
					themes[(HighlighterTheme)i] = SyntaxTheme::importTheme(_highlighterThemeFilenames[i]);
				}
			}

			g_logger_info("Successfully imported default languages and themes for syntax highlighters.");
		}

		const SyntaxHighlighter* getHighlighter(HighlighterLanguage language)
		{
			auto iter = grammars.find(language);
			if (iter != grammars.end())
			{
				return iter->second;
			}

			return nullptr;
		}

		const SyntaxTheme* getTheme(HighlighterTheme theme)
		{
			auto iter = themes.find(theme);
			if (iter != themes.end())
			{
				return iter->second;
			}

			return nullptr;
		}

		void free()
		{
			for (auto [k, v] : grammars)
			{
				v->free();
				v->~SyntaxHighlighter();
				g_memory_free(v);
			}
			grammars.clear();

			for (auto [k, v] : themes)
			{
				SyntaxTheme::free(v);
			}
			themes.clear();
		}
	}

	// --------------- Internal Functions ---------------
	size_t applyTheme(const GrammarMatch& match, size_t highlightCursor, const SyntaxTheme& theme, CodeHighlights& out, const TokenRule* parentRule)
	{
		if (highlightCursor > match.start)
		{
			g_logger_warning("Somehow two matches overlap...");
			// Skip this if the matches intersect
			return highlightCursor;
		}

		const TokenRule& defaultThemeRule = theme.defaultRule;
		if (!parentRule)
		{
			parentRule = &defaultThemeRule;
		}

		if (highlightCursor < match.start)
		{
			// If no match covers this space, then just color this bit of text the parent's color
			HighlightSegment segment = getSegmentFrom(highlightCursor, match.start, *parentRule);
			out.segments.emplace_back(segment);
			highlightCursor = match.start;
		}

		// First get this match's best color and default to parent's theme if this one doesn't
		// have a best match
		const TokenRule* myBestMatch = theme.match(match.scope);
		if (!myBestMatch)
		{
			myBestMatch = parentRule;
		}

		// If this match has children, try to match the children to a style first
		for (auto subMatch : match.subMatches)
		{
			size_t newCursor = applyTheme(subMatch, highlightCursor, theme, out, myBestMatch);
			highlightCursor = newCursor;
		}

		// If the children didn't cover this entire highlight region, then cover the remaining region with the
		// parent's best matched style
		if (highlightCursor < match.end)
		{
			HighlightSegment segment = getSegmentFrom(highlightCursor, match.end, *myBestMatch);
			out.segments.emplace_back(segment);
		}

		return match.end;
	}

	static HighlightSegment getSegmentFrom(size_t startIndex, size_t endIndex, const TokenRule& rule)
	{
		HighlightSegment segment = {};
		segment.startPos = startIndex;
		segment.endPos = endIndex;

		for (const ThemeSetting& setting : rule.settings)
		{
			if (setting.type == ThemeSettingType::ForegroundColor)
			{
				if (setting.foregroundColor.has_value())
				{
					segment.color = *setting.foregroundColor;
				}
				else
				{
					g_logger_warning("Something bad happened here...");
				}
			}
		}

		return segment;
	}

	static int printMatches(char* bufferPtr, size_t bufferSizeLeft, const GrammarMatch& match, const std::string& code, int level)
	{
		int totalNumBytesWritten = 0;
		std::string val = code.substr(match.start, (match.end - match.start)).c_str();
		for (size_t i = 0; i < val.length(); i++)
		{
			if (val[i] == '\n')
			{
				val[i] = '\\';
				val.insert(i + 1, "n");
				i++;
			}
		}

		int numBytesWritten;
		if (level > 1)
		{
			numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "%*c'%s': '%s'\n", level * 2, ' ', match.scope.friendlyName.c_str(), val.c_str());
		}
		else
		{
			numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'%s': '%s'\n", match.scope.friendlyName.c_str(), val.c_str());
		}

		if (numBytesWritten > 0 && numBytesWritten < bufferSizeLeft)
		{
			bufferSizeLeft -= numBytesWritten;
			bufferPtr += numBytesWritten;
		}
		else
		{
			return -1;
		}

		totalNumBytesWritten += numBytesWritten;

		for (auto subMatch : match.subMatches)
		{
			numBytesWritten = printMatches(bufferPtr, bufferSizeLeft, subMatch, code, level + 1);
			totalNumBytesWritten += numBytesWritten;
			if (numBytesWritten > 0 && numBytesWritten < bufferSizeLeft)
			{
				bufferSizeLeft -= numBytesWritten;
				bufferPtr += numBytesWritten;
			}
			else
			{
				return -1;
			}
		}

		return totalNumBytesWritten;
	}
}