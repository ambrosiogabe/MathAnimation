#include "parsers/SyntaxTheme.h"
#include "platform/Platform.h"
#include "svg/Styles.h"

#include <nlohmann/json.hpp>
#include <../tinyxml2/tinyxml2.h>

using namespace tinyxml2;

namespace MathAnim
{
	using namespace nlohmann;

	// -------------- Internal Functions --------------
	static uint32 parseOrGetDefaultColor(SyntaxTheme* theme, nlohmann::json const& j, std::string const& propertyName, std::string const& fallbackColorValueHex);
	static uint32 tryParseColor(SyntaxTheme* theme, nlohmann::json const& j, std::string const& propertyName);
	static SyntaxTheme* importThemeFromJson(const json& json, const char* filepath);
	static SyntaxTheme* importThemeFromXml(const XMLDocument& document, const char* filepath);
	static const XMLElement* getValue(const XMLElement* element, const std::string& keyName);

	void PackedSyntaxStyle::mergeWith(PackedSyntaxStyle other)
	{
		if (!this->getBackgroundColor())
		{
			this->setBackgroundColor(other.getBackgroundColor());
		}

		if (this->getFontStyle() == CssFontStyle::None)
		{
			this->setFontStyle(other.getFontStyle());
		}

		if (!this->getForegroundColor())
		{
			this->setForegroundColor(other.getForegroundColor());
		}

		if (!this->getLanguageId())
		{
			this->setLanguageId(other.getLanguageId());
		}

		if (!this->getStandardTokenType())
		{
			this->setStandardTokenType(other.getStandardTokenType());
		}
	}

	void PackedSyntaxStyle::overwriteMergeWith(PackedSyntaxStyle other)
	{
		if (other.getBackgroundColor())
		{
			this->setBackgroundColor(other.getBackgroundColor());
		}

		if (other.getFontStyle() != CssFontStyle::None)
		{
			this->setFontStyle(other.getFontStyle());
		}

		if (other.getForegroundColor())
		{
			this->setForegroundColor(other.getForegroundColor());
		}

		if (other.getLanguageId())
		{
			this->setLanguageId(other.getLanguageId());
		}

		if (other.getStandardTokenType())
		{
			this->setStandardTokenType(other.getStandardTokenType());
		}
	}

	void SyntaxTrieNode::insert(std::string const& inName, ScopeSelector const& scope, PackedSyntaxStyle const& inStyle, std::vector<ScopeSelector> const& inAncestors, size_t subScopeIndex)
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
				if (!this->children[subScope].style.isEmpty())
				{
					auto& styleToModify = this->children[subScope].style;

					if (inStyle.getBackgroundColor())
					{
						if (styleToModify.getBackgroundColor())
						{
							g_logger_warning("The scope rule '{}' tried to set the 'backgroundColor' twice. This rule is ambiguous.", scope.getFriendlyName());
						}
						else
						{
							styleToModify.setBackgroundColor(inStyle.getBackgroundColor());
						}
					}

					if (inStyle.getFontStyle() != CssFontStyle::None)
					{
						if (styleToModify.getFontStyle() != CssFontStyle::None)
						{
							g_logger_warning("The scope rule '{}' tried to set the 'fontStyle' twice. This rule is ambiguous.", scope.getFriendlyName());
						}
						else
						{
							styleToModify.setFontStyle(inStyle.getFontStyle());
						}
					}

					if (inStyle.getForegroundColor())
					{
						if (styleToModify.getForegroundColor())
						{
							g_logger_warning("The scope rule '{}' tried to set the 'foregroundColor' twice. This rule is ambiguous.", scope.getFriendlyName());
						}
						else
						{
							styleToModify.setForegroundColor(inStyle.getForegroundColor());
						}
					}

					if (inStyle.getLanguageId())
					{
						if (styleToModify.getLanguageId())
						{
							g_logger_warning("The scope rule '{}' tried to set the 'languageId' twice. This rule is ambiguous.", scope.getFriendlyName());
						}
						else
						{
							styleToModify.setLanguageId(inStyle.getLanguageId());
						}
					}

					if (inStyle.getStandardTokenType())
					{
						if (styleToModify.getStandardTokenType())
						{
							g_logger_warning("The scope rule '{}' tried to set the 'standardTokenType' twice. This rule is ambiguous.", scope.getFriendlyName());
						}
						else
						{
							styleToModify.setStandardTokenType(inStyle.getStandardTokenType());
						}
					}

					return;
				}

