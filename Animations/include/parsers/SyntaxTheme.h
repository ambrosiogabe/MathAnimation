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
	};

	struct SyntaxTheme
	{
		TokenRule defaultRule;
		CssColor defaultForeground;
		CssColor defaultBackground;
		std::vector<TokenRule> tokenColors;

		const TokenRule* match(const std::vector<ScopedName>& ancestorScopes) const;

		static SyntaxTheme* importTheme(const char* filepath);
		static void free(SyntaxTheme* theme);
	};
}

#endif 