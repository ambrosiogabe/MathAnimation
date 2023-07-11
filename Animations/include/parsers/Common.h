#ifndef MATH_ANIM_PARSERS_COMMON_H
#define MATH_ANIM_PARSERS_COMMON_H
#include "core.h"

namespace MathAnim
{
	struct ParserInfo
	{
		const char* text;
		size_t textLength;
		size_t cursor;
	};

	struct ScopedName
	{
		std::vector<std::string> dotSeparatedScopes;

		bool matches(const ScopedName& other, int* levelMatched, float* levelMatchPercent) const;
		std::string getFriendlyName() const;

		static ScopedName from(const std::string& string);
	};

	// Information on how scopes work here https://macromates.com/manual/en/scope_selectors
	// 
	// "string" matches anything starting with "string"
	//   Examples: "string.quoted.double.cpp" "string.quoted" "string"
	//             are all valid matches for the selector "string"
	//
	// An empty scope matches all scopes, but has the lowest ranking.
	//
	// Descendants also work like CSS descendants. See the link above for more info.
	
	struct ScopeRule
	{
		std::vector<ScopedName> scopes;

		bool matches(const std::vector<ScopedName>& ancestors, int* scopeMatched, int* descendantMatched, int* levelMatched, float* levelMatchPercent) const;
	};

	struct ScopeRuleCollection
	{
		std::vector<ScopeRule> scopeRules;
		std::string friendlyName;

		bool matches(const std::vector<ScopedName>& ancestors, int* ruleMatched, int* scopeMatched, int* descendantMatched, int* levelMatched, float* levelMatchPercent) const;

		static ScopeRuleCollection from(const std::string& str);
	};

	namespace Parser
	{
		ParserInfo openParserForFile(const char* filepath);
		ParserInfo openParserForFile(const std::string& filepath);
		void freeParser(ParserInfo& parser);

		bool parseNumber(ParserInfo& parserInfo, float* out);
		void skipWhitespaceAndCommas(ParserInfo& parserInfo);
		void skipWhitespace(ParserInfo& parserInfo);

		inline char advance(ParserInfo& parserInfo) { char c = parserInfo.cursor < parserInfo.textLength ? parserInfo.text[parserInfo.cursor] : '\0'; parserInfo.cursor++; return c; }
		inline char peek(const ParserInfo& parserInfo, int advance = 0) { return parserInfo.cursor + advance > parserInfo.textLength - 1 ? '\0' : parserInfo.text[parserInfo.cursor + advance]; }
		inline bool consume(ParserInfo& parserInfo, char expected) { if (peek(parserInfo) == expected) { advance(parserInfo); return true; } return false; }
		inline bool isDigit(char c) { return (c >= '0' && c <= '9'); }
		inline bool isNumberPart(char c) { return isDigit(c) || c == '-' || c == '.'; }
		inline bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
		inline bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0'; }

		inline bool consumeNewline(ParserInfo& parser)
		{
			if (consume(parser, '\n'))
			{
				return true;
			}

			// could be carriage return \r\n
			if (consume(parser, '\r'))
			{
				if (consume(parser, '\n'))
				{
					return true;
				}
			}

			return false;
		}
	}
}

#endif 