				this->children[subScope].style = inStyle;
				this->children[subScope].name = inName;
			}
			else
			{
				SyntaxTrieParentRule newParentRule = {};
				newParentRule.ancestors = inAncestors;
				newParentRule.style = inStyle;

				this->children[subScope].parentRules.push_back(newParentRule);
			}

			// End the recursion
			return;
		}

		// Recurse and complete adding this sub-scope into the tree
		iter->second.insert(inName, scope, inStyle, inAncestors, subScopeIndex + 1);
	}

	void SyntaxTrieNode::calculateInheritedStyles(PackedSyntaxStyle inInheritedStyle, bool isRoot)
	{
		// Don't use the root as a source of inheritance, because it messes some style resolution stuff up.
		// See test `descendantSelectorSucceedsWithPartialMatches_2` for more an example of where this happens.
		if (!isRoot)
		{
			inInheritedStyle.overwriteMergeWith(this->style);
			this->inheritedStyle = inInheritedStyle;
		}

		for (auto& parentRule : this->parentRules)
		{
			parentRule.inheritedStyle = inInheritedStyle;
			parentRule.inheritedStyle.overwriteMergeWith(parentRule.style);
		}

		for (auto& [k, v] : this->children)
		{
			v.calculateInheritedStyles(inInheritedStyle, false);
		}
	}

	static void printTabs(std::string& res, size_t tabDepth = 0)
	{
		for (size_t t = 0; t < tabDepth * 2; t++)
		{
			res += ' ';
		}
	}

	static void printThemeSettings(SyntaxTheme const& theme, PackedSyntaxStyle style, PackedSyntaxStyle inheritedStyle, std::string& res, size_t tabDepth = 0)
	{
		if (style.getForegroundColor())
		{
			printTabs(res, tabDepth);
			res += "(ForegroundColor: ";
			res += toHexString(theme.getColor(style.getForegroundColor())) + ")\n";
		}
		else if (inheritedStyle.getForegroundColor())
		{
			printTabs(res, tabDepth);
			res += "(>>>ForegroundColor: ";
			res += toHexString(theme.getColor(inheritedStyle.getForegroundColor())) + ")\n";
		}

		if (style.getFontStyle() != CssFontStyle::None)
		{
			printTabs(res, tabDepth);
			res += "(FontStyle: " + Css::toString(style.getFontStyle()) + ")\n";
		}
		else if (inheritedStyle.getFontStyle() != CssFontStyle::None)
		{
			printTabs(res, tabDepth);
			res += "(>>>FontStyle: " + Css::toString(inheritedStyle.getFontStyle()) + ")\n";
		}

		if (style.getBackgroundColor())
		{
			printTabs(res, tabDepth);
			res += "(BackgroundColor: ";
			res += toHexString(theme.getColor(style.getBackgroundColor())) + ")\n";
		}
		else if (inheritedStyle.getBackgroundColor())
		{
			printTabs(res, tabDepth);
			res += "(>>>BackgroundColor: ";
			res += toHexString(theme.getColor(inheritedStyle.getBackgroundColor())) + ")\n";
		}
	}

	static void recursivePrint(SyntaxTheme const& theme, SyntaxTrieNode const& node, std::string& res, size_t tabDepth = 0)
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

				printThemeSettings(theme, parent.style, parent.inheritedStyle, res, tabDepth);
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

			printThemeSettings(theme, child.second.style, child.second.inheritedStyle, res, tabDepth);

			// Print children, depth-first style
			recursivePrint(theme, child.second, res, tabDepth + 1);
		}
	}

	void SyntaxTrieNode::print(SyntaxTheme const& theme) const
	{
		std::string res = "";
		recursivePrint(theme, *this, res);
		g_logger_info("Tree: \n{}\n", res);
	}

	PackedSyntaxStyle SyntaxTheme::match(const std::vector<ScopedName>& ancestorScopes) const
	{
		PackedSyntaxStyle resolvedStyle = {};

		// Set the resolved theme to the global default settings in case we don't find a match
		{
			resolvedStyle.setForegroundColor(this->defaultForeground);
			resolvedStyle.setFontStyle(CssFontStyle::Normal);
		}

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
						// Hard merge with inherited styles then the actual styles to get
						// both inherited and default styles applied
						resolvedStyle.overwriteMergeWith(parentRule.inheritedStyle);
						resolvedStyle.overwriteMergeWith(parentRule.style);

						foundParentRuleMatch = true;
						break;
					}
				}
			}

			// If no parent rule match was found, just take the node's settings
			if (!foundParentRuleMatch)
			{
				resolvedStyle.overwriteMergeWith(node->inheritedStyle);
				resolvedStyle.overwriteMergeWith(node->style);
			}
		}

		return resolvedStyle;
	}

	// TODO: This is a duplicate of the function above, but it also keeps track of the name matched for debug purposes.
	//       There's probably a way to combine these two if I really want to
	DebugPackedSyntaxStyle SyntaxTheme::debugMatch(const std::vector<ScopedName>& ancestorScopes, bool* usingDefaultSettings) const
	{
		DebugPackedSyntaxStyle resolvedStyle = {};
		if (usingDefaultSettings != nullptr) *usingDefaultSettings = true;

		// Set the resolved theme to the global default settings in case we don't find a match
		{
			resolvedStyle.style.setForegroundColor(this->defaultForeground);
			resolvedStyle.style.setFontStyle(CssFontStyle::Normal);
		}

		// Ancestors are always passed in order from broad->narrow
		//   Ex. source.lua -> punctuation.string.begin
		std::string newStyleMatched = "";
		for (auto const& ancestor : ancestorScopes)
		{
			SyntaxTrieNode const* node = &this->root;
			newStyleMatched = "";

			// Query the trie and find out what styles apply to this ancestor
			for (auto const& scope : ancestor.dotSeparatedScopes)
			{
				auto iter = node->children.find(scope.getScopeName());
				if (iter == node->children.end())
				{
					// Strip last '.' off the resolved theme match
					newStyleMatched = newStyleMatched.substr(0, newStyleMatched.length() - 1);
					break;
				}

				// Drill down to the deepest node
				node = &iter->second;
				newStyleMatched += scope.getScopeName() + ".";
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
						// Hard merge with inherited styles then the actual styles to get
						// both inherited and default styles applied
						resolvedStyle.style.overwriteMergeWith(parentRule.inheritedStyle);
						resolvedStyle.style.overwriteMergeWith(parentRule.style);

						for (auto const& resolvedAncestor : parentRule.ancestors)
						{
							newStyleMatched = resolvedAncestor.getFriendlyName() + " " + newStyleMatched;
						}

						if (resolvedStyle.style.getForegroundColor() != 0)
						{
							foundParentRuleMatch = true;
							resolvedStyle.styleMatched = newStyleMatched;
						}
						break;
					}
				}
			}

			// If no parent rule match was found, just take the node's settings
			if (!foundParentRuleMatch)
			{
				resolvedStyle.style.overwriteMergeWith(node->inheritedStyle);
				resolvedStyle.style.overwriteMergeWith(node->style);

				if (node->style.getForegroundColor() != 0)
				{
					resolvedStyle.styleMatched = newStyleMatched;
				}
			}

			if (usingDefaultSettings != nullptr) *usingDefaultSettings = (node == &root);
		}

		return resolvedStyle;
	}

	uint32 SyntaxTheme::getOrCreateColorIndex(std::string const& colorStr, Vec4 const& color)
	{
		if (auto iter = this->colorMap.find(colorStr); iter == this->colorMap.end())
		{
			uint32 newColorIndex = (uint32)this->colors.size();
			g_logger_assert(newColorIndex < (1 << 9), "Invalid color. We can only have a maximum of '{}' unique colors in a style.", (1 << 9));
			this->colorMap[colorStr] = newColorIndex;
			this->colors.push_back(color);
			return newColorIndex;
		}
		else
		{
			return iter->second;
		}
	}

	Vec4 const& SyntaxTheme::getColor(uint32 id) const
	{
		g_logger_assert(id < this->colors.size(), "Invalid id '{}'. Only '{}' colors are available.", id, this->colors.size());
		return this->colors[id];
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
				SyntaxTheme* theme = importThemeFromJson(j, filepathStr);
				theme->root.calculateInheritedStyles(PackedSyntaxStyle{}, true);
				return theme;
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

			SyntaxTheme* theme = importThemeFromXml(doc, filepathStr);
			theme->root.calculateInheritedStyles(PackedSyntaxStyle{}, true);
			return theme;
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
	static uint32 parseOrGetDefaultColor(SyntaxTheme* theme, nlohmann::json const& j, std::string const& propertyName, std::string const& fallbackColorValueHex)
	{
		if (j.contains(propertyName))
		{
			std::string const& colorStr = j[propertyName];
			CssColor color = Css::colorFromString(colorStr);
			g_logger_assert(color.styleType == CssStyleType::Value, "Cannot inherit color in default styles.");
			return theme->getOrCreateColorIndex(colorStr, color.color);
		}

		Vec4 colorValue = toHex(fallbackColorValueHex);
		return theme->getOrCreateColorIndex(fallbackColorValueHex, colorValue);
	}

	// @returns 0 (which represents an invalid color) when parsing fails
	static uint32 tryParseColor(SyntaxTheme* theme, nlohmann::json const& j, std::string const& propertyName)
	{
		if (j.contains(propertyName))
		{
			std::string const& colorStr = j[propertyName];
			CssColor color = Css::colorFromString(colorStr);
			g_logger_assert(color.styleType == CssStyleType::Value, "Cannot inherit color in default styles.");
			return theme->getOrCreateColorIndex(colorStr, color.color);
		}

		return 0;
	}

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

		// Set the 0th and 1rst color to an ugly pink for debugging purposes. The 0th and 1rst index should always be invalid.
		theme->colors.push_back("#f542f2"_hex);
		theme->colors.push_back("#f542f2"_hex);

		if (j.contains("colors"))
		{
			nlohmann::json const& colorsJson = j["colors"];

			theme->defaultForeground = parseOrGetDefaultColor(theme, colorsJson, "editor.foreground", "#FFFFFF");
			theme->defaultBackground = parseOrGetDefaultColor(theme, colorsJson, "editor.background", "#000000");

			theme->editorLineHighlightBackground = tryParseColor(theme, colorsJson, "editor.lineHighlightBackground");
			theme->editorSelectionBackground = tryParseColor(theme, colorsJson, "editor.selectionBackground");
			theme->editorCursorForeground = tryParseColor(theme, colorsJson, "editorCursor.foreground");

			theme->editorLineNumberForeground = tryParseColor(theme, colorsJson, "editorLineNumber.foreground");
			theme->editorLineNumberActiveForeground = tryParseColor(theme, colorsJson, "editorLineNumber.activeForeground");

			theme->scrollbarSliderBackground = tryParseColor(theme, colorsJson, "scrollbarSlider.background");
			theme->scrollbarSliderActiveBackground = tryParseColor(theme, colorsJson, "scrollbarSlider.activeBackground");
			theme->scrollbarSliderHoverBackground = tryParseColor(theme, colorsJson, "scrollbarSlider.hoverBackground");

			// Initialize the root of our tree (these are global settings)
			theme->root.style.setForegroundColor(theme->defaultForeground);
			theme->root.style.setBackgroundColor(theme->defaultBackground);
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
					// This is presumably the default foreground/background settings and not a token rule
					if (color["settings"].contains("foreground"))
					{
						std::string const& foregroundColorStr = color["settings"]["foreground"];
						CssColor cssColor = Css::colorFromString(foregroundColorStr);
						g_logger_assert(cssColor.styleType == CssStyleType::Value, "Cannot inherit color in default styles.");
						theme->defaultForeground = theme->getOrCreateColorIndex(foregroundColorStr, cssColor.color);

						theme->root.style.setForegroundColor(theme->defaultForeground);
					}

					if (color["settings"].contains("background"))
					{
						std::string const& backgroundColorStr = color["settings"]["background"];
						CssColor cssColor = Css::colorFromString(backgroundColorStr);
						g_logger_assert(cssColor.styleType == CssStyleType::Value, "Cannot inherit color in default styles.");
						theme->defaultBackground = theme->getOrCreateColorIndex(backgroundColorStr, cssColor.color);

						theme->root.style.setBackgroundColor(theme->defaultBackground);
					}
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

			PackedSyntaxStyle trieThemeSettings = {};
			if (settingsJson.contains("foreground"))
			{
				const std::string& foregroundColorStr = settingsJson["foreground"];
				CssColor cssColor = Css::colorFromString(foregroundColorStr);
				if (cssColor.styleType == CssStyleType::Inherit)
				{
					trieThemeSettings.setForegroundColorInherited();
				}
				else
				{
					trieThemeSettings.setForegroundColor(theme->getOrCreateColorIndex(foregroundColorStr, cssColor.color));
				}
			}

			if (settingsJson.contains("background"))
			{
				const std::string& backgroundColorStr = settingsJson["background"];
				CssColor cssColor = Css::colorFromString(backgroundColorStr);
				if (cssColor.styleType == CssStyleType::Inherit)
				{
					trieThemeSettings.setBackgroundColorInherited();
				}
				else
				{
					trieThemeSettings.setBackgroundColor(theme->getOrCreateColorIndex(backgroundColorStr, cssColor.color));
				}
			}

			if (settingsJson.contains("fontStyle"))
			{
				std::string const& fontStyleStr = settingsJson["fontStyle"];
				trieThemeSettings.setFontStyle(Css::fontStyleFromString(fontStyleStr));
			}

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
				//theme->defaultForeground = { Vec4{ 1, 1, 1, 1 }, CssStyleType::Value };
				//theme->defaultBackground = { Vec4{ 0, 0, 0, 0 }, CssStyleType::Value };

				if (themeSettingsDict)
				{
					const XMLElement* backgroundValue = getValue(themeSettingsDict, "background");
					if (backgroundValue && (std::strcmp(backgroundValue->Name(), "string") == 0))
					{
						//theme->defaultBackground = Css::colorFromString(backgroundValue->Value());
					}

					const XMLElement* foregroundValue = getValue(themeSettingsDict, "foreground");
					if (foregroundValue && (std::strcmp(foregroundValue->Name(), "string") == 0))
					{
						//theme->defaultForeground = Css::colorFromString(foregroundValue->Value());
					}
				}

				g_logger_error("TODO: Fix XML theme support.");
				//TokenRule defaultThemeRule = {};
				//ThemeSetting defaultThemeSetting = {};
				//defaultThemeSetting.type = ThemeSettingType::ForegroundColor;
				//defaultThemeSetting.foregroundColor = theme->defaultForeground;
				//defaultThemeRule.settings.push_back(defaultThemeSetting);
				//theme->defaultRule = defaultThemeRule;
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

				g_logger_error("TODO: Fix XML theme support.");
				//TokenRule rule = {};
				//rule.name = name;
				//rule.scopeCollection.emplace_back(ScopeRuleCollection::from(scopeValue->Value()));

				//if (foregroundSetting)
				//{
				//	const std::string& colorHexStr = foregroundSetting->Value();
				//	ThemeSetting foregroundSettingAsThemeSetting = {};
				//	foregroundSettingAsThemeSetting.type = ThemeSettingType::ForegroundColor;
				//	foregroundSettingAsThemeSetting.foregroundColor = Css::colorFromString(colorHexStr);
				//	rule.settings.emplace_back(foregroundSettingAsThemeSetting);
				//}

				//theme->tokenColors.emplace_back(rule);
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
