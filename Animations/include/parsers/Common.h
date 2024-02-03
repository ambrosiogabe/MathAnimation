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

	struct ScopeCapture
	{
		// This is the original capture text
		std::string captureRegex;
		// This is where we should start replacing the capture text with the capture
		size_t captureReplaceStart;
		// This is where we should end replacing the capture text with the capture
		size_t captureReplaceEnd;
		int captureIndex;
		std::string capture;
	};

	struct Scope
	{
		Scope() 
			: name(std::nullopt), capture(std::nullopt)
		{
		}

		std::optional<std::string> name;
		std::optional<ScopeCapture> capture;

		std::string getFriendlyName() const;
		const std::string& getScopeName() const;

		bool operator==(const Scope& other) const;
		bool operator!=(const Scope& other) const;

		static MathAnim::Scope from(const std::string& string);
	};

	struct ScopeSelector;
	struct ScopedName
	{
		std::vector<Scope> dotSeparatedScopes;

		bool matches(ScopeSelector const& other) const;
		std::string getFriendlyName() const;

		static ScopedName from(const std::string& string);

		bool strictEquals(ScopedName const& other) const
		{
			if (other.dotSeparatedScopes.size() != this->dotSeparatedScopes.size())
			{
				return false;
			}

			for (size_t i = 0; i < this->dotSeparatedScopes.size(); i++)
			{
				if (this->dotSeparatedScopes[i] != other.dotSeparatedScopes[i])
				{
					return false;
				}
			}

			return true;
		}
	};

	struct ScopeSelector
	{
		std::vector<std::string> dotSeparatedScopes;

		std::string getFriendlyName() const;

		static ScopeSelector from(const std::string& string);
	};

	struct ScopeDescendantSelector
	{
		std::vector<ScopeSelector> descendants;
	};

	struct ScopeSelectorCollection
	{
		std::vector<ScopeDescendantSelector> descendantSelectors;
		std::string friendlyName;

		static ScopeSelectorCollection from(std::string const& str);
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