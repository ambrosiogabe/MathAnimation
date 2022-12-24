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

	static int printMatches(char* bufferPtr, size_t bufferSizeLeft, const GrammarMatch& match, const std::string& code, int level = 1);
	static int printMatches(char* bufferPtr, size_t bufferSizeLeft, const GrammarMatch& match, const std::string& code, int level)
	{
		int totalNumBytesWritten = 0;
		std::string val = code.substr(match.start, (match.end - match.start)).c_str();
		for (size_t i = 0; i < val.length(); i++)
		{
			if (val[i] == '\n')
			{
				val[i] = '\\';
				val.insert(i + 1, "n");
				i++;
			}
		}

		int numBytesWritten;
		if (level > 1)
		{
			numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "%*c'%s': '%s'\n", level * 2, ' ', match.name.friendlyName.c_str(), val.c_str());
		}
		else
		{
			numBytesWritten = snprintf(bufferPtr, bufferSizeLeft, "'%s': '%s'\n", match.name.friendlyName.c_str(), val.c_str());
		}

		if (numBytesWritten > 0 && numBytesWritten < bufferSizeLeft)
		{
			bufferSizeLeft -= numBytesWritten;
			bufferPtr += numBytesWritten;
		}
		else
		{
			return -1;
		}

		totalNumBytesWritten += numBytesWritten;

		for (auto subMatch : match.subMatches)
		{
			numBytesWritten = printMatches(bufferPtr, bufferSizeLeft, subMatch, code, level + 1);
			totalNumBytesWritten += numBytesWritten;
			if (numBytesWritten > 0 && numBytesWritten < bufferSizeLeft)
			{
				bufferSizeLeft -= numBytesWritten;
				bufferPtr += numBytesWritten;
			}
			else
			{
				return -1;
			}
		}

		return totalNumBytesWritten;
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

		constexpr size_t bufferSize = 1024 * 10;
		char buffer[bufferSize] = "\0";
		char* bufferPtr = buffer;
		size_t bufferSizeLeft = bufferSize;
		for (auto match : matches)
		{
			int numBytesWritten = printMatches(bufferPtr, bufferSizeLeft, match, code);
			if (numBytesWritten > 0 && numBytesWritten < bufferSizeLeft)
			{
				bufferSizeLeft -= numBytesWritten;
				bufferPtr += numBytesWritten;
			}
			else
			{
				break;
			}
		}

		g_logger_info("Matches:\n%s", buffer);

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