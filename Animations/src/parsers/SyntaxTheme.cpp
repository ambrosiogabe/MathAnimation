#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"

#include <nlohmann/json.hpp>
#include <../tinyxml2/tinyxml2.h>

using namespace tinyxml2;

namespace MathAnim
{
	using namespace nlohmann;

	// -------------- Internal Functions --------------
	static SyntaxTheme* importThemeFromJson(const json& json, const char* filepath);
	static SyntaxTheme* importThemeFromXml(const XMLDocument& document, const char* filepath);
	static const XMLElement* getValue(const XMLElement* element, const std::string& keyName);

	const TokenRule* SyntaxTheme::match(const ScopeRule& scope) const
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
				if (scope.contains(ruleScope, &levelMatched))
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

	SyntaxTheme* SyntaxTheme::importTheme(const char* filepathStr)
	{
		if (!Platform::fileExists(filepathStr))
		{
			g_logger_warning("Tried to parse syntax theme at filepath that does not exist: '%s'", filepathStr);
			return nullptr;
		}

		std::filesystem::path filepath = std::filesystem::path(filepathStr);

		if (filepath.extension() == ".json")
		{
			try
			{
				std::ifstream file(filepath);
				json j = json::parse(file, nullptr, true, true);
				return importThemeFromJson(j, filepathStr);
			}
			catch (json::parse_error& ex)
			{
				g_logger_error("Could not load TextMate theme in file '%s' as Json.\n\tJson Error: %s.", filepathStr, ex.what());
			}
		}
		else if (filepath.extension() == "tmTheme" || filepath.extension() == "xml")
		{
			XMLDocument doc;
			int res = doc.LoadFile(filepathStr);
			if (res != XML_SUCCESS)
			{
				// If XML fails print both errors and return nullptr
				const char* xmlError = doc.ErrorStr();
				g_logger_error("Could not load TextMate theme in file '%s' as XML.\n\tXML Error: %s", filepathStr, xmlError);
				return nullptr;
			}

			return importThemeFromXml(doc, filepathStr);
		}

		g_logger_warning("Unsupported theme file: '%s'", filepathStr);
		return nullptr;
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
				theme->defaultForeground = fromCssColor(foregroundColorStr);
			}
			else
			{
				theme->defaultForeground = Vec4{ 1, 1, 1, 1 };
			}

			if (j["colors"].contains("editor.background"))
			{
				std::string backgroundColorStr = j["colors"]["editor.background"];
				theme->defaultBackground = fromCssColor(backgroundColorStr);
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
			bool nameIsFirstScope = false;
			if (!color.contains("name"))
			{
				nameIsFirstScope = true;
			}

			std::string name = nameIsFirstScope
				? "Undefined"
				: color["name"];

			if (!color.contains("scope"))
			{
				if (color.contains("settings"))
				{
					TokenRule defaultThemeRule = {};

					// This is presumably the defualt foreground/background settings and not a token rule
					if (color["settings"].contains("foreground"))
					{
						theme->defaultForeground = toHex(color["settings"]["foreground"]);
					}

					if (color["settings"].contains("background"))
					{
						theme->defaultBackground = toHex(color["settings"]["background"]);
					}

					ThemeSetting defaultThemeSetting = {};
					defaultThemeSetting.type = ThemeSettingType::ForegroundColor;
					defaultThemeSetting.foregroundColor = theme->defaultForeground;
					defaultThemeRule.settings.push_back(defaultThemeSetting);
					theme->defaultRule = defaultThemeRule;
				}
				else
				{
					g_logger_warning("TokenColor '%s' has no 'scope' property in syntax theme '%s'.", name.c_str(), filepath);
				}

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
					rule.scopes.emplace_back(ScopeRule::from(scope));
				}
			}
			else if (color["scope"].is_string())
			{
				rule.scopes.emplace_back(ScopeRule::from(color["scope"]));
			}

			if (settingsJson.contains("foreground"))
			{
				const std::string& colorHexStr = settingsJson["foreground"];
				ThemeSetting setting = {};
				setting.type = ThemeSettingType::ForegroundColor;
				setting.foregroundColor = fromCssColor(colorHexStr);
				rule.settings.emplace_back(setting);
			}

