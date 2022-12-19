#include "scripting/ScriptAnalyzer.h"
#include "platform/Platform.h"

#include <lua.h>
#include <luacode.h>
#include <lualib.h>
#include <Luau/Frontend.h>
#include <Luau/BuiltinDefinitions.h>

// ------------------------------- Internal Types -------------------------------
struct ScriptFileResolver : public Luau::FileResolver
{
	std::string anonymousSource;
	std::string anonymousName;
	std::string scriptDirectory;

	ScriptFileResolver(const std::string& scriptDirectory);

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
static void reportError(const Luau::Frontend* frontend, ReportFormat format, const Luau::TypeError& error);
static void report(ReportFormat format, const char* name, const Luau::Location& loc, const char* type, const char* message);

namespace MathAnim
{
	ScriptAnalyzer::ScriptAnalyzer(const std::string& scriptDirectory)
		: m_scriptDirectory(scriptDirectory)
	{
		Luau::FrontendOptions frontendOptions;
		frontendOptions.retainFullTypeGraphs = false;

		fileResolver = (ScriptFileResolver*)g_memory_allocate(sizeof(ScriptFileResolver));
		new(fileResolver)ScriptFileResolver(scriptDirectory);
		configResolver = (ScriptConfigResolver*)g_memory_allocate(sizeof(ScriptConfigResolver));
		new(configResolver)ScriptConfigResolver();
		frontend = (Luau::Frontend*)g_memory_allocate(sizeof(Luau::Frontend));
		new(frontend)Luau::Frontend(fileResolver, configResolver, frontendOptions);

		Luau::registerBuiltinGlobals(frontend->typeChecker);
		Luau::freeze(frontend->typeChecker.globalTypes);
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
			g_logger_error("Error opening %s", filename.c_str());
		}

		for (auto& error : cr.errors)
		{
			reportError(frontend, ReportFormat::Default, error);
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
			g_logger_error("Error opening %s", scriptName.c_str());
		}

		for (auto& error : cr.errors)
		{
			reportError(frontend, ReportFormat::Default, error);
		}

		frontend->clear();
		return cr.errors.size() == 0;
	}

	void ScriptAnalyzer::free()
	{
		if (fileResolver)
		{
			fileResolver->~FileResolver();
			g_memory_free(fileResolver);
		}

		if (configResolver)
		{
			configResolver->~ConfigResolver();
			g_memory_free(configResolver);
		}

		if (frontend)
		{
			frontend->~Frontend();
			g_memory_free(frontend);
		}

		fileResolver = nullptr;
		configResolver = nullptr;
		frontend = nullptr;
	}
}

// ------------------------------- File Resolver -------------------------------
ScriptFileResolver::ScriptFileResolver(const std::string& inScriptDirectory)
{
	scriptDirectory = inScriptDirectory;
}

void ScriptFileResolver::setAnonymousFile(const std::string& source, const std::string& name)
{
	anonymousSource = source;
	anonymousName = name;
}

std::optional<Luau::SourceCode> ScriptFileResolver::readSource(const Luau::ModuleName& name)
{
	std::string scriptPath = scriptDirectory + name;
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

	FILE* fp = fopen(scriptPath.c_str(), "r");
	if (!fp)
	{
		g_logger_warning("Could not open file '%s', error opening file.", scriptPath.c_str());
		return std::nullopt;
	}

	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	RawMemory memory;
	memory.init(fileSize);
	fread(memory.data, fileSize, 1, fp);
	fclose(fp);

	res.source = (char*)memory.data;
	memory.free();

	return res;
}

std::optional<Luau::ModuleInfo> ScriptFileResolver::resolveModule(const Luau::ModuleInfo* context, Luau::AstExpr* node)
{
	if (Luau::AstExprConstantString* expr = node->as<Luau::AstExprConstantString>())
	{
		Luau::ModuleName name = std::string(expr->value.data, expr->value.size) + ".luau";
		name = std::string(expr->value.data, expr->value.size) + ".lua";

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
}

const Luau::Config& ScriptConfigResolver::getConfig(const Luau::ModuleName& name) const
{
	return defaultConfig;
}

// ------------------------------- Internal Functions -------------------------------
static void reportError(const Luau::Frontend* frontend, ReportFormat format, const Luau::TypeError& error)
{
	std::string humanReadableName = frontend->fileResolver->getHumanReadableModuleName(error.moduleName);

	if (const Luau::SyntaxError* syntaxError = Luau::get_if<Luau::SyntaxError>(&error.data))
		report(format, humanReadableName.c_str(), error.location, "SyntaxError", syntaxError->message.c_str());
	else
		report(format, humanReadableName.c_str(), error.location, "TypeError",
			Luau::toString(error, Luau::TypeErrorToStringOptions{ frontend->fileResolver }).c_str());
}

static void report(ReportFormat format, const char* name, const Luau::Location& loc, const char* type, const char* message)
{
	switch (format)
	{
	case ReportFormat::Default:
		g_logger_error("%s(%d,%d): %s: %s\n", name, loc.begin.line + 1, loc.begin.column + 1, type, message);
		break;

	case ReportFormat::Luacheck:
	{
		// Note: luacheck's end column is inclusive but our end column is exclusive
		// In addition, luacheck doesn't support multi-line messages, so if the error is multiline we'll fake end column as 100 and hope for the best
		int columnEnd = (loc.begin.line == loc.end.line) ? loc.end.column : 100;

		// Use stdout to match luacheck behavior
		g_logger_error("%s:%d:%d-%d: (W0) %s: %s\n", name, loc.begin.line + 1, loc.begin.column + 1, columnEnd, type, message);
		break;
	}

	case ReportFormat::Gnu:
		// Note: GNU end column is inclusive but our end column is exclusive
		g_logger_error("%s:%d.%d-%d.%d: %s: %s\n", name, loc.begin.line + 1, loc.begin.column + 1, loc.end.line + 1, loc.end.column, type, message);
		break;
	}
}