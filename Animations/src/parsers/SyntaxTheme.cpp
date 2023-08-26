#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"

#include <nlohmann/json.hpp>
#include <../tinyxml2/tinyxml2.h>

using namespace tinyxml2;

namespace MathAnim
{
	using namespace nlohmann;

	struct InternalScopeMatch
	{
		size_t globalRuleIndex;
		ScopeRuleCollectionMatch match;
	};

	// -------------- Internal Functions --------------
	static SyntaxTheme* importThemeFromJson(const json& json, const char* filepath);
	static SyntaxTheme* importThemeFromXml(const XMLDocument& document, const char* filepath);
	static const XMLElement* getValue(const XMLElement* element, const std::string& keyName);

	const ThemeSetting* TokenRule::getSetting(ThemeSettingType type) const
	{
		for (const ThemeSetting& setting : settings)
		{
			if (setting.type == type)
			{
				return &setting;
			}
		}

		return nullptr;
	}

	TokenRuleMatch SyntaxTheme::match(const std::vector<ScopedName>& ancestorScopes) const
	{
		// Pick the best rule according to the guide laid out here https://macromates.com/manual/en/scope_selectors
		/*
		* 1. Match the element deepest down in the scope e.g. "string" wins over "source.php" when
		*    the scope is "source.php string.quoted".
		*
		* 2. Match most of the deepest element e.g. "string.quoted" wins over "string".
		*
		* 3. Rules 1 and 2 applied again to the scope selector when removing the deepest element
		*    (in the case of a tie), e.g. "text source string" wins over "source string".
		*/

		std::vector<InternalScopeMatch> potentialMatches = {};

		int deepestScopeMatched = 0;
		for (size_t ruleIndex = 0; ruleIndex < tokenColors.size(); ruleIndex++)
		{
			const auto& rule = tokenColors[ruleIndex];
			for (size_t collectionIndex = 0; collectionIndex < rule.scopeCollection.size(); collectionIndex++)
			{
				const auto& scopeRule = rule.scopeCollection[collectionIndex];
				auto match = scopeRule.matches(ancestorScopes);
				if (match.has_value())
				{
					if (match->scopeRule.deepestScopeMatched > deepestScopeMatched)
					{
						deepestScopeMatched = match->scopeRule.deepestScopeMatched;
					}

					potentialMatches.emplace_back(InternalScopeMatch{
						ruleIndex,
						match.value()
						});
					break;
				}
			}
		}

		/*
		* First pass to cull the list, follow this rule to cull potential matches:
		*
		*  1. Match the element deepest down in the scope e.g. "string" wins over "source.php" when
		*    the scope is "source.php string.quoted".
		*/

		for (auto iter = potentialMatches.begin(); iter != potentialMatches.end();)
		{
			if (iter->match.scopeRule.deepestScopeMatched < deepestScopeMatched)
			{
				iter = potentialMatches.erase(iter);
			}
			else
			{
				iter++;
			}
		}

		/**
		* Second pass to cull the list, follow this rule to cull more potential matches:
		*
		*  2. Match most of the deepest element e.g. "string.quoted" wins over "string".
		*
		*  NOTE (Deprecated?): I think the note below this one no longer applies. I've removed the code
		*                      that used to do what the note below said and it still works alright /shrug.
		*  NOTE:
		*  This is kind of vague. What happens if you get "string.quoted.at.json" vs "string.quoted" for the pattern "string.quoted"
		*  I believe "string.quoted" should win since it matches 100%, but the spec doesn't specify this. So
		*  first I'll cull by the percentage matched of the total number of levels. In this case "string.quoted.at.json"
		*  would match 0.5 and "string.quoted" would match 1.0, so "string.quoted" would win. Then, if you have
		*  "string.quoted" vs "string", "string.quoted" would win since it goes two levels deep and is more specific.
		*/
		int maxLevelMatched = 0;
		for (auto iter = potentialMatches.begin(); iter != potentialMatches.end(); iter++)
		{
			if (iter->match.scopeRule.ancestorMatches[0].levelMatched > maxLevelMatched)
			{
				maxLevelMatched = iter->match.scopeRule.ancestorMatches[0].levelMatched;
			}
		}

		for (auto iter = potentialMatches.begin(); iter != potentialMatches.end();)
		{
			if (iter->match.scopeRule.ancestorMatches[0].levelMatched < maxLevelMatched)
			{
				iter = potentialMatches.erase(iter);
			}
			else
			{
				iter++;
			}
		}

		/*
		* 3. Rules 1 and 2 applied again to the scope selector when removing the deepest element
		*    (in the case of a tie), e.g. "text source string" wins over "source string".
		* 
		* NOTE: Right now I'm just culling whichever matches have less ancestors matched.
		*/
		size_t maxAncestorsMatched = 0;
		for (size_t i = 0; i < potentialMatches.size(); i++) {
			if (potentialMatches[i].match.scopeRule.ancestorMatches.size() > maxAncestorsMatched)
			{
				maxAncestorsMatched = potentialMatches[i].match.scopeRule.ancestorMatches.size();
			}
		}

		for (auto iter = potentialMatches.begin(); iter != potentialMatches.end();)
		{
			if (iter->match.scopeRule.ancestorMatches.size() < maxAncestorsMatched)
			{
				iter = potentialMatches.erase(iter);
			}
			else
			{
				iter++;
			}
		}

		if (potentialMatches.size() > 0)
		{
			// Take the best ancestor match
			std::vector<ScopedNameMatch> deepestAncestorLevelMatches = potentialMatches[0].match.scopeRule.ancestorMatches;
			for (size_t i = 0; i < potentialMatches.size(); i++)
			{
				for (size_t j = 0; j < potentialMatches[i].match.scopeRule.ancestorMatches.size(); j++)
				{
					if (j < deepestAncestorLevelMatches.size() && 
						potentialMatches[i].match.scopeRule.ancestorMatches[j].levelMatched >
						deepestAncestorLevelMatches[j].levelMatched)
					{
						deepestAncestorLevelMatches[j].levelMatched = potentialMatches[i].match.scopeRule.ancestorMatches[j].levelMatched;
					}
					else if (j >= deepestAncestorLevelMatches.size())
					{
						deepestAncestorLevelMatches.push_back(potentialMatches[i].match.scopeRule.ancestorMatches[j]);
					}
				}
			}

			for (auto iter = potentialMatches.begin(); iter != potentialMatches.end();)
			{
				bool erased = false;
				for (size_t j = 0; j < iter->match.scopeRule.ancestorMatches.size(); j++)
				{
					if (j < deepestAncestorLevelMatches.size() &&
						iter->match.scopeRule.ancestorMatches[j].levelMatched <
						deepestAncestorLevelMatches[j].levelMatched)
					{
						iter = potentialMatches.erase(iter);
						erased = true;
						break;
					}
				}

				if (!erased)
				{
					iter++;
				}
			}

			const auto& match = potentialMatches[0];
			const auto& tokenRuleMatch = tokenColors[match.globalRuleIndex];

			std::string matchedOnStr = "";
			for (size_t i = 0; i < match.match.scopeRule.ancestorMatches.size(); i++)
			{
				const auto& ancestor = match.match.scopeRule.ancestorMatches[i];
				const auto& ancestorName = match.match.scopeRule.ancestorNames[i];
				for (size_t j = 0; j < ancestor.levelMatched; j++)
				{
					g_logger_assert(j < ancestorName.dotSeparatedScopes.size(), "How did that happen?");
					matchedOnStr += ancestorName.dotSeparatedScopes[j].getScopeName();
					if (j < ancestor.levelMatched - 1)
					{
						matchedOnStr += ".";
					}
				}

				if (i < match.match.scopeRule.ancestorMatches.size() - 1)
				{
					matchedOnStr += " ";
				}
			}

			return {
				&tokenRuleMatch,
				matchedOnStr
			};
		}

		return {
			nullptr,
			""
		};
	}

