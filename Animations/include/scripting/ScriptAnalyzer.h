#ifndef MATH_ANIM_SCRIPT_ANALYZER
#define MATH_ANIM_SCRIPT_ANALYZER
#include "core.h"
#include "parsers/SyntaxHighlighter.h"

#pragma warning( push )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4324 )
#pragma warning( disable : 4324 )
#include <Luau/Autocomplete.h>
#pragma warning( pop )

namespace Luau
{
	struct FileResolver;
	struct ConfigResolver;
	struct Frontend;
}

namespace MathAnim
{
	struct AutocompleteSuggestion
	{
		std::string text;
		Luau::AutocompleteEntry data;
		// Rank from 0.0-100.0 where 100.0 is a perfect match
		float rank;
		bool containsQuery;
	};

	struct FunctionParameter
	{
		std::optional<std::string> name;
		std::string stringifiedType;
	};

	struct FunctionIntellisense 
	{
		std::string fnName;
		std::vector<FunctionParameter> parameters;
		std::vector<std::string> returnTypes;

		CodeHighlights highlightInfo;
	};

	class ScriptAnalyzer
	{
	public:
		ScriptAnalyzer(const std::filesystem::path& scriptDirectory);

		bool analyze(const std::string& filename);
		bool analyze(const std::string& sourceCode, const std::string& scriptName);

		FunctionIntellisense getFunctionParameterIntellisense(std::string const& sourceCode, std::string const& scriptName, uint32 line, uint32 column);
		std::vector<AutocompleteSuggestion> getSuggestions(std::string const& sourceCode, std::string const& scriptName, uint32 line, uint32 column);
		void sortSuggestionsByQuery(std::string const& query, std::vector<AutocompleteSuggestion>& suggestions, std::vector<int>& visibleSuggestions);

		void free();

	private:
		const std::filesystem::path m_scriptDirectory;
		Luau::FileResolver* fileResolver;
		Luau::ConfigResolver* configResolver;
		Luau::Frontend* frontend;
	};
}

#endif 