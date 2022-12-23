#ifndef MATH_ANIM_GRAMMAR_H
#define MATH_ANIM_GRAMMAR_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct GrammarMatch;
	struct ParserInfo;
	struct PatternRepository;

	struct ScopedName
	{
		std::vector<std::string> scopes;
		std::string friendlyName;

		bool contains(const ScopedName& other);

		static ScopedName from(const std::string& str);
	};

	struct Capture
	{
		size_t index;
		ScopedName name;
	};

	struct CaptureList
	{
		// Map from capture index to the scoped name for that capture
		std::vector<Capture> captures;

		static CaptureList from(const nlohmann::json& j);
	};

	struct SimpleSyntaxPattern
	{
		std::optional<ScopedName> name;
		regex_t* regMatch;
		std::optional<CaptureList> captures;

		bool match(const std::string& str, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const;

		void free();
	};

	struct SyntaxPattern; // Forward declare since this is a recursive type
	struct ComplexSyntaxPattern
	{
		std::optional<ScopedName> name;
		regex_t* begin;
		regex_t* end;
		std::optional<CaptureList> beginCaptures;
		std::optional<CaptureList> endCaptures;
		std::optional<std::vector<SyntaxPattern>> patterns;

		bool match(const std::string& str, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const;

		void free();
	};

	struct PatternArray
	{
		std::vector<SyntaxPattern> patterns;

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

		bool match(const std::string& str, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const;

		void free();
	};

	struct PatternRepository
	{
		std::unordered_map<std::string, SyntaxPattern> patterns;
	};

	struct GrammarMatch
	{
		size_t start;
		size_t end;
		ScopedName name;
	};

	// This loosely follows the rules set out by TextMate grammars.
	// More info here: https://macromates.com/manual/en/language_grammars
	struct Grammar
	{
		std::string name;
		ScopedName scopeName;
		std::string fileTypes;
		std::vector<SyntaxPattern> patterns;
		PatternRepository repository;
		OnigRegion* region;

		bool getNextMatch(const std::string& code, std::vector<GrammarMatch>* outMatches) const;

		static Grammar* importGrammar(const char* filepath);
		static void free(Grammar* grammar);
	};
}

#endif 