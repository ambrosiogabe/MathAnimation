#include "parsers/Grammar.h"
#include "platform/Platform.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	using namespace nlohmann;

	// ----------- Internal Functions -----------
	static Grammar* importGrammarFromJson(const json& j);
	static SyntaxPattern parsePattern(const json& json);
	static PatternArray parsePatternsArray(const json& json);
	static regex_t* onigFromString(const std::string& str);
	static std::optional<GrammarMatch> getFirstMatch(const std::string& str, size_t start, size_t end, regex_t* reg, OnigRegion* region);
	static std::vector<GrammarMatch> checkForMatches(const std::string& str, size_t startOffset, size_t endOffset, const PatternRepository& repo, regex_t* reg, OnigRegion* region, std::optional<CaptureList> captures);

	CaptureList CaptureList::from(const json& j)
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
				cap.patternArray = parsePatternsArray(val["patterns"]);
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

	bool SimpleSyntaxPattern::match(const std::string& str, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const
	{
		std::vector<GrammarMatch> subMatches = checkForMatches(str, start, end, repo, this->regMatch, region, this->captures);

		bool captureWholePattern = false;
		if (this->scope.has_value())
		{
			std::optional<GrammarMatch> match = getFirstMatch(str, start, end, this->regMatch, region);
			if (match.has_value() && match->start < end && match->end <= end)
			{
				match->subMatches.insert(match->subMatches.end(), subMatches.begin(), subMatches.end());
				match->scope = (*this->scope);
				outMatches->push_back(*match);
				captureWholePattern = true;
			}
		}

		if (!captureWholePattern)
		{
			// If this rule has no scoped name, then add all the submatches by themselves
			outMatches->insert(outMatches->end(), subMatches.begin(), subMatches.end());
		}

		return subMatches.size() > 0 || captureWholePattern;
	}

	void SimpleSyntaxPattern::free()
	{
		if (regMatch)
		{
			onig_free(regMatch);
		}

		regMatch = nullptr;
	}

	bool ComplexSyntaxPattern::match(const std::string& str, size_t start, size_t endOffset, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const
	{
		// If the begin/end pair doesn't have a match, then this rule isn't a success
		std::optional<GrammarMatch> beginBlockMatch = getFirstMatch(str, start, endOffset, this->begin, region);
		if (!beginBlockMatch.has_value() || beginBlockMatch->start >= endOffset)
		{
			return false;
		}

		// This match can go to the end of the string
		std::optional<GrammarMatch> endBlockMatch = getFirstMatch(str, beginBlockMatch->end, str.length(), this->end, region);
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

		// If the begin/end pair *does* have a match, then get all grammar matches for the begin/end blocks and
		// run the extra patterns against the text between begin/end
		std::vector<GrammarMatch> beginMatches = checkForMatches(str, beginBlockMatch->start, beginBlockMatch->end, repo, this->begin, region, this->beginCaptures);
		std::vector<GrammarMatch> endMatches = checkForMatches(str, endBlockMatch->start, endBlockMatch->end, repo, this->end, region, this->endCaptures);

		GrammarMatch res = {};
		res.subMatches.insert(res.subMatches.end(), beginMatches.begin(), beginMatches.end());

		if (this->patterns.has_value())
		{
			size_t inBetweenStart = beginBlockMatch->end;
			size_t inBetweenEnd = endBlockMatch->start;
			while (inBetweenStart < inBetweenEnd)
			{
				size_t lastSubMatchesSize = res.subMatches.size();
				if (!patterns->match(str, inBetweenStart, inBetweenEnd, repo, region, &res.subMatches))
				{
					break;
				}

				for (size_t i = lastSubMatchesSize; i < res.subMatches.size(); i++)
				{
					if (res.subMatches[i].end > inBetweenStart)
					{
						inBetweenStart = res.subMatches[i].end;
					}
				}
			}
			//for (auto pattern : *patterns)
			//{
			//	pattern.match(str, inBetweenStart, inBetweenEnd, repo, region, &res.subMatches);
			//}
		}

		res.subMatches.insert(res.subMatches.end(), endMatches.begin(), endMatches.end());

		if (this->scope.has_value())
		{
			res.start = beginBlockMatch->start;
			res.end = endBlockMatch->end;
			res.scope = *this->scope;
			outMatches->push_back(res);
			return true;
		}

		outMatches->insert(outMatches->end(), res.subMatches.begin(), res.subMatches.end());
		return res.subMatches.size() > 0;
	}

	void ComplexSyntaxPattern::free()
	{
		if (begin)
		{
			onig_free(begin);
		}

		if (end)
		{
			onig_free(end);
		}

		begin = nullptr;
		end = nullptr;
	}

	bool PatternArray::match(const std::string& str, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const
	{
		std::vector<GrammarMatch> tmpRes = {};
		size_t tmpResMin = SIZE_MAX;
		size_t tmpResMax = 0;
		for (const auto& pattern : patterns)
		{
			std::vector<GrammarMatch> currentRes = {};
			if (pattern.match(str, start, end, repo, region, &currentRes))
			{
				// Find the "surface area" this pattern matches.
				// Where "surface area" is the (begin-end) substring that it matches
				size_t currentResMin = SIZE_MAX;
				size_t currentResMax = 0;
				for (const auto& match : currentRes)
				{
					if (match.start < currentResMin)
					{
						currentResMin = match.start;
					}
					if (match.end > currentResMax)
					{
						currentResMax = match.end;
					}
				}

				// The largest surface area that occurs the earliest wins out of all matches
				size_t currentSurfaceArea = currentResMax - currentResMin;
				size_t tmpSurfaceArea = tmpResMax - tmpResMin;
				if (tmpRes.size() == 0 || 
					currentResMin < tmpResMin || (
						currentResMin == tmpResMin &&
						currentSurfaceArea > tmpSurfaceArea
					))
				{
					tmpRes = currentRes;
					tmpResMin = currentResMin;
					tmpResMax = currentResMax;
				}
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
			pattern.free();
		}
	}

	bool SyntaxPattern::match(const std::string& str, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const
	{
		switch (type)
		{
		case PatternType::Array:
		{
			if (patternArray.has_value())
			{
				return patternArray->match(str, start, end, repo, region, outMatches);
			}
		}
		break;
		case PatternType::Complex:
		{
			if (complexPattern.has_value())
			{
				return complexPattern->match(str, start, end, repo, region, outMatches);
			}
		}
		break;
		case PatternType::Include:
		{
			if (patternInclude.has_value())
			{
				auto iter = repo.patterns.find(patternInclude.value());
				if (iter != repo.patterns.end())
				{
					return iter->second.match(str, start, end, repo, region, outMatches);
				}
				g_logger_warning("Unable to resolve pattern reference '{}'.", patternInclude.value());
			}
		}
		break;
		case PatternType::Simple:
		{
			if (simplePattern.has_value())
			{
				return simplePattern->match(str, start, end, repo, region, outMatches);
			}
		}
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

			tree.insertNode(newNode, match.start);

			if (match.subMatches.size() > 0)
			{
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
					tree.insertNode(gapNode, cursorIndex);
					cursorIndex += gapNode.sourceSpan.size;
				}

				// Insert an atomic node as a child of the current node
				SourceGrammarTreeNode atomicNode = {};
				atomicNode.sourceSpan = newNode.sourceSpan;
				atomicNode.nextNodeDelta = 1;
				atomicNode.scope = std::nullopt;
				tree.insertNode(atomicNode, cursorIndex);
				cursorIndex += atomicNode.sourceSpan.size;
			}
		}

		return cursorIndex;
	}

	// NOTE: This is for debugging the syntax highligher
	static void printTree(const SourceGrammarTree& tree)
	{
		constexpr size_t bufferSize = 1024 * 10;
		static char buffer[bufferSize];

		char* bufferPtr = buffer;
		size_t bufferSizeLeft = bufferSize;
		size_t indentationLevel = 0;
		for (size_t i = 0; i < tree.tree.size(); i++)
		{
			if (tree.tree[i].parentDelta == 1)
			{
				// We've gone down a level
				indentationLevel++;
			}

			for (size_t indent = 0; indent < indentationLevel; indent++)
			{
				int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "  ");
				bufferPtr += numBytesWritten;
				bufferSizeLeft -= numBytesWritten;
			}

			const std::optional<ScopedName>& scope = tree.tree[i].scope;
			if (scope)
			{
				int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'%s': ", scope->getFriendlyName().c_str());
				bufferPtr += numBytesWritten;
				bufferSizeLeft -= numBytesWritten;
			}
			else
			{
				int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'ATOM': ");
				bufferPtr += numBytesWritten;
				bufferSizeLeft -= numBytesWritten;
			}

			{
				if (scope)
				{
					std::string offsetVal =
						std::string("<")
						+ std::to_string(tree.tree[i].sourceSpan.relativeStart)
						+ ", "
						+ std::to_string(tree.tree[i].sourceSpan.size)
						+ ">";
					int numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'%s'\n", offsetVal.c_str());
					bufferPtr += numBytesWritten;
					bufferSizeLeft -= numBytesWritten;
				}
				else
				{
					// Only print atoms
					size_t absStart = tree.tree[i].sourceSpan.relativeStart;
					size_t parentIndex = i;
					while (parentIndex != 0)
					{
						parentIndex = parentIndex - tree.tree[parentIndex].parentDelta;
						absStart += tree.tree[parentIndex].sourceSpan.relativeStart;
					}

					std::string val = tree.codeBlock.substr(absStart, tree.tree[i].sourceSpan.size);
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
					bufferSizeLeft -= numBytesWritten;
				}
			}

			// If next node's parent is < our parent then we're going up 1 or more levels
			if (i + 1 < tree.tree.size())
			{
				const SourceGrammarTreeNode& nextNode = tree.tree[i + 1];
				size_t nextNodesParentIndex = (i + 1) - nextNode.parentDelta;
				size_t thisNodesParentIndex = i - tree.tree[i].parentDelta;
				while (nextNodesParentIndex < thisNodesParentIndex && indentationLevel > 0)
				{
					// We're going up a level
					indentationLevel--;
					thisNodesParentIndex = thisNodesParentIndex - tree.tree[thisNodesParentIndex].parentDelta;
				}
			}
		}

		g_logger_info("SourceGrammarTree:\n{}", buffer);
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
			printTree(res);
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

			std::vector<GrammarMatch> tmpRes = {};
			if (this->patterns.match(code, start, lineEnd, this->repository, region, outMatches))
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

			if (grammar->region)
			{
				onig_region_free(grammar->region, 1);
			}
			grammar->region = nullptr;

			grammar->~Grammar();
			g_memory_free(grammar);
		}
	}

	// ----------- Internal Functions -----------
	static Grammar* importGrammarFromJson(const json& j)
	{
		Grammar* res = (Grammar*)g_memory_allocate(sizeof(Grammar));
		new(res)Grammar();

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
				SyntaxPattern pattern = parsePattern(val);
				if (pattern.type != PatternType::Invalid)
				{
					res->repository.patterns[key] = pattern;
				}
				else
				{
					g_logger_warning("Invalid pattern parsed.");
				}
			}
		}

		if (j.contains("patterns"))
		{
			const json& patternsArray = j["patterns"];
			res->patterns = parsePatternsArray(patternsArray);
		}

		return res;
	}

	static SyntaxPattern parsePattern(const json& json)
	{
		SyntaxPattern dummy = {};
		dummy.type = PatternType::Invalid;

		// Include patterns are the easiest... So do those first
		if (json.contains("include"))
		{
			std::string includeMatch = json["include"];
			if (includeMatch.length() > 0 && includeMatch[0] == '#')
			{
				includeMatch = includeMatch.substr(1);
			}

			SyntaxPattern patternRef;
			patternRef.type = PatternType::Include;
			patternRef.patternInclude = includeMatch;

			return patternRef;
		}
		// match is mutually exclusive with begin/end. So this is a simple pattern.
		else if (json.contains("match"))
		{
			SyntaxPattern res;
			res.type = PatternType::Simple;

			SimpleSyntaxPattern p = {};
			p.regMatch = onigFromString(json["match"]);

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
				p.captures = CaptureList::from(json["captures"]);
			}
			else
			{
				p.captures = std::nullopt;
			}

			res.simplePattern = p;
			return res;
		}
		// begin/end is mutually exclusive with match. So this is a complex pattern.
		else if (json.contains("begin"))
		{
			SyntaxPattern res;
			res.type = PatternType::Complex;

			if (!json.contains("end"))
			{
				g_logger_error("Pattern '{}' has invalid complex pattern. Pattern has begin, but no end.", "UNKNOWN");
				return dummy;
			}

			ComplexSyntaxPattern c = {};
			c.begin = onigFromString(json["begin"]);
			c.end = onigFromString(json["end"]);

			if (json.contains("name"))
			{
				c.scope = ScopedName::from(json["name"]);
			}
			else
			{
				c.scope = std::nullopt;
			}

			if (json.contains("beginCaptures"))
			{
				c.beginCaptures = CaptureList::from(json["beginCaptures"]);
			}
			else
			{
				c.beginCaptures = std::nullopt;
			}

			if (json.contains("endCaptures"))
			{
				c.endCaptures = CaptureList::from(json["endCaptures"]);
			}
			else
			{
				c.endCaptures = std::nullopt;
			}

			if (json.contains("patterns"))
			{
				c.patterns = parsePatternsArray(json["patterns"]);
			}
			else
			{
				c.patterns = std::nullopt;
			}

			res.complexPattern.emplace(c);
			return res;
		}
		// If this pattern only contains a patterns array then return
		// that
		else if (json.contains("patterns"))
		{
			SyntaxPattern res = {};
			res.type = PatternType::Array;
			res.patternArray = parsePatternsArray(json["patterns"]);
			return res;
		}

		return dummy;
	}

	static PatternArray parsePatternsArray(const json& json)
	{
		PatternArray res = {};

		for (json::const_iterator it = json.begin(); it != json.end(); it++)
		{
			SyntaxPattern pattern = parsePattern(*it);
			if (pattern.type != PatternType::Invalid)
			{
				res.patterns.emplace_back(pattern);
			}
			else
			{
				g_logger_warning("Invalid pattern parsed.");
			}
		}

		return res;
	}

	static regex_t* onigFromString(const std::string& str)
	{
		const char* pattern = str.c_str();
		const char* patternEnd = pattern + str.length();

		int options = ONIG_OPTION_NONE;
		options |= ONIG_OPTION_CAPTURE_GROUP;
		options |= ONIG_OPTION_MULTILINE;

		// Enable capture history for Oniguruma
		OnigSyntaxType syn;
		onig_copy_syntax(&syn, ONIG_SYNTAX_DEFAULT);
		onig_set_syntax_op2(&syn,
			onig_get_syntax_op2(&syn) | ONIG_SYN_OP2_ATMARK_CAPTURE_HISTORY);

		regex_t* reg;
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
			g_logger_error("Oniguruma Error: '{}'", s);
			return nullptr;
		}

		return reg;
	}

	static std::optional<GrammarMatch> getFirstMatch(const std::string& str, size_t startOffset, size_t endOffset, regex_t* reg, OnigRegion* region)
	{
		std::optional<GrammarMatch> res = std::nullopt;

		const char* cstr = str.c_str();
		const char* start = cstr + startOffset;
		const char* end = cstr + str.length();
		const char* range = cstr + endOffset;
		int searchRes = onig_search(
			reg,
			(uint8*)cstr,
			(uint8*)end,
			(uint8*)start,
			(uint8*)range,
			region,
			ONIG_OPTION_NONE
		);

		if (searchRes >= 0)
		{
			if (region->num_regs > 0)
			{
				// Only accept valid matches
				if (region->beg[0] >= 0 && region->end[0] > region->beg[0])
				{
					GrammarMatch match = {};
					match.start = (size_t)region->beg[0];
					match.end = (size_t)region->end[0];
					match.scope = ScopedName::from("source.FIRST_MATCH");
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

	struct CaptureCallbackData
	{
		const CaptureList& captures;
		std::vector<GrammarMatch>* matches;
		const std::string& str;
		const PatternRepository& repo;
	};

	//static int traverseCaptureList(int groupIndex, int beg, int end, int /*level*/, int at, void* arg)
	//{
	//	if (at != ONIG_TRAVERSE_CALLBACK_AT_FIRST)
	//		return -1; /* error */

	//	CaptureCallbackData* callbackData = (CaptureCallbackData*)arg;

	//	for (auto capture : callbackData->captures.captures)
	//	{
	//		if (groupIndex == capture.index)
	//		{
	//			// Only accept valid matches
	//			if (beg >= 0 && end > beg)
	//			{
	//				size_t captureBegin = (size_t)beg;
	//				size_t captureEnd = (size_t)end;
	//				if (capture.scope.has_value())
	//				{
	//					GrammarMatch match = {};
	//					match.start = captureBegin;
	//					match.end = captureEnd;
	//					match.scope = *capture.scope;
	//					callbackData->matches->emplace_back(match);
	//				}
	//				else if (capture.patternArray.has_value())
	//				{
	//					OnigRegion* region = onig_region_new();
	//					capture.patternArray->match(
	//						callbackData->str,
	//						captureBegin,
	//						captureEnd,
	//						callbackData->repo,
	//						region,
	//						callbackData->matches
	//					);
	//					onig_region_free(region, 1);
	//				}
	//				else
	//				{
	//					g_logger_error("Capture group in Onigiruma expression did not have a scoped name or a pattern array.");
	//				}
	//			}
	//		}
	//	}

	//	return 0;
	//}

	static std::vector<GrammarMatch> checkForMatches(const std::string& str, size_t startOffset, size_t endOffset, const PatternRepository& repo, regex_t* reg, OnigRegion* region, std::optional<CaptureList> captures)
	{
		std::vector<GrammarMatch> res = {};

		onig_region_clear(region);

		const char* cstr = str.c_str();
		const char* start = cstr + startOffset;
		const char* end = cstr + str.length();
		const char* range = cstr + endOffset;
		int searchRes = onig_search(
			reg,
			(uint8*)cstr,
			(uint8*)end,
			(uint8*)start,
			(uint8*)range,
			region,
			ONIG_OPTION_NONE
		);

		if (searchRes >= 0)
		{
			if (captures.has_value())
			{
				//CaptureCallbackData callbackData = {
				//	captures.value(),
				//	&res,
				//	str,
				//	repo
				//};

				//// Traverse the capture list and add relevant captures to our info
				//searchRes = onig_capture_tree_traverse(
				//	region,
				//	ONIG_TRAVERSE_CALLBACK_AT_FIRST,
				//	traverseCaptureList,
				//	(void*)&callbackData
				//);

				for (auto capture : captures->captures)
				{
					if (region->num_regs > capture.index)
					{
						// Only accept valid matches
						if (region->beg[capture.index] >= 0 && region->end[capture.index] > region->beg[capture.index])
						{
							size_t captureBegin = (size_t)region->beg[capture.index];
							size_t captureEnd = (size_t)region->end[capture.index];
							if (capture.scope.has_value())
							{
								GrammarMatch match = {};
								match.start = captureBegin;
								match.end = captureEnd;
								match.scope = *capture.scope;
								res.emplace_back(match);
							}
							else if (capture.patternArray.has_value())
							{
								OnigRegion* subRegion = onig_region_new();
								capture.patternArray->match(str, captureBegin, captureEnd, repo, subRegion, &res);
								onig_region_free(subRegion, 1);
							}
							else
							{
								g_logger_error("Capture group in Onigiruma expression did not have a scoped name or a pattern array.");
							}
						}
					}
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
}