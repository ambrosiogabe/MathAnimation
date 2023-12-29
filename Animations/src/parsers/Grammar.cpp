#include "parsers/Grammar.h"
#include "platform/Platform.h"
#include "math/CMath.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	using namespace nlohmann;

	// ----------- Internal Functions -----------
	static Grammar* importGrammarFromJson(const json& j);
	static SyntaxPattern* const parsePattern(const json& json, Grammar* self);
	static PatternArray parsePatternsArray(const json& json, Grammar* self);
	static OnigRegex onigFromString(const std::string& str, bool multiLine);
	static ScopedName getScopeWithCaptures(const std::string& str, const ScopedName& originalScope, const OnigRegion* region);
	static void getFirstMatchInRegset(const std::string& str, size_t anchor, size_t startOffset, size_t endOffset, const PatternArray& pattern, int* patternMatched);

	static size_t pushMatchesToLineWithParent(GrammarLineInfo& line, GrammarMatchV2 const& parent, std::vector<GrammarMatchV2> const& subMatches, SyntaxTheme const& theme, size_t currentByte);
	static size_t pushMatchesToLine(GrammarLineInfo& line, std::vector<GrammarMatchV2> const& subMatches, SyntaxTheme const& theme, size_t currentByte);
	static std::optional<GrammarMatchV2> getFirstMatchV2(const std::string& str, size_t anchor, size_t startOffset, size_t endOffset, OnigRegex reg, OnigRegion* region, const std::optional<ScopedName>& scope);
	static std::vector<GrammarMatchV2> getCapturesV2(GrammarLineInfo& line, const std::string& str, SyntaxTheme const& theme, const PatternRepository& repo, OnigRegion* region, std::optional<CaptureList> captures, Grammar const* self);

	static void freePattern(SyntaxPattern* const pattern);

	// --- Construct Regset Helpers ---
	static void addPatternToRegset(OnigRegSet* regset, PatternArray& ogPatternArray, const SyntaxPattern& pattern, const Grammar* self, uint64 patternArrayIndex);
	static void constructRegexesFromPattern(SyntaxPattern& pattern, Grammar* self);
	static void constructRegsetFromPatterns(PatternArray& patternArray, Grammar* self);

	void Capture::free()
	{
		if (patternArray.has_value())
		{
			patternArray->free();
		}
	}

	void CaptureList::free()
	{
		for (auto& capture : captures)
		{
			capture.free();
		}

		captures.clear();
	}

	CaptureList CaptureList::from(const json& j, Grammar* self)
	{
		CaptureList res = {};

		for (auto& [key, val] : j.items())
		{
			size_t captureNumber = 0;
			try
			{
				captureNumber = std::stoi(key);
			}
			catch (std::invalid_argument const& ex)
			{
				g_logger_warning("Invalid capture list. Key '{}' is not a number.\n'{}'", key, ex.what());
				continue;
			}
			catch (std::out_of_range const& ex)
			{
				g_logger_warning("Invalid capture list. Key '{}' is too large.\n'{}'", key, ex.what());
				continue;
			}

			if (val.contains("name"))
			{
				ScopedName scope = ScopedName::from(val["name"]);
				Capture cap = {};
				cap.index = captureNumber;
				cap.scope = scope;
				cap.patternArray = std::nullopt;
				res.captures.emplace_back(cap);
			}
			else if (val.contains("patterns"))
			{
				Capture cap = {};
				cap.index = captureNumber;
				cap.scope = std::nullopt;
				cap.patternArray = parsePatternsArray(val["patterns"], self);
				res.captures.emplace_back(cap);
			}
			else
			{
				std::string stringifiedJson = val.dump(4);
				g_logger_warning("Invalid capture list. Capture '{}' does not contain a 'name' key for val, '{}'.", key, stringifiedJson);
			}
		}

		return res;
	}

	void SimpleSyntaxPattern::pushScopeToAncestorStack(GrammarLineInfo& line, std::optional<GrammarMatchV2> const& match) const
	{
		if (this->scope.has_value())
		{
			if (match.has_value() && match->scope.has_value())
			{
				// NOTE: Prefer pushing the matches scope here because it will occasionally consist of a regex that gets replaced
				//       with a capture group. Like: get[$1] -> getter
				line.ancestors.push_back(match->scope.value());
			}
			else
			{
				line.ancestors.push_back(this->scope.value());
			}
		}
	}

	void SimpleSyntaxPattern::popScopeFromAncestorStack(GrammarLineInfo& line) const
	{
		if (this->scope.has_value())
		{
			line.ancestors.pop_back();
		}
	}

	MatchSpan SimpleSyntaxPattern::tryParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, Grammar const* self) const
	{
		std::optional<GrammarMatchV2> match = getFirstMatchV2(code, anchor, start, end, this->regMatch, region, this->scope);

		size_t currentByte = start;
		if (match.has_value() && match->start > currentByte)
		{
			// Push an empty token to fill in the gap before we push a new scope to the ancestor stack
			SourceSyntaxToken token = {};
			token.debugAncestorStack = line.ancestors;
			token.relativeStart = (uint32)(currentByte - line.byteStart);
			token.style = theme.match(line.ancestors);

			line.tokens.emplace_back(token);
			currentByte = match->start;
		}

		// Push new scope to ancestor stack
		pushScopeToAncestorStack(line, match);

		if (match.has_value() && match->start < end && match->start >= start && match->end <= end)
		{
			// `region` now contains any potential captures, so we'll search for any captures now
			std::vector<GrammarMatchV2> subMatches = getCapturesV2(line, code, theme, repo, region, this->captures, self);

			// If this simple pattern has a scope name, just add the match with submatches as children
			if (scope.has_value())
			{
				size_t res = pushMatchesToLineWithParent(line, match.value(), subMatches, theme, currentByte);
				popScopeFromAncestorStack(line);
				return { match->start, res };
			}
			// Otherwise, just add the children to the out matches
			else
			{
				// Push any applicable sub-matches to the line info
				size_t res = pushMatchesToLine(line, subMatches, theme, currentByte);
				popScopeFromAncestorStack(line);
				return { match->start, res };
			}
		}

		return { start, start };
	}

	void SimpleSyntaxPattern::free()
	{
		if (regMatch)
		{
			onig_free(regMatch);
		}

		if (captures.has_value())
		{
			captures->free();
		}

		regMatch = nullptr;
	}

	void ComplexSyntaxPattern::pushScopeToAncestorStack(GrammarLineInfo& line) const
	{
		if (this->scope.has_value())
		{
			line.ancestors.push_back(this->scope.value());
		}
	}

	void ComplexSyntaxPattern::popScopeFromAncestorStack(GrammarLineInfo& line) const
	{
		if (this->scope.has_value())
		{
			line.ancestors.pop_back();
		}
	}

	MatchSpan ComplexSyntaxPattern::tryParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t endOffset, const PatternRepository& repo, OnigRegion* region, Grammar const* self, GrammarPatternGid gid) const
	{
		// If the begin/end pair doesn't have a match, then this rule isn't a success
		std::optional<GrammarMatchV2> beginBlockMatch = getFirstMatchV2(code, anchor, start, endOffset, this->begin, region, std::nullopt);
		if (!beginBlockMatch.has_value() || beginBlockMatch->start >= endOffset || beginBlockMatch->start < start)
		{
			return { start, start };
		}

		// If beginBlockMatch is valid, then we'll use the result stored in `region` to find any captures
		std::vector<GrammarMatchV2> beginSubMatches = getCapturesV2(line, code, theme, repo, region, this->beginCaptures, self);

		// We'll be storing information for how to resume parsing if we get interrupted
		GrammarResumeParseInfo resumeInfo = {};

		// If there is no gap, then this will equal the start location and nothing will happen
		resumeInfo.gapTokenStart = beginBlockMatch->start;

		if (beginBlockMatch->start > start)
		{
			// Make sure to record where this gap is stored just in case we need to pop it when we pop the rest of this
			// off the stack
			resumeInfo.gapTokenStart = start;

			// Fill in the gap if needed before we push our scope to the stack
			SourceSyntaxToken token = {};
			token.relativeStart = (uint32)(start - line.byteStart);
			token.debugAncestorStack = line.ancestors;
			token.style = theme.match(line.ancestors);

			line.tokens.emplace_back(token);
			start = beginBlockMatch->start;
		}

		pushScopeToAncestorStack(line);
		size_t endOfBeginBlockSubMatches = pushMatchesToLine(line, beginSubMatches, theme, start);

		if (this->end.isDynamic)
		{
			// Generate an on the fly pattern
			std::string regexPatternToTest = this->end.regexText;

			// Substitute backrefs in the string
			int offsetToAdd = 0;
			for (auto& backref : this->end.backrefs)
			{
				g_logger_assert(backref.captureIndex < region->num_regs, "An invalid regex was created in the grammar. This was the regex: '{}'", this->end.regexText);

				int replacementStringBegin = region->beg[backref.captureIndex];
				int replacementStringEnd = region->end[backref.captureIndex];

				g_logger_assert(replacementStringEnd >= replacementStringBegin, "An invalid backreference was captured for pattern: '{}'", this->end.regexText);

				int replaceBeginOffset = (int)backref.strReplaceStart + offsetToAdd;
				int replaceEndOffset = (int)backref.strReplaceEnd + offsetToAdd + 1;

				std::string beginRegex = regexPatternToTest.substr(0, replaceBeginOffset);
				std::string endRegex = regexPatternToTest.substr(replaceEndOffset);
				std::string replacement = code.substr(replacementStringBegin, (replacementStringEnd - replacementStringBegin));
				regexPatternToTest = beginRegex + replacement + endRegex;

				offsetToAdd += (replacementStringEnd - replacementStringBegin) - (replaceEndOffset - replaceBeginOffset);
			}

			resumeInfo.dynamicEndPatternStr = regexPatternToTest;
		}

		// We're potentially entering a pattern that could exceed this line, so we'll store the gid here in case
		// we need to resume parsing at a later time
		resumeInfo.gid = gid;
		resumeInfo.anchor = beginBlockMatch->end;
		resumeInfo.currentByte = endOfBeginBlockSubMatches;
		resumeInfo.originalStart = beginBlockMatch->start;
		resumeInfo.hasParentScope = this->scope.has_value();
		line.patternStack.emplace_back(resumeInfo);

		return resumeParse(line, code, theme, resumeInfo.currentByte, resumeInfo.dynamicEndPatternStr, beginBlockMatch->end, beginBlockMatch->end, code.length(), repo, region, self);
	}

	MatchSpan ComplexSyntaxPattern::resumeParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t currentByte, std::string const& dynamicEndPattern, size_t anchor, size_t start, size_t /*endOffset*/, const PatternRepository& repo, OnigRegion* region, Grammar const* self) const
	{
		OnigRegex endPattern = this->end.simpleRegex;
		if (this->end.isDynamic)
		{
			// After substituting backrefs generate an on-the-fly end pattern to match
			endPattern = onigFromString(dynamicEndPattern, true);
			g_logger_assert(endPattern != nullptr, "Failed to generate dynamic regex pattern with backreferences for pattern: '{}'", this->end.regexText);
		}

		// This match can go to the end of the string
		std::optional<GrammarMatchV2> endBlockMatch = getFirstMatchV2(code, anchor, start, code.length(), endPattern, region, std::nullopt);
		std::vector<GrammarMatchV2> endSubMatches = {};
		if (!endBlockMatch.has_value())
		{
			GrammarMatchV2 eof;
			eof.start = code.length();
			eof.end = code.length();
			eof.scope = ScopedName::from("source.ERROR_EOF_NO_CLOSING_MATCH");

			// If there was no end group matched, then automatically use the end of the file as the end block
			// which is specified in the rules for textmates
			endBlockMatch = eof;
		}
		else
		{
			// Check for any captures in the endBlock if it was valid
			endSubMatches = getCapturesV2(line, code, theme, repo, region, this->endCaptures, self);
		}

		// Push an empty token here to fill in the gap if needed
		if (start > currentByte)
		{
			SourceSyntaxToken emptyToken = {};
			emptyToken.relativeStart = (uint32)(currentByte - line.byteStart);
			emptyToken.debugAncestorStack = line.ancestors;
			emptyToken.style = theme.match(line.ancestors);

			line.tokens.emplace_back(emptyToken);
			currentByte = start;
		}

		if (this->patterns.has_value())
		{
			size_t inBetweenStart = start;
			size_t inBetweenEnd = endBlockMatch->start;

			// We want to consider the rest of the line when matching
			size_t endOfLine = line.byteStart + line.numBytes;

			while (inBetweenStart < inBetweenEnd)
			{
				// NOTE: We start searching at `beginBlockMatch->end` so that if any patterns are using anchors in
				//       their regexes, the anchor will appropriately start at the beginning of the `inBetween` span
				size_t lastTokensSize = line.tokens.size();
				size_t lastPatternStackSize = line.patternStack.size();
				auto span = patterns->tryParse(line, code, theme, anchor, inBetweenStart, endOfLine, repo, region, self);
				currentByte = span.matchEnd;
				if (currentByte == inBetweenStart)
				{
					break;
				}

				// Discard any matches that begin outside of our inBetweenBlock
				if (span.matchStart >= inBetweenEnd)
				{
					line.tokens.erase(line.tokens.begin() + lastTokensSize, line.tokens.end());

					// Pop any pattern stacks and their ancestors too since we won't be using them now
					while (line.patternStack.size() > lastPatternStackSize)
					{
						auto const& resumeInfoToPop = line.patternStack[line.patternStack.size() - 1];
						if (resumeInfoToPop.hasParentScope)
						{
							line.ancestors.pop_back();
						}

						line.patternStack.pop_back();
					}

					// Reset currentByte
					currentByte = inBetweenStart;
					break;
				}
				else if (span.matchStart < inBetweenStart)
				{
					// TODO: Is this block needed?

					line.tokens.erase(line.tokens.begin() + lastTokensSize, line.tokens.end());
					// Reset currentByte
					currentByte = inBetweenStart;

					// Pop any pattern stacks and their ancestors too since we won't be using them now
					while (line.patternStack.size() > lastPatternStackSize)
					{
						auto const& resumeInfoToPop = line.patternStack[line.patternStack.size() - 1];
						if (resumeInfoToPop.hasParentScope)
						{
							line.ancestors.pop_back();
						}

						line.patternStack.pop_back();
					}
					break;
				}
				else
				{
					// New in between start is where we stopped parsing: the currentByte
					inBetweenStart = currentByte;
				}

				// ----
				// NOTE: If a match in between exceeds the current end of our match, we have to try to find
				//       a new end that satisfies this match
				if (inBetweenStart > endBlockMatch->start)
				{
					// ----
					// NOTE: If a match ends on a newline and exceeds the current end block, we'll stop matching.
					//       So this basically short-circuits the rest of this process. This is for the test case
					//       `withLua_endBlockDoesNotExceedWhenItsStoppedOnANewline`
					if (inBetweenStart > 0 && code[inBetweenStart - 1] == '\n')
					{
						endBlockMatch->start = inBetweenStart;
						if (endBlockMatch->start > endBlockMatch->end)
						{
							endBlockMatch->end = endBlockMatch->start;
						}

						break;
					}
					// ----

					endBlockMatch = getFirstMatchV2(code, inBetweenStart, inBetweenStart, code.length(), endPattern, region, std::nullopt);
					if (!endBlockMatch.has_value())
					{
						GrammarMatchV2 eof;
						eof.start = code.length();
						eof.end = code.length();
						eof.scope = ScopedName::from("source.ERROR_EOF_NO_CLOSING_MATCH");

						// If there was no end group matched, then automatically use the end of the file as the end block
						// which is specified in the rules for textmates
						endBlockMatch = eof;
						endSubMatches = {};
					}
					else
					{
						// Check for any captures in the endBlock if it was valid
						endSubMatches = getCapturesV2(line, code, theme, repo, region, this->endCaptures, self);
					}

					inBetweenEnd = endBlockMatch->start;

					// Stop matching if we've hit the end of the line and add all current sub-matches to the line info.
					// This complex pattern will resume matching if/when the caller decides it should
					if (currentByte > endOfLine)
					{
						goto complexPattern_AddMatchesAndReturnEarly;
					}
				}
				// ----
				else if (inBetweenStart == endBlockMatch->end)
				{
					break;
				}
			}
		}

		if (endBlockMatch->end > line.byteStart + line.numBytes)
		{
			goto complexPattern_AddMatchesAndReturnEarly;
		}

		// Free dynamic regex if necessary
		if (this->end.isDynamic)
		{
			onig_free(endPattern);
		}

		{
			// Add in end sub-matches
			size_t newCursorPos = pushMatchesToLineWithParent(line, endBlockMatch.value(), endSubMatches, theme, currentByte);
			popScopeFromAncestorStack(line);

			// Get the original start from the resume information
			size_t originalStart = line.patternStack[line.patternStack.size() - 1].originalStart;

			// Pop the pattern
			line.patternStack.pop_back();

			return { originalStart, newCursorPos };
		}

		// This is if we've hit the end of the line early and want to return true
	complexPattern_AddMatchesAndReturnEarly:

		// Free dynamic regex if necessary
		if (this->end.isDynamic)
		{
			onig_free(endPattern);
		}

		{
			// Push an empty token with current ancestor stack to fill the rest of the line
			// Only push the token if it's within this line's boundaries
			if (currentByte < line.byteStart + line.numBytes)
			{
				SourceSyntaxToken emptyToken = {};
				emptyToken.relativeStart = (uint32)(currentByte - line.byteStart);
				emptyToken.debugAncestorStack = line.ancestors;
				emptyToken.style = theme.match(line.ancestors);

				line.tokens.emplace_back(emptyToken);
			}

			// Get the original start from the resume information
			size_t originalStart = line.patternStack[line.patternStack.size() - 1].originalStart;

			// NOTE: If we're returning early, it's because we've reached the end of the line, so return
			//       the end of line byte so the caller knows to stop parsing this line.
			return { originalStart, endBlockMatch->end };
		}
	}

	void ComplexSyntaxPattern::free()
	{
		if (begin)
		{
			onig_free(begin);
		}

		if (end.simpleRegex)
		{
			onig_free(end.simpleRegex);
		}

		if (beginCaptures.has_value())
		{
			beginCaptures->free();
		}

		if (endCaptures.has_value())
		{
			endCaptures->free();
		}

		if (patterns.has_value())
		{
			patterns->free();
		}

		begin = nullptr;
		end.simpleRegex = nullptr;
	}

	MatchSpan PatternArray::tryParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, Grammar const* self) const
	{
		int onigPatternMatched = -1;
		getFirstMatchInRegset(code, anchor, start, end, *this, &onigPatternMatched);

		// See if there's a $self pattern that we can try to match on
		bool selfPatternExists = this->firstSelfPatternArrayIndex != UINT64_MAX;
		uint64 selfPatternIndexMatch = UINT64_MAX;
		if (selfPatternExists)
		{
			int onigSelfPatternMatched = -1;
			getFirstMatchInRegset(code, anchor, start, end, self->patterns, &onigSelfPatternMatched);

			if (onigSelfPatternMatched != -1)
			{
				selfPatternIndexMatch = this->firstSelfPatternArrayIndex;
			}
		}

		bool useSelfPattern = false;
		if (onigPatternMatched != -1)
		{
			auto patternIndexIter = this->onigIndexMap.find(onigPatternMatched);
			if (patternIndexIter != this->onigIndexMap.end())
			{
				auto patternIter = self->globalPatternIndex.find(patternIndexIter->second);
				if (patternIter != self->globalPatternIndex.end())
				{
					if (patternIter->second->patternArrayIndex < selfPatternIndexMatch)
					{
						return patternIter->second->tryParse(line, code, theme, anchor, start, end, repo, region);
					}
					else
					{
						// If a $self matches and matches earlier in the patterns array, then it has
						// higher precedence and we should return the results of a self->match...
						useSelfPattern = true;
					}
				}
				else
				{
					g_logger_error("Invalid GID '{}' found for pattern in SimpleSyntaxPattern.", patternIndexIter->second);
				}
			}
		}

		if (useSelfPattern || (onigPatternMatched == -1 && selfPatternExists))
		{
			return self->patterns.tryParse(line, code, theme, anchor, start, end, repo, region, self);
		}

		return { start, start };
	}

	MatchSpan PatternArray::tryParseAll(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, Grammar const* self) const
	{
		size_t cursor = start;

		MatchSpan res = { start, start };
		bool setStart = false;

		while (cursor < end)
		{
			auto span = this->tryParse(line, code, theme, anchor, cursor, end, repo, region, self);
			if (!setStart)
			{
				setStart = true;
				res.matchStart = span.matchStart;
			}

			size_t newCursorPos = span.matchEnd;
			if (newCursorPos == start)
			{
				// If we hit the end of the line or the end anchor, stop parsing
				if (cursor + 1 >= line.byteStart + line.numBytes || cursor + 1 >= end)
				{
					cursor = line.byteStart + line.numBytes;
					break;
				}
				cursor++;
			}
			else
			{
				// If we hit the end of the line or the end anchor, stop parsing
				if (newCursorPos >= line.byteStart + line.numBytes || newCursorPos >= end)
				{
					return { res.matchStart, (size_t)CMath::min((uint32)newCursorPos, line.byteStart + line.numBytes) };
				}
				cursor = newCursorPos;
			}
		}

		return { res.matchStart, cursor };
	}

	void PatternArray::free()
	{
		for (auto& pattern : patterns)
		{
			freePattern(pattern);
		}
	}

	MatchSpan SyntaxPattern::tryParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t endOffset, const PatternRepository& repo, OnigRegion* region) const
	{
		switch (type)
		{
		case PatternType::Array:
		{
			if (patternArray.has_value())
			{
				return patternArray->tryParse(line, code, theme, anchor, start, endOffset, repo, region, this->self);
			}
		}
		break;
		case PatternType::Complex:
		{
			if (complexPattern.has_value())
			{
				return complexPattern->tryParse(line, code, theme, anchor, start, endOffset, repo, region, this->self, this->gid);
			}
		}
		break;
		case PatternType::Include:
		{
			if (patternInclude.has_value())
			{
				auto iter = repo.patterns.find(*patternInclude);
				if (iter != repo.patterns.end())
				{
					return iter->second->tryParse(line, code, theme, anchor, start, endOffset, repo, region);
				}
				// NOTE: $self means to include a self-reference to our own grammar
				else if (*patternInclude == "$self")
				{
					return this->self->patterns.tryParse(line, code, theme, anchor, start, endOffset, repo, region, self);

				}
				g_logger_warning("Unable to resolve pattern reference '{}'.", *patternInclude);
			}
		}
		break;
		case PatternType::Simple:
		{
			if (simplePattern.has_value())
			{
				return simplePattern->tryParse(line, code, theme, anchor, start, endOffset, repo, region, self);
			}
		}
		break;
		case PatternType::Invalid:
			break;
		}

		return { start, start };
	}

	void SyntaxPattern::free()
	{
		switch (type)
		{
		case PatternType::Simple:
		{
			if (simplePattern.has_value())
			{
				simplePattern->free();
			}
		}
		break;
		case PatternType::Complex:
		{
			if (complexPattern.has_value())
			{
				complexPattern->free();
			}
		}
		break;
		case PatternType::Array:
		{
			if (patternArray.has_value())
			{
				patternArray->free();
			}
		}
		break;
		case PatternType::Include:
		case PatternType::Invalid:
			break;
		}
	}

	std::vector<GrammarLineInfo>::iterator SourceGrammarTree::getLineForByte(size_t byte)
	{
		for (auto iter = sourceInfo.begin(); iter != sourceInfo.end(); iter++)
		{
			if (iter->byteStart <= byte && iter->byteStart + iter->numBytes > byte)
			{
				return iter;
			}
		}

		return sourceInfo.end();
	}

	std::vector<ScopedName> SourceGrammarTree::getAllAncestorScopesAtChar(size_t cursorPos) const
	{
		for (auto const& line : this->sourceInfo)
		{
			if (cursorPos >= line.byteStart && cursorPos <= line.byteStart + line.numBytes)
			{
				if (line.tokens.size() == 0)
				{
					return {};
				}

				size_t tokenIndex = line.tokens.size() - 1;
				for (size_t i = 1; i < line.tokens.size(); i++)
				{
					if ((line.tokens[i].relativeStart + line.byteStart) > cursorPos)
					{
						tokenIndex = i - 1;
						break;
					}
				}

				return line.tokens[tokenIndex].debugAncestorStack;
			}
		}

		return {};
	}

	std::string SourceGrammarTree::getMatchTextAtChar(size_t cursorPos) const
	{
		for (auto const& line : this->sourceInfo)
		{
			if (cursorPos >= line.byteStart && cursorPos <= line.byteStart + line.numBytes)
			{
				if (line.tokens.size() == 0)
				{
					return "";
				}

				size_t tokenIndex = line.tokens.size() - 1;
				for (size_t i = 1; i < line.tokens.size(); i++)
				{
					if ((line.tokens[i].relativeStart + line.byteStart) > cursorPos)
					{
						tokenIndex = i - 1;
						break;
					}
				}

				size_t tokenStartByte = line.tokens[tokenIndex].relativeStart + line.byteStart;
				size_t tokenEndByte = tokenIndex == line.tokens.size() - 1
					? line.byteStart + line.numBytes
					: line.tokens[tokenIndex + 1].relativeStart + line.byteStart;
				return std::string(codeBlock + tokenStartByte, tokenEndByte - tokenStartByte);
			}
		}

		return "";
	}

	//static bool checkBufferUnderflow(size_t sizeLeft, size_t numBytesToRemove)
	//{
	//	if (sizeLeft >= numBytesToRemove)
	//	{
	//		return false;
	//	}

	//	g_logger_error("We have a buffer underflow. Please pass a larger buffer to the tree.");
	//	return true;
	//}

	std::string SourceGrammarTree::getStringifiedTree(Grammar const& grammar, size_t bufferSize) const
	{
		std::string res = "";
		res.reserve(bufferSize);

		ScopedName const& rootScope = grammar.scope;
		std::vector<ScopedName> ancestorStack = {};

		res += "<" + rootScope.getFriendlyName() + ">";
		res += '\n';

		bool isFirstLineAndToken = true;
		for (auto const& line : this->sourceInfo)
		{
			for (size_t tokenIndex = 0; tokenIndex < line.tokens.size(); tokenIndex++)
			{
				auto const& token = line.tokens[tokenIndex];
				bool lastTokenClosed = false;

				// Diff this stack with the current one we're using

				// First make sure the ancestor stack isn't bigger then this token stack
				if (ancestorStack.size() > token.debugAncestorStack.size())
				{
					if (!isFirstLineAndToken)
					{
						// Close off the quote for the last token
						res += "'\n";
						lastTokenClosed = true;
					}

					// Pop any excess contexts off the stack
					size_t ancestor = ancestorStack.size() - 1;
					do
					{
						res += std::string((ancestor + 1) * 2, ' ');
						res += "</" + ancestorStack[ancestor].getFriendlyName() + ">";
						res += '\n';
						ancestorStack.pop_back();
						if (ancestor == 0)
						{
							break;
						}
						ancestor--;
					} while (ancestor >= token.debugAncestorStack.size());
				}

				// Now:
				//   ancestorStack.size() <= token.debugAncestorStack.size()

				// Next, figure out where (if at all) the stacks differ
				size_t stackDifferPoint = ancestorStack.size();
				for (size_t ancestor = 0; ancestor < ancestorStack.size(); ancestor++)
				{
					if (!ancestorStack[ancestor].strictEquals(token.debugAncestorStack[ancestor]))
					{
						stackDifferPoint = ancestor;
						break;
					}
				}

				// Then pop all different ancestors
				if (ancestorStack.size() > 0 && stackDifferPoint != ancestorStack.size())
				{
					if (!lastTokenClosed && !isFirstLineAndToken)
					{
						// Close off the quote for the last token
						res += "'\n";
						lastTokenClosed = true;
					}

					size_t ancestor = ancestorStack.size() - 1;
					do
					{
						res += std::string((ancestor + 1) * 2, ' ');
						res += "</" + ancestorStack[ancestor].getFriendlyName() + ">";
						res += '\n';
						ancestorStack.pop_back();
						if (ancestor > 0)
						{
							ancestor--;
						}
					} while (ancestor > stackDifferPoint);
				}

				// Then push any contexts that need to be pushed
				for (size_t ancestor = ancestorStack.size(); ancestor < token.debugAncestorStack.size(); ancestor++)
				{
					if (!lastTokenClosed && !isFirstLineAndToken)
					{
						// Close off the quote for the last token
						res += "'\n";
						lastTokenClosed = true;
					}

					res += std::string((ancestor + 1) * 2, ' ');
					res += "<" + token.debugAncestorStack[ancestor].getFriendlyName() + ">";
					res += '\n';
					ancestorStack.push_back(token.debugAncestorStack[ancestor]);
				}

				// Finally... print the token text
				size_t tokenByteEnd = line.byteStart + line.numBytes;
				if (tokenIndex < line.tokens.size() - 1)
				{
					tokenByteEnd = line.tokens[tokenIndex + 1].relativeStart + line.byteStart;
				}

				if (lastTokenClosed || isFirstLineAndToken)
				{
					res += std::string((ancestorStack.size() + 1) * 2, ' ');
					res += "'";
				}

				// Sanitize the string, render any special characters like '\n' escaped
				std::string sanitizedString = std::string(codeBlock + token.relativeStart + line.byteStart, tokenByteEnd - (token.relativeStart + line.byteStart));
				for (size_t i = 0; i < sanitizedString.length(); i++)
				{
					if (sanitizedString[i] == '\n')
					{
						sanitizedString = sanitizedString.substr(0, i) + "\\n" + sanitizedString.substr(i + 1);
						i++;
					}
					else if (sanitizedString[i] == '\t')
					{
						sanitizedString = sanitizedString.substr(0, i) + "\\t" + sanitizedString.substr(i + 1);
						i++;
					}
					else if (sanitizedString[i] == '\\')
					{
						sanitizedString = sanitizedString.substr(0, i) + "\\\\" + sanitizedString.substr(i + 1);
						i++;
					}
					else if (sanitizedString[i] == '\r')
					{
						sanitizedString = sanitizedString.substr(0, i) + "\\r" + sanitizedString.substr(i + 1);
						i++;
					}
					else if (sanitizedString[i] == '\'')
					{
						sanitizedString = sanitizedString.substr(0, i) + "\\'" + sanitizedString.substr(i + 1);
						i++;
					}
				}

				res += sanitizedString;

				isFirstLineAndToken = false;
			}
		}

		// Close off last token's quotes
		res += "'\n";

		// Pop off remaining ancestors
		if (ancestorStack.size() > 0)
		{
			size_t ancestor = ancestorStack.size() - 1;
			while (!ancestorStack.empty())
			{
				res += std::string((ancestor + 1) * 2, ' ');
				res += "</" + ancestorStack[ancestor].getFriendlyName() + ">\n";
				ancestorStack.pop_back();
				if (ancestor > 0)
				{
					ancestor--;
				}
			}
		}

		res += "</" + rootScope.getFriendlyName() + ">";
		res += '\n';

		return res;
	}

	// NOTE: Some help on how this tree should end up looking
	// 
	// For the source code looking like this (and the rules defined here https://macromates.com/blog/2005/introduction-to-scopes/#htmlxml-analogy):
	// 
	//   `char const* str = "Hello world\n";`
	// 
	// The resulting tree would look like this:
	// 
	// 0: c_source { next: 18, parent: 0, start: 0, size: 34 },
	//    1: storage: { next: 2, parent: -1, start: 0, size: 4 },
	//       2: ATOM: { next: 1, parent: -1, start: 0, size: 4 },         `char`
	//    3: ATOM: { next: 1, parent: -3, start: 4, size: 1 },            ` `
	//    4: modifier: { next: 2, parent: -4, start: 5, size: 5 },
	//       5: ATOM: { next: 1, parent: -1, start: 0, size: 5 },         `const`
	//    6: operator: { next: 2, parent: -6, start: 10, size: 1 },
	//       7: ATOM: { next: 1, parent: -1, start: 0, size: 1 },         `*`
	//    8: ATOM: { next: 1, parent: -8, start: 11, size: 5 },           ` str `
	//    9: operator: { next: 2, parent: -9, start: 16, size: 1 },
	//       10: ATOM: { next: 1, parent: -1, start: 0, size: 1 },        `=`
	//    11: ATOM: { next: 1, parent: -11, start: 17, size: 1 },         ` `
	//    12: string: { next: 5, parent: -12, start: 18, size: 15 },
	//       13: ATOM: { next: 1, parent: -1, start: 0, size: 12 },       `"Hello world`
	//       14: constant: { next: 2, parent: -2, start: 12, size: 2 },
	//          15: ATOM: { next: 1, parent: -1, start: 0, size: 2 },     `\n`
	//       16: ATOM: { next: 1, parent: -4, start: 14, size: 1 },       `"`
	//    17: ATOM: { next: 1, parent: -17, start: 33, size: 1 },         `;`
	// 18: END

	SourceGrammarTree Grammar::initCodeBlock(const char* code, size_t codeLength) const
	{
		SourceGrammarTree res = {};
		res.codeBlock = code;
		res.codeLength = codeLength;

		// Initialize each line stack
		size_t lineStart = 0;
		// Used for validation
		size_t lineCounter = 0;
		for (size_t i = 0; i < res.codeLength; i++)
		{
			if (code[i] == '\n')
			{
				GrammarLineInfo info = {};
				info.byteStart = (uint32)lineStart;
				info.numBytes = (uint32)(i - lineStart + 1);
				g_logger_assert(lineCounter == res.sourceInfo.size(), "Somehow we pushed an invalid line to the stack.");
				res.sourceInfo.emplace_back(info);
				lineCounter++;

				lineStart = i + 1;
			}
		}

		if (lineStart < res.codeLength)
		{
			GrammarLineInfo info = {};
			info.byteStart = (uint32)lineStart;
			info.numBytes = res.codeLength == 0 ? 0 : (uint32)(res.codeLength - info.byteStart);
			res.sourceInfo.emplace_back(info);
		}

		return res;
	}

	size_t Grammar::updateFromByte(SourceGrammarTree& tree, SyntaxTheme const& theme, uint32_t byteOffset, uint32_t maxNumLinesToUpdate) const
	{
		size_t numLinesUpdated = 1;
		size_t currentLineInfoIndex = tree.sourceInfo.size();
		while (numLinesUpdated <= maxNumLinesToUpdate)
		{
			std::vector<GrammarResumeParseInfo> oldPatternStack = {};

			// TODO: Optimization technique, replace this with a binary search
			GrammarLineInfo* lineInfo = nullptr;

			// First find the line that we're updating
			{
				if (currentLineInfoIndex == tree.sourceInfo.size())
				{
					for (size_t i = 0; i < tree.sourceInfo.size(); i++)
					{
						auto& info = tree.sourceInfo[i];
						if (byteOffset >= info.byteStart && byteOffset < info.byteStart + info.numBytes)
						{
							currentLineInfoIndex = i;
						}
					}
				}

				if (currentLineInfoIndex < tree.sourceInfo.size())
				{
					lineInfo = &tree.sourceInfo[currentLineInfoIndex];
				}
				else
				{
					return numLinesUpdated;
				}

				GrammarLineInfo* prevLineInfo = nullptr;
				if (currentLineInfoIndex > 0 && currentLineInfoIndex <= tree.sourceInfo.size())
				{
					prevLineInfo = &tree.sourceInfo[currentLineInfoIndex - 1];
				}

				// Keep track of the old pattern stack so we know if we need to continue parsing the next line
				oldPatternStack = lineInfo->patternStack;

				// Reset this line's info to all the previous lines stuff since it will carry forward from there
				if (prevLineInfo)
				{
					lineInfo->ancestors = prevLineInfo->ancestors;
					lineInfo->patternStack = prevLineInfo->patternStack;
					lineInfo->tokens = {};
				}
				else
				{
					lineInfo->ancestors = {};
					lineInfo->patternStack = {};
					lineInfo->tokens = {};
				}
			}

			// If the offset was greater than the source code length, then we've hit the end of the file
			if (lineInfo == nullptr)
			{
				return numLinesUpdated;
			}

			lineInfo->needsToBeUpdated = false;

			size_t start = lineInfo->byteStart;
			while (start < lineInfo->byteStart + lineInfo->numBytes)
			{
				// First figure find out which pattern we were last parsing with. If we find a pattern on the line's pattern stack
				// then we'll resume parsing this line using that pattern.
				// Otherwise, we'll resume parsing this line with the base Grammar.
				SyntaxPattern const* pattern = nullptr;
				if (lineInfo->patternStack.size() > 0)
				{
					size_t lastPatternGid = lineInfo->patternStack.size() - 1;
					if (auto patternIter = this->globalPatternIndex.find(lineInfo->patternStack[lastPatternGid].gid);
						patternIter != this->globalPatternIndex.end())
					{
						pattern = patternIter->second;
					}
					else
					{
						g_logger_error("Somehow ended up with a pattern gid '{}' which was invalid.", lineInfo->patternStack[lastPatternGid].gid);
					}
				}

				// Next, resume parsing. If we find more matches and we're not at the end of the line yet, then
				// continue parsing starting from the end of the final match. Otherwise, we'll stop parsing.

				// TODO: Way too many parameters, compact into a helper struct
				if (pattern)
				{
					g_logger_assert(pattern->type == PatternType::Complex, "Cannot resume parsing except from complex patterns.");
					g_logger_assert(pattern->complexPattern.has_value(), "Invalid complex pattern encountered.");

					auto const& resumeInfo = lineInfo->patternStack[lineInfo->patternStack.size() - 1];

					auto span = pattern->complexPattern->resumeParse(
						*lineInfo,
						tree.codeBlock,
						theme,
						start,
						resumeInfo.dynamicEndPatternStr,
						resumeInfo.anchor,
						start,
						lineInfo->byteStart + lineInfo->numBytes,
						this->repository,
						region,
						this
					);

					size_t newCursorPos = span.matchEnd;
					if (newCursorPos == start || newCursorPos >= lineInfo->byteStart + lineInfo->numBytes)
					{
						// TODO: This duplicates a block slightly below, see if they should be combined

						// Found all the matches for this line, we can stop parsing now
						// If no matches were found, just add an empty token so we have comprehensive coverage with all tokens
						if (lineInfo->tokens.size() == 0)
						{
							SourceSyntaxToken emptyToken = {};
							emptyToken.relativeStart = 0;
							emptyToken.style = theme.match(lineInfo->ancestors);
							lineInfo->tokens.emplace_back(emptyToken);
							newCursorPos = lineInfo->byteStart + lineInfo->numBytes;
						}

						// Likewise, if we didn't find a match and we still haven't reached the end of the line,
						// then push an empty token with the current ancestors
						if (newCursorPos < lineInfo->byteStart + lineInfo->numBytes)
						{
							SourceSyntaxToken emptyToken = {};
							emptyToken.relativeStart = (uint32)(newCursorPos - lineInfo->byteStart);
							emptyToken.style = theme.match(lineInfo->ancestors);
							lineInfo->tokens.emplace_back(emptyToken);
						}
						break;
					}

					start = newCursorPos;
				}
				else
				{
					auto span = this->patterns.tryParse(
						*lineInfo,
						tree.codeBlock,
						theme,
						lineInfo->byteStart,
						start,
						lineInfo->byteStart + lineInfo->numBytes,
						this->repository,
						region,
						this
					);

					size_t newCursorPos = span.matchEnd;
					if (newCursorPos == start || newCursorPos >= lineInfo->byteStart + lineInfo->numBytes)
					{
						// Found all the matches for this line, we can stop parsing now
						// If no matches were found, just add an empty token so we have comprehensive coverage with all tokens
						if (lineInfo->tokens.size() == 0)
						{
							SourceSyntaxToken emptyToken = {};
							emptyToken.relativeStart = 0;
							emptyToken.style = theme.match(lineInfo->ancestors);
							lineInfo->tokens.emplace_back(emptyToken);
							newCursorPos = lineInfo->byteStart + lineInfo->numBytes;
						}

						// Likewise, if we didn't find a match and we still haven't reached the end of the line,
						// then push an empty token with the current ancestors
						if (newCursorPos < lineInfo->byteStart + lineInfo->numBytes)
						{
							SourceSyntaxToken emptyToken = {};
							emptyToken.relativeStart = (uint32)(newCursorPos - lineInfo->byteStart);
							emptyToken.style = theme.match(lineInfo->ancestors);
							lineInfo->tokens.emplace_back(emptyToken);
						}
						break;
					}

					start = newCursorPos;
				}
			}

			// If the pattern stack for this line didn't change, then we don't need to parse any more lines
			if (lineInfo->patternStack == oldPatternStack)
			{
				return numLinesUpdated;
			}

			byteOffset = lineInfo->byteStart + lineInfo->numBytes;
			numLinesUpdated++;
			currentLineInfoIndex++;
		}

		// If we get to here, mark the next line as needing an update just in case.
		// That way the caller can pick up parsing again if needed when they scroll or something
		if (currentLineInfoIndex < tree.sourceInfo.size())
		{
			tree.sourceInfo[currentLineInfoIndex].needsToBeUpdated = true;
		}

		return numLinesUpdated - 1;
	}

	SourceGrammarTree Grammar::parseCodeBlock(const char* code, size_t codeLength, SyntaxTheme const& theme) const
	{
		SourceGrammarTree res = initCodeBlock(code, codeLength);

		size_t currentLine = 1;
		while (currentLine <= res.sourceInfo.size())
		{
			currentLine += updateFromByte(res, theme, res.sourceInfo[currentLine - 1].byteStart);
		}

		return res;
	}

	Grammar* Grammar::importGrammar(const char* filepath)
	{
		if (!Platform::fileExists(filepath))
		{
			g_logger_warning("Tried to parse grammar at filepath that does not exist: '{}'", filepath);
			return nullptr;
		}

		std::ifstream file(filepath);

		try
		{
			json j = json::parse(file, nullptr, true, true);
			return importGrammarFromJson(j);
		}
		catch (json::exception& ex)
		{
			g_logger_error("Error importing language grammar: '{}'\n\t'{}'", filepath, ex.what());
		}

		return nullptr;
	}

	void Grammar::free(Grammar* grammar)
	{
		if (grammar)
		{
			grammar->patterns.free();
			for (auto [k, v] : grammar->repository.patterns)
			{
				freePattern(v);
			}
			grammar->repository.patterns = {};

			if (grammar->region)
			{
				onig_region_free(grammar->region, 1);
			}
			grammar->region = nullptr;

			g_memory_delete(grammar);
		}
	}

	// ----------- Internal Functions -----------
	static Grammar* importGrammarFromJson(const json& j)
	{
		Grammar* res = g_memory_new Grammar();

		res->region = onig_region_new();

		if (j.contains("name"))
		{
			res->name = j["name"];
		}

		if (j.contains("scopeName"))
		{
			const std::string& scope = j["scopeName"];
			res->scope = ScopedName::from(scope);
		}

		res->repository = {};
		if (j.contains("repository"))
		{
			for (auto& [key, val] : j["repository"].items())
			{
				SyntaxPattern* const pattern = parsePattern(val, res);
				if (pattern->type != PatternType::Invalid)
				{
					res->repository.patterns[key] = pattern;
				}
				else
				{
					g_logger_warning("Invalid pattern parsed.");
					freePattern(pattern);
				}
			}
		}

		if (j.contains("patterns"))
		{
			const json& patternsArray = j["patterns"];
			res->patterns = parsePatternsArray(patternsArray, res);
		}

		// Create the regsets/regexes from the parsed patterns
		{
			constructRegsetFromPatterns(res->patterns, res);

			for (auto& pattern : res->repository.patterns)
			{
				constructRegexesFromPattern(*pattern.second, res);
			}
		}

		return res;
	}

	static SyntaxPattern* const parsePattern(const json& json, Grammar* self)
	{
		SyntaxPattern* const res = g_memory_new SyntaxPattern();
		res->type = PatternType::Invalid;
		res->self = self;

		// Include patterns are the easiest... So do those first
		if (json.contains("include"))
		{
			std::string includeMatch = json["include"];
			if (includeMatch.length() > 0 && includeMatch[0] == '#')
			{
				includeMatch = includeMatch.substr(1);
			}

			res->type = PatternType::Include;
			res->patternInclude.emplace(includeMatch);
		}
		// match is mutually exclusive with begin/end. So this is a simple pattern.
		else if (json.contains("match"))
		{
			res->type = PatternType::Simple;

			SimpleSyntaxPattern p = {};
			p.regMatch = onigFromString(json["match"], false);

			if (json.contains("name"))
			{
				p.scope = ScopedName::from(json["name"]);
			}
			else
			{
				p.scope = std::nullopt;
			}

			if (json.contains("captures"))
			{
				p.captures = CaptureList::from(json["captures"], self);
			}
			else
			{
				p.captures = std::nullopt;
			}

			res->simplePattern.emplace(p);
		}
		// begin/end is mutually exclusive with match. So this is a complex pattern.
		else if (json.contains("begin"))
		{
			if (!json.contains("end"))
			{
				g_logger_error("Pattern '{}' has invalid complex pattern. Pattern has begin, but no end.", "UNKNOWN");
				return res;
			}

			res->type = PatternType::Complex;

			ComplexSyntaxPattern c = {};
			c.begin = onigFromString(json["begin"], false);

			if (!c.begin)
			{
				return res;
			}

			// Check if this end pattern has a dynamic backreferences and pre-process it if it does
			std::string const& endPattern = json["end"];
			{
				int numBeginCaptures = onig_number_of_captures(c.begin);
				DynamicRegexCapture capture = {};
				int digitStart = 0;
				bool parsingDigit = false;
				for (int i = 0; i < (int)endPattern.size(); i++)
				{
					if (i < (int)endPattern.size() - 1 && endPattern[i] == '\\' && Parser::isDigit(endPattern[i + 1]))
					{
						c.end.isDynamic = true;
						digitStart = i + 1;
						parsingDigit = true;
					}
					else if (parsingDigit && !Parser::isDigit(endPattern[i]))
					{
						std::string substr = endPattern.substr(digitStart, i - digitStart);
						capture.captureIndex = std::atoi(substr.c_str());

						if (capture.captureIndex < 0 || capture.captureIndex > numBeginCaptures)
						{
							g_logger_error("Invalid backreference in regex '{}'. Cannot backreference '{}', only '{}' captures in the begin block.", endPattern, capture.captureIndex, numBeginCaptures);
							onig_free(c.begin);
							return res;
						}

						capture.strReplaceStart = digitStart - 1;
						capture.strReplaceEnd = i - 1;

						c.end.backrefs.push_back(capture);
						parsingDigit = false;
					}
					else if (parsingDigit && i == (int)endPattern.size() - 1)
					{
						std::string substr = endPattern.substr(digitStart, i - digitStart + 1);
						capture.captureIndex = std::atoi(substr.c_str());

						if (capture.captureIndex < 0 || capture.captureIndex > numBeginCaptures)
						{
							g_logger_error("Invalid backreference in regex '{}'. Cannot backreference '{}', only '{}' captures in the begin block.", endPattern, capture.captureIndex, numBeginCaptures);
							onig_free(c.begin);
							return res;
						}

						capture.strReplaceStart = digitStart - 1;
						capture.strReplaceEnd = i;

						c.end.backrefs.push_back(capture);
						parsingDigit = false;
					}
				}
			}

			if (!c.end.isDynamic)
			{
				c.end.simpleRegex = onigFromString(json["end"], true);

				if (!c.end.simpleRegex)
				{
					onig_free(c.begin);
					return res;
				}
			}
			else
			{
				c.end.regexText = json["end"];
			}

			if (json.contains("name"))
			{
				c.scope = ScopedName::from(json["name"]);
			}
			else
			{
				c.scope = std::nullopt;
			}

			// Using captures key is shorthand for begin/end captures both using this ruleset.
			// See here https://macromates.com/manual/en/language_grammars#rule_keys
			if (json.contains("captures"))
			{
				CaptureList captureList = CaptureList::from(json["captures"], self);
				c.beginCaptures = captureList;
				c.endCaptures = captureList;
			}
			else
			{
				c.beginCaptures = std::nullopt;
				c.endCaptures = std::nullopt;
			}

			// NOTE: I'm not completely sure, but I feel like if you specify both captures
			//       and a begin/end captures, then the begin/end captures should take precedence.
			//       So that's what happens here.
			if (json.contains("beginCaptures"))
			{
				c.beginCaptures = CaptureList::from(json["beginCaptures"], self);
			}

			if (json.contains("endCaptures"))
			{
				c.endCaptures = CaptureList::from(json["endCaptures"], self);
			}

			if (json.contains("patterns"))
			{
				c.patterns = parsePatternsArray(json["patterns"], self);
			}
			else
			{
				c.patterns = std::nullopt;
			}

			res->complexPattern.emplace(c);
		}
		// If this pattern only contains a patterns array then return
		// that
		else if (json.contains("patterns"))
		{
			res->type = PatternType::Array;
			res->patternArray.emplace(parsePatternsArray(json["patterns"], self));
		}

		// Add GID and add the pattern to global index lookup
		if (res->type != PatternType::Invalid)
		{
			GrammarPatternGid gid = self->gidCounter++;
			res->gid = gid;
			self->globalPatternIndex.emplace(gid, res);
		}

		return res;
	}

	static PatternArray parsePatternsArray(const json& json, Grammar* self)
	{
		PatternArray res = {};

		for (json::const_iterator it = json.begin(); it != json.end(); it++)
		{
			SyntaxPattern* const pattern = parsePattern(*it, self);
			if (pattern->type != PatternType::Invalid)
			{
				res.patterns.emplace_back(pattern);
			}
			else
			{
				g_logger_warning("Invalid pattern parsed.");
				freePattern(pattern);
			}
		}

		return res;
	}

	// --- Construct Regset Helpers ---

	static void addPatternToRegset(OnigRegSet* regset, PatternArray& ogPatternArray, SyntaxPattern& pattern, const Grammar* self, uint64 patternArrayIndex)
	{
		int addRegToSetErr = ONIG_NORMAL;
		bool addedRegex = true;
		size_t onigIndex = (size_t)onig_regset_number_of_regex(regset);

		pattern.patternArrayIndex = patternArrayIndex;

		switch (pattern.type)
		{
		case PatternType::Simple:
			if (pattern.simplePattern.has_value())
			{
				addRegToSetErr = onig_regset_add(regset, pattern.simplePattern->regMatch);
			}
			break;
		case PatternType::Complex:
			if (pattern.complexPattern.has_value())
			{
				addRegToSetErr = onig_regset_add(regset, pattern.complexPattern->begin);
			}
			break;
		case PatternType::Include:
			if (pattern.patternInclude.has_value())
			{
				const auto& includeName = *pattern.patternInclude;
				auto iter = self->repository.patterns.find(includeName);
				if (iter != self->repository.patterns.end())
				{
					addPatternToRegset(regset, ogPatternArray, *iter->second, self, patternArrayIndex);
				}
				else if (includeName == "$self")
				{
					if (ogPatternArray.firstSelfPatternArrayIndex > patternArrayIndex)
					{
						ogPatternArray.firstSelfPatternArrayIndex = patternArrayIndex;
					}
				}
			}
			break;
		case PatternType::Array:
			// NOTE: This will only be used when flattening an include pattern array
			if (pattern.patternArray.has_value())
			{
				for (const auto& subPattern : pattern.patternArray->patterns)
				{
					addPatternToRegset(regset, ogPatternArray, *subPattern, self, patternArrayIndex);
				}

				addedRegex = false;
			}
			break;
		case PatternType::Invalid:
			break;
		}

		if (addRegToSetErr != ONIG_NORMAL)
		{
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str((UChar*)s, addRegToSetErr);
			g_logger_error("Oniguruma Regset::Add Error: '{}'", &s[0]);
		}
		else if (addedRegex)
		{
			ogPatternArray.onigIndexMap[onigIndex] = pattern.gid;
		}
	}

	static void constructRegexesFromPattern(SyntaxPattern& pattern, Grammar* self)
	{
		switch (pattern.type)
		{
		case PatternType::Array:
			if (pattern.patternArray.has_value())
			{
				constructRegsetFromPatterns(*pattern.patternArray, self);
			}
			break;
		case PatternType::Simple:
			if (pattern.simplePattern.has_value() && pattern.simplePattern->captures.has_value())
			{
				for (auto& capture : pattern.simplePattern->captures.value().captures)
				{
					if (capture.patternArray.has_value())
					{
						constructRegsetFromPatterns(*capture.patternArray, self);
					}
				}
			}
			break;
		case PatternType::Complex:
			if (pattern.complexPattern.has_value())
			{
				if (pattern.complexPattern->beginCaptures.has_value())
				{
					for (auto& capture : pattern.complexPattern->beginCaptures->captures)
					{
						if (capture.patternArray.has_value())
						{
							constructRegsetFromPatterns(*capture.patternArray, self);
						}
					}
				}

				if (pattern.complexPattern->endCaptures.has_value())
				{
					for (auto& capture : pattern.complexPattern->endCaptures->captures)
					{
						if (capture.patternArray.has_value())
						{
							constructRegsetFromPatterns(*capture.patternArray, self);
						}
					}
				}

				if (pattern.complexPattern->patterns.has_value())
				{
					constructRegsetFromPatterns(*pattern.complexPattern->patterns, self);
				}
			}
			break;
		case PatternType::Include:
		case PatternType::Invalid:
			break;
		}
	}

	static void constructRegsetFromPatterns(PatternArray& patternArray, Grammar* self)
	{
		// Create regset that contains all patterns/include_patterns in this array
		int parseRes = onig_regset_new(&patternArray.regset, 0, NULL);
		if (parseRes != ONIG_NORMAL)
		{
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str((UChar*)s, parseRes);
			g_logger_error("Oniguruma Regset::Create Error: '{}'", &s[0]);
			return;
		}

		patternArray.firstSelfPatternArrayIndex = UINT64_MAX;

		// Add each pattern to the regset
		uint64 patternArrayIndex = 0;
		for (const auto& pattern : patternArray.patterns)
		{
			addPatternToRegset(patternArray.regset, patternArray, *pattern, self, patternArrayIndex);
			patternArrayIndex++;

			// Also construct any regexes recursively
			constructRegexesFromPattern(*pattern, self);
		}
	}

	static OnigRegex onigFromString(const std::string& str, bool multiLine)
	{
		const char* pattern = str.c_str();
		const char* patternEnd = pattern + str.length();

		int options = ONIG_OPTION_NONE;
		options |= ONIG_OPTION_CAPTURE_GROUP;
		if (multiLine)
		{
			options |= ONIG_OPTION_MULTILINE;
		}

		// Enable capture history for Oniguruma
		OnigSyntaxType syn;
		onig_copy_syntax(&syn, ONIG_SYNTAX_DEFAULT);
		onig_set_syntax_op2(&syn,
			onig_get_syntax_op2(&syn) | ONIG_SYN_OP2_ATMARK_CAPTURE_HISTORY);

		OnigRegex reg;
		OnigErrorInfo error;
		int parseRes = onig_new(
			&reg,
			(uint8*)pattern,
			(uint8*)patternEnd,
			options,
			ONIG_ENCODING_ASCII,
			&syn,
			&error
		);

		if (parseRes != ONIG_NORMAL)
		{
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str((UChar*)s, parseRes, &error);
			g_logger_error("Oniguruma Error: '{}'", &s[0]);
			return nullptr;
		}

		return reg;
	}

	static int getFirstValidMatchInRange(const std::string& str, size_t anchor, size_t startOffset, size_t endOffset, OnigRegex reg, OnigRegion* region)
	{
		g_logger_assert(startOffset <= str.size(), "Invalid startOffset: startOffset > str.size(): '{}' > '{}'", startOffset, str.size());
		g_logger_assert(endOffset <= str.size(), "Invalid endOffset: endOffset > str.size(): '{}' > '{}'", endOffset, str.size());

		const char* targetStr = str.c_str();
		const char* targetStrEnd = targetStr + str.length();

		const char* searchEnd = targetStr + endOffset;

		int searchRes = ONIG_MISMATCH;

		// Find the first result that's within our search range
		while (true)
		{
			g_logger_assert(anchor <= str.size(), "Invalid anchor: anchor > str.size(): '{}' > '{}'", anchor, str.size());
			const char* searchStart = targetStr + anchor;

			bool startIsAnchorPos = anchor == startOffset;

			onig_region_clear(region);
			searchRes = onig_search(
				reg,
				(uint8*)targetStr,
				(uint8*)targetStrEnd,
				(uint8*)searchStart,
				(uint8*)searchEnd,
				region,
				startIsAnchorPos
				? ONIG_OPTION_NONE
				: ONIG_OPTION_NOT_BEGIN_POSITION
			);

			if (searchRes >= 0 && region->num_regs > 0)
			{
				if (region->beg[0] >= 0 && region->end[0] >= region->beg[0])
				{
					if (region->beg[0] >= startOffset)
					{
						// This match is good
						break;
					}
					else if (region->end[0] <= startOffset && anchor != (size_t)region->end[0])
					{
						// This match is bad, but there might be a good match later on,
						// increase the startOffset if we can
						anchor = (size_t)region->end[0];
					}
					else if (anchor != startOffset)
					{
						// No good matches, try from the actual startOffset
						anchor = startOffset;
					}
					else
					{
						// No good matches
						break;
					}
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}

		return searchRes;
	}

	static ScopedName getScopeWithCaptures(const std::string& str, const ScopedName& originalScope, const OnigRegion* region)
	{
		ScopedName res = originalScope;

		for (auto& dotSeparatedScope : res.dotSeparatedScopes)
		{
			// Assign the capture if necessary
			if (dotSeparatedScope.capture.has_value() &&
				dotSeparatedScope.capture->captureIndex < region->num_regs &&
				dotSeparatedScope.capture->captureIndex >= 0)
			{
				int captureIndex = dotSeparatedScope.capture->captureIndex;
				if (region->beg[captureIndex] >= 0 && region->end[captureIndex] > region->beg[captureIndex])
				{
					auto& captureRegex = *dotSeparatedScope.capture;
					size_t captureStart = region->beg[captureIndex];
					size_t captureLength = region->end[captureIndex] - captureStart;

					std::string resultString = captureRegex.capture.substr(0, captureRegex.captureReplaceStart);
					resultString += str.substr(captureStart, captureLength);
					resultString += captureRegex.captureRegex.substr(captureRegex.captureReplaceEnd);

					dotSeparatedScope.capture->capture = resultString;
				}
			}
		}

		return res;
	}

	static void getFirstMatchInRegset(const std::string& str, size_t anchor, size_t startOffset, size_t endOffset, const PatternArray& pattern, int* patternMatched)
	{
		const char* targetStr = str.c_str();
		const char* targetStrEnd = targetStr + str.length();

		const char* searchStart = targetStr + startOffset;
		const char* searchEnd = targetStr + endOffset;

		bool startIsAnchorPos = anchor == startOffset;

		int strMatchPos;
		int searchRes = onig_regset_search(
			pattern.regset,
			(uint8*)targetStr,
			(uint8*)targetStrEnd,
			(uint8*)searchStart,
			(uint8*)searchEnd,
			ONIG_REGSET_POSITION_LEAD,
			startIsAnchorPos
			? ONIG_OPTION_NONE
			: ONIG_OPTION_NOT_BEGIN_POSITION,
			&strMatchPos
		);

		if (searchRes != ONIG_MISMATCH && searchRes < 0)
		{
			// Error
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str((UChar*)s, searchRes);
			g_logger_error("Oniguruma Error: '{}'", &s[0]);
			*patternMatched = -1;
		}
		else
		{
			*patternMatched = searchRes;
		}
	}

	static size_t pushMatchesToLineWithParent(GrammarLineInfo& line, GrammarMatchV2 const& parent, std::vector<GrammarMatchV2> const& subMatches, SyntaxTheme const& theme, size_t currentByte)
	{
		if (subMatches.size() == 0 || (subMatches.size() > 0 && subMatches[0].start > parent.start))
		{
			// Don't push 0-sized matches
			if (currentByte != parent.end)
			{
				// Push the style for the currentByte up to this point
				SourceSyntaxToken token = {};
				token.relativeStart = (uint32)(currentByte - line.byteStart);
				token.style = theme.match(line.ancestors);

				// TODO: OPTIMIZE: Profile how expensive this is and consider moving to debug only
				token.debugAncestorStack = line.ancestors;

				line.tokens.emplace_back(token);
			}

			if (subMatches.size() > 0)
			{
				currentByte = subMatches[0].start;
			}
			else
			{
				return parent.end;
			}
		}

		currentByte = pushMatchesToLine(line, subMatches, theme, currentByte);

		if (currentByte < parent.end && parent.scope.has_value())
		{
			// Push a token to fill in the gap
			SourceSyntaxToken token = {};
			token.debugAncestorStack = line.ancestors;
			token.relativeStart = (uint32)(currentByte - line.byteStart);
			token.style = theme.match(line.ancestors);

			line.tokens.emplace_back(token);

			currentByte = parent.end;
		}

		return currentByte;
	}

	static size_t pushMatchesToLine(GrammarLineInfo& line, std::vector<GrammarMatchV2> const& subMatches, SyntaxTheme const& theme, size_t currentByte)
	{
		size_t maxEndPosition = currentByte;

		size_t originalStackSize = line.ancestors.size();
		std::vector<size_t> ancestorByteEndStack = {};

		// Figure out what the styles should be for all this crap
		size_t lastSubMatchStart = subMatches.size() > 0 ? subMatches[0].start : 0;
		for (size_t i = 0; i < subMatches.size(); i++)
		{
			auto const& currentSubMatch = subMatches[i];
			g_logger_assert(currentSubMatch.start >= lastSubMatchStart, "Sub-Matches should have start points in strictly increasing order.");
			lastSubMatchStart = currentSubMatch.start;

			if (!currentSubMatch.scope.has_value())
			{
				g_logger_assert(false, "How did this happen?");
				continue;
			}

			// Fill in the gap
			if (currentSubMatch.start > currentByte)
			{
				// Push the style for the currentByte up to this point, without this sub-match's ancestor
				SourceSyntaxToken token = {};
				token.relativeStart = (uint32)(currentByte - line.byteStart);
				token.style = theme.match(line.ancestors);

				// TODO: OPTIMIZE: Profile how expensive this is and consider moving to debug only
				token.debugAncestorStack = line.ancestors;

				line.tokens.emplace_back(token);

				currentByte = currentSubMatch.start;
			}

			// Push this submatch ancestor onto the stack
			line.ancestors.push_back(currentSubMatch.scope.value());
			ancestorByteEndStack.push_back(currentSubMatch.end);

			// If this sub-match contains this byte, then we'll push it onto the stack
			if (currentSubMatch.end > currentByte)
			{
				// Push the style for the currentByte up to this point
				SourceSyntaxToken token = {};
				token.relativeStart = (uint32)(currentByte - line.byteStart);
				token.style = theme.match(line.ancestors);

				// TODO: OPTIMIZE: Profile how expensive this is and consider moving to debug only
				token.debugAncestorStack = line.ancestors;

				line.tokens.emplace_back(token);

				// Pop this ancestor off the stack since we're done with it now
				currentByte = currentSubMatch.end;

				// Next sub-match is technically a child of this sub-match
				if (i < subMatches.size() - 1 && subMatches[i + 1].start < currentSubMatch.end)
				{
					currentByte = subMatches[i + 1].start;
				}

				if (currentByte >= currentSubMatch.end)
				{
					line.ancestors.pop_back();
					ancestorByteEndStack.pop_back();
				}
			}

			// Keep track of the end of the furthest match so we can return where this whole set of matches ends
			if (currentSubMatch.end > maxEndPosition)
			{
				maxEndPosition = currentSubMatch.end;
			}
		}

		if (line.ancestors.size() > originalStackSize)
		{
			// Push the final token onto the stack
			SourceSyntaxToken token = {};
			token.relativeStart = (uint32)(currentByte - line.byteStart);
			token.style = theme.match(line.ancestors);

			// TODO: OPTIMIZE: Profile how expensive this is and consider moving to debug only
			token.debugAncestorStack = line.ancestors;

			line.tokens.emplace_back(token);

			// Pop excess ancestors off the stack
			while (line.ancestors.size() > originalStackSize)
			{
				line.ancestors.pop_back();
			}
		}

		return maxEndPosition;
	}

	static std::optional<GrammarMatchV2> getFirstMatchV2(const std::string& str, size_t anchor, size_t startOffset, size_t endOffset, OnigRegex reg, OnigRegion* region, const std::optional<ScopedName>& scope)
	{
		std::optional<GrammarMatchV2> res = std::nullopt;

		int searchRes = getFirstValidMatchInRange(str, anchor, startOffset, endOffset, reg, region);
		if (searchRes >= 0)
		{
			if (region->num_regs > 0)
			{
				// Only accept valid matches
				if (region->beg[0] >= 0 && region->end[0] >= region->beg[0])
				{
					GrammarMatchV2 match = {};
					match.start = (size_t)region->beg[0];
					match.end = (size_t)region->end[0];
					if (!scope.has_value())
					{
						match.scope = ScopedName::from("source.FIRST_MATCH");
					}
					else
					{
						match.scope = getScopeWithCaptures(str, *scope, region);
					}

					res = match;
				}
			}
		}
		else if (searchRes != ONIG_MISMATCH)
		{
			// Error
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str((UChar*)s, searchRes);
			g_logger_error("Oniguruma Error: '{}'", s);
			onig_region_free(region, 1 /* 1:free self, 0:free contents only */);
		}

		return res;
	}

	static std::vector<GrammarMatchV2> getCapturesV2(GrammarLineInfo& line, const std::string& str, SyntaxTheme const& theme, const PatternRepository& repo, OnigRegion* region, std::optional<CaptureList> captures, Grammar const* self)
	{
		std::vector<GrammarMatchV2> res = {};

		if (captures.has_value())
		{
			for (auto capture : captures->captures)
			{
				if (region->num_regs > capture.index)
				{
					// Only accept valid matches
					if (region->beg[capture.index] >= 0 && region->end[capture.index] > region->beg[capture.index])
					{
						size_t captureBegin = (size_t)region->beg[capture.index];
						size_t captureEnd = (size_t)region->end[capture.index];

						// Assign any captures that exist in the scope
						if (capture.scope.has_value())
						{
							GrammarMatchV2 match = {};
							match.start = captureBegin;
							match.end = captureEnd;
							match.scope = getScopeWithCaptures(str, *capture.scope, region);
							res.emplace_back(match);
						}
						else if (capture.patternArray.has_value())
						{
							OnigRegion* subRegion = onig_region_new();
							GrammarLineInfo lineShallowCopy = line;
							lineShallowCopy.tokens = {};

							// This will put all captures into the lineShallowCopy.tokens bit, then we can harvest that for our matches
							capture.patternArray->tryParseAll(lineShallowCopy, str, theme, captureBegin, captureBegin, captureEnd, repo, subRegion, self);
							onig_region_free(subRegion, 1);

							// Get the matches out of lineShallowCopy.tokens
							for (size_t i = 0; i < lineShallowCopy.tokens.size(); i++)
							{
								const auto& currentToken = lineShallowCopy.tokens[i];

								// The ancestor stack should only ever be one deeper than our current stack. If it's deeper than that, 
								// then I have no clue what's going on here.
								g_logger_assert(currentToken.debugAncestorStack.size() <= line.ancestors.size() + 1, "Capture group can only use pattern arrays that recurse no more than once.");
								g_logger_assert(currentToken.debugAncestorStack.size() > 0, "Must have at least one valid scope for a capture.");

								GrammarMatchV2 dummyMatch = {};
								dummyMatch.start = currentToken.relativeStart + line.byteStart;
								dummyMatch.end = i < lineShallowCopy.tokens.size() - 1
									? lineShallowCopy.tokens[i + 1].relativeStart + line.byteStart
									: captureEnd;
								dummyMatch.scope = currentToken.debugAncestorStack[currentToken.debugAncestorStack.size() - 1];
								res.emplace_back(dummyMatch);
							}
						}
						else
						{
							g_logger_error("Capture group in Oniguruma expression did not have a scoped name or a pattern array.");
						}
					}
				}
			}
		}

		// NOTE: Sometimes capture groups should really be children of other capture groups, so 
		//       we do one final pass here and make sure if a capture group should be a child of
		//       another group in here, that's how it actually gets represented
		//size_t potentialParentIndex = 0;
		//while (potentialParentIndex < res.size())
		//{
		//	for (size_t potentialChildIndex = 0; potentialChildIndex < res.size();)
		//	{
		//		// Skip ourself
		//		if (potentialChildIndex == potentialParentIndex)
		//		{
		//			potentialChildIndex++;
		//			continue;
		//		}

		//		auto& potentialChild = res[potentialChildIndex];
		//		auto& potentialParent = res[potentialParentIndex];

		//		// This is really a child of the parent capture so re-insert it at the appropriate index
		//		if (potentialChild.start >= potentialParent.start && potentialChild.end <= potentialParent.end)
		//		{
		//			GrammarMatchV2 potentialChildCopy = potentialChild;
		//			if (potentialChildIndex < potentialParentIndex)
		//			{
		//				potentialParentIndex--;
		//			}

		//			res.erase(res.begin() + potentialChildIndex);
		//			res.insert(res.begin() + potentialParentIndex + 1, potentialChildCopy);
		//		}
		//		else
		//		{
		//			potentialChildIndex++;
		//		}
		//	}

		//	potentialParentIndex++;
		//}

		return res;
	}

	static void freePattern(SyntaxPattern* const pattern)
	{
		pattern->free();
		g_memory_delete(pattern);
	}
}