#include "parsers/Common.h"

namespace MathAnim
{
	std::string Scope::getFriendlyName() const
	{
		if (name.has_value())
		{
			return name.value();
		}
		else if (capture.has_value())
		{
			return capture->capture + "[$" + std::to_string(capture->captureIndex) + "]";
		}

		static std::string undefined = "UNDEFINED";
		return undefined;
	}

	const std::string& Scope::getScopeName() const
	{
		if (name.has_value())
		{
			return name.value();
		}
		else if (capture.has_value())
		{
			return capture->capture;
		}

		static std::string undefined = "UNDEFINED";
		return undefined;
	}

	Scope Scope::from(const std::string& string)
	{
		Scope res;
		res.capture = std::nullopt;
		res.name = std::nullopt;

		if (string.length() == 0)
		{
			return res;
		}

		// This is a capture scope
		if (string[0] == '$')
		{
			for (size_t i = 1; i < string.length(); i++)
			{
				if (!Parser::isDigit(string[i]))
				{
					g_logger_error("Invalid scope '{}' encountered while parsing scoped name. Capture scopes must start with '$' and consist of only digits after the '$'.", string);
					return res;
				}
			}

			int captureIndex = std::stoi(string.substr(1));
			res.capture = {
				captureIndex,
				""
			};
		}
		else
		{
			// NOTE: Apparently textmate allows wildcards in scopes, so *url* is a valid selector even though
			//       it's not a valid CSS selector.

			// NOTE: I will ignore any unicode in my matching
			// Valid scopes are any valid CSS name which follows this grammar:
			// 
			// nmstart      [_a-z]
			// nmchar       [_a-z0-9-]
			// ident        -?{nmstart}{nmchar}*
			if (!Parser::isAlpha(string[0]) && string[0] != '_' && string[0] != '-' && string[0] != '*')
			{
				g_logger_error("Invalid scope '{}' encountered while parsing scoped name. Scope must start with '_', '-', or alpha character", string);
				return res;
			}

			for (size_t i = 1; i < string.length(); i++)
			{
				if (!Parser::isAlpha(string[i]) && !Parser::isDigit(string[i]) && string[i] != '-' && string[i] != '_' && string[i] != '*')
				{
					g_logger_error("Invalid scope '{}' encountered while parsing scoped name. Scope can only consist of alpha characters, digits, and dashes.", string);
					return res;
				}
			}

			res.name = string;
		}

		return res;
	}

	bool Scope::operator==(const Scope& other) const
	{
		return this->getScopeName() == other.getScopeName();
	}

	bool Scope::operator!=(const Scope& other) const
	{
		return this->getScopeName() != other.getScopeName();
	}

	std::optional<ScopedNameMatch> ScopedName::matches(const ScopedName& other) const
	{
		bool matched = true;
		int levelMatched = 0;
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

			levelMatched = (levelMatched)+1;
		}

		//*levelMatchPercent = (float)(*levelMatched) / (float)dotSeparatedScopes.size();
		if (matched)
		{
			return ScopedNameMatch{ levelMatched };
		}

