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

		bool contains(const ScopedName& other, int* levelMatched) const;
	};

	struct ScopeRule
	{
		std::vector<ScopedName> scopes;
		std::string friendlyName;

		bool contains(const ScopeRule& other, int* levelMatched) const;

		static ScopeRule from(const std::string& str);
	};

	namespace Parser
	{
		ParserInfo openParserForFile(const char* filepath);
		ParserInfo openParserForFile(const std::string& filepath);

		bool parseNumber(ParserInfo& parserInfo, float* out);
		void skipWhitespaceAndCommas(ParserInfo& parserInfo);
		void skipWhitespace(ParserInfo& parserInfo);

		inline char advance(ParserInfo& parserInfo) { char c = parserInfo.cursor < parserInfo.textLength ? parserInfo.text[parserInfo.cursor] : '\0'; parserInfo.cursor++; return c; }
		static inline char peek(const ParserInfo& parserInfo, int advance = 0) { return parserInfo.cursor + advance > parserInfo.textLength - 1 ? '\0' : parserInfo.text[parserInfo.cursor + advance]; }
		static inline bool isDigit(char c) { return (c >= '0' && c <= '9'); }
		static inline bool isNumberPart(char c) { return isDigit(c) || c == '-' || c == '.'; }
		static inline bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
		static inline bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0'; }
	}
}

#endif 