#include "parsers/Common.h"

namespace MathAnim
{
	bool ScopedName::matches(const ScopedName& other, int* levelMatched, float* levelMatchPercent) const
	{
		*levelMatched = 0;

		bool matched = true;
		for (size_t index = 0; index < dotSeparatedScopes.size(); index++)
		{
			if (index >= other.dotSeparatedScopes.size())
			{
				break;
			}
			else if (dotSeparatedScopes[index] != other.dotSeparatedScopes[index])
			{
				matched = false;
				break;
			}

			*levelMatched = (*levelMatched) + 1;
		}

		*levelMatchPercent = (float)(*levelMatched) / (float)dotSeparatedScopes.size();

		return matched;
	}

	std::string ScopedName::getFriendlyName() const
	{
		std::string res = "";
		for (size_t i = 0; i < dotSeparatedScopes.size(); i++)
		{
			const auto& scope = dotSeparatedScopes[i];
			res += scope;
			if (i < dotSeparatedScopes.size() - 1)
			{
				res += ".";
			}
		}

		return res;
	}

	ScopedName ScopedName::from(const std::string& str)
	{
		size_t scopeStart = 0;
		ScopedName scope = {};
		for (size_t i = 0; i < str.length(); i++)
		{
			if (str[i] != '.' && str[i] != '-' && str[i] != '_' && !Parser::isAlpha(str[i]))
			{
				g_logger_error("Invalid scope name encountered. Scope name '{}' contains invalid characters.", str);
			}

			if (str[i] == '.')
			{
				std::string scopeStr = str.substr(scopeStart, i - scopeStart);
				scope.dotSeparatedScopes.emplace_back(scopeStr);
				scopeStart = i + 1;
			}
		}

		if (scopeStart > 0 && scopeStart < str.length())
		{
			// Add the last scope
			std::string scopeStr = str.substr(scopeStart, str.length() - scopeStart);
			scope.dotSeparatedScopes.emplace_back(scopeStr);
		}

		return scope;
	}

	bool ScopeRule::matches(const std::vector<ScopedName>& ancestors, int* scopeMatched, int* descendantMatched, int* levelMatched, float* levelMatchPercent) const
	{
		*descendantMatched = 0;
		*levelMatched = 0;

		if (scopes.size() == 0)
		{
			return false;
		}

		// ScopeRule: "string"
		// ScopeRule: "source string"
		// ScopeRule: "source"
		// Scope:     "foo source.php string.quoted"
		for (size_t i = 0; i < ancestors.size(); i++)
		{
			if (scopes[*descendantMatched].matches(ancestors[i], levelMatched, levelMatchPercent))
			{
				*descendantMatched = (*descendantMatched + 1);
				i = (i + 1);
				while (*descendantMatched < scopes.size() && i < ancestors.size())
				{
					// Didn't get a full match on the rule, so this doesn't count as a match
					if (!scopes[*descendantMatched].matches(ancestors[i], levelMatched, levelMatchPercent))
					{
						*descendantMatched = 0;
						*levelMatched = 0;
						return false;
					}

					*descendantMatched = (*descendantMatched) + 1;
				}

				// We matched as much as we could, which means this rule matches this ancestor hiearchy
				// The deepest level we matched is i (0-indexed)
				*scopeMatched = *descendantMatched - 1;
				*descendantMatched = ((int)i + 1);
				return true;
			}
		}

		return false;
	}

	bool ScopeRuleCollection::matches(const std::vector<ScopedName>& ancestors, int* ruleMatched, int* scopeMatched, int* descendantMatched, int* levelMatched, float* levelMatchPercent) const
	{
		bool matched = false;
		int descendantMatchedLocal = 0;
		int levelMatchedLocal = 0;
		float levelMatchPercentLocal = 0.0f;
		int scopeMatchedLocal = 0;
		for (size_t i = 0; i < scopeRules.size(); i++)
		{
			const auto& scopeRule = scopeRules[i];
			if (scopeRule.matches(ancestors, &scopeMatchedLocal, &descendantMatchedLocal, &levelMatchedLocal, &levelMatchPercentLocal))
			{
				if ((descendantMatchedLocal > *descendantMatched) ||
					(descendantMatchedLocal == *descendantMatched && levelMatchPercentLocal > *levelMatchPercent) ||
					(descendantMatchedLocal == *descendantMatched && levelMatchPercentLocal == *levelMatchPercent 
						&& levelMatchedLocal > *levelMatched))
				{
					*scopeMatched = scopeMatchedLocal;
					*descendantMatched = descendantMatchedLocal;
					*levelMatched = levelMatchedLocal;
					*levelMatchPercent = levelMatchPercentLocal;
					*ruleMatched = (int)i;
				}
				matched = true;
			}
		}

		return matched;
	}

	ScopeRuleCollection ScopeRuleCollection::from(const std::string& str)
	{
		ScopeRuleCollection res = {};

		res.friendlyName = str;

		size_t scopeStart = 0;
		ScopeRule currentRule = {};
		ScopedName currentScope = {};
		for (size_t i = 0; i < str.length(); i++)
		{
			if (str[i] == '.' || str[i] == ' ' || str[i] == ',')
			{
				std::string scope = str.substr(scopeStart, i - scopeStart);
				if (scope != "")
				{
					currentScope.dotSeparatedScopes.emplace_back(scope);
				}
				scopeStart = i + 1;
			}

			// NOTE: Important that this is a separate if-stmt. This means the last dotted scope
			// will get added before starting the descendant scope after the space
			if (str[i] == ' ' || str[i] == ',')
			{
				// Space separated scope
				if (currentScope.dotSeparatedScopes.size() > 0)
				{
					currentRule.scopes.emplace_back(currentScope);
				}
				currentScope = {};
			}

			if (str[i] == ',')
			{
				res.scopeRules.emplace_back(currentRule);
				currentRule = {};
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
			currentRule.scopes.emplace_back(currentScope);
		}

		if (currentRule.scopes.size() > 0)
		{
			res.scopeRules.emplace_back(currentRule);
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
				g_logger_warning("Could not load file '{}' while trying to open parser. Error: '{}'.", filepath, strerror(errno));
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

		void freeParser(ParserInfo& parser)
		{
			if (parser.text)
			{
				g_memory_free((void*)parser.text);
			}

			parser.text = nullptr;
			parser.textLength = 0;
			parser.cursor = 0;
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
				g_logger_assert(numberEnd - numberStart <= maxSmallBufferSize, "Cannot parse number greater than '{}' characters big.", maxSmallBufferSize);
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