		return std::nullopt;
	}

	std::string ScopedName::getFriendlyName() const
	{
		std::string res = "";
		for (size_t i = 0; i < dotSeparatedScopes.size(); i++)
		{
			const auto& scope = dotSeparatedScopes[i];
			res += scope.getFriendlyName();
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
			if (str[i] == '.')
			{
				std::string scopeStr = str.substr(scopeStart, i - scopeStart);
				scope.dotSeparatedScopes.emplace_back(Scope::from(scopeStr));
				scopeStart = i + 1;
			}
		}

		if (scopeStart > 0 && scopeStart < str.length())
		{
			// Add the last scope
			std::string scopeStr = str.substr(scopeStart, str.length() - scopeStart);
			scope.dotSeparatedScopes.emplace_back(Scope::from(scopeStr));
		}

		return scope;
	}

	std::optional<ScopeRuleMatch> ScopeRule::matches(const std::vector<ScopedName>& ancestors) const
	{
		if (scopes.size() == 0)
		{
			return std::nullopt;
		}

		ScopeRuleMatch res = {};
		size_t scopeIndex = 0;
		for (size_t ancestorIndex = 0; ancestorIndex < ancestors.size(); ancestorIndex++)
		{
			auto scopeNameMatch = scopes[scopeIndex].matches(ancestors[ancestorIndex]);
			if (scopeNameMatch.has_value())
			{
				res.ancestorMatches.emplace_back(scopeNameMatch.value());
				res.ancestorNames.push_back(ancestors[ancestorIndex]);
				res.deepestScopeMatched = (int)ancestorIndex + 1;

				ancestorIndex = (ancestorIndex + 1);
				scopeIndex = (scopeIndex + 1);
				while (scopeIndex < scopes.size() && ancestorIndex < ancestors.size())
				{
					// Didn't get a full match on the rule, so this doesn't count as a match
					auto nextScopeNameMatch = scopes[scopeIndex].matches(ancestors[ancestorIndex]);
					if (!nextScopeNameMatch.has_value())
					{
						ancestorIndex++;
					}
					else
					{
						res.ancestorMatches.emplace_back(nextScopeNameMatch.value());
						res.ancestorNames.push_back(ancestors[ancestorIndex]);
						res.deepestScopeMatched = (int)ancestorIndex + 1;
						scopeIndex++;
					}
				}

				// Must match all scopes in the descendant heirarchy otherwise this isn't
				// actually a match
				if (scopeIndex != scopes.size())
				{
					return std::nullopt;
				}
				else
				{
					return res;
				}
			}
		}

		return std::nullopt;
	}

	std::optional<ScopeRuleCollectionMatch> ScopeRuleCollection::matches(const std::vector<ScopedName>& ancestors) const
	{
		std::optional<ScopeRuleCollectionMatch> res = std::nullopt;

		int deepestScopeMatched = 0;
		for (size_t i = 0; i < scopeRules.size(); i++)
		{
			const auto& scopeRule = scopeRules[i];
			auto scopeRuleMatch = scopeRule.matches(ancestors);
			if (!scopeRuleMatch.has_value())
			{
				continue;
			}

			if (scopeRuleMatch->deepestScopeMatched > deepestScopeMatched)
			{
				res = ScopeRuleCollectionMatch{
					(int)i,
					scopeRuleMatch.value()
				};
			}
			else if (scopeRuleMatch->deepestScopeMatched > 0 && scopeRuleMatch->deepestScopeMatched == deepestScopeMatched)
			{
				if (scopeRuleMatch->ancestorMatches.size() > res->scopeRule.ancestorMatches.size())
				{
					res = ScopeRuleCollectionMatch{
						(int)i,
						scopeRuleMatch.value()
					};
				}
				else if (scopeRuleMatch->ancestorMatches.size() == res->scopeRule.ancestorMatches.size())
				{
					if (scopeRuleMatch->ancestorMatches[0].levelMatched > res->scopeRule.ancestorMatches[0].levelMatched)
					{
						res = ScopeRuleCollectionMatch{
							(int)i,
							scopeRuleMatch.value()
						};
					}
				}
			}
		}

		return res;
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
			if (str[i] == '.' || str[i] == ' ' || str[i] == ',' || str[i] == '>')
			{
				std::string scope = str.substr(scopeStart, i - scopeStart);
				if (scope != "")
				{
					currentScope.dotSeparatedScopes.emplace_back(Scope::from(scope));
				}
				scopeStart = i + 1;

				if (str[i] == '>')
				{
					static bool shouldWarn = true;
					if (shouldWarn)
					{
						g_logger_warning("No support for child selectors yet '>' in SyntaxTheme's.");
						shouldWarn = false;
					}
				}
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
			currentScope.dotSeparatedScopes.emplace_back(Scope::from(str.substr(scopeStart, str.length() - scopeStart)));
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
				g_memory_copyMem(smallBuffer, maxSmallBufferSize,(void*)(parserInfo.text + numberStart), sizeof(char) * (numberEnd - numberStart));
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