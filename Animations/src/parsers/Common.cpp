#include "parsers/Common.h"

namespace MathAnim
{
	bool ScopedName::contains(const ScopedName& other, int* levelMatched) const
	{
		*levelMatched = 0;

		for (auto iter = other.scopes.crbegin(); iter != other.scopes.crend(); iter++)
		{
			*levelMatched = 0;

			for (auto myIter = scopes.cbegin(); myIter != scopes.cend(); myIter++)
			{
				if (*myIter == *iter)
				{
					*levelMatched = (*levelMatched) + 1;
				}
			}

			if ((*levelMatched) > 0)
			{
				return true;
			}
		}

		return false;
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
				res.scopes.emplace_back(scope);
				scopeStart = i + 1;
			}
		}

		// Don't forget about the final scope in the string
		if (scopeStart < str.length())
		{
			res.scopes.emplace_back(str.substr(scopeStart, str.length() - scopeStart));
		}

		return res;
	}
}