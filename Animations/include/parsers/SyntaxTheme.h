#ifndef MATH_ANIM_SYNTAX_THEME_H
#define MATH_ANIM_SYNTAX_THEME_H
#include "parsers/Common.h"
#include "svg/Styles.h"

namespace MathAnim
{
	enum class ThemeSettingType : uint8
	{
		None = 0,
		ForegroundColor
	};

	struct ThemeSetting
	{
		ThemeSettingType type;
		std::optional<CssColor> foregroundColor;
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

	struct SyntaxTheme
	{
		TokenRule defaultRule;
		CssColor defaultForeground;
		CssColor defaultBackground;
		std::vector<TokenRule> tokenColors;

		TokenRuleMatch match(const std::vector<ScopedName>& ancestorScopes) const;
		const ThemeSetting* match(const std::vector<ScopedName>& ancestorScopes, ThemeSettingType settingType) const;

		static SyntaxTheme* importTheme(const char* filepath);
		static void free(SyntaxTheme* theme);
	};
}

#endif 