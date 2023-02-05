#include "parsers/Common.h"

namespace MathAnim
{
	bool ScopedName::contains(const ScopedName& other, int* levelMatched) const
	{
		*levelMatched = 0;

		for (size_t index = 0; index < other.dotSeparatedScopes.size(); index++)
		{
			if (index >= dotSeparatedScopes.size())
			{
				break;
			}
			else if (dotSeparatedScopes[index] == other.dotSeparatedScopes[index])
			{
				*levelMatched = (*levelMatched) + 1;
			}
			else
			{
				// If we don't match all selectors then this is not a match
				*levelMatched = 0;
				break;
			}
		}

		return (*levelMatched) > 0;
	}

	bool ScopeRule::contains(const ScopeRule& other, int* levelMatched) const
	{
		if (scopes.size() > 0 && other.scopes.size() > 0)
		{
			return scopes[0].contains(other.scopes[0], levelMatched);
		}

		return false;
	}

	ScopeRule ScopeRule::from(const std::string& str)
	{
		ScopeRule res = {};

		res.friendlyName = str;

		size_t scopeStart = 0;
		ScopedName currentScope = {};
		for (size_t i = 0; i < str.length(); i++)
		{
			if (str[i] == '.' || str[i] == ' ')
			{
				std::string scope = str.substr(scopeStart, i - scopeStart);
				currentScope.dotSeparatedScopes.emplace_back(scope);
				scopeStart = i + 1;
			}
			
			// NOTE: Important that this is a separate if-stmt. This means the last dotted scope
			// will get added before starting the descendant scope after the space
			if (str[i] == ' ')
			{
				// Space separated scope
				res.scopes.emplace_back(currentScope);
				currentScope = {};
			}
		}

		// Don't forget about the final scope in the string
		if (scopeStart < str.length())
		{
			currentScope.dotSeparatedScopes.emplace_back(str.substr(scopeStart, str.length() - scopeStart));
		}

		if (currentScope.dotSeparatedScopes.size() > 0)
		{
			res.scopes.emplace_back(currentScope);
		}

		return res;
	}

	namespace Parser
	{
		ParserInfo openParserForFile(const char* filepath)
		{
			FILE* fp = fopen(filepath, "rb");
			if (!fp)
			{
				g_logger_warning("Could not load file '%s' while trying to open parser. Error: '%s'.", filepath, strerror(errno));
				return {};
			}

			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			ParserInfo parserInfo;
			parserInfo.cursor = 0;
			parserInfo.textLength = fileSize;
			parserInfo.text = (const char*)g_memory_allocate(sizeof(char) * fileSize);
			fread((void*)parserInfo.text, fileSize, 1, fp);
			fclose(fp);

			return parserInfo;
		}

		ParserInfo openParserForFile(const std::string& filepath)
		{
			return openParserForFile(filepath.c_str());
		}

		bool parseNumber(ParserInfo& parserInfo, float* out)
		{
			size_t numberStart = parserInfo.cursor;
			size_t numberEnd = parserInfo.cursor;

			bool seenDot = false;
			if (Parser::peek(parserInfo) == '-')
			{
				numberEnd++;
				Parser::advance(parserInfo);
			}

			while (Parser::isDigit(Parser::peek(parserInfo)) || (Parser::peek(parserInfo) == '.' && !seenDot))
			{
				seenDot = seenDot || Parser::peek(parserInfo) == '.';
				numberEnd++;
				Parser::advance(parserInfo);
			}

			if (numberEnd > numberStart)
			{
				constexpr int maxSmallBufferSize = 32;
				char smallBuffer[maxSmallBufferSize];
				g_logger_assert(numberEnd - numberStart <= maxSmallBufferSize, "Cannot parse number greater than %d characters big.", maxSmallBufferSize);
				g_memory_copyMem(smallBuffer, (void*)(parserInfo.text + numberStart), sizeof(char) * (numberEnd - numberStart));
				smallBuffer[numberEnd - numberStart] = '\0';
				// TODO: atof is not safe use a safer modern alternative
				*out = (float)atof(smallBuffer);
				return true;
			}

			*out = 0.0f;
			return false;
		}

		void skipWhitespaceAndCommas(ParserInfo& parserInfo)
		{
			while (Parser::isWhitespace(Parser::peek(parserInfo)) || Parser::peek(parserInfo) == ',')
			{
				Parser::advance(parserInfo);
				if (Parser::peek(parserInfo) == '\0')
					break;
			}
		}

		void skipWhitespace(ParserInfo& parserInfo)
		{
			while (Parser::isWhitespace(Parser::peek(parserInfo)))
			{
				Parser::advance(parserInfo);
				if (Parser::peek(parserInfo) == '\0')
					break;
			}
		}
	}
}