#ifndef MATH_ANIM_GRAMMAR_H
#define MATH_ANIM_GRAMMAR_H
#include "core.h"
#include "parsers/Common.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	// Internal forward decls
	struct Grammar;
	struct PatternRepository;
	struct GrammarMatch;
	struct SyntaxPattern;

	// Typedefs
	typedef uint64 GrammarPatternGid;

	// Structs
	struct PatternArray
	{
		std::vector<SyntaxPattern*> patterns;
		std::unordered_map<uint64, GrammarPatternGid> onigIndexMap;
		uint64 firstSelfPatternArrayIndex;
		OnigRegSet* regset;

		bool match(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches, Grammar const* self) const;
		bool matchAll(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches, Grammar const* self) const;

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

		bool match(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches, Grammar const* self) const;

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

		bool match(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches, Grammar const* self) const;

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

		bool match(const std::string& str, size_t anchor, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const;

		void free();
	};

	struct PatternRepository
	{
		std::unordered_map<std::string, SyntaxPattern*> patterns;
	};

	struct GrammarMatch
	{
		size_t start;
		size_t end;
		std::optional<ScopedName> scope;
		std::vector<GrammarMatch> subMatches;
	};

	// Data for source grammar trees
	struct Span
	{
		size_t relativeStart;
		size_t size;
	};

	struct SourceGrammarTreeNode
	{
		size_t nextNodeDelta;
		size_t parentDelta;
		Span sourceSpan;
		std::optional<ScopedName> scope;
		bool isAtomicNode;
	};

	// This corresponds to the tree represented here: https://macromates.com/blog/2005/introduction-to-scopes/#htmlxml-analogy
	struct SourceGrammarTree
	{
		std::vector<SourceGrammarTreeNode> tree;
		ScopedName rootScope;
		std::string codeBlock;

		void insertNode(const SourceGrammarTreeNode& node, size_t sourceSpanOffset);
		void removeNode(size_t nodeIndex);

		std::vector<ScopedName> getAllAncestorScopes(size_t node) const;
		std::vector<ScopedName> getAllAncestorScopesAtChar(size_t cursorPos) const;

		// Default buffer size of 10KB
		std::string getStringifiedTree(size_t bufferSize = 1024 * 10) const;


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

		SourceGrammarTree parseCodeBlock(const std::string& code, bool printDebugStuff = false) const;
		bool getNextMatch(const std::string& code, std::vector<GrammarMatch>* outMatches) const;

		static Grammar* importGrammar(const char* filepath);
		static void free(Grammar* grammar);
	};
}

#endif 