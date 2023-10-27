#include "parsers/Grammar.h"
#include "platform/Platform.h"

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
	static std::optional<GrammarMatch> getFirstMatch(const std::string& str, size_t anchor, size_t startOffset, size_t endOffset, OnigRegex reg, OnigRegion* region, const std::optional<ScopedName>& scope);
	static std::vector<GrammarMatch> getCaptures(const std::string& str, const PatternRepository& repo, OnigRegion* region, std::optional<CaptureList> captures, Grammar const* self);
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

	bool SimpleSyntaxPattern::match(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches, Grammar const* self) const
	{
		std::optional<GrammarMatch> match = getFirstMatch(str, anchor, start, end, this->regMatch, region, this->scope);
		if (match.has_value() && match->start < end && match->start >= start && match->end <= end)
		{
			// `region` now contains any potential captures, so we'll search for any captures now
			std::vector<GrammarMatch> subMatches = getCaptures(str, repo, region, this->captures, self);

			// If this simple pattern has a scope name, just add the match with submatches as children
			if (scope.has_value())
			{
				match->subMatches.insert(match->subMatches.end(), subMatches.begin(), subMatches.end());
				outMatches->push_back(*match);
				return true;
			}
			// Otherwise, just add the children to the out matches
			else
			{
				outMatches->insert(outMatches->end(), subMatches.begin(), subMatches.end());
				return subMatches.size() > 0;
			}
		}

		return false;
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

	bool ComplexSyntaxPattern::match(const std::string& str, size_t anchor, size_t start, size_t endOffset, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches, Grammar const* self) const
	{
		// If the begin/end pair doesn't have a match, then this rule isn't a success
		std::optional<GrammarMatch> beginBlockMatch = getFirstMatch(str, anchor, start, endOffset, this->begin, region, std::nullopt);
		if (!beginBlockMatch.has_value() || beginBlockMatch->start >= endOffset || beginBlockMatch->start < start)
		{
			return false;
		}

		// If beginBlockMatch is valid, then we'll use the result stored in `region` to find any captures
		std::vector<GrammarMatch> beginMatches = getCaptures(str, repo, region, this->beginCaptures, self);

		OnigRegex endPattern = this->end.simpleRegex;
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
				std::string replacement = str.substr(replacementStringBegin, (replacementStringEnd - replacementStringBegin));
				regexPatternToTest = beginRegex + replacement + endRegex;

				offsetToAdd += (replacementStringEnd - replacementStringBegin) - (replaceEndOffset - replaceBeginOffset);
			}

			// After substituting backrefs generate an on-the-fly end pattern to match
			endPattern = onigFromString(regexPatternToTest, true);

			g_logger_assert(endPattern != nullptr, "Failed to generate dynamic regex pattern with backreferences for pattern: '{}'", this->end.regexText);
		}

		// This match can go to the end of the string
		std::optional<GrammarMatch> endBlockMatch = getFirstMatch(str, beginBlockMatch->end, beginBlockMatch->end, str.length(), endPattern, region, std::nullopt);
		std::vector<GrammarMatch> endMatches = {};
		if (!endBlockMatch.has_value())
		{
			GrammarMatch eof;
			eof.start = str.length();
			eof.end = str.length();
			eof.scope = ScopedName::from("source.ERROR_EOF_NO_CLOSING_MATCH");

			// If there was no end group matched, then automatically use the end of the file as the end block
			// which is specified in the rules for textmates
			endBlockMatch = eof;
		}
		else
		{
			// Check for any captures in the endBlock if it was valid
			endMatches = getCaptures(str, repo, region, this->endCaptures, self);
		}

		GrammarMatch res = {};
		res.subMatches.insert(res.subMatches.end(), beginMatches.begin(), beginMatches.end());

		if (this->patterns.has_value())
		{
			size_t inBetweenStart = beginBlockMatch->end;
			size_t inBetweenEnd = endBlockMatch->start;

			// We want to consider the rest of the line when matching
			size_t endOfLine = inBetweenEnd;
			for (; endOfLine < str.length(); endOfLine++)
			{
				if (str[endOfLine] == '\n')
				{
					break;
				}
			}

			while (inBetweenStart < inBetweenEnd)
			{
				// NOTE: We start searching at `beginBlockMatch->end` so that if any patterns are using anchors in
				//       their regexes, the anchor will appropriately start at the beginning of the `inBetween` span
				size_t lastSubMatchesSize = res.subMatches.size();
				if (!patterns->match(str, beginBlockMatch->end, inBetweenStart, endOfLine, repo, region, &res.subMatches, self))
				{
					break;
				}

				// Discard any matches that begin outside of our inBetweenBlock
				bool anyMatchesBeganInBetweenScope = false;
				for (size_t i = lastSubMatchesSize; i < res.subMatches.size(); i++)
				{
					if (res.subMatches[i].start >= inBetweenEnd)
					{
						res.subMatches.erase(res.subMatches.begin() + i);
						i--;
					}
					else if (res.subMatches[i].start < inBetweenStart)
					{
						res.subMatches.erase(res.subMatches.begin() + i);
						i--;
					}
					else
					{
						anyMatchesBeganInBetweenScope = true;
					}
				}

				// Figure out the new inBetweenStart
				for (size_t i = lastSubMatchesSize; i < res.subMatches.size(); i++)
				{
					if (res.subMatches[i].end >= inBetweenStart)
					{
						if (res.subMatches[i].start == res.subMatches[i].end && res.subMatches[i].start != endBlockMatch->end)
						{
							inBetweenStart = res.subMatches[i].end + 1;
						}
						else
						{
							inBetweenStart = res.subMatches[i].end;
						}
					}
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
					if (str[inBetweenStart - 1] == '\n')
					{
						endBlockMatch->start = inBetweenStart;
						if (endBlockMatch->start > endBlockMatch->end)
						{
							endBlockMatch->end = endBlockMatch->start;
						}
						break;
					}
					// ----

					endBlockMatch = getFirstMatch(str, inBetweenStart, inBetweenStart, str.length(), endPattern, region, std::nullopt);
					if (!endBlockMatch.has_value())
					{
						GrammarMatch eof;
						eof.start = str.length();
						eof.end = str.length();
						eof.scope = ScopedName::from("source.ERROR_EOF_NO_CLOSING_MATCH");

						// If there was no end group matched, then automatically use the end of the file as the end block
						// which is specified in the rules for textmates
						endBlockMatch = eof;
						endMatches = {};
					}
					else
					{
						// Check for any captures in the endBlock if it was valid
						endMatches = getCaptures(str, repo, region, this->endCaptures, self);
					}

					inBetweenEnd = endBlockMatch->start;

					// Consider whole line
					endOfLine = inBetweenEnd;
					for (; endOfLine < str.length(); endOfLine++)
					{
						if (str[endOfLine] == '\n')
						{
							break;
						}
					}
				}
				// ----
				else if (inBetweenStart == endBlockMatch->end || !anyMatchesBeganInBetweenScope)
				{
					break;
				}
			}
		}

		// Free dynamic regex if necessary
		if (this->end.isDynamic)
		{
			onig_free(endPattern);
		}

		res.subMatches.insert(res.subMatches.end(), endMatches.begin(), endMatches.end());

		res.start = beginBlockMatch->start;
		res.end = endBlockMatch->end;

		if (this->scope.has_value())
		{
			res.scope = *this->scope;
		}
		else
		{
			res.scope = std::nullopt;
		}

		outMatches->push_back(res);
		return true;
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

	bool PatternArray::match(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches, Grammar const* self) const
	{
		std::vector<GrammarMatch> tmpRes = {};
		int onigPatternMatched = -1;
		getFirstMatchInRegset(str, anchor, start, end, *this, &onigPatternMatched);

		// See if there's a $self pattern that we can try to match on
		bool selfPatternExists = this->firstSelfPatternArrayIndex != UINT64_MAX;
		uint64 selfPatternIndexMatch = UINT64_MAX;
		if (selfPatternExists)
		{
			int onigSelfPatternMatched = -1;
			getFirstMatchInRegset(str, anchor, start, end, self->patterns, &onigSelfPatternMatched);

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
						size_t lastOutMatchesSize = outMatches->size();
						patternIter->second->match(str, anchor, start, end, repo, region, outMatches);
						return outMatches->size() != lastOutMatchesSize;
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
			return self->patterns.match(str, anchor, start, end, repo, region, outMatches, self);
		}

		return false;
	}

	bool PatternArray::matchAll(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches, Grammar const* self) const
	{
		size_t cursor = start;

		std::vector<GrammarMatch> tmpRes = {};
		while (cursor < end)
		{
			if (this->match(str, anchor, cursor, end, repo, region, &tmpRes, self))
			{
				size_t oldStart = cursor;
				for (size_t i = 0; i < tmpRes.size(); i++)
				{
					if (tmpRes[i].end > cursor)
					{
						cursor = tmpRes[i].end;
					}
				}

				if (cursor == oldStart)
				{
					cursor++;
				}
			}
			else
			{
				cursor++;
			}
		}

		if (tmpRes.size() > 0)
		{
			outMatches->insert(outMatches->end(), tmpRes.begin(), tmpRes.end());
			return true;
		}

		return false;
	}

	void PatternArray::free()
	{
		for (auto& pattern : patterns)
		{
			freePattern(pattern);
		}
	}

	bool SyntaxPattern::match(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const
	{
		switch (type)
		{
		case PatternType::Array:
		{
			if (patternArray.has_value())
			{
				return patternArray->match(str, anchor, start, end, repo, region, outMatches, self);
			}
		}
		break;
		case PatternType::Complex:
		{
			if (complexPattern.has_value())
			{
				return complexPattern->match(str, anchor, start, end, repo, region, outMatches, self);
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
					return iter->second->match(str, anchor, start, end, repo, region, outMatches);
				}
				// NOTE: $self means to include a self-reference to our own grammar
				else if (*patternInclude == "$self")
				{
					return this->self->patterns.match(str, anchor, start, end, repo, region, outMatches, self);

				}
				g_logger_warning("Unable to resolve pattern reference '{}'.", *patternInclude);
			}
		}
		break;
		case PatternType::Simple:
		{
			if (simplePattern.has_value())
			{
				return simplePattern->match(str, anchor, start, end, repo, region, outMatches, self);
			}
		}
		break;
		case PatternType::Invalid:
			break;
		}

		return false;
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

	void SourceGrammarTree::insertNode(const SourceGrammarTreeNode& node, size_t sourceSpanOffset)
	{
		size_t nodeAbsStart = node.sourceSpan.relativeStart + sourceSpanOffset;
		size_t nodeAbsEnd = nodeAbsStart + node.sourceSpan.size;

		// Assume that we won't find a place to insert this node
		size_t insertIndex = this->tree.size();
		size_t parentDelta = 0;
		for (size_t i = 0; i < this->tree.size();)
		{
			size_t absoluteOffset = 0;
			{
				size_t parentIndex = i;
				while (parentIndex != 0)
				{
					parentIndex = parentIndex - tree[parentIndex].parentDelta;
					absoluteOffset += tree[parentIndex].sourceSpan.relativeStart;
				}
			}

			const SourceGrammarTreeNode& currentNode = this->tree[i];
			size_t currentNodeAbsStart = currentNode.sourceSpan.relativeStart + absoluteOffset;
			size_t currentNodeAbsEnd = currentNodeAbsStart + currentNode.sourceSpan.size;
			if (nodeAbsStart >= currentNodeAbsStart && nodeAbsEnd <= currentNodeAbsEnd)
			{
				// The node we're inserting will be a child of the current node
				insertIndex = i + 1;
				i += 1;
				parentDelta = 1;

				absoluteOffset += currentNode.sourceSpan.relativeStart;
			}
			else if (nodeAbsStart < currentNodeAbsStart)
			{
				// The current node starts after our node, so we'll break now
				// insert the new node and update the surrounding nodes
				break;
			}
			else
			{
				// The node is a sibling of the current node
				insertIndex = i + currentNode.nextNodeDelta;
				i += currentNode.nextNodeDelta;
				parentDelta += currentNode.nextNodeDelta;
			}
		}

		// Insert the node
		{
			SourceGrammarTreeNode nodeToInsert = node;
			nodeToInsert.parentDelta = parentDelta;
			tree.insert(tree.begin() + insertIndex, nodeToInsert);
		}

		// Update the new node's relative offset
		{
			size_t parentAbsOffset = 0;
			size_t parentIndex = insertIndex;
			while (parentIndex != 0)
			{
				parentIndex = parentIndex - tree[parentIndex].parentDelta;
				parentAbsOffset += tree[parentIndex].sourceSpan.relativeStart;
			}

			g_logger_assert(nodeAbsStart >= parentAbsOffset, "This should never happen...?");
			tree[insertIndex].sourceSpan.relativeStart = nodeAbsStart - parentAbsOffset;
		}

		// Update siblings
		{
			size_t nodeToInsertParentIndex = insertIndex - tree[insertIndex].parentDelta;
			size_t firstSiblingIndex = insertIndex + tree[insertIndex].nextNodeDelta;
			for (size_t i = firstSiblingIndex; i < tree.size();)
			{
				size_t thisNodesParentIndex = i - (tree[i].parentDelta + tree[insertIndex].nextNodeDelta);
				if (thisNodesParentIndex != nodeToInsertParentIndex)
				{
					// This means we've traversed all siblings and don't need to update anything else
					// so we can break out of the loop
					break;
				}

				tree[i].parentDelta += tree[insertIndex].nextNodeDelta;
				i += tree[i].nextNodeDelta;
			}
		}

		// Update parents next node deltas
		{
			size_t parentIndex = insertIndex;
			while (parentIndex != 0)
			{
				parentIndex = parentIndex - tree[parentIndex].parentDelta;
				tree[parentIndex].nextNodeDelta += tree[insertIndex].nextNodeDelta;
			}
		}
	}

	std::vector<ScopedName> SourceGrammarTree::getAllAncestorScopes(size_t node) const
	{
		std::vector<ScopedName> ancestorScopes = {};
		if (tree[node].scope)
		{
			ancestorScopes.push_back(*tree[node].scope);
		}

		size_t parentIndex = node;
		while (parentIndex != 0 && parentIndex >= tree[parentIndex].parentDelta)
		{
			parentIndex = parentIndex - tree[parentIndex].parentDelta;
			if (tree[parentIndex].scope)
			{
				ancestorScopes.insert(ancestorScopes.begin(), *tree[parentIndex].scope);
			}
		}

		return ancestorScopes;
	}

	std::vector<ScopedName> SourceGrammarTree::getAllAncestorScopesAtChar(size_t cursorPos) const
	{
		size_t nodePos = 0;
		{
			size_t absPos = 0;
			for (size_t i = 0; i < tree.size();)
			{
				size_t absSpanStart = tree[i].sourceSpan.relativeStart + absPos;
				size_t absSpanEnd = absSpanStart + tree[i].sourceSpan.size;
				if (cursorPos >= absSpanStart && cursorPos < absSpanEnd)
				{
					// This is a leaf node, so this is the node we're looking for
					if (tree[i].nextNodeDelta == 1)
					{
						nodePos = i;
						break;
					}

					// Otherwise, the cursorPos is at one of the children of this branch
					// so we start iterating through the children
					absPos += tree[i].sourceSpan.relativeStart;
					i += 1;
				}
				else
				{
					// Skip this node since the cursor doesn't live in it
					i += tree[i].nextNodeDelta;
				}
			}
		}

		std::vector<ScopedName> ancestorScopes = {};
		if (tree[nodePos].scope)
		{
			ancestorScopes.push_back(*tree[nodePos].scope);
		}

		size_t parentIndex = nodePos;
		while (parentIndex != 0 && parentIndex >= tree[parentIndex].parentDelta)
		{
			parentIndex = parentIndex - tree[parentIndex].parentDelta;
			if (tree[parentIndex].scope)
			{
				ancestorScopes.insert(ancestorScopes.begin(), *tree[parentIndex].scope);
			}
		}

		return ancestorScopes;
	}

	static bool checkBufferUnderflow(size_t sizeLeft, size_t numBytesToRemove)
	{
		if (sizeLeft >= numBytesToRemove)
		{
			return false;
		}

		g_logger_error("We have a buffer underflow. Please pass a larger buffer to the tree.");
		return true;
	}

	std::string SourceGrammarTree::getStringifiedTree(size_t bufferSize) const
	{
		char* buffer = (char*)g_memory_allocate(bufferSize * sizeof(char));

		char* bufferPtr = buffer;
		size_t bufferSizeLeft = bufferSize;
		size_t indentationLevel = 0;
		for (size_t i = 0; i < tree.size(); i++)
		{
			if (tree[i].parentDelta == 1)
			{
				// We've gone down a level
				indentationLevel++;
			}

			for (size_t indent = 0; indent < indentationLevel; indent++)
			{
				int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "  ");
				bufferPtr += numBytesWritten;
				if (checkBufferUnderflow(bufferSizeLeft, numBytesWritten))
				{
					// Break out of all loops
					goto end;
				}
				bufferSizeLeft -= numBytesWritten;
			}

			if (tree[i].isAtomicNode)
			{
				int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'ATOM': ");
				bufferPtr += numBytesWritten;
				if (checkBufferUnderflow(bufferSizeLeft, numBytesWritten))
				{
					// Break out of all loops
					goto end;
				}
				bufferSizeLeft -= numBytesWritten;
			}
			else
			{
				if (tree[i].scope)
				{
					const std::optional<ScopedName>& scope = tree[i].scope;
					int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'%s': ", scope->getFriendlyName().c_str());
					bufferPtr += numBytesWritten;
					if (checkBufferUnderflow(bufferSizeLeft, numBytesWritten))
					{
						// Break out of all loops
						goto end;
					}
					bufferSizeLeft -= numBytesWritten;
				}
				else
				{
					int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'NULL_SCOPE': ");
					bufferPtr += numBytesWritten;
					if (checkBufferUnderflow(bufferSizeLeft, numBytesWritten))
					{
						// Break out of all loops
						goto end;
					}
					bufferSizeLeft -= numBytesWritten;
				}
			}

			{
				if (!tree[i].isAtomicNode)
				{
					std::string offsetVal =
						std::string("<")
						+ std::to_string(tree[i].sourceSpan.relativeStart)
						+ ", "
						+ std::to_string(tree[i].sourceSpan.size)
						+ ">";
					int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'%s'\n", offsetVal.c_str());
					bufferPtr += numBytesWritten;
					if (checkBufferUnderflow(bufferSizeLeft, numBytesWritten))
					{
						// Break out of all loops
						goto end;
					}
					bufferSizeLeft -= numBytesWritten;
				}
				else
				{
					// Only print atoms
					size_t absStart = tree[i].sourceSpan.relativeStart;
					size_t parentIndex = i;
					while (parentIndex != 0)
					{
						parentIndex = parentIndex - tree[parentIndex].parentDelta;
						absStart += tree[parentIndex].sourceSpan.relativeStart;
					}

					std::string val = codeBlock.substr(absStart, tree[i].sourceSpan.size);
					for (size_t cIndex = 0; cIndex < val.length(); cIndex++)
					{
						if (val[cIndex] == '\n')
						{
							val[cIndex] = '\\';
							val.insert(cIndex + 1, "n");
							cIndex++;
						}
					}
					int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'%s'\n", val.c_str());
					bufferPtr += numBytesWritten;
					if (checkBufferUnderflow(bufferSizeLeft, numBytesWritten))
					{
						// Break out of all loops
						goto end;
					}
					bufferSizeLeft -= numBytesWritten;
				}
			}

			// If next node's parent is < our parent then we're going up 1 or more levels
			if (i + 1 < tree.size())
			{
				const SourceGrammarTreeNode& nextNode = tree[i + 1];
				size_t nextNodesParentIndex = (i + 1) - nextNode.parentDelta;
				size_t thisNodesParentIndex = i - tree[i].parentDelta;
				while (nextNodesParentIndex < thisNodesParentIndex && indentationLevel > 0)
				{
					// We're going up a level
					indentationLevel--;
					thisNodesParentIndex = thisNodesParentIndex - tree[thisNodesParentIndex].parentDelta;
				}
			}
		}

	end:
		if ((size_t)(bufferPtr - buffer) < bufferSize)
		{
			bufferPtr[0] = '\0';
		}
		else
		{
			// We had a buffer overrun, truncate the string
			buffer[bufferSize - 1] = '\0';
		}

		std::string res = std::string((const char*)buffer);
		g_memory_free(buffer);

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

	static size_t addMatchesToTree(SourceGrammarTree& tree, const std::vector<GrammarMatch>& matches, size_t cursorIndex)
	{
		for (const auto& match : matches)
		{
			// Construct a grammar tree node and insert it into our tree
			SourceGrammarTreeNode newNode = {};
			newNode.sourceSpan.relativeStart = 0;
			newNode.sourceSpan.size = match.end - match.start;
			newNode.nextNodeDelta = 1;
			newNode.scope = match.scope;
			newNode.isAtomicNode = false;

			tree.insertNode(newNode, match.start);

			if (match.subMatches.size() > 0)
			{
				if (cursorIndex < match.start)
				{
					// Fill in the gap
					SourceGrammarTreeNode gapNode = {};
					gapNode.sourceSpan.relativeStart = 0;
					gapNode.sourceSpan.size = match.start - cursorIndex;
					gapNode.nextNodeDelta = 1;
					gapNode.scope = std::nullopt;
					gapNode.isAtomicNode = true;
					tree.insertNode(gapNode, cursorIndex);
					cursorIndex += gapNode.sourceSpan.size;
				}

				// Recursively add sub-matches
				cursorIndex = addMatchesToTree(tree, match.subMatches, cursorIndex);

				// If the sub-matches didn't fill this match entirely, fill in the gap
				if (cursorIndex < match.end)
				{
					// Fill in the gap
					SourceGrammarTreeNode gapNode = {};
					gapNode.sourceSpan.relativeStart = 0;
					gapNode.sourceSpan.size = match.end - cursorIndex;
					gapNode.nextNodeDelta = 1;
					gapNode.scope = std::nullopt;
					gapNode.isAtomicNode = true;
					tree.insertNode(gapNode, cursorIndex);
					cursorIndex += gapNode.sourceSpan.size;
				}
			}
			else
			{
				if (cursorIndex < match.start)
				{
					// Fill in the gap
					SourceGrammarTreeNode gapNode = {};
					gapNode.sourceSpan.relativeStart = 0;
					gapNode.sourceSpan.size = match.start - cursorIndex;
					gapNode.nextNodeDelta = 1;
					gapNode.scope = std::nullopt;
					gapNode.isAtomicNode = true;
					tree.insertNode(gapNode, cursorIndex);
					cursorIndex += gapNode.sourceSpan.size;
				}

				if (cursorIndex > match.start)
				{
					g_logger_error("How did this happen?");
				}

				// Insert an atomic node as a child of the current node
				SourceGrammarTreeNode atomicNode = {};
				atomicNode.sourceSpan = newNode.sourceSpan;
				atomicNode.nextNodeDelta = 1;
				atomicNode.scope = std::nullopt;
				atomicNode.isAtomicNode = true;
				tree.insertNode(atomicNode, cursorIndex);
				cursorIndex += atomicNode.sourceSpan.size;
			}
		}

		return cursorIndex;
	}

	SourceGrammarTree Grammar::parseCodeBlock(const std::string& code, bool printDebugStuff) const
	{
		std::vector<GrammarMatch> matches = {};
		while (this->getNextMatch(code, &matches))
		{
		}

		SourceGrammarTree res{};
		res.rootScope = this->scope;
		res.codeBlock = code;

		// Construct and insert the root node which will cover all the source code
		{
			SourceGrammarTreeNode rootNode = {};
			rootNode.parentDelta = 0;
			rootNode.nextNodeDelta = 1;
			rootNode.scope = this->scope;
			rootNode.sourceSpan.relativeStart = 0;
			rootNode.sourceSpan.size = code.length();

			res.insertNode(rootNode, 0);
		}

		addMatchesToTree(res, matches, 0);

		if (printDebugStuff)
		{
			std::string stringifiedTree = res.getStringifiedTree();
			g_logger_info("Stringified Tree: {}\n", stringifiedTree);
		}

		return res;
	}

	bool Grammar::getNextMatch(const std::string& code, std::vector<GrammarMatch>* outMatches) const
	{
		size_t start = 0;
		for (size_t i = 0; i < outMatches->size(); i++)
		{
			if ((*outMatches)[i].end > start)
			{
				start = (*outMatches)[i].end;
			}
		}

		size_t lineEnd = start;
		while (lineEnd < code.length())
		{
			for (; lineEnd < code.length(); lineEnd++)
			{
				if (code[lineEnd] == '\n')
				{
					lineEnd++;
					break;
				}
			}

			if (this->patterns.match(code, start, start, lineEnd, this->repository, region, outMatches, this))
			{
				return true;
			}

			start = lineEnd;
		}

		return false;
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
					size_t captureStart = region->beg[captureIndex];
					size_t captureLength = region->end[captureIndex] - captureStart;
					dotSeparatedScope.capture->capture = str.substr(captureStart, captureLength);
				}
			}
		}

		return res;
	}

	static void getFirstMatchInRegset(const std::string& str, size_t anchor, size_t startOffset, size_t endOffset, const PatternArray& pattern, int* patternMatched)
	{
		std::optional<GrammarMatch> res = std::nullopt;

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

	static std::optional<GrammarMatch> getFirstMatch(const std::string& str, size_t anchor, size_t startOffset, size_t endOffset, OnigRegex reg, OnigRegion* region, const std::optional<ScopedName>& scope)
	{
		std::optional<GrammarMatch> res = std::nullopt;

		int searchRes = getFirstValidMatchInRange(str, anchor, startOffset, endOffset, reg, region);
		if (searchRes >= 0)
		{
			if (region->num_regs > 0)
			{
				// Only accept valid matches
				if (region->beg[0] >= 0 && region->end[0] >= region->beg[0])
				{
					GrammarMatch match = {};
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

	static std::vector<GrammarMatch> getCaptures(const std::string& str, const PatternRepository& repo, OnigRegion* region, std::optional<CaptureList> captures, Grammar const* self)
	{
		std::vector<GrammarMatch> res = {};

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
							GrammarMatch match = {};
							match.start = captureBegin;
							match.end = captureEnd;
							match.scope = getScopeWithCaptures(str, *capture.scope, region);
							res.emplace_back(match);
						}
						else if (capture.patternArray.has_value())
						{
							OnigRegion* subRegion = onig_region_new();
							capture.patternArray->matchAll(str, captureBegin, captureBegin, captureEnd, repo, subRegion, &res, self);
							onig_region_free(subRegion, 1);
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
		size_t potentialParentIndex = 0;
		while (potentialParentIndex < res.size())
		{
			for (size_t potentialChildIndex = 0; potentialChildIndex < res.size();)
			{
				// Skip ourself
				if (potentialChildIndex == potentialParentIndex)
				{
					potentialChildIndex++;
					continue;
				}

				auto& potentialChild = res[potentialChildIndex];
				auto& potentialParent = res[potentialParentIndex];
				// This is really a child of the parent capture
				if (potentialChild.start >= potentialParent.start && potentialChild.end <= potentialParent.end)
				{
					potentialParent.subMatches.push_back(potentialChild);
					if (potentialChildIndex < potentialParentIndex)
					{
						potentialParentIndex--;
					}

					res.erase(res.begin() + potentialChildIndex);
				}
				else
				{
					potentialChildIndex++;
				}
			}

			potentialParentIndex++;
		}

		return res;
	}

	static void freePattern(SyntaxPattern* const pattern)
	{
		pattern->free();
		g_memory_delete(pattern);
	}
}