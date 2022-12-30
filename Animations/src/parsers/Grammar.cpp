#include "parsers/Grammar.h"
#include "platform/Platform.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	using namespace nlohmann;

	// ----------- Internal Functions -----------
	static Grammar* importGrammarFromJson(const json& j);
	static SyntaxPattern parsePattern(const std::string& key, const json& json);
	static std::vector<SyntaxPattern> parsePatternsArray(const json& json);
	static regex_t* onigFromString(const std::string& str, bool multiline);
	static std::optional<GrammarMatch> getFirstMatch(const std::string& str, size_t start, size_t end, regex_t* reg, OnigRegion* region);
	static std::vector<GrammarMatch> checkForMatches(const std::string& str, size_t start, size_t end, regex_t* reg, OnigRegion* region, std::optional<CaptureList> captures);

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
				ScopeRule scope = ScopeRule::from(val["name"]);
				Capture cap = {};
				cap.index = captureNumber;
				cap.scope = scope;
				res.captures.emplace_back(cap);
			}
			else
			{
				g_logger_warning("Invalid capture list. Capture '%s' does not contain a 'name' key.", key.c_str());
			}
		}

		return res;
	}

	bool SimpleSyntaxPattern::match(const std::string& str, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const
	{
		std::vector<GrammarMatch> subMatches = checkForMatches(str, start, end, this->regMatch, region, this->captures);

		bool captureWholePattern = false;
		if (this->scope.has_value())
		{
			std::optional<GrammarMatch> match = getFirstMatch(str, start, end, this->regMatch, region);
			if (match.has_value() && match->start < end && match->end <= end)
			{
				match->subMatches.insert(match->subMatches.end(), subMatches.begin(), subMatches.end());
				match->scope = (*this->scope);
				outMatches->push_back(*match);
				captureWholePattern = true;
			}
		}

		if (!captureWholePattern)
		{
			// If this rule has no scoped name, then add all the submatches by themselves
			outMatches->insert(outMatches->end(), subMatches.begin(), subMatches.end());
		}

		return subMatches.size() > 0 || captureWholePattern;
	}

	void SimpleSyntaxPattern::free()
	{
		if (regMatch)
		{
			onig_free(regMatch);
		}

		regMatch = nullptr;
	}

	bool ComplexSyntaxPattern::match(const std::string& str, size_t start, size_t endOffset, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const
	{
		// If the begin/end pair doesn't have a match, then this rule isn't a success
		std::optional<GrammarMatch> beginBlockMatch = getFirstMatch(str, start, endOffset, this->begin, region);
		if (!beginBlockMatch.has_value() || beginBlockMatch->start >= endOffset)
		{
			return false;
		}

		size_t endBlockOffset = beginBlockMatch->end - start;
		// This match can go to the end of the string
		std::optional<GrammarMatch> endBlockMatch = getFirstMatch(str, start + endBlockOffset, str.length(), this->end, region);
		if (!endBlockMatch.has_value())
		{
			GrammarMatch eof;
			eof.start = str.length();
			eof.end = str.length();
			eof.scope = ScopeRule::from("EOF_NO_END_MATCH");

			// If there was no end group matched, then automatically use the end of the file as the end block
			// which is specified in the rules for textmates
			endBlockMatch = eof;
		}

		// If the begin/end pair *does* have a match, then get all grammar matches for the begin/end blocks and
		// run the extra patterns against the text between begin/end
		std::vector<GrammarMatch> beginMatches = checkForMatches(str, beginBlockMatch->start, beginBlockMatch->end, this->begin, region, this->beginCaptures);
		std::vector<GrammarMatch> endMatches = checkForMatches(str, endBlockMatch->start, endBlockMatch->end, this->end, region, this->endCaptures);

		GrammarMatch res = {};
		res.subMatches.insert(res.subMatches.end(), beginMatches.begin(), beginMatches.end());

		if (this->patterns.has_value())
		{
			size_t inBetweenStart = beginBlockMatch->end;
			size_t inBetweenEnd = endBlockMatch->start;
			for (auto pattern : *patterns)
			{
				pattern.match(str, inBetweenStart, inBetweenEnd, repo, region, &res.subMatches);
			}
		}

		res.subMatches.insert(res.subMatches.end(), endMatches.begin(), endMatches.end());

		if (this->scope.has_value())
		{
			res.start = beginBlockMatch->start;
			res.end = endBlockMatch->end;
			res.scope = *this->scope;
			outMatches->push_back(res);
			return true;
		}

		outMatches->insert(outMatches->end(), res.subMatches.begin(), res.subMatches.end());
		return res.subMatches.size() > 0;
	}

	void ComplexSyntaxPattern::free()
	{
		if (begin)
		{
			onig_free(begin);
		}

		if (end)
		{
			onig_free(end);
		}

		begin = nullptr;
		end = nullptr;
	}

	void PatternArray::free()
	{
		for (auto pattern : patterns)
		{
			pattern.free();
		}
	}

	bool SyntaxPattern::match(const std::string& str, size_t start, size_t end, const PatternRepository& repo, OnigRegion* region, std::vector<GrammarMatch>* outMatches) const
	{
		switch (type)
		{
		case PatternType::Array:
		{
			if (patternArray.has_value())
			{
				for (auto pattern : patternArray->patterns)
				{
					if (pattern.match(str, start, end, repo, region, outMatches))
					{
						return true;
					}
				}
			}
		}
		break;
		case PatternType::Complex:
		{
			if (complexPattern.has_value())
			{
				return complexPattern->match(str, start, end, repo, region, outMatches);
			}
		}
		break;
		case PatternType::Include:
		{
			if (patternInclude.has_value())
			{
				auto iter = repo.patterns.find(patternInclude.value());
				if (iter != repo.patterns.end())
				{
					return iter->second.match(str, start, end, repo, region, outMatches);
				}
				g_logger_warning("Unable to resolve pattern reference '%s'.", patternInclude.value().c_str());
			}
		}
		break;
		case PatternType::Simple:
		{
			if (simplePattern.has_value())
			{
				return simplePattern->match(str, start, end, repo, region, outMatches);
			}
		}
		case PatternType::Invalid:
			break;
		}

		return false;
	}

	void SyntaxPattern::free()
	{
		switch (type)
		{
		case PatternType::Simple:
		{
			if (simplePattern.has_value())
			{
				simplePattern->free();
			}
		}
		break;
		case PatternType::Complex:
		{
			if (complexPattern.has_value())
			{
				complexPattern->free();
			}
		}
		break;
		case PatternType::Array:
		{
			if (patternArray.has_value())
			{
				patternArray->free();
			}
		}
		break;
		case PatternType::Include:
		case PatternType::Invalid:
			break;
		}
	}

	bool Grammar::getNextMatch(const std::string& code, std::vector<GrammarMatch>* outMatches) const
	{
		size_t start = 0;
		if (outMatches->size() > 0)
		{
			start = (*outMatches)[outMatches->size() - 1].end;
		}

		size_t lineEnd = start;
		while (lineEnd < code.length())
		{
			for (; lineEnd < code.length(); lineEnd++)
			{
				if (code[lineEnd] == '\n')
				{
					lineEnd++;
					break;
				}
			}

			std::vector<GrammarMatch> tmpRes = {};
			for (auto pattern : this->patterns)
			{
				std::vector<GrammarMatch> currentRes = {};
				if (pattern.match(code, start, lineEnd, this->repository, region, &currentRes))
				{
					// The first match wins
					if (tmpRes.size() == 0 || (
						currentRes[0].start < tmpRes[0].start
						))
					{
						tmpRes = currentRes;
					}
				}
			}

			// The first match wins
			if (tmpRes.size() > 0)
			{
				outMatches->insert(outMatches->end(), tmpRes.begin(), tmpRes.end());
				return true;
			}

			start = lineEnd;
		}

		return false;
	}

	Grammar* Grammar::importGrammar(const char* filepath)
	{
		if (!Platform::fileExists(filepath))
		{
			g_logger_warning("Tried to parse grammar at filepath that does not exist: '%s'", filepath);
			return nullptr;
		}

		std::ifstream file(filepath);

		try
		{
			json j = json::parse(file, nullptr, true, true);
			return importGrammarFromJson(j);
		}
		catch (json::exception& ex)
		{
			g_logger_error("Error importing language grammar: '%s'\n\t%s", filepath, ex.what());
		}

		return nullptr;
	}

	void Grammar::free(Grammar* grammar)
	{
		if (grammar)
		{
			for (size_t i = 0; i < grammar->patterns.size(); i++)
			{
				grammar->patterns[i].free();
			}

			if (grammar->region)
			{
				onig_region_free(grammar->region, 1);
			}
			grammar->region = nullptr;

			grammar->~Grammar();
			g_memory_free(grammar);
		}
	}

	// ----------- Internal Functions -----------
	static Grammar* importGrammarFromJson(const json& j)
	{
		Grammar* res = (Grammar*)g_memory_allocate(sizeof(Grammar));
		new(res)Grammar();

		res->region = onig_region_new();

		if (j.contains("name"))
		{
			res->name = j["name"];
		}

		if (j.contains("scopeName"))
		{
			const std::string& scope = j["scopeName"];
			res->scope = ScopeRule::from(scope);
		}

		res->repository = {};
		if (j.contains("repository"))
		{
			for (auto& [key, val] : j["repository"].items())
			{
				SyntaxPattern pattern = parsePattern(key, val);
				if (pattern.type != PatternType::Invalid)
				{
					res->repository.patterns[key] = pattern;
				}
				else
				{
					g_logger_warning("Invalid pattern parsed.");
				}
			}
		}

		if (j.contains("patterns"))
		{
			const json& patternsArray = j["patterns"];
			res->patterns = parsePatternsArray(patternsArray);
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

			SimpleSyntaxPattern p = {};
			p.regMatch = onigFromString(json["match"], false);

			if (json.contains("name"))
			{
				p.scope = ScopeRule::from(json["name"]);
			}
			else
			{
				p.scope = std::nullopt;
			}

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

			if (!json.contains("end"))
			{
				g_logger_error("Pattern '%s' has invalid complex pattern. Pattern has begin, but no end.");
				return dummy;
			}

			ComplexSyntaxPattern c = {};
			c.begin = onigFromString(json["begin"], false);
			c.end = onigFromString(json["end"], true);

			if (json.contains("name"))
			{
				c.scope = ScopeRule::from(json["name"]);
			}
			else
			{
				c.scope = std::nullopt;
			}

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

	static regex_t* onigFromString(const std::string& str, bool multiline)
	{
		const char* pattern = str.c_str();
		const char* patternEnd = pattern + str.length();

		int options = ONIG_OPTION_NONE;
		options |= ONIG_OPTION_CAPTURE_GROUP;
		options |= ONIG_OPTION_MULTILINE;

		regex_t* reg;
		OnigErrorInfo error;
		int parseRes = onig_new(
			&reg,
			(uint8*)pattern,
			(uint8*)patternEnd,
			options,
			ONIG_ENCODING_ASCII,
			ONIG_SYNTAX_DEFAULT,
			&error
		);

		if (parseRes != ONIG_NORMAL)
		{
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str((UChar*)s, parseRes, &error);
			g_logger_error("Oniguruma Error: %s", s);
			return nullptr;
		}

		return reg;
	}

	static std::optional<GrammarMatch> getFirstMatch(const std::string& str, size_t startOffset, size_t endOffset, regex_t* reg, OnigRegion* region)
	{
		std::optional<GrammarMatch> res = std::nullopt;

		const char* cstr = str.c_str();
		const char* start = cstr + startOffset;
		const char* end = cstr + str.length();
		const char* range = cstr + endOffset;
		int searchRes = onig_search(
			reg,
			(uint8*)cstr,
			(uint8*)end,
			(uint8*)start,
			(uint8*)range,
			region,
			ONIG_OPTION_NONE
		);

		if (searchRes >= 0)
		{
			if (region->num_regs > 0)
			{
				// Only accept valid matches
				if (region->beg[0] >= 0 && region->end[0] > region->beg[0])
				{
					GrammarMatch match = {};
					match.start = (size_t)region->beg[0];
					match.end = (size_t)region->end[0];
					match.scope = ScopeRule::from("FIRST_MATCH");
					res = match;
				}
			}
		}
		else if (searchRes != ONIG_MISMATCH)
		{
			// Error
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str((UChar*)s, searchRes);
			g_logger_error("Oniguruma Error: %s", s);
			onig_region_free(region, 1 /* 1:free self, 0:free contents only */);
		}

		return res;
	}

	static std::vector<GrammarMatch> checkForMatches(const std::string& str, size_t startOffset, size_t endOffset, regex_t* reg, OnigRegion* region, std::optional<CaptureList> captures)
	{
		std::vector<GrammarMatch> res = {};

		const char* cstr = str.c_str();
		const char* start = cstr + startOffset;
		const char* end = cstr + str.length();
		const char* range = cstr + endOffset;
		int searchRes = onig_search(
			reg,
			(uint8*)cstr,
			(uint8*)end,
			(uint8*)start,
			(uint8*)range,
			region,
			ONIG_OPTION_NONE
		);

		if (searchRes >= 0)
		{
			if (captures.has_value())
			{
				for (auto capture : captures->captures)
				{
					if (region->num_regs > capture.index)
					{
						// Only accept valid matches
						if (region->beg[capture.index] >= 0 && region->end[capture.index] > region->beg[capture.index])
						{
							GrammarMatch match = {};
							match.start = (size_t)region->beg[capture.index];
							match.end = (size_t)region->end[capture.index];
							match.scope = capture.scope;
							res.emplace_back(match);
						}
					}
				}
			}
		}
		else if (searchRes != ONIG_MISMATCH)
		{
			// Error
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str((UChar*)s, searchRes);
			g_logger_error("Oniguruma Error: %s", s);
			onig_region_free(region, 1 /* 1:free self, 0:free contents only */);
		}

		return res;
	}
}