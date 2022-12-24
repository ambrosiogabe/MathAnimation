#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	using namespace nlohmann;

	// -------------- Internal Functions --------------
	static SyntaxTheme* importThemeFromJson(const json& json, const char* filepath);

	const TokenRule* SyntaxTheme::match(const ScopedName& scope) const
	{
		const TokenRule* result = nullptr;
		int maxLevelMatched = 0;

		// Pick the best rule according to the guide laid out here https://macromates.com/manual/en/scope_selectors
		// (At least I'm trying to follow these stupid esoteric instructions smh)
		/*
		* 1. Match the element deepest down in the scope e.g. string wins over source.php when
		*    the scope is source.php string.quoted.
		*
		* 2. Match most of the deepest element e.g. string.quoted wins over string.
		*
		* 3. Rules 1 and 2 applied again to the scope selector when removing the deepest element
		*    (in the case of a tie), e.g. text source string wins over source string.
		*/
		for (const auto& rule : tokenColors)
		{
			for (const auto& ruleScope : rule.scopes)
			{
				int levelMatched;
				if (ruleScope.contains(scope, &levelMatched))
				{
					if (levelMatched > maxLevelMatched)
					{
						result = &rule;
						maxLevelMatched = levelMatched;
					}
				}
			}
		}

		return result;
	}

	SyntaxTheme* SyntaxTheme::importTheme(const char* filepath)
	{
		if (!Platform::fileExists(filepath))
		{
			g_logger_warning("Tried to parse syntax theme at filepath that does not exist: '%s'", filepath);
			return nullptr;
		}

		std::ifstream file(filepath);
		json j = json::parse(file);

		return importThemeFromJson(j, filepath);

	}

	void SyntaxTheme::free(SyntaxTheme* theme)
	{
		if (theme)
		{
			theme->~SyntaxTheme();
			g_memory_free(theme);
		}
	}

	// -------------- Internal Functions --------------
	static SyntaxTheme* importThemeFromJson(const json& j, const char* filepath)
	{
		if (!j.contains("tokenColors"))
		{
			g_logger_warning("Cannot import syntax theme that has no 'tokenColors' property in file: '%s'", filepath);
			return nullptr;
		}

		const json& tokenColors = j["tokenColors"];
		if (!tokenColors.is_array())
		{
			g_logger_warning("Syntax theme is malformed. The property 'tokenColors' is not an array in file: '%s'", filepath);
		}

		SyntaxTheme* theme = (SyntaxTheme*)g_memory_allocate(sizeof(SyntaxTheme));
		new(theme)SyntaxTheme();

		if (j.contains("colors"))
		{
			if (j["colors"].contains("editor.foreground"))
			{
				std::string foregroundColorStr = j["colors"]["editor.foreground"];
				theme->defaultForeground = toHex(foregroundColorStr);
			}
			else
			{
				theme->defaultForeground = Vec4{ 1, 1, 1, 1 };
			}

			if (j["colors"].contains("editor.background"))
			{
				std::string backgroundColorStr = j["colors"]["editor.background"];
				theme->defaultBackground = toHex(backgroundColorStr);
			}
			else
			{
				theme->defaultBackground = Vec4{ 0, 0, 0, 0 };
			}

			TokenRule defaultThemeRule = {};
			ThemeSetting defaultThemeSetting = {};
			defaultThemeSetting.type = ThemeSettingType::ForegroundColor;
			defaultThemeSetting.foregroundColor = theme->defaultForeground;
			defaultThemeRule.settings.push_back(defaultThemeSetting);
			theme->defaultRule = defaultThemeRule;
		}

		for (auto color : tokenColors)
		{
			if (!color.contains("name"))
			{
				g_logger_warning("TokenColor has no 'name' property in syntax theme '%s'.", filepath);
				continue;
			}

			const std::string& name = color["name"];

			if (!color.contains("scope"))
			{
				g_logger_warning("TokenColor '%s' has no 'scope' property in syntax theme '%s'.", name.c_str(), filepath);
				continue;
			}

			if (!color["scope"].is_array() && !color["scope"].is_string())
			{
				g_logger_warning("TokenColor '%s' has malformed 'scope' property. Expected array or string but got something else.", name.c_str());
				continue;
			}

			if (!color.contains("settings"))
			{
				g_logger_warning("TokenColor '%s' has no 'settings' property. Skipping this rule since it can't style anything without settings.", name.c_str());
				continue;
			}

			const json& settingsJson = color["settings"];
			if (settingsJson.contains("foreground"))
			{
				if (!settingsJson["foreground"].is_string())
				{
					g_logger_warning("TokenColor '%s' 'settings.foreground' key is malformed. Expected a string and got something else.", name.c_str());
					continue;
				}
			}

			TokenRule rule = {};
			rule.name = name;
			if (color["scope"].is_array())
			{
				for (auto scope : color["scope"])
				{
					rule.scopes.emplace_back(ScopedName::from(scope));
				}
			}
			else if (color["scope"].is_string())
			{
				rule.scopes.emplace_back(ScopedName::from(color["scope"]));
			}

			if (settingsJson.contains("foreground"))
			{
				const std::string& colorHexStr = settingsJson["foreground"];
				ThemeSetting setting = {};
				setting.type = ThemeSettingType::ForegroundColor;
				setting.foregroundColor = toHex(colorHexStr);
				rule.settings.emplace_back(setting);
			}

			theme->tokenColors.emplace_back(rule);
		}

		return theme;
	}
}