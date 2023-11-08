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

	void SyntaxTrieNode::insert(std::string const& inName, ScopeSelector const& scope, SyntaxTrieTheme const& inTheme, std::vector<ScopeSelector> const& inAncestors, SyntaxTrieTheme& inheritedTheme, size_t subScopeIndex)
	{
		g_logger_assert(subScopeIndex < scope.dotSeparatedScopes.size(), "Invalid sub-scope index '{}' encountered while generating theme trie. Out of range for sub-scopes of size '{}'.", subScopeIndex, scope.dotSeparatedScopes.size());

		std::string const& subScope = scope.dotSeparatedScopes[subScopeIndex];
		auto iter = this->children.find(subScope);

		// We've reached the end of this path in the trie, now insert the styles here and assert that
		// a style/parent-style hasn't already been set for this particular rule
		if (subScopeIndex == scope.dotSeparatedScopes.size() - 1)
		{
			// Add the node if it hasn't been added already
			if (iter == this->children.end())
			{
				this->children[subScope] = {};
				auto& newChild = this->children[subScope];

				// Merge inherited themes so our final theme has all inherited properties + manually set properties
				for (auto& [k, v] : inheritedTheme.settings)
				{
					if (newChild.theme.settings.find(k) == newChild.theme.settings.end())
					{
						newChild.theme.settings[k] = v;
						newChild.theme.settings[k].inherited = true;
					}
				}
			}

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
		if (iter == this->children.end())
		{
			// Create a new scope for this guy
			this->children[subScope] = {};
			this->children[subScope].theme = inheritedTheme;
			this->children[subScope].insert(inName, scope, inTheme, inAncestors, inheritedTheme, subScopeIndex + 1);
		}
		else
		{
			// Merge the inherited theme props with this node's props so we can propagate inherited themes downwards
			for (auto& [k, v] : iter->second.theme.settings)
			{
				inheritedTheme.settings[k] = v;
			}

			iter->second.insert(inName, scope, inTheme, inAncestors, inheritedTheme, subScopeIndex + 1);
		}
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

	TokenRuleMatch SyntaxTheme::match(const std::vector<ScopedName>& ancestorScopes) const
	{
		// Make sure these two things behave the same
		ThemeSetting trieMatch = this->matchTrie(ancestorScopes);

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
		for (size_t i = 0; i < potentialMatches.size(); i++)
		{
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

			if (auto* setting = tokenRuleMatch.getSetting(ThemeSettingType::ForegroundColor); setting != nullptr)
			{
				g_logger_assert(trieMatch.foregroundColor.value().color == setting->foregroundColor.value().color, "The trie does not return the same result here.");
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

	ThemeSetting SyntaxTheme::matchTrie(const std::vector<ScopedName>& ancestorScopes) const
	{
		ThemeSetting res = {};
		res.foregroundColor = this->defaultForeground;
		res.type = ThemeSettingType::ForegroundColor;

		// Ancestors are always passed in order from broad->narrow
		//   Ex. source.lua -> punctuation.string.begin
		for (auto const& ancestor : ancestorScopes)
		{
			SyntaxTrieNode const* node = &this->root;

			// Query the trie and find out what styles apply to this ancestor
			for (auto const& scope : ancestor.dotSeparatedScopes)
			{
				auto iter = node->children.find(scope.getScopeName());
				if (iter == node->children.end())
				{
					break;
				}

				// Drill down to the deepest node
				node = &iter->second;
			}

			if (auto setting = node->theme.getSetting(ThemeSettingType::ForegroundColor); setting != nullptr)
			{
				res.foregroundColor = setting->foregroundColor.value();
			}

			if (node->parentRules.size() > 0)
			{
				// TODO: Check if this node is a descendant of one of these parent rules
				//       If it is, use that style instead since it's more specific
			}
		}

		return res;
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