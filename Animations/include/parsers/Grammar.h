#ifndef MATH_ANIM_GRAMMAR_H
#define MATH_ANIM_GRAMMAR_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct ParserInfo;

	struct ScopedName
	{
		std::vector<std::string> scopes;

		bool contains(const ScopedName& other);

		static ScopedName from(const std::string& str);
	};

	struct CaptureList
	{
		// Map from capture index to the scoped name for that capture
		std::unordered_map<size_t, ScopedName> captures;

		static CaptureList from(const nlohmann::json& j);
	};

	struct SimpleSyntaxPattern
	{
		// TODO: Replace with Onigurama regex lib
		// std::regex match;
		std::string match;
		std::optional<CaptureList> captures;
	};

	struct SyntaxPattern; // Forward declare since this is a recursive type
	struct ComplexSyntaxPattern
	{
		// TODO: Replace with Onigurama regex lib
		// std::regex begin;
		// std::regex end;
		std::string begin;
		std::string end;
		std::optional<CaptureList> beginCaptures;
		std::optional<CaptureList> endCaptures;
		std::optional<std::vector<SyntaxPattern>> patterns;
	};

	struct PatternArray
	{
		std::vector<SyntaxPattern> patterns;
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
		std::optional<ScopedName> name;
		std::optional<SimpleSyntaxPattern> simplePattern;
		std::optional<ComplexSyntaxPattern> complexPattern;
		std::optional<PatternArray> patternArray;
		std::optional<std::string> patternInclude;
	};

	struct PatternRepository
	{
		std::string repositoryName;
		std::vector<SyntaxPattern> patterns;
	};

	// This loosely follows the rules set out by TextMate grammars.
	// More info here: https://macromates.com/manual/en/language_grammars
	struct Grammar
	{
		std::string name;
		ScopedName scopeName;
		std::string fileTypes;
		std::vector<SyntaxPattern> patterns;
		std::unordered_map<std::string, PatternRepository> repositories;

		static Grammar importGrammar(const char* filepath);
		static Grammar importGrammarFromString(const char* string);
	};
}

#endif 