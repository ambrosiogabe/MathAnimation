#include "parsers/SyntaxHighlighter.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"

namespace MathAnim
{
	CodeHighlightIter& CodeHighlightIter::next(size_t bytePos)
	{
		if (currentLineIter == tree->sourceInfo.end())
		{
			return *this;
		}

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

	CodeHighlightDebugInfo SyntaxHighlighter::getAncestorsFor(SyntaxTheme const* theme, CodeHighlights const& highlights, size_t cursorPos) const
	{
		if (!this->grammar)
		{
			return {};
		}

		if (!theme)
		{
			return {};
		}

		if (cursorPos >= highlights.tree.codeLength)
		{
			return {};
		}

		CodeHighlightDebugInfo res = {};
		res.matchText = highlights.tree.getMatchTextAtChar(cursorPos);

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

		res.ancestors = highlights.tree.getAllAncestorScopesAtChar(cursorPos);
		res.settings = theme->debugMatch(res.ancestors, &res.usingDefaultSettings);

		return res;
	}

	CodeHighlights SyntaxHighlighter::parse(const char* code, size_t codeLength, const SyntaxTheme& theme) const
	{
		if (!this->grammar)
		{
			return {};
		}

		CodeHighlights res = {};
		res.tree = grammar->parseCodeBlock(code, codeLength, theme);
		res.theme = &theme;

		return res;
	}

	Vec2i SyntaxHighlighter::checkForUpdatesFrom(CodeHighlights& highlights, size_t lineToCheckFrom, size_t maxLinesToUpdate) const
	{
		g_logger_assert(lineToCheckFrom > 0, "There is no 0th line number.");

		size_t lineInfoIndexToUpdateFrom = lineToCheckFrom - 1;
		if (lineInfoIndexToUpdateFrom >= highlights.tree.sourceInfo.size())
		{
			return { (int32)highlights.tree.sourceInfo.size(), (int32)highlights.tree.sourceInfo.size() };
		}

		int32 firstLineUpdated = (int32)highlights.tree.sourceInfo.size();
		int32 lastLineUpdated = firstLineUpdated;

		// Finally, actually reparse any effected lines, including all characters in between the insertion points
		{
			size_t numLinesUpdated = 0;
			while (numLinesUpdated < maxLinesToUpdate)
			{
				if (lineInfoIndexToUpdateFrom + numLinesUpdated >= highlights.tree.sourceInfo.size())
				{
					break;
				}

				auto currentLineInfo = highlights.tree.sourceInfo.begin() + lineInfoIndexToUpdateFrom + numLinesUpdated;
				if (currentLineInfo->needsToBeUpdated)
				{
					if (firstLineUpdated == (int32)highlights.tree.sourceInfo.size())
					{
						firstLineUpdated = (int32)(lineInfoIndexToUpdateFrom + numLinesUpdated + 1);
					}

					size_t numLinesParsed = this->grammar->updateFromByte(highlights.tree, *highlights.theme, currentLineInfo->byteStart, (uint32)(maxLinesToUpdate - numLinesUpdated));
					numLinesUpdated += numLinesParsed;

					lastLineUpdated = (int32)(lineInfoIndexToUpdateFrom + numLinesUpdated + 1);
				}
				else
				{
					numLinesUpdated++;
				}
			}
		}

		if (firstLineUpdated == highlights.tree.sourceInfo.size())
		{
			return { (int32)highlights.tree.sourceInfo.size(), (int32)highlights.tree.sourceInfo.size() };
		}

		return { firstLineUpdated, lastLineUpdated };
	}

	Vec2i SyntaxHighlighter::insertText(CodeHighlights& highlights, const char* newCodeBlock, size_t newCodeBlockLength, size_t insertStart, size_t insertEnd, size_t maxLinesToUpdate) const
	{
		int32 firstLineUpdated = (int32)highlights.tree.sourceInfo.size();
		int32 lastLineUpdated = firstLineUpdated;

		size_t oldCodeLength = highlights.tree.codeLength;

		// First find the line that we need to update (or the end of the list if the insertion occurred at the end of the code block
		highlights.tree.codeBlock = newCodeBlock;
		highlights.tree.codeLength = newCodeBlockLength;

		size_t lineInfoIndexToUpdateFrom = highlights.tree.sourceInfo.size();
		for (size_t i = 0; i < highlights.tree.sourceInfo.size(); i++)
		{
			auto& lineInfo = highlights.tree.sourceInfo[i];
			if (lineInfo.byteStart <= insertStart && lineInfo.byteStart + lineInfo.numBytes > insertStart)
			{
				lineInfoIndexToUpdateFrom = i;
				break;
			}
		}

		if (lineInfoIndexToUpdateFrom == highlights.tree.sourceInfo.size())
		{
			g_logger_assert(insertStart >= oldCodeLength, "We should only be updating from the end of the list when the insertion occurred at the end of the old source information.");

			if (highlights.tree.sourceInfo.size() == 0)
			{
				GrammarLineInfo emptyLine = {};
				highlights.tree.sourceInfo.emplace_back(emptyLine);
			}
			else
			{
				// Subtract 1 so we begin updating the last line of the code
				lineInfoIndexToUpdateFrom--;
			}
		}

		// Next, update the line beginnings and line lengths for all effected lines (or create new lines if new lines were made)
		// and also figure out which of those lines need to be updated
		{
			auto currentLineInfo = highlights.tree.sourceInfo.begin() + lineInfoIndexToUpdateFrom;

			size_t cursorIndex = insertStart;
			while (cursorIndex < highlights.tree.codeLength)
			{
				// If we hit a newline or the end of the code, update the line we're currently on
				if (highlights.tree.codeBlock[cursorIndex] == '\n' || cursorIndex == highlights.tree.codeLength - 1)
				{
					// Update the current line
					size_t newNumBytes = cursorIndex - currentLineInfo->byteStart + 1;
					bool nextLineNeedsUpdatingToo = false;
					if (currentLineInfo->numBytes != newNumBytes)
					{
						currentLineInfo->needsToBeUpdated = true;
						// Also update the next line if possible
						nextLineNeedsUpdatingToo = true;
					}

					currentLineInfo->numBytes = (uint32)newNumBytes;
					currentLineInfo++;

					// If there was a newline in the insertion text, then
					// insert a new line in our source info here
					if (cursorIndex >= insertStart && cursorIndex < insertEnd && highlights.tree.codeBlock[cursorIndex] == '\n')
					{
						// Create a new entry for the new line info if we're not at the end of the file
						currentLineInfo = highlights.tree.sourceInfo.insert(currentLineInfo, GrammarLineInfo{});
						currentLineInfo->needsToBeUpdated = true;
						currentLineInfo->byteStart = (uint32)(cursorIndex + 1);
						currentLineInfo->numBytes = 0;
					}

					if (currentLineInfo != highlights.tree.sourceInfo.end())
					{
						// We don't want to overwrite anything to false that does need to be updated, so we do an if check here
						if (nextLineNeedsUpdatingToo)
						{
							currentLineInfo->needsToBeUpdated = true;
						}
						currentLineInfo->byteStart = (uint32)(cursorIndex + 1);
					}

					if (currentLineInfo == highlights.tree.sourceInfo.end() && cursorIndex < highlights.tree.codeLength - 1)
					{
						// Create a new entry for the new line info if we're not at the end of the file
						highlights.tree.sourceInfo.push_back({});
						currentLineInfo = highlights.tree.sourceInfo.begin() + (highlights.tree.sourceInfo.size() - 1);
						currentLineInfo->needsToBeUpdated = true;
						currentLineInfo->byteStart = (uint32)(cursorIndex + 1);
						currentLineInfo->numBytes = 1;
					}
				}

				cursorIndex++;
			}
		}

		// Finally, actually reparse any effected lines, including all characters in between the insertion points
		{
			size_t cursorIndex = insertStart;
			size_t numLinesUpdated = 0;
			while (numLinesUpdated < maxLinesToUpdate)
			{
				if (firstLineUpdated == highlights.tree.sourceInfo.size())
				{
					firstLineUpdated = (int32)(lineInfoIndexToUpdateFrom + numLinesUpdated + 1);
				}

				size_t numLinesParsed = this->grammar->updateFromByte(highlights.tree, *highlights.theme, (uint32)cursorIndex, (uint32)(maxLinesToUpdate - numLinesUpdated));
				numLinesUpdated += numLinesParsed;

				lastLineUpdated = (int32)(lineInfoIndexToUpdateFrom + numLinesUpdated + 1);

				if (lineInfoIndexToUpdateFrom + numLinesUpdated >= highlights.tree.sourceInfo.size())
				{
					break;
				}

				auto currentLineInfo = highlights.tree.sourceInfo.begin() + lineInfoIndexToUpdateFrom + numLinesUpdated;
				cursorIndex = currentLineInfo->byteStart;

				// If we've passed the insertion characters, then we only want to update lines that need to be udpated from
				// here on out. So we'll skip over lines that didn't change
				if (insertEnd < currentLineInfo->byteStart)
				{
					// Skip any lines that don't need to be updated
					while (!currentLineInfo->needsToBeUpdated)
					{
						currentLineInfo++;
						numLinesUpdated++;

						if (numLinesUpdated > maxLinesToUpdate)
						{
							break;
						}

						if (currentLineInfo == highlights.tree.sourceInfo.end())
						{
							break;
						}
					}

					if (lineInfoIndexToUpdateFrom + numLinesUpdated >= highlights.tree.sourceInfo.size())
					{
						break;
					}

					currentLineInfo = highlights.tree.sourceInfo.begin() + lineInfoIndexToUpdateFrom + numLinesUpdated;
					cursorIndex = currentLineInfo->byteStart;
				}
			}
		}

		return { firstLineUpdated, lastLineUpdated };
	}

	std::string SyntaxHighlighter::getStringifiedParseTreeFor(const std::string& code, SyntaxTheme const& theme) const
	{
		if (!this->grammar)
		{
			return {};
		}

		SourceGrammarTree grammarTree = grammar->parseCodeBlock(code.c_str(), code.length(), theme);
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