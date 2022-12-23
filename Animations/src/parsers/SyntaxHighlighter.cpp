#include "parsers/SyntaxHighlighter.h"
#include "parsers/Grammar.h"
#include "parsers/Common.h"
#include "parsers/SyntaxTheme.h"

namespace MathAnim
{
	SyntaxHighlighter::SyntaxHighlighter(const std::filesystem::path& grammar)
	{
		this->grammar = Grammar::importGrammar(grammar.string().c_str());
	}

	CodeHighlights SyntaxHighlighter::parse(const std::string& code, const SyntaxTheme& theme)
	{
		if (!this->grammar)
		{
			return {};
		}

		std::vector<GrammarMatch> matches = {};
		while (grammar->getNextMatch(code, &matches))
		{
			if (matches.size() > 30)
			{
				break;
			}
		}

		for (auto match : matches)
		{
			g_logger_info("Match for '%s': '%s'", match.name.friendlyName.c_str(), code.substr(match.start, (match.end - match.start)).c_str());
		}

		return {};
	}

	void SyntaxHighlighter::free()
	{
		if (grammar)
		{
			Grammar::free(grammar);
		}

		grammar = nullptr;
	}
}