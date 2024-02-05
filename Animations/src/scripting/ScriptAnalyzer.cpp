#include "scripting/ScriptAnalyzer.h"
#include "scripting/MathAnimGlobals.h"
#include "platform/Platform.h"
#include "editor/panels/ConsoleLog.h"

#include <rapidfuzz/fuzz.hpp>

#pragma warning( push )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4324 )
#pragma warning( disable : 4324 )
#include <lua.h>
#include <luacode.h>
#include <lualib.h>
#include <Luau/BuiltinDefinitions.h>
#include <Luau/Frontend.h>
#pragma warning( pop )

// ------------------------------- Internal Types -------------------------------
struct ScriptFileResolver : public Luau::FileResolver
{
	std::string anonymousSource;
	std::string anonymousName;
	const std::filesystem::path scriptDirectory;

	ScriptFileResolver(const std::filesystem::path& scriptDirectory);

	void setAnonymousFile(const std::string& source, const std::string& name);

	virtual std::optional<Luau::SourceCode> readSource(const Luau::ModuleName& name) override;

	std::optional<Luau::ModuleInfo> resolveModule(const Luau::ModuleInfo* context, Luau::AstExpr* node) override;

	std::string getHumanReadableModuleName(const Luau::ModuleName& name) const override;
};

struct ScriptConfigResolver : public Luau::ConfigResolver
{
	Luau::Config defaultConfig;

	ScriptConfigResolver();

	virtual const Luau::Config& getConfig(const Luau::ModuleName& name) const override;
};

enum class ReportFormat
{
	Default,
	Luacheck,
	Gnu,
};

// ------------------------------- Internal Functions -------------------------------
static void reportError(const Luau::Frontend* frontend, const char* filepath, ReportFormat format, const Luau::TypeError& error);
static void report(ReportFormat format, const char* filepath, const Luau::Location& loc, const char* type, const char* message);

// -- Internal Data --
constexpr auto autocompleteKindPrecedence = fixedSizeArray<int, (int)Luau::AutocompleteEntryKind::GeneratedFunction + 1>(
	0, // Property,
	1, // Binding,
	4, // Keyword,
	5, // String,
	2, // Type,
	3, // Module,
	6 // GeneratedFunction,
);

namespace MathAnim
{
	ScriptAnalyzer::ScriptAnalyzer(const std::filesystem::path& scriptDirectory)
		: m_scriptDirectory(scriptDirectory)
	{
		Luau::FrontendOptions frontendOptions;
		frontendOptions.retainFullTypeGraphs = true;

		fileResolver = g_memory_new ScriptFileResolver(scriptDirectory);
		configResolver = g_memory_new ScriptConfigResolver();
		frontend = g_memory_new Luau::Frontend(fileResolver, configResolver, frontendOptions);

		// Register the bundled builtin globals that come with Luau
		Luau::registerBuiltinGlobals(*frontend, frontend->globals, true);

		Luau::freeze(frontend->globals.globalTypes);
		Luau::freeze(frontend->globalsForAutocomplete.globalTypes);

		{
			// Register our own builtin globals 
			Luau::GlobalTypes& globals = frontend->globalsForAutocomplete;
			unfreeze(globals.globalTypes);
			Luau::LoadDefinitionFileResult loadResult = frontend->loadDefinitionFile(
				globals,
				globals.globalScope,
				MathAnimGlobals::getBuiltinDefinitionSource(),
				"math-anim",
				false, /* Capture comments */
				true /* Typecheck for autocomplete */
			);
			freeze(globals.globalTypes);

			if (!loadResult.success)
			{
				g_logger_error("The ScriptAnalyzer failed to load math-anim builtin definitions. Errors:");
				for (int i = 0; i < loadResult.parseResult.errors.size(); i++)
				{
					g_logger_error("{} at line : column {}:{}",
						loadResult.parseResult.errors[i].getMessage(),
						loadResult.parseResult.errors[i].getLocation().begin.line,
						loadResult.parseResult.errors[i].getLocation().begin.column);
				}
			}
		}

		{
			// Register our own builtin globals 
			Luau::GlobalTypes& globals = frontend->globalsForAutocomplete;
			unfreeze(globals.globalTypes);
			Luau::LoadDefinitionFileResult loadResult = frontend->loadDefinitionFile(
				globals,
				globals.globalScope,
				MathAnimGlobals::getMathAnimApiTypes(),
				"math-anim",
				false, /* Capture comments */
				true /* Typecheck for autocomplete */
			);
			freeze(globals.globalTypes);

			if (!loadResult.success)
			{
				g_logger_error("The ScriptAnalyzer failed to load math-anim builtin definitions. Errors:");
				for (int i = 0; i < loadResult.parseResult.errors.size(); i++)
				{
					g_logger_error("{} at line : column {}:{}",
						loadResult.parseResult.errors[i].getMessage(),
						loadResult.parseResult.errors[i].getLocation().begin.line,
						loadResult.parseResult.errors[i].getLocation().begin.column);
				}
			}
		}
	}

