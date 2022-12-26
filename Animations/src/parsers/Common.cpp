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
			else if (*levelMatched > 0)
			{
				// If the we matched a previous selector and stopped matching then
				// this is the deepest it goes
				break;
			}
		}

		return (*levelMatched) > 0;
	}

	ScopedName ScopedName::from(const std::string& str)
	{
		ScopedName res = {};

		res.friendlyName = str;

		size_t scopeStart = 0;
		for (size_t i = 0; i < str.length(); i++)
		{
			if (str[i] == '.')
			{
				std::string scope = str.substr(scopeStart, i - scopeStart);
				res.dotSeparatedScopes.emplace_back(scope);
				scopeStart = i + 1;
			}
			else if (str[i] == ' ')
			{
				// Space separated scope
			}
		}

		// Don't forget about the final scope in the string
		if (scopeStart < str.length())
		{
			res.dotSeparatedScopes.emplace_back(str.substr(scopeStart, str.length() - scopeStart));
		}

		return res;
	}
}