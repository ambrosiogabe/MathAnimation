#ifndef MATH_ANIM_SYNTAX_THEME_H
#define MATH_ANIM_SYNTAX_THEME_H
#include "parsers/Common.h"
#include "svg/Styles.h"

#include <cppUtils/cppPrint.hpp>
#include <cppUtils/cppUtils.hpp>

namespace MathAnim
{
	enum class ThemeSettingType : uint8
	{
		None = 0,
		ForegroundColor,
		FontStyle,
	};

	struct ThemeSetting
	{
		ThemeSettingType type;
		std::optional<CssColor> foregroundColor;
		std::optional<CssFontStyle> fontStyle;
	};

	struct TokenRule
	{
		std::string name;
		std::vector<ScopeRuleCollection> scopeCollection;
		std::vector<ThemeSetting> settings;

		const ThemeSetting* getSetting(ThemeSettingType type) const;
	};

	struct TokenRuleMatch
	{
		const TokenRule* matchedRule;
		std::string styleMatched;

		inline const ThemeSetting* getSetting(ThemeSettingType type) const { return matchedRule ? matchedRule->getSetting(type) : nullptr; }
	};

	struct SyntaxTrieTheme
	{
		// Resolved settings to apply if this node is a match
		std::unordered_map<ThemeSettingType, ThemeSetting> settings;

		const ThemeSetting* getSetting(ThemeSettingType type) const;
	};

	struct SyntaxTrieParentRule
	{
		std::vector<ScopedName> ancestors;
		SyntaxTrieTheme theme;
	};

	struct SyntaxTrieNode
	{
		std::string name;
		SyntaxTrieTheme theme;
		std::vector<SyntaxTrieParentRule> parentRules;

		// Map from sub-scope to child
		// Ex. In the scope "var.identifier", the var node would have one child in the map:
		//       <"identifier", Node>
		std::unordered_map<std::string, SyntaxTrieNode> children;

		void insert(std::string const& name, ScopedName const& scope, SyntaxTrieTheme const& theme, std::vector<ScopedName> const& ancestors = {}, size_t subScopeIndex = 0);
	};

	struct SyntaxTheme
	{
		// TODO: Deprecate me, old way to query themes
		TokenRule defaultRule;
		CssColor defaultForeground;
		CssColor defaultBackground;
		std::vector<TokenRule> tokenColors;

		// TODO: Switch to this, once we verify it's working correctly
		SyntaxTrieNode root;
		std::unordered_map<std::string, uint32> colorMap;
		std::vector<Vec4> colors;

		TokenRuleMatch match(const std::vector<ScopedName>& ancestorScopes) const;
		const ThemeSetting* match(const std::vector<ScopedName>& ancestorScopes, ThemeSettingType settingType) const;

		static SyntaxTheme* importTheme(const char* filepath);
		static void free(SyntaxTheme* theme);
	};
}

// Print functions
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::ThemeSettingType& style);

#endif 