	bool ScriptAnalyzer::analyze(const std::string& filename)
	{
		if (!fileResolver || !configResolver || !frontend)
		{
			static bool displayWarning = true;
			if (displayWarning)
			{
				g_logger_warning("Tried to analyze a script, but the script analyzer was not initialized properly. Suppressing this message now.");
				displayWarning = false;
			}
			return false;
		}

		Luau::CheckResult cr;

		if (frontend->isDirty(filename))
			cr = frontend->check(filename);

		if (!frontend->getSourceModule(filename))
		{
			g_logger_error("Error opening '{}'", filename);
		}

		std::string scriptFilepath = (m_scriptDirectory / filename).make_preferred().lexically_normal().string();
		for (auto& error : cr.errors)
		{
			reportError(frontend, scriptFilepath.c_str(), ReportFormat::Default, error);
		}

		frontend->clear();
		return cr.errors.size() == 0;
	}

	bool ScriptAnalyzer::analyze(const std::string& sourceCode, const std::string& scriptName)
	{
		if (!fileResolver || !configResolver || !frontend)
		{
			static bool displayWarning = true;
			if (displayWarning)
			{
				g_logger_warning("Tried to analyze a script, but the script analyzer was not initialized properly. Suppressing this message now.");
				displayWarning = false;
			}
			return false;
		}

		ScriptFileResolver* scriptFileResolver = dynamic_cast<ScriptFileResolver*>(fileResolver);
		scriptFileResolver->setAnonymousFile(sourceCode, scriptName);

		Luau::CheckResult cr;

		if (frontend->isDirty(scriptName))
			cr = frontend->check(scriptName);

		if (!frontend->getSourceModule(scriptName))
		{
			g_logger_error("Error opening {}", scriptName);
		}

		for (auto& error : cr.errors)
		{
			reportError(frontend, scriptName.c_str(), ReportFormat::Default, error);
		}

		frontend->clear();
		return cr.errors.size() == 0;
	}

	static std::optional<Luau::AutocompleteEntryMap> nullCallback(std::string /*tag*/, std::optional<const Luau::ClassType*> /*ptr*/, std::optional<std::string> /*contents*/)
	{
		return std::nullopt;
	}

	static void sortSuggestionsByRankAndKind(std::vector<AutocompleteSuggestion>& suggestions)
	{
		std::sort(suggestions.begin(), suggestions.end(), [](AutocompleteSuggestion const& a, AutocompleteSuggestion const& b)
			{
				if (a.data.kind == b.data.kind)
				{
					return a.rank > b.rank;
				}

				// If they're different kinds of stuff, rank by kind of suggestion.
				// For example, suggestions for variable properties should rank higher than keywords
				return autocompleteKindPrecedence[(int)a.data.kind] < autocompleteKindPrecedence[(int)b.data.kind];
			});
	}

	std::vector<AutocompleteSuggestion> ScriptAnalyzer::getSuggestions(std::string const& sourceCode, std::string const& scriptName, uint32 line, uint32 column)
	{
		if (!fileResolver || !configResolver || !frontend)
		{
			static bool displayWarning = true;
			if (displayWarning)
			{
				g_logger_warning("Tried to sandbox a script, but the script analyzer was not initialized properly. Suppressing this message now.");
				displayWarning = false;
			}
			return {};
		}

		ScriptFileResolver* scriptFileResolver = dynamic_cast<ScriptFileResolver*>(fileResolver);
		scriptFileResolver->setAnonymousFile(sourceCode, scriptName);

		Luau::CheckResult cr;
		frontend->markDirty(scriptName);
		frontend->check(scriptName);

		Luau::FrontendOptions opts;
		opts.forAutocomplete = true;
		cr = frontend->check(scriptName, opts);

		auto autocompleteRes = Luau::autocomplete(
			*frontend,
			scriptName,
			Luau::Position(line - 1, column - 1),
			nullCallback
		);

		std::vector<AutocompleteSuggestion> suggestions = {};
		for (auto& [key, val] : autocompleteRes.entryMap)
		{
			AutocompleteSuggestion suggestion = {};
			suggestion.text = key;
			suggestion.data = val;
			suggestion.rank = 0.0f;
			suggestions.emplace_back(suggestion);
		}

		frontend->clear();

		sortSuggestionsByRankAndKind(suggestions);

		return suggestions;
	}

