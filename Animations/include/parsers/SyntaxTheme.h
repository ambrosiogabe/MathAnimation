#ifndef MATH_ANIM_SYNTAX_THEME_H
#define MATH_ANIM_SYNTAX_THEME_H
#include "parsers/Common.h"

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
		std::optional<Vec4> foregroundColor;
	};

	struct TokenRule
	{
		std::string name;
		std::vector<ScopeRule> scopes;
		std::vector<ThemeSetting> settings;
	};

	struct SyntaxTheme
	{
		TokenRule defaultRule;
		Vec4 defaultForeground;
		Vec4 defaultBackground;
		std::vector<TokenRule> tokenColors;

		const TokenRule* match(const ScopeRule& scope) const;

		static SyntaxTheme* importTheme(const char* filepath);
		static void free(SyntaxTheme* theme);
	};
}

#endif 