#include "parsers/SyntaxHighlighter.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"

namespace MathAnim
{
	CodeHighlightIter& CodeHighlightIter::next(size_t bytePos)
	{
		if (collection && currentIter != collection->end() && bytePos >= currentIter->endPos)
		{
			currentIter++;
		}

		return *this;
	}

	CodeHighlightIter CodeHighlights::begin(size_t bytePos)
	{
		CodeHighlightIter res = {};
		res.collection = &this->segments;

		// Find where the iterator should start
		res.currentIter = this->segments.begin();
		for (size_t i = 0; i <= bytePos; i++)
		{
			if (res.currentIter != this->segments.end() && i >= res.currentIter->endPos)
			{
				res.currentIter++;
			}
		}

		return res;
	}

	CodeHighlightIter CodeHighlights::end()
	{
		CodeHighlightIter res = {};
		res.collection = &this->segments;
		res.currentIter = this->segments.end();
		return res;
	}

	SyntaxHighlighter::SyntaxHighlighter(const std::filesystem::path& grammar)
	{
		this->grammar = Grammar::importGrammar(grammar.string().c_str());
	}

	CodeHighlightDebugInfo SyntaxHighlighter::getAncestorsFor(SyntaxTheme const* theme, const std::string& code, size_t cursorPos) const
	{
		if (!this->grammar)
		{
			return {};
		}

		if (!theme)
		{
			return {};
		}

		if (cursorPos >= code.length())
		{
			return {};
		}

		CodeHighlightDebugInfo res  = {};

		SourceGrammarTree grammarTree = grammar->parseCodeBlock(code, *theme, false);
		res.matchText = grammarTree.getMatchTextAtChar(cursorPos);

		// Since this is just used for debug info, replace the matchText control characters with escaped (e.g \n) chars
		for (size_t i = 0; i < res.matchText.length(); i++)
		{
			if (res.matchText[i] == '\n')
			{
				res.matchText[i] = '\\';
				res.matchText.insert(res.matchText.begin() + i + 1, 'n');
				i++;
			}

			if (res.matchText[i] == '\t')
			{
				res.matchText[i] = '\\';
				res.matchText.insert(res.matchText.begin() + i + 1, 't');
				i++;
			}

			if (res.matchText[i] == '\r')
			{
				res.matchText[i] = '\\';
				res.matchText.insert(res.matchText.begin() + i + 1, 'r');
				i++;
			}
		}

		res.ancestors = grammarTree.getAllAncestorScopesAtChar(cursorPos);
		res.settings = theme->debugMatch(res.ancestors, &res.usingDefaultSettings);

		return res;
	}

	CodeHighlights SyntaxHighlighter::parse(const std::string& code, const SyntaxTheme& theme, bool printDebugInfo) const
	{
		if (!this->grammar)
		{
			return {};
		}

		CodeHighlights res = {};
		res.tree = grammar->parseCodeBlock(code, theme, printDebugInfo);
		res.codeBlock = code;
		res.theme = &theme;

		for (auto const& line : res.tree.sourceInfo)
		{
			for (size_t i = 0; i < line.tokens.size(); i++)
			{
				HighlightSegment segment = {};
				segment.startPos = line.tokens[i].relativeStart + line.byteStart;
				segment.endPos = line.byteStart + line.numBytes;
				
				if (i < line.tokens.size() - 1)
				{
					segment.endPos = line.tokens[i + 1].relativeStart + line.byteStart;
				}

				segment.color = theme.getColor(line.tokens[i].style.getForegroundColor());

				res.segments.emplace_back(segment);
			}
		}

		return res;
	}

	void SyntaxHighlighter::reparseSection(CodeHighlights& /*codeHighlights*/, const std::string& /*newCode*/, size_t, size_t, bool /*printDebugInfo*/) const
	{
		if (!this->grammar)
		{
			return;
		}

		//codeHighlights.tree = grammar->parseCodeBlock(codeHighlights.codeBlock, printDebugInfo);
		//codeHighlights.segments = {};
		//codeHighlights.codeBlock = newCode;

		//// For each atom in our grammar tree, output a highlight segment with the
		//// appropriate style for that atom
		//size_t highlightCursor = 0;
		//for (size_t child = 0; child < codeHighlights.tree.tree.size(); child++)
		//{
		//	if (codeHighlights.tree.tree[child].isAtomicNode)
		//	{
		//		std::vector<ScopedName> ancestorScopes = codeHighlights.tree.getAllAncestorScopes(child);

		//		auto settings = codeHighlights.theme->match(ancestorScopes);
		//		Vec4 settingForeground = codeHighlights.theme->getColor(settings.getForegroundColor());

		//		size_t absStart = highlightCursor;
		//		size_t nodeSize = codeHighlights.tree.tree[child].sourceSpan.size;

		//		HighlightSegment segment = {};
		//		segment.startPos = absStart;
		//		segment.endPos = absStart + nodeSize;
		//		segment.color = settingForeground;

		//		codeHighlights.segments.emplace_back(segment);
		//		highlightCursor += nodeSize;
		//	}
		//}

		//if (highlightCursor < newCode.length())
		//{
		//	HighlightSegment finalSegment = {};
		//	finalSegment.startPos = highlightCursor;
		//	finalSegment.endPos = newCode.length();
		//	finalSegment.color = codeHighlights.theme->getColor(codeHighlights.theme->defaultForeground);

		//	codeHighlights.segments.emplace_back(finalSegment);
		//}
	}

	std::string SyntaxHighlighter::getStringifiedParseTreeFor(const std::string& code, SyntaxTheme const& theme) const
	{
		if (!this->grammar)
		{
			return {};
		}

		SourceGrammarTree grammarTree = grammar->parseCodeBlock(code, theme, false);
		return grammarTree.getStringifiedTree(*grammar);
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
		static std::unordered_map<std::filesystem::path, SyntaxHighlighter*> importedGrammars = {};

		void init()
		{
			for (int i = 0; i < (int)HighlighterLanguage::Length; i++)
			{
				if (Platform::fileExists(_highlighterLanguageFilenames[i]))
				{
					SyntaxHighlighter* highlighter = g_memory_new SyntaxHighlighter(_highlighterLanguageFilenames[i]);
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

		void importGrammar(const char* filename)
		{
			std::filesystem::path absPath = std::filesystem::absolute(std::filesystem::path(filename));
			if (auto iter = importedGrammars.find(absPath); iter != importedGrammars.end())
			{
				return;
			}

			if (Platform::fileExists(absPath.string().c_str()))
			{
				SyntaxHighlighter* highlighter = g_memory_new SyntaxHighlighter(absPath);
				importedGrammars[absPath] = highlighter;
			}
		}

		const SyntaxHighlighter* getImportedHighlighter(const char* filename)
		{
			std::filesystem::path absPath = std::filesystem::absolute(std::filesystem::path(filename));
			if (auto iter = importedGrammars.find(absPath); iter != importedGrammars.end())
			{
				return iter->second;
			}

			return nullptr;
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
			for (auto [k, v] : importedGrammars)
			{
				v->free();
				g_memory_delete(v);
			}
			importedGrammars.clear();

			for (auto [k, v] : grammars)
			{
				v->free();
				g_memory_delete(v);
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