	void ScriptAnalyzer::sortSuggestionsByQuery(std::string const& query, std::vector<AutocompleteSuggestion>& suggestions, std::vector<int>& visibleSuggestions)
	{
		visibleSuggestions.clear();

		// Re-rank all suggestions according to new query
		int index = 0;
		for (auto& suggestion : suggestions)
		{
			suggestion.rank = (float)rapidfuzz::fuzz::partial_ratio(query, suggestion.text);

			// Only do more expensive string checks if ranking is similar
			if (suggestion.rank > 50.0f)
			{
				suggestion.containsQuery = suggestion.text.find(query) != std::string::npos;

				if (suggestion.containsQuery)
				{
					visibleSuggestions.push_back(index);
				}
			}
			else if (query == "")
			{
				visibleSuggestions.push_back(index);
			}

			index++;
		}

		// Re-sort suggestions
		sortSuggestionsByRankAndKind(suggestions);
	}

	void ScriptAnalyzer::free()
	{
		g_memory_delete(fileResolver);
		g_memory_delete(configResolver);
		g_memory_delete(frontend);

		fileResolver = nullptr;
		configResolver = nullptr;
		frontend = nullptr;
	}
}

// ------------------------------- File Resolver -------------------------------
ScriptFileResolver::ScriptFileResolver(const std::filesystem::path& inScriptDirectory)
	: scriptDirectory(inScriptDirectory)
{
}

void ScriptFileResolver::setAnonymousFile(const std::string& source, const std::string& name)
{
	anonymousSource = source;
	anonymousName = name;
}

std::optional<Luau::SourceCode> ScriptFileResolver::readSource(const Luau::ModuleName& name)
{
	if (name == "math-anim" || name == "math-anim.luau")
	{
		Luau::SourceCode res;
		res.type = res.Module;
		res.source = MathAnim::MathAnimGlobals::getMathAnimModule();
		return res;
	}

	std::string scriptPath = (scriptDirectory / name).string();
	if (!MathAnim::Platform::fileExists(scriptPath.c_str()) && anonymousName == name)
	{
		Luau::SourceCode res;
		res.source = anonymousSource;
		res.type = res.Module;
		anonymousName = "UNDEFINED";
		anonymousSource = "";
		return res;
	}

	Luau::SourceCode res;
	res.type = res.Module;

	FILE* fp = fopen(scriptPath.c_str(), "rb");
	if (!fp)
	{
		g_logger_warning("Could not open file '{}', error opening file.", scriptPath);
		return std::nullopt;
	}

	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	RawMemory memory;
	memory.init(fileSize + 1);
	size_t amtRead = fread(memory.data, fileSize, 1, fp);
	fclose(fp);

	if (amtRead != 1)
	{
		g_logger_error("Error reading file '{}'.", scriptPath);
		memory.free();
		return std::nullopt;
	}

	memory.data[fileSize] = '\0';
	res.source = (char*)memory.data;
	memory.free();

	return res;
}

std::optional<Luau::ModuleInfo> ScriptFileResolver::resolveModule(const Luau::ModuleInfo*, Luau::AstExpr* node)
{
	if (Luau::AstExprConstantString* expr = node->as<Luau::AstExprConstantString>())
	{
		Luau::ModuleName name = std::string(expr->value.data, expr->value.size) + ".luau";
		return { {name} };
	}

	return std::nullopt;
}

std::string ScriptFileResolver::getHumanReadableModuleName(const Luau::ModuleName& name) const
{
	if (name == "-")
		return "stdin";
	return name;
}

// ------------------------------- Config Resolver -------------------------------
ScriptConfigResolver::ScriptConfigResolver()
{
	defaultConfig.mode = Luau::Mode::Strict;
	defaultConfig.enabledLint.warningMask = ~0ull;
	defaultConfig.parseOptions.captureComments = true;
}

const Luau::Config& ScriptConfigResolver::getConfig(const Luau::ModuleName&) const
{
	return defaultConfig;
}

// ------------------------------- Internal Functions -------------------------------
static void reportError(const Luau::Frontend* frontend, const char* filepath, ReportFormat format, const Luau::TypeError& error)
{
	if (const Luau::SyntaxError* syntaxError = Luau::get_if<Luau::SyntaxError>(&error.data))
		report(format, filepath, error.location, "SyntaxError", syntaxError->message.c_str());
	else
		report(format, filepath, error.location, "TypeError",
		Luau::toString(error, Luau::TypeErrorToStringOptions{ frontend->fileResolver }).c_str());
}

static void report(ReportFormat format, const char* filepath, const Luau::Location& loc, const char* type, const char* message)
{
	switch (format)
	{
	case ReportFormat::Default:
		MathAnim::ConsoleLog::error(filepath, loc.begin.line + 1, "Analysis Failed: %s: %s", type, message);
		break;

	case ReportFormat::Luacheck:
	{
		// Use stdout to match luacheck behavior
		MathAnim::ConsoleLog::error(filepath, loc.begin.line + 1, "Analysis Failed: (W0) %s: %s", type, message);
		break;
	}

	case ReportFormat::Gnu:
		// Note: GNU end column is inclusive but our end column is exclusive
		MathAnim::ConsoleLog::error(filepath, loc.begin.line + 1, "Analysis Failed: %s: %s", type, message);
		break;
	}
}