			if (rule.scopes.size() > 0)
			{
				if (nameIsFirstScope)
				{
					rule.name = rule.scopes[0].friendlyName;
				}

				theme->tokenColors.emplace_back(rule);
			}
			else
			{
				g_logger_warning("TokenColor '%s' is malformed. No scopes were successfully parsed.", name.c_str());
			}
		}

		return theme;
	}

	static SyntaxTheme* importThemeFromXml(const XMLDocument& xml, const char* filepath)
	{
		const XMLElement* overallDict = xml.FirstChildElement("dict");
		if (!overallDict)
		{
			g_logger_warning("Cannot import syntax theme that has no 'dict' XML group in file: '%s'", filepath);
			return nullptr;
		}

		const XMLElement* firstKey = overallDict->FirstChildElement("key");
		if (!firstKey)
		{
			g_logger_warning("Expected syntax theme in file '%s' to have structure <dict> <key></key> </dict> but no GlobalKey found.", filepath);
			return nullptr;
		}

		while (firstKey != nullptr && firstKey->GetText() != std::string("settings"))
		{
			firstKey = firstKey->NextSiblingElement("key");
		}

		if (!firstKey)
		{
			g_logger_warning("Could not import syntax theme in file '%s'. Could not find GlobalKey 'settings'.", filepath);
			return nullptr;
		}

		const XMLElement* arrayElement = firstKey->NextSiblingElement();
		if (!arrayElement)
		{
			g_logger_warning("Could not import syntax theme in file '%s'. GlobalKey 'settings' had no value.", filepath);
			return nullptr;
		}

		if (arrayElement->Name() != std::string("array"))
		{
			g_logger_warning("Could not import syntax theme in file '%s'. Expected GlobalKey 'settings' to have value of type <array> instead got <%s>.", filepath, arrayElement->Name());
			return nullptr;
		}

		SyntaxTheme* theme = (SyntaxTheme*)g_memory_allocate(sizeof(SyntaxTheme));
		new(theme)SyntaxTheme();

		const XMLElement* setting = arrayElement->FirstChildElement();
		while (setting != nullptr)
		{
			const XMLElement* settingDict = setting->FirstChildElement("dict");
			if (settingDict == nullptr)
			{
				continue;
			}

			const XMLElement* settingDictKey = settingDict->FirstChildElement("key");
			if (settingDictKey == nullptr)
			{
				continue;
			}

			if (settingDictKey->Value() == std::string("settings"))
			{
				// Parse the settings for this theme
				const XMLElement* themeSettingsDict = settingDictKey->NextSiblingElement("dict");
				theme->defaultForeground = Vec4{ 1, 1, 1, 1 };
				theme->defaultBackground = Vec4{ 0, 0, 0, 0 };

				if (themeSettingsDict)
				{
					const XMLElement* backgroundValue = getValue(themeSettingsDict, "background");
					if (backgroundValue && (std::strcmp(backgroundValue->Name(), "string") == 0))
					{
						theme->defaultBackground = fromCssColor(backgroundValue->Value());
					}

					const XMLElement* foregroundValue = getValue(themeSettingsDict, "foreground");
					if (foregroundValue && (std::strcmp(foregroundValue->Name(), "string") == 0))
					{
						theme->defaultForeground = fromCssColor(foregroundValue->Value());
					}
				}

				TokenRule defaultThemeRule = {};
				ThemeSetting defaultThemeSetting = {};
				defaultThemeSetting.type = ThemeSettingType::ForegroundColor;
				defaultThemeSetting.foregroundColor = theme->defaultForeground;
				defaultThemeRule.settings.push_back(defaultThemeSetting);
				theme->defaultRule = defaultThemeRule;
			}
			else if (settingDictKey->Value() == std::string("name"))
			{
				const XMLElement* nameValue = settingDictKey->NextSiblingElement();
				if (nameValue == nullptr || nameValue->Name() != std::string("string"))
				{
					continue;
				}
				std::string name = nameValue->Value();

				const XMLElement* scopeValue = getValue(settingDict, "scope");
				if (!scopeValue)
				{
					g_logger_warning("TokenColor '%s' has no 'scope' key in syntax theme '%s'.", name.c_str(), filepath);
					continue;
				}

				if (scopeValue->Name() != std::string("string"))
				{
					g_logger_warning("TokenColor '%s' has malformed 'scope' value. Expected array or string but got something else.", name.c_str());
					continue;
				}

				const XMLElement* themeSettings = getValue(settingDict, "settings");
				if (themeSettings)
				{
					g_logger_warning("TokenColor '%s' has no 'settings' value. Skipping this rule since it can't style anything without settings.", name.c_str());
					continue;
				}

				const XMLElement* foregroundSetting = getValue(themeSettings, "foreground");
				if (foregroundSetting)
				{
					if (foregroundSetting->Name() != std::string("string"))
					{
						g_logger_warning("TokenColor '%s' 'settings.foreground' key is malformed. Expected a string and got something else.", name.c_str());
						continue;
					}
				}

				TokenRule rule = {};
				rule.name = name;
				rule.scopes.emplace_back(ScopeRule::from(scopeValue->Value()));

				if (foregroundSetting)
				{
					const std::string& colorHexStr = foregroundSetting->Value();
					ThemeSetting setting = {};
					setting.type = ThemeSettingType::ForegroundColor;
					setting.foregroundColor = fromCssColor(colorHexStr);
					rule.settings.emplace_back(setting);
				}

				theme->tokenColors.emplace_back(rule);
			}
		}

		return theme;
	}

	static const XMLElement* getValue(const XMLElement* parentElement, const std::string& keyName)
	{
		const XMLElement* key = parentElement->FirstChildElement("key");
		while (key != nullptr && key->Value() != keyName)
		{
			key = key->NextSiblingElement("key");
		}

		if (key)
		{
			return key->NextSiblingElement();
		}

		return nullptr;
	}
}