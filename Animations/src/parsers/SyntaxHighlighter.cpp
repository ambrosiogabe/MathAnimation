#include "parsers/SyntaxHighlighter.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"
#include "math/CMath.h"

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

		// Reparse any lines marked as needing to be reparsed
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

		return { firstLineUpdated, lastLineUpdated };
	}

	Vec2i SyntaxHighlighter::insertText(CodeHighlights& highlights, const char* newCodeBlock, size_t newCodeBlockLength, size_t insertStart, size_t insertEnd, size_t maxLinesToUpdate) const
	{
		size_t oldCodeLength = highlights.tree.codeLength;
		highlights.tree.codeBlock = newCodeBlock;
		highlights.tree.codeLength = newCodeBlockLength;

		{
			// First find the line that we need to update (or the end of the list if the insertion occurred at the end of the code block
			auto lineInfoToUpdateFrom = highlights.tree.getLineForByte(insertStart);

			// Insert a new lineInfo if needed
			if (lineInfoToUpdateFrom == highlights.tree.sourceInfo.end())
			{
				g_logger_assert(insertStart >= oldCodeLength, "We should only be updating from the end of the list when the insertion occurred at the end of the old source information.");

				if (highlights.tree.sourceInfo.size() == 0)
				{
					GrammarLineInfo emptyLine = {};
					highlights.tree.sourceInfo.emplace_back(emptyLine);
				}

				// Go back 1 from the end so we begin updating the last line of the code
				lineInfoToUpdateFrom = std::prev(highlights.tree.sourceInfo.end());
			}

			if (lineInfoToUpdateFrom != highlights.tree.sourceInfo.begin())
			{
				// Make sure to mark the previous line as needing an update, just to ensure that
				// we don't miss anything that could've broken a multiline parse
				auto prevLine = std::prev(lineInfoToUpdateFrom);
				if (prevLine != highlights.tree.sourceInfo.end())
				{
					prevLine->needsToBeUpdated = true;
				}
			}

			// Next, update the line beginnings and line lengths for all effected lines (or create new lines if new lines were made)
			// and also figure out which of those lines need to be updated
			auto currentLineInfo = lineInfoToUpdateFrom;
			size_t cursorIndex = insertStart;

			// Insert any newlines and figure out which lines need to be updated
			while (cursorIndex < highlights.tree.codeLength) 
			{
				// If we hit a newline or the end of the code, update the line we're currently on
				if (highlights.tree.codeBlock[cursorIndex] == '\n' || cursorIndex == highlights.tree.codeLength - 1)
				{
					// Update the current line
					currentLineInfo->numBytes = (uint32)(cursorIndex - currentLineInfo->byteStart + 1);
					currentLineInfo->needsToBeUpdated = true;
					currentLineInfo++;

					// If there was a newline in the insertion text, then insert a new line in our source info here
					if (cursorIndex >= insertStart && cursorIndex < insertEnd && highlights.tree.codeBlock[cursorIndex] == '\n')
					{
						currentLineInfo = highlights.tree.sourceInfo.insert(currentLineInfo, GrammarLineInfo{});
					}
					// Create a new entry for the new line info if we're not at the end of the file, but we *are* at the end of the list
					else if (currentLineInfo == highlights.tree.sourceInfo.end() && cursorIndex < highlights.tree.codeLength - 1)
					{
						currentLineInfo = highlights.tree.sourceInfo.insert(currentLineInfo, GrammarLineInfo{});
					}

					// Update the beginning of the next line
					if (currentLineInfo != highlights.tree.sourceInfo.end())
					{
						currentLineInfo->needsToBeUpdated = true;
						currentLineInfo->byteStart = (uint32)(cursorIndex + 1);
					}

					// Make sure to exit after we finish parsing the line after the insertion end
					if (cursorIndex >= insertEnd)
					{
						break;
					}
				}

				cursorIndex++;
			}

			// Next, update the remaining line beginning/endings
			for (; currentLineInfo != highlights.tree.sourceInfo.end(); currentLineInfo++)
			{
				if (currentLineInfo != highlights.tree.sourceInfo.begin())
				{
					auto prevLineInfo = std::prev(currentLineInfo);
					currentLineInfo->byteStart = prevLineInfo->byteStart + prevLineInfo->numBytes;

					g_logger_assert(currentLineInfo->byteStart + currentLineInfo->numBytes <= highlights.tree.codeLength, "We shouldn't be exceeding the capacity of our string length here.");
				}
			}
		}

		// Finally, actually reparse any effected lines
		// There's a chance that this iterator got invalidated, so get a new iterator to be safe
		auto lineInfoToUpdateFrom = highlights.tree.getLineForByte(insertStart);
		size_t lineInfoIndexToUpdateFrom = lineInfoToUpdateFrom - highlights.tree.sourceInfo.begin();

		// Make sure to start at the previous line just in case something changed there
		if (lineInfoIndexToUpdateFrom > 0)
		{
			lineInfoIndexToUpdateFrom--;
		}

		return checkForUpdatesFrom(highlights, lineInfoIndexToUpdateFrom + 1, maxLinesToUpdate);
	}

	Vec2i SyntaxHighlighter::removeText(CodeHighlights& highlights, const char* newCodeBlock, size_t newCodeBlockLength, size_t removeStart, size_t numLinesRemoved, size_t maxLinesToUpdate) const
	{
		// Remove an equivalent number of lines that were removed from this selection, starting from
		// the same line that the removeStart began
		size_t lineIndexStartedRemovingFrom = highlights.tree.sourceInfo.size();
		size_t numLinesToRemove = numLinesRemoved;
		for (auto lineIter = highlights.tree.getLineForByte(removeStart); lineIter != highlights.tree.sourceInfo.end();)
		{
			if (lineIndexStartedRemovingFrom == highlights.tree.sourceInfo.size())
			{
				lineIndexStartedRemovingFrom = lineIter - highlights.tree.sourceInfo.begin();
			}

			if (numLinesToRemove == 0)
			{
				break;
			}

			// Remove this line
			lineIter = highlights.tree.sourceInfo.erase(lineIter);
			numLinesToRemove--;
		}

		size_t oldCodeLength = highlights.tree.codeLength;
		highlights.tree.codeBlock = newCodeBlock;
		highlights.tree.codeLength = newCodeBlockLength;

		// If the new code block is empty, just return 0 lines updated
		if (highlights.tree.codeLength == 0)
		{
			return Vec2i{0, 0};
		}

		// Update beginnings/endings of all lines and mark any lines that need to be updated
		{
			auto currentLineInfo = highlights.tree.sourceInfo.begin() + lineIndexStartedRemovingFrom;
			g_logger_assert(currentLineInfo != highlights.tree.sourceInfo.end(), "Cannot remove syntax highlighting from beyond the end of the file.");
			
			if (currentLineInfo != highlights.tree.sourceInfo.begin())
			{
				// Make sure to mark the previous line as needing an update, just to ensure that
				// we don't miss anything that could've broken a multiline parse
				auto prevLine = std::prev(currentLineInfo);
				if (prevLine != highlights.tree.sourceInfo.end())
				{
					prevLine->needsToBeUpdated = true;
				}
			}

			// First, fix the current line info's start byte
			size_t prevLineEnd = 0;
			if (lineIndexStartedRemovingFrom > 0)
			{
				auto prevLineInfo = std::prev(currentLineInfo);
				prevLineEnd = prevLineInfo->byteStart + prevLineInfo->numBytes;
			}

			currentLineInfo->byteStart = (uint32)prevLineEnd;

			// Insert any newlines and figure out which lines need to be updated
			size_t cursorIndex = removeStart;
			while (cursorIndex < oldCodeLength)
			{
				// If we hit a newline or the end of the code, update the line we're currently on
				if (cursorIndex >= highlights.tree.codeLength || cursorIndex == highlights.tree.codeLength - 1 || highlights.tree.codeBlock[cursorIndex] == '\n')
				{
					// Update the current line
					uint32 endOfLine = CMath::min((uint32)cursorIndex, (uint32)highlights.tree.codeLength - 1);
					currentLineInfo->numBytes = endOfLine - currentLineInfo->byteStart + 1;
					currentLineInfo->needsToBeUpdated = true;
					currentLineInfo++;

					// Update the beginning of the next line
					if (currentLineInfo != highlights.tree.sourceInfo.end())
					{
						currentLineInfo->needsToBeUpdated = true;
						currentLineInfo->byteStart = (uint32)(cursorIndex + 1);
					}

					// We only need to check for updates at the line where the removal started, and the line right after it
					break;
				}

				cursorIndex++;
			}

			// Next, update the remaining line beginning/endings
			for (; currentLineInfo != highlights.tree.sourceInfo.end(); currentLineInfo++)
			{
				if (currentLineInfo != highlights.tree.sourceInfo.begin())
				{
					auto prevLineInfo = std::prev(currentLineInfo);
					currentLineInfo->byteStart = prevLineInfo->byteStart + prevLineInfo->numBytes;

					g_logger_assert(currentLineInfo->byteStart + currentLineInfo->numBytes <= highlights.tree.codeLength, "We shouldn't be exceeding the capacity of our string length here.");
				}
			}
		}

		// Make sure to start updating from 1 line previous just in case
		if (lineIndexStartedRemovingFrom > 0)
		{
			lineIndexStartedRemovingFrom--;
		}

		// Finally, actually reparse any effected lines
		return checkForUpdatesFrom(highlights, lineIndexStartedRemovingFrom + 1, maxLinesToUpdate);
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