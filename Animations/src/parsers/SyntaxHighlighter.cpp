#include "parsers/SyntaxHighlighter.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"

namespace MathAnim
{
	CodeHighlightIter& CodeHighlightIter::next(size_t bytePos)
	{
		// Check if we're at the end of the token
		if (tree && currentTokenIter != currentLineIter->tokens.end())
		{
			// Get the next token end pos
			size_t thisTokenEndPos = currentLineIter->byteStart + currentLineIter->numBytes;
			auto nextToken = currentTokenIter + 1;
			if (nextToken != currentLineIter->tokens.end())
			{
				thisTokenEndPos = currentLineIter->byteStart + nextToken->relativeStart;
			}

			if (bytePos >= thisTokenEndPos)
			{
				currentTokenIter++;
			}
		}

		// Then update the line iterator if we reached the end of this line
		if (tree && currentTokenIter == currentLineIter->tokens.end())
		{
			currentLineIter++;
			if (currentLineIter != tree->sourceInfo.end())
			{
				currentTokenIter = currentLineIter->tokens.begin();
			}
		}

		return *this;
	}

	CodeHighlightIter CodeHighlights::begin(size_t bytePos)
	{
		CodeHighlightIter res = {};
		res.tree = &this->tree;

		if (res.tree)
		{
			// Find where the line iterator should start
			res.currentLineIter = this->tree.sourceInfo.begin();
			while (res.currentLineIter != this->tree.sourceInfo.end())
			{
				if (res.currentLineIter->byteStart <= bytePos && res.currentLineIter->byteStart + res.currentLineIter->numBytes > bytePos)
				{
					res.currentTokenIter = res.currentLineIter->tokens.begin();
					break;
				}

				res.currentLineIter++;
			}

			// Find where the token iterator should start
			if (res.currentLineIter != this->tree.sourceInfo.end())
			{
				res.currentTokenIter = res.currentLineIter->tokens.begin();
				while (res.currentTokenIter != res.currentLineIter->tokens.end())
				{
					size_t tokenStart = res.currentTokenIter->relativeStart + res.currentLineIter->byteStart;

					size_t tokenEnd = res.currentLineIter->byteStart + res.currentLineIter->numBytes;
					auto nextTokenIter = res.currentTokenIter + 1;
					if (nextTokenIter != res.currentLineIter->tokens.end())
					{
						tokenEnd = nextTokenIter->relativeStart + res.currentLineIter->byteStart;
					}

					if (tokenStart <= bytePos && tokenEnd > bytePos)
					{
						break;
					}

					res.currentTokenIter++;
				}
			}
		}

		return res;
	}

	CodeHighlightIter CodeHighlights::end()
	{
		CodeHighlightIter res = {};
		res.tree = &this->tree;
		res.currentLineIter = this->tree.sourceInfo.end();
		if (this->tree.sourceInfo.size() > 0)
		{
			res.currentTokenIter = this->tree.sourceInfo[this->tree.sourceInfo.size() - 1].tokens.end();
		}

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

		CodeHighlightDebugInfo res = {};

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

		return res;
	}

	void SyntaxHighlighter::reparseSection(CodeHighlights& /*codeHighlights*/, const std::string& /*newCode*/, size_t, size_t, bool /*printDebugInfo*/) const
	{
		if (!this->grammar)
		{
			return;
		}
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