	const ThemeSetting* SyntaxTheme::match(const std::vector<ScopedName>& ancestorScopes, ThemeSettingType settingType) const
	{
		TokenRuleMatch matchedRule = match(ancestorScopes);
		return matchedRule.getSetting(settingType);
	}

	SyntaxTheme* SyntaxTheme::importTheme(const char* filepathStr)
	{
		if (!Platform::fileExists(filepathStr))
		{
			g_logger_warning("Tried to parse syntax theme at filepath that does not exist: '{}'", filepathStr);
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
				g_logger_error("Could not load TextMate theme in file '{}' as Json.\n\tJson Error: '{}'", filepathStr, ex.what());
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
				g_logger_error("Could not load TextMate theme in file '{}' as XML.\n\tXML Error: '{}'", filepathStr, xmlError);
				return nullptr;
			}

			return importThemeFromXml(doc, filepathStr);
		}

		g_logger_warning("Unsupported theme file: '{}'", filepathStr);
		return nullptr;
	}

	void SyntaxTheme::free(SyntaxTheme* theme)
	{
		if (theme)
		{
			g_memory_delete(theme);
		}
	}

	// -------------- Internal Functions --------------
	static SyntaxTheme* importThemeFromJson(const json& j, const char* filepath)
	{
		if (!j.contains("tokenColors"))
		{
			g_logger_warning("Cannot import syntax theme that has no 'tokenColors' property in file: '{}'", filepath);
			return nullptr;
		}

		const json& tokenColors = j["tokenColors"];
		if (!tokenColors.is_array())
		{
			g_logger_warning("Syntax theme is malformed. The property 'tokenColors' is not an array in file: '{}'", filepath);
		}

		SyntaxTheme* theme = g_memory_new SyntaxTheme();

		if (j.contains("colors"))
		{
			if (j["colors"].contains("editor.foreground"))
			{
				std::string foregroundColorStr = j["colors"]["editor.foreground"];
				theme->defaultForeground = Css::colorFromString(foregroundColorStr);
			}
			else
			{
				theme->defaultForeground = {
					Vec4{ 1, 1, 1, 1 },
					CssStyleType::Value
				};
			}

			if (j["colors"].contains("editor.background"))
			{
				std::string backgroundColorStr = j["colors"]["editor.background"];
				theme->defaultBackground = Css::colorFromString(backgroundColorStr);
			}
			else
			{
				theme->defaultBackground = {
					Vec4{ 0, 0, 0, 0 },
					CssStyleType::Value
				};
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
						theme->defaultForeground = Css::colorFromString(color["settings"]["foreground"]);
					}

					if (color["settings"].contains("background"))
					{
						theme->defaultBackground = Css::colorFromString(color["settings"]["background"]);
					}

					ThemeSetting defaultThemeSetting = {};
					defaultThemeSetting.type = ThemeSettingType::ForegroundColor;
					defaultThemeSetting.foregroundColor = theme->defaultForeground;
					defaultThemeRule.settings.push_back(defaultThemeSetting);
					theme->defaultRule = defaultThemeRule;
				}
				else
				{
					g_logger_warning("TokenColor '{}' has no 'scope' property in syntax theme '{}'.", name, filepath);
				}

				continue;
			}

			if (!color["scope"].is_array() && !color["scope"].is_string())
			{
				g_logger_warning("TokenColor '{}' has malformed 'scope' property. Expected array or string but got something else.", name);
				continue;
			}

			if (!color.contains("settings"))
			{
				g_logger_warning("TokenColor '{}' has no 'settings' property. Skipping this rule since it can't style anything without settings.", name);
				continue;
			}

			const json& settingsJson = color["settings"];
			if (settingsJson.contains("foreground"))
			{
				if (!settingsJson["foreground"].is_string())
				{
					g_logger_warning("TokenColor '{}' 'settings.foreground' key is malformed. Expected a string and got something else.", name);
					continue;
				}
			}

			TokenRule rule = {};
			rule.name = name;
			if (color["scope"].is_array())
			{
				for (auto scope : color["scope"])
				{
					rule.scopeCollection.emplace_back(ScopeRuleCollection::from(scope));
				}
			}
			else if (color["scope"].is_string())
			{
				rule.scopeCollection.emplace_back(ScopeRuleCollection::from(color["scope"]));
			}

			if (settingsJson.contains("foreground"))
			{
				const std::string& foregroundColorStr = settingsJson["foreground"];
				ThemeSetting setting = {};
				setting.type = ThemeSettingType::ForegroundColor;
				setting.foregroundColor = Css::colorFromString(foregroundColorStr);
				rule.settings.emplace_back(setting);
			}

			if (rule.scopeCollection.size() > 0)
			{
				if (nameIsFirstScope)
				{
					rule.name = rule.scopeCollection[0].friendlyName;
				}

				theme->tokenColors.emplace_back(rule);
			}
			else
			{
				g_logger_warning("TokenColor '{}' is malformed. No scopes were successfully parsed.", name);
			}
		}

		return theme;
	}

	static SyntaxTheme* importThemeFromXml(const XMLDocument& xml, const char* filepath)
	{
		const XMLElement* overallDict = xml.FirstChildElement("dict");
		if (!overallDict)
		{
			g_logger_warning("Cannot import syntax theme that has no 'dict' XML group in file: '{}'", filepath);
			return nullptr;
		}

		const XMLElement* firstKey = overallDict->FirstChildElement("key");
		if (!firstKey)
		{
			g_logger_warning("Expected syntax theme in file '{}' to have structure <dict> <key></key> </dict> but no GlobalKey found.", filepath);
			return nullptr;
		}

		while (firstKey != nullptr && firstKey->GetText() != std::string("settings"))
		{
			firstKey = firstKey->NextSiblingElement("key");
		}

		if (!firstKey)
		{
			g_logger_warning("Could not import syntax theme in file '{}'. Could not find GlobalKey 'settings'.", filepath);
			return nullptr;
		}

		const XMLElement* arrayElement = firstKey->NextSiblingElement();
		if (!arrayElement)
		{
			g_logger_warning("Could not import syntax theme in file '{}'. GlobalKey 'settings' had no value.", filepath);
			return nullptr;
		}

		if (arrayElement->Name() != std::string("array"))
		{
			g_logger_warning("Could not import syntax theme in file '{}'. Expected GlobalKey 'settings' to have value of type <array> instead got <{}>.", filepath, arrayElement->Name());
			return nullptr;
		}

		SyntaxTheme* theme = g_memory_new SyntaxTheme();

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
				theme->defaultForeground = { Vec4{ 1, 1, 1, 1 }, CssStyleType::Value };
				theme->defaultBackground = { Vec4{ 0, 0, 0, 0 }, CssStyleType::Value };

				if (themeSettingsDict)
				{
					const XMLElement* backgroundValue = getValue(themeSettingsDict, "background");
					if (backgroundValue && (std::strcmp(backgroundValue->Name(), "string") == 0))
					{
						theme->defaultBackground = Css::colorFromString(backgroundValue->Value());
					}

					const XMLElement* foregroundValue = getValue(themeSettingsDict, "foreground");
					if (foregroundValue && (std::strcmp(foregroundValue->Name(), "string") == 0))
					{
						theme->defaultForeground = Css::colorFromString(foregroundValue->Value());
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
					g_logger_warning("TokenColor '{}' has no 'scope' key in syntax theme '{}'.", name, filepath);
					continue;
				}

				if (scopeValue->Name() != std::string("string"))
				{
					g_logger_warning("TokenColor '{}' has malformed 'scope' value. Expected array or string but got something else.", name);
					continue;
				}

				const XMLElement* themeSettings = getValue(settingDict, "settings");
				if (themeSettings)
				{
					g_logger_warning("TokenColor '{}' has no 'settings' value. Skipping this rule since it can't style anything without settings.", name);
					continue;
				}

				const XMLElement* foregroundSetting = getValue(themeSettings, "foreground");
				if (foregroundSetting)
				{
					if (foregroundSetting->Name() != std::string("string"))
					{
						g_logger_warning("TokenColor '{}' 'settings.foreground' key is malformed. Expected a string and got something else.", name);
						continue;
					}
				}

				TokenRule rule = {};
				rule.name = name;
				rule.scopeCollection.emplace_back(ScopeRuleCollection::from(scopeValue->Value()));

				if (foregroundSetting)
				{
					const std::string& colorHexStr = foregroundSetting->Value();
					ThemeSetting foregroundSettingAsThemeSetting = {};
					foregroundSettingAsThemeSetting.type = ThemeSettingType::ForegroundColor;
					foregroundSettingAsThemeSetting.foregroundColor = Css::colorFromString(colorHexStr);
					rule.settings.emplace_back(foregroundSettingAsThemeSetting);
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