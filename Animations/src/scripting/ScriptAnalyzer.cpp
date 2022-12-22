#include "scripting/ScriptAnalyzer.h"
#include "scripting/MathAnimGlobals.h"
#include "platform/Platform.h"
#include "editor/ConsoleLog.h"

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

namespace MathAnim
{
	ScriptAnalyzer::ScriptAnalyzer(const std::filesystem::path& scriptDirectory)
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

		// Register the bundled builtin globals that come with Luau
		Luau::registerBuiltinGlobals(frontend->typeChecker);
		{
			// Register our own builtin globals 
			Luau::LoadDefinitionFileResult loadResult =
				Luau::loadDefinitionFile(
					frontend->typeChecker,
					frontend->typeChecker.globalScope,
					MathAnimGlobals::getBuiltinDefinitionSource(),
					"math-anim"
				);
			if (!loadResult.success)
			{
				g_logger_error("The ScriptAnalyzer failed to load math-anim builtin definitions. Errors:");
				for (int i = 0; i < loadResult.parseResult.errors.size(); i++)
				{
					g_logger_error("%s at line:column %d:%d",
						loadResult.parseResult.errors[i].getMessage().c_str(),
						loadResult.parseResult.errors[i].getLocation().begin.line,
						loadResult.parseResult.errors[i].getLocation().begin.column);
				}
			}

			Luau::TypeArena& arena = frontend->typeChecker.globalTypes;
			//arena.addType(loadResult.module.get()->astTypes[0]);
		}
		{
			// Register our own builtin types 
			Luau::LoadDefinitionFileResult loadResult =
				Luau::loadDefinitionFile(
					frontend->typeChecker,
					frontend->typeChecker.globalScope,
					MathAnimGlobals::getMathAnimApiTypes(),
					"math-anim"
				);
			if (!loadResult.success)
			{
				g_logger_error("The ScriptAnalyzer failed to load math-anim builtin definitions. Errors:");
				for (int i = 0; i < loadResult.parseResult.errors.size(); i++)
				{
					g_logger_error("%s at line:column %d:%d",
						loadResult.parseResult.errors[i].getMessage().c_str(),
						loadResult.parseResult.errors[i].getLocation().begin.line,
						loadResult.parseResult.errors[i].getLocation().begin.column);
				}
			}
		}
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
			g_logger_error("Error opening %s", scriptName.c_str());
		}

		for (auto& error : cr.errors)
		{
			reportError(frontend, scriptName.c_str(), ReportFormat::Default, error);
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
		g_logger_warning("Could not open file '%s', error opening file.", scriptPath.c_str());
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
		g_logger_error("Error reading file '%s'.", scriptPath.c_str());
		memory.free();
		return std::nullopt;
	}

	memory.data[fileSize] = '\0';
	res.source = (char*)memory.data;
	memory.free();

	return res;
}

std::optional<Luau::ModuleInfo> ScriptFileResolver::resolveModule(const Luau::ModuleInfo* context, Luau::AstExpr* node)
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
}

const Luau::Config& ScriptConfigResolver::getConfig(const Luau::ModuleName& name) const
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
		//ConsoleLogError("%s(%d,%d): %s: %s", name, loc.begin.line + 1, loc.begin.column + 1, type, message);
		MathAnim::ConsoleLog::error(filepath, loc.begin.line + 1, "Analysis Failed: %s: %s", type, message);
		break;

	case ReportFormat::Luacheck:
	{
		// Note: luacheck's end column is inclusive but our end column is exclusive
		// In addition, luacheck doesn't support multi-line messages, so if the error is multiline we'll fake end column as 100 and hope for the best
		int columnEnd = (loc.begin.line == loc.end.line) ? loc.end.column : 100;

		// Use stdout to match luacheck behavior
		//ConsoleLogError("%s:%d:%d-%d: (W0) %s: %s", name, loc.begin.line + 1, loc.begin.column + 1, columnEnd, type, message);
		MathAnim::ConsoleLog::error(filepath, loc.begin.line + 1, "Analysis Failed: (W0) %s: %s", type, message);
		break;
	}

	case ReportFormat::Gnu:
		// Note: GNU end column is inclusive but our end column is exclusive
		//ConsoleLogError("%s:%d.%d-%d.%d: %s: %s", name, loc.begin.line + 1, loc.begin.column + 1, loc.end.line + 1, loc.end.column, type, message);
		MathAnim::ConsoleLog::error(filepath, loc.begin.line + 1, "Analysis Failed: %s: %s", type, message);
		break;
	}
}