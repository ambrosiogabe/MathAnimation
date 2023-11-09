#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"
#include "svg/Styles.h"

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

	const ThemeSetting* SyntaxTrieTheme::getSetting(ThemeSettingType type) const
	{
		if (auto iter = settings.find(type); iter != settings.end())
		{
			return &iter->second;
		}

		return nullptr;
	}

	Vec4 const& SyntaxTrieTheme::getForegroundColor(SyntaxTheme const* theme) const
	{
		if (auto setting = getSetting(ThemeSettingType::ForegroundColor); setting != nullptr)
		{
			return setting->foregroundColor.value().color;
		}

		static Vec4 defaultWhite = "#FFF"_hex;
		return theme != nullptr ? theme->defaultForeground.color : defaultWhite;
	}

	CssColor const& SyntaxTrieTheme::getForegroundCssColor(SyntaxTheme const* theme) const
	{
		if (auto setting = getSetting(ThemeSettingType::ForegroundColor); setting != nullptr)
		{
			return setting->foregroundColor.value();
		}

		static CssColor defaultWhite = {};
		defaultWhite.color = "#FFFFFF"_hex;
		defaultWhite.styleType = CssStyleType::Value;

		return theme != nullptr ? theme->defaultForeground : defaultWhite;
	}

	CssFontStyle SyntaxTrieTheme::getFontStyle() const
	{
		if (auto setting = getSetting(ThemeSettingType::FontStyle); setting != nullptr)
		{
			return setting->fontStyle.value();
		}

		return CssFontStyle::Normal;
	}

	void SyntaxTrieNode::insert(std::string const& inName, ScopeSelector const& scope, SyntaxTrieTheme const& inTheme, std::vector<ScopeSelector> const& inAncestors, SyntaxTrieTheme& inheritedTheme, size_t subScopeIndex)
	{
		g_logger_assert(subScopeIndex < scope.dotSeparatedScopes.size(), "Invalid sub-scope index '{}' encountered while generating theme trie. Out of range for sub-scopes of size '{}'.", subScopeIndex, scope.dotSeparatedScopes.size());

		std::string const& subScope = scope.dotSeparatedScopes[subScopeIndex];
		auto iter = this->children.find(subScope);

		// Add the node if it hasn't been added already
		if (iter == this->children.end())
		{
			this->children[subScope] = {};

			// Reassign iter so it now has a value
			iter = this->children.find(subScope);

			// Merge inherited themes so our final theme has all inherited properties + manually set properties
			for (auto& [k, v] : inheritedTheme.settings)
			{
				if (iter->second.theme.settings.find(k) == iter->second.theme.settings.end())
				{
					iter->second.theme.settings[k] = v;
					iter->second.theme.settings[k].inherited = true;
				}
			}
		}
		else
		{
			// Merge this node's settings with the inherited settings
			for (auto& [k, v] : iter->second.theme.settings)
			{
				inheritedTheme.settings[k] = v;
				inheritedTheme.settings[k].inherited = true;
			}

			// Merge the inherited settings with this node
			for (auto& [k, v] : inheritedTheme.settings)
			{
				if (iter->second.theme.settings.find(k) == iter->second.theme.settings.end())
				{
					iter->second.theme.settings[k] = v;
					iter->second.theme.settings[k].inherited = true;
				}
			}
		}

		// We've reached the end of this path in the trie, now insert the styles here and assert that
		// a style/parent-style hasn't already been set for this particular rule
		if (subScopeIndex == scope.dotSeparatedScopes.size() - 1)
		{
			// Add the style to the parent rule or the node
			if (inAncestors.size() == 0)
			{
				// Some themes are ill-formed (naughty developers!). If an ill-formed theme is found,
				// we'll log a warning then discard the newer value and prefer the first-match instead
				// 
				// Sometimes though, the developers are merging different theme settings, like adding font-style
				// or a color to a group of themes. In those cases, we'll just merge the themes.
				if (this->children[subScope].theme.settings.size() != 0)
				{
					auto& themesToAddTo = this->children[subScope].theme.settings;

					for (auto& [k, v] : inTheme.settings)
					{
						// If the setting exists and wasn't inherited, somebody tried to add it twice
						if (auto themeIter = themesToAddTo.find(k); themeIter != themesToAddTo.end() && !themeIter->second.inherited)
						{
							g_logger_warning("The scope rule '{}' tried to set the theme setting '{}' twice. This rule is ambiguous.", scope.getFriendlyName(), k);
						}
						else
						{
							themesToAddTo[k] = v;
							themesToAddTo[k].inherited = false;
						}
					}

					return;
				}

				this->children[subScope].theme = inTheme;
				this->children[subScope].name = inName;
			}
			else
			{
				SyntaxTrieParentRule newParentRule = {};
				newParentRule.ancestors = inAncestors;
				newParentRule.theme = inTheme;

				// Merge inherited themes so our final theme has all inherited properties + manually set properties
				for (auto& [k, v] : inheritedTheme.settings)
				{
					if (newParentRule.theme.settings.find(k) == newParentRule.theme.settings.end())
					{
						newParentRule.theme.settings[k] = v;
						newParentRule.theme.settings[k].inherited = true;
					}
				}

				this->children[subScope].parentRules.push_back(newParentRule);
			}

			// End the recursion
			return;
		}

		// Recurse and complete adding this sub-scope into the tree
		iter->second.insert(inName, scope, inTheme, inAncestors, inheritedTheme, subScopeIndex + 1);
	}

	static void printThemeSettings(SyntaxTrieTheme const& theme, std::string& res, size_t tabDepth = 0)
	{
		for (auto& [k, v] : theme.settings)
		{
			for (size_t t = 0; t < tabDepth * 2; t++)
			{
				res += ' ';
			}
			res += "(";

			if (v.inherited)
			{
				res += ">>>";
			}

			if (k == ThemeSettingType::FontStyle)
			{
				res += "FontStyle: ";
				switch (v.fontStyle.value())
				{
				case CssFontStyle::None:
					res += "None";
					break;
				case CssFontStyle::Bold:
					res += "Bold";
					break;
				case CssFontStyle::Inherit:
					res += "Inherit";
					break;
				case CssFontStyle::Italic:
					res += "Italic";
					break;
				case CssFontStyle::Normal:
					res += "Normal";
					break;
				}

				res += ")";
				res += '\n';
			}
			else if (k == ThemeSettingType::ForegroundColor)
			{
				res += "ForegroundColor: ";
				res += toHexString(v.foregroundColor.value().color);
				res += ")";
				res += '\n';
			}
		}
	}

	static void recursivePrint(SyntaxTrieNode const& node, std::string& res, size_t tabDepth = 0)
	{
		if (node.parentRules.size() != 0)
		{
			for (size_t i = 0; i < node.parentRules.size(); i++)
			{
				auto& parent = node.parentRules[i];

				// Print parent rules
				for (size_t t = 0; t < tabDepth * 2; t++)
				{
					res += ' ';
				}
				res += "-^";

				for (auto& ancestor : parent.ancestors)
				{
					res += ancestor.getFriendlyName();
					res += ' ';
				}

				res += '\n';

				printThemeSettings(parent.theme, res, tabDepth);
			}
		}

		for (auto& child : node.children)
		{
			// Print node connection
			for (size_t i = 0; i < tabDepth * 2; i++)
			{
				res += ' ';
			}
			res += "|->";
			res += child.first;
			res += '\n';

			printThemeSettings(child.second.theme, res, tabDepth);

			// Print children, depth-first style
			recursivePrint(child.second, res, tabDepth + 1);
		}
	}

	void SyntaxTrieNode::print() const
	{
		std::string res = "";
		recursivePrint(*this, res);
		g_logger_info("Tree: \n{}\n", res);
	}

	SyntaxTrieTheme SyntaxTheme::match(const std::vector<ScopedName>& ancestorScopes) const
	{
		SyntaxTrieTheme resolvedTheme = {};
		resolvedTheme.usingDefaultSettings = false;
		resolvedTheme.styleMatched = "";

		// Set the resolved theme to the global default settings in case we don't find a match
		{
			auto& foregroundSetting = resolvedTheme.settings[ThemeSettingType::ForegroundColor];
			auto& fontStyleSetting = resolvedTheme.settings[ThemeSettingType::FontStyle];

			foregroundSetting.foregroundColor = this->defaultForeground;
			foregroundSetting.type = ThemeSettingType::ForegroundColor;

			fontStyleSetting.fontStyle = CssFontStyle::Normal;
			fontStyleSetting.type = ThemeSettingType::FontStyle;
		}

		// Ancestors are always passed in order from broad->narrow
		//   Ex. source.lua -> punctuation.string.begin
		for (auto const& ancestor : ancestorScopes)
		{
			SyntaxTrieNode const* node = &this->root;
			resolvedTheme.styleMatched = "";

			// Query the trie and find out what styles apply to this ancestor
			for (auto const& scope : ancestor.dotSeparatedScopes)
			{
				auto iter = node->children.find(scope.getScopeName());
				if (iter == node->children.end())
				{
					// Strip last '.' off the resolved theme match
					resolvedTheme.styleMatched = resolvedTheme.styleMatched.substr(0, resolvedTheme.styleMatched.length() - 1);
					break;
				}

				// Drill down to the deepest node
				node = &iter->second;
				resolvedTheme.styleMatched += scope.getScopeName() + ".";
			}

			// First check if we match a parent rule, if we do, we'll take that because 
			// a parent rule match has more specificity then a normal match
			bool foundParentRuleMatch = false;
			if (node->parentRules.size() > 0)
			{
				for (auto const& parentRule : node->parentRules)
				{
					// parentRule.ancestors goes from broad -> narrow
					//   Ex: source.js -> string.quoted

					// If this parent rule matches, we should expect to find all the ancestors listed in the same order
					auto parentRuleAncestorCheck = parentRule.ancestors.begin();
					for (auto const& ancestorCheck : ancestorScopes)
					{
						// We have a match
						if (parentRuleAncestorCheck == parentRule.ancestors.end())
						{
							break;
						}

						if (ancestorCheck.matches(*parentRuleAncestorCheck))
						{
							parentRuleAncestorCheck++;
						}
					}

					// We have a match
					if (parentRuleAncestorCheck == parentRule.ancestors.end())
					{
						// Set all settings appropriately
						for (auto [k, v] : parentRule.theme.settings)
						{
							resolvedTheme.settings[k] = v;
						}

						for (auto const& resolvedAncestor : parentRule.ancestors)
						{
							resolvedTheme.styleMatched = resolvedAncestor.getFriendlyName() + " " + resolvedTheme.styleMatched;
						}

						foundParentRuleMatch = true;
						break;
					}
				}
			}

			// If no parent rule match was found, just take the node's settings
			if (!foundParentRuleMatch)
			{
				for (auto [k, v] : node->theme.settings)
				{
					resolvedTheme.settings[k] = v;
				}
			}

			resolvedTheme.usingDefaultSettings = node == &root;
		}

		return resolvedTheme;
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

		// Initialize the trie
		theme->root = {};

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

			// Initialize the root of our tree (these are global settings)
			theme->root.theme.settings[ThemeSettingType::ForegroundColor] = defaultThemeSetting;
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

					// Clear the global settings first
					theme->root.theme.settings = {};
					// Then initialize the root of our tree with these new global settings
					theme->root.theme.settings[ThemeSettingType::ForegroundColor] = defaultThemeSetting;
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

			// TODO: This is duplicated, remove the block below once we verify that the trie is working correctly
			SyntaxTrieTheme trieThemeSettings = {};
			if (settingsJson.contains("foreground"))
			{
				const std::string& foregroundColorStr = settingsJson["foreground"];
				ThemeSetting trieTheme = ThemeSetting{};
				trieTheme.type = ThemeSettingType::ForegroundColor;
				trieTheme.foregroundColor = Css::colorFromString(foregroundColorStr);

				trieThemeSettings.settings[ThemeSettingType::ForegroundColor] = trieTheme;
			}

			if (settingsJson.contains("fontStyle"))
			{
				std::string const& fontStyleStr = settingsJson["fontStyle"];
				ThemeSetting trieTheme = ThemeSetting{};
				trieTheme.type = ThemeSettingType::FontStyle;
				trieTheme.fontStyle = Css::fontStyleFromString(fontStyleStr);

				trieThemeSettings.settings[ThemeSettingType::FontStyle] = trieTheme;
			}

			TokenRule rule = {};
			rule.name = name;
			if (color["scope"].is_array())
			{
				for (auto scope : color["scope"])
				{
					auto selectorCollection = ScopeSelectorCollection::from(scope);

					// Insert every rule into the trie so we have a resolved path for this specific rule
					for (auto& descendantSelector : selectorCollection.descendantSelectors)
					{
						// If there's a parent scope, grab it
						if (descendantSelector.descendants.size() >= 2)
						{
							ScopeSelector child = descendantSelector.descendants[descendantSelector.descendants.size() - 1];
							descendantSelector.descendants.pop_back();

							// Insert the ancestors scope
							theme->root.insert(name, child, trieThemeSettings, descendantSelector.descendants);
						}
						else
						{
							// Insert the child scope
							theme->root.insert(name, descendantSelector.descendants[0], trieThemeSettings);
						}
					}

					// TODO: Remove this once we get the new trie implementation working
					rule.scopeCollection.push_back(ScopeRuleCollection::from(scope));
				}
			}
			else if (color["scope"].is_string())
			{
				// TODO: This is duplicated from the stuff right above, it should prolly be a function

				auto selectorCollection = ScopeSelectorCollection::from(color["scope"]);

				// Insert every rule into the trie so we have a resolved path for this specific rule
				for (auto& descendantSelector : selectorCollection.descendantSelectors)
				{
					// If there's a parent scope, grab it
					if (descendantSelector.descendants.size() >= 2)
					{
						ScopeSelector child = descendantSelector.descendants[descendantSelector.descendants.size() - 1];
						descendantSelector.descendants.pop_back();

						// Insert the ancestors scope
						theme->root.insert(name, child, trieThemeSettings, descendantSelector.descendants);
					}
					else
					{
						// Insert the child scope
						theme->root.insert(name, descendantSelector.descendants[0], trieThemeSettings);
					}
				}

				// TODO: Remove this once we get the new trie implementation working
				rule.scopeCollection.push_back(ScopeRuleCollection::from(color["scope"]));
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

CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::ThemeSettingType& style)
{
	switch (style)
	{
	case MathAnim::ThemeSettingType::FontStyle:
		ostream << "<ThemeSettingType:FontStyle>";
		break;
	case MathAnim::ThemeSettingType::ForegroundColor:
		ostream << "<ThemeSettingType:ForegroundColor>";
		break;
	case MathAnim::ThemeSettingType::None:
		ostream << "<ThemeSettingType:None>";
		break;
	};

	return ostream;
}