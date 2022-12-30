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
}