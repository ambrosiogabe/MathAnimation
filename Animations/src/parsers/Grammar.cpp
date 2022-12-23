#include "parsers/Grammar.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	using namespace nlohmann;

	// ----------- Internal Functions -----------
	static Grammar importGrammarFromJson(const json& j);
	static SyntaxPattern parsePattern(const std::string& key, const json& json);
	static std::vector<SyntaxPattern> parsePatternsArray(const json& json);

	ScopedName ScopedName::from(const std::string& str)
	{
		ScopedName res = {};

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

	CaptureList CaptureList::from(const json& j)
	{
		CaptureList res = {};

		for (auto& [key, val] : j.items())
		{
			size_t captureNumber = 0;
			try
			{
				captureNumber = std::stoi(key);
			}
			catch (std::invalid_argument const& ex)
			{
				g_logger_warning("Invalid capture list. Key '%s' is not a number.\n%s", key.c_str(), ex.what());
				continue;
			}
			catch (std::out_of_range const& ex)
			{
				g_logger_warning("Invalid capture list. Key '%s' is too large.\n%s", key.c_str(), ex.what());
				continue;
			}

			if (val.contains("name"))
			{
				ScopedName name = ScopedName::from(val["name"]);
				res.captures[captureNumber] = name;
			}
			else
			{
				g_logger_warning("Invalid capture list. Capture '%s' does not contain a 'name' key.", key.c_str());
			}
		}

		return res;
	}

	static Grammar importGrammarFromString(const char* string)
	{
		json j = json::parse(string);
		return importGrammarFromJson(j);
	}

	Grammar Grammar::importGrammar(const char* filepath)
	{
		std::ifstream file(filepath);
		json j = json::parse(file);

		return importGrammarFromJson(j);
	}

	// ----------- Internal Functions -----------
	static Grammar importGrammarFromJson(const json& j)
	{
		Grammar res = {};

		if (j.contains("name"))
		{
			res.name = j["name"];
		}

		if (j.contains("scopeName"))
		{
			const std::string& scope = j["scopeName"];
			res.scopeName = ScopedName::from(scope);
		}

		// First parse all repositories that don't include other repositories
		if (j.contains("repository"))
		{
			for (auto& [key, val] : j["repository"].items())
			{
				PatternRepository repository = {};
				repository.repositoryName = key;
				SyntaxPattern pattern = parsePattern(key, val);
				if (pattern.type != PatternType::Invalid)
				{
					repository.patterns.emplace_back(pattern);
				}
				else
				{
					g_logger_warning("Invalid pattern parsed.");
				}

				res.repositories[key] = repository;
			}
		}

		if (j.contains("patterns"))
		{
			const json& patternsArray = j["patterns"];
			res.patterns = parsePatternsArray(patternsArray);
		}

		return res;
	}

	static SyntaxPattern parsePattern(const std::string& key, const json& json)
	{
		SyntaxPattern dummy = {};
		dummy.type = PatternType::Invalid;

		// Include patterns are the easiest... So do those first
		if (json.contains("include"))
		{
			std::string includeMatch = json["include"];
			if (includeMatch.length() > 0 && includeMatch[0] == '#')
			{
				includeMatch = includeMatch.substr(1);
			}

			SyntaxPattern patternRef;
			patternRef.type = PatternType::Include;
			patternRef.patternInclude = includeMatch;

			return patternRef;
		}
		// match is mutually exclusive with begin/end. So this is a simple pattern.
		else if (json.contains("match"))
		{
			SyntaxPattern res;
			res.type = PatternType::Simple;
			if (json.contains("name"))
			{
				res.name = ScopedName::from(json["name"]);
			}
			else
			{
				res.name = std::nullopt;
			}

			SimpleSyntaxPattern p = {};
			p.match = std::string(json["match"]);

			if (json.contains("captures"))
			{
				p.captures = CaptureList::from(json["captures"]);
			}
			else
			{
				p.captures = std::nullopt;
			}

			res.simplePattern = p;
			return res;
		}
		// begin/end is mutually exclusive with match. So this is a complex pattern.
		else if (json.contains("begin"))
		{
			SyntaxPattern res;
			res.type = PatternType::Complex;
			if (json.contains("name"))
			{
				res.name = ScopedName::from(json["name"]);
			}
			else
			{
				res.name = std::nullopt;
			}

			if (!json.contains("end"))
			{
				g_logger_error("Pattern '%s' has invalid complex pattern. Pattern has begin, but no end.");
				return dummy;
			}

			ComplexSyntaxPattern c = {};
			c.begin = std::string(json["begin"]);
			c.end = std::string(json["end"]);

			if (json.contains("beginCaptures"))
			{
				c.beginCaptures = CaptureList::from(json["beginCaptures"]);
			}
			else
			{
				c.beginCaptures = std::nullopt;
			}

			if (json.contains("endCaptures"))
			{
				c.endCaptures = CaptureList::from(json["endCaptures"]);
			}
			else
			{
				c.endCaptures = std::nullopt;
			}

			if (json.contains("patterns"))
			{
				c.patterns = parsePatternsArray(json["patterns"]);
			}
			else
			{
				c.patterns = std::nullopt;
			}

			res.complexPattern = c;
			return res;
		}
		// If this pattern only contains a patterns array then return
		// that
		else if (json.contains("patterns"))
		{
			SyntaxPattern res = {};
			res.type = PatternType::Array;
			PatternArray pa = {};
			pa.patterns = parsePatternsArray(json["patterns"]);
			res.patternArray = pa;
			return res;
		}

		return dummy;
	}

	static std::vector<SyntaxPattern> parsePatternsArray(const json& json)
	{
		std::vector<SyntaxPattern> res;

		for (json::const_iterator it = json.begin(); it != json.end(); it++)
		{
			SyntaxPattern pattern = parsePattern("anonymous", *it);
			if (pattern.type != PatternType::Invalid)
			{
				res.emplace_back(pattern);
			}
			else
			{
				g_logger_warning("Invalid pattern parsed.");
			}
		}

		return res;
	}
}