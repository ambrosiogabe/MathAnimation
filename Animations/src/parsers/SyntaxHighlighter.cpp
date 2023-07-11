#include "parsers/SyntaxHighlighter.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"

namespace MathAnim
{
	SyntaxHighlighter::SyntaxHighlighter(const std::filesystem::path& grammar)
	{
		this->grammar = Grammar::importGrammar(grammar.string().c_str());
	}

	std::vector<ScopedName> SyntaxHighlighter::getAncestorsFor(const std::string& code, size_t cursorPos) const
	{
		if (!this->grammar)
		{
			return {};
		}

		SourceGrammarTree grammarTree = grammar->parseCodeBlock(code);
		return grammarTree.getAllAncestorScopesAtChar(cursorPos);
	}

	CodeHighlights SyntaxHighlighter::parse(const std::string& code, const SyntaxTheme& theme, bool printDebugInfo) const
	{
		if (!this->grammar)
		{
			return {};
		}

		SourceGrammarTree grammarTree = grammar->parseCodeBlock(code, printDebugInfo);
		CodeHighlights res = {};
		res.codeBlock = code;

		// For each atom in our grammar tree, output a highlight segment with the
		// appropriate style for that atom
		size_t highlightCursor = 0;
		for (size_t child = 0; child < grammarTree.tree.size(); child++)
		{
			// Any nodes without a scope are an ATOM node
			if (!grammarTree.tree[child].scope)
			{
				std::vector<ScopedName> ancestorScopes = grammarTree.getAllAncestorScopes(child);
				const ThemeSetting* setting = theme.match(ancestorScopes, ThemeSettingType::ForegroundColor);

				Vec4 settingForeground = Vec4{ 1, 1, 1, 1 };
				if (setting)
				{
					g_logger_assert(setting->foregroundColor.has_value(), "Something bad happened here.");
					settingForeground = setting->foregroundColor.value().color;
				}

				size_t absStart = highlightCursor;
				size_t nodeSize = grammarTree.tree[child].sourceSpan.size;

				HighlightSegment segment = {};
				segment.startPos = absStart;
				segment.endPos = absStart + nodeSize;
				segment.color = settingForeground;

				res.segments.emplace_back(segment);
				highlightCursor += nodeSize;
			}
		}

		if (highlightCursor < code.length())
		{
			Vec4 settingColor = Vec4{ 1, 1, 1, 1 };
			const ThemeSetting* setting = theme.defaultRule.getSetting(ThemeSettingType::ForegroundColor);
			if (setting && setting->foregroundColor.has_value())
			{
				settingColor = setting->foregroundColor.value().color;
			}

			HighlightSegment finalSegment = {};
			finalSegment.startPos = highlightCursor;
			finalSegment.endPos = code.length();
			finalSegment.color = settingColor;

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
}