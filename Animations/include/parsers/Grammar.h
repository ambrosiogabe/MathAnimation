#ifndef MATH_ANIM_GRAMMAR_H
#define MATH_ANIM_GRAMMAR_H
#include "core.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	// Internal forward decls
	struct Grammar;
	struct PatternRepository;
	struct GrammarMatchV2;
	struct SyntaxPattern;
	struct GrammarLineInfo;
	struct SourceGrammarTree;

	// Typedefs
	typedef uint64 GrammarPatternGid;

	struct MatchSpan
	{
		size_t matchStart;
		size_t matchEnd;
	};

	// Structs
	struct PatternArray
	{
		std::vector<SyntaxPattern*> patterns;
		std::unordered_map<uint64, GrammarPatternGid> onigIndexMap;
		uint64 firstSelfPatternArrayIndex;
		OnigRegSet* regset;

		// @returns The overall span of this match
		MatchSpan tryParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, Grammar const* self) const;
		// @returns The overall span of all matches contained by this pattern array
		MatchSpan tryParseAll(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, Grammar const* self) const;

		void free();
	};

	struct Capture
	{
		size_t index;
		// If this is set, this is a simple capture and the scope name is just used as the captured name
		std::optional<ScopedName> scope;
		// If this is set, then the capture scope name is based on the best match in the pattern array
		std::optional<PatternArray> patternArray;

		void free();
	};

	struct CaptureList
	{
		// Map from capture index to the scoped name for that capture
		std::vector<Capture> captures;

		void free();

		static CaptureList from(const nlohmann::json& j, Grammar* self);
	};

	struct DynamicRegexCapture
	{
		size_t captureIndex;
		size_t strReplaceStart;
		size_t strReplaceEnd;
	};

	struct DynamicRegex
	{
		bool isDynamic;
		OnigRegex simpleRegex;
		std::string regexText;
		std::vector<DynamicRegexCapture> backrefs;
	};

	struct SimpleSyntaxPattern
	{
		std::optional<ScopedName> scope;
		OnigRegex regMatch;
		std::optional<CaptureList> captures;

		void pushScopeToAncestorStack(GrammarLineInfo& line) const;
		void popScopeFromAncestorStack(GrammarLineInfo& line) const;

		// @returns The overall span of this match
		MatchSpan tryParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, Grammar const* self) const;

		void free();
	};

	struct ComplexSyntaxPattern
	{
		std::optional<ScopedName> scope;
		OnigRegex begin;
		DynamicRegex end;
		std::optional<CaptureList> beginCaptures;
		std::optional<CaptureList> endCaptures;
		std::optional<PatternArray> patterns;

		void pushScopeToAncestorStack(GrammarLineInfo& line) const;
		void popScopeFromAncestorStack(GrammarLineInfo& line) const;

		// @returns The overall span of this match
		MatchSpan tryParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, Grammar const* self, GrammarPatternGid gid) const;
		// @returns The overall span of this match
		MatchSpan resumeParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t currentByte, OnigRegex endPattern, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, Grammar const* self) const;

		void free();
	};

	enum class PatternType : uint8
	{
		Invalid = 0,
		Simple,
		Complex,
		Include,
		Array
	};

	struct SyntaxPattern
	{
		PatternType type;
		std::optional<SimpleSyntaxPattern> simplePattern;
		std::optional<ComplexSyntaxPattern> complexPattern;
		std::optional<PatternArray> patternArray;
		std::optional<std::string> patternInclude;
		const Grammar* self;
		GrammarPatternGid gid;
		uint64 patternArrayIndex;
		
		// @returns The overall span of this match
		MatchSpan tryParse(GrammarLineInfo& line, std::string const& code, SyntaxTheme const& theme, size_t anchor, size_t start, size_t endOffset, const PatternRepository& repo, OnigRegion* region) const;

		void free();
	};

	struct PatternRepository
	{
		std::unordered_map<std::string, SyntaxPattern*> patterns;
	};

	struct GrammarMatchV2
	{
		size_t start;
		size_t end;
		std::optional<ScopedName> scope;
	};

	struct SourceSyntaxToken
	{
		// The byte that this token starts at in the text
		uint32 startByte;
		PackedSyntaxStyle style;
		std::vector<ScopedName> debugAncestorStack;
	};

	struct GrammarResumeParseInfo
	{
		GrammarPatternGid gid;
		OnigRegex endPattern;
		size_t anchor;
		size_t currentByte;
		size_t originalStart;
		size_t gapTokenStart;

		inline bool operator==(GrammarResumeParseInfo const& other) const
		{
			return gid == other.gid && endPattern == other.endPattern && anchor == other.anchor;
		}

		inline bool operator!=(GrammarResumeParseInfo const& other) const
		{
			return !(*this == other);
		}
	};

	struct GrammarLineInfo
	{
		std::vector<SourceSyntaxToken> tokens;
		std::vector<ScopedName> ancestors;
		std::vector<GrammarResumeParseInfo> patternStack;
		uint32 byteStart;
		uint32 numBytes;
	};

	// This corresponds to the tree represented here: https://macromates.com/blog/2005/introduction-to-scopes/#htmlxml-analogy
	struct SourceGrammarTree
	{
		std::vector<GrammarLineInfo> sourceInfo;
		std::string codeBlock;

		std::vector<ScopedName> getAllAncestorScopesAtChar(size_t cursorPos) const;
		std::string getMatchTextAtChar(size_t cursorPos) const;

		// Default buffer size of 10KB
		std::string getStringifiedTree(Grammar const& grammar, size_t bufferSize = 1024 * 10) const;
	};

	// This loosely follows the rules set out by TextMate grammars.
	// More info here: https://macromates.com/manual/en/language_grammars
	struct Grammar
	{
		std::string name;
		ScopedName scope;
		std::string fileTypes;
		PatternArray patterns;
		PatternRepository repository;
		OnigRegion* region;
		std::unordered_map<GrammarPatternGid, SyntaxPattern const *const> globalPatternIndex;
		GrammarPatternGid gidCounter;

		SourceGrammarTree initCodeBlock(const std::string& code) const;

		// @returns -- The number of lines updated
		size_t updateFromByte(SourceGrammarTree& tree, SyntaxTheme const& theme, uint32_t byteOffset = 0, uint32_t maxNumLinesToUpdate = 30) const;

		// @deprecated -- Do it yourself, no really. Do it yourself.
		SourceGrammarTree Grammar::parseCodeBlock(const std::string& code, SyntaxTheme const& theme, bool printDebugStuff) const;

		static Grammar* importGrammar(const char* filepath);
		static void free(Grammar* grammar);
	};
}

#endif 