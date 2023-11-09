#ifdef _MATH_ANIM_TESTS
#include "SyntaxThemeTests.h"

#include "core.h"
#include "parsers/SyntaxHighlighter.h"
#include "parsers/SyntaxTheme.h"
#include "parsers/Grammar.h"

using namespace CppUtils;

namespace MathAnim
{
	namespace SyntaxThemeTests
	{
		// -------------------- Init/Teardown --------------------
		DEFINE_BEFORE_ALL(beforeAll)
		{
			g_logger_level oldLevel = g_logger_get_level();
			g_logger_set_level(g_logger_level_None);
			Highlighters::init();
			g_logger_set_level(oldLevel);
			END_BEFORE_ALL;
		}

		DEFINE_AFTER_ALL(afterAll)
		{
			Highlighters::free();
			END_AFTER_ALL;
		}

		// -------------------- Tests --------------------
		DEFINE_TEST(basicMatchingGetsAppropriateTheme_1)
		{
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::Gruvbox);

			const std::vector<ScopedName> ancestorScopes = {
				ScopedName::from("storage.type.annotation")
			};

			auto res = theme->match(ancestorScopes);

			ASSERT_EQUAL(res.getFontStyle(), CssFontStyle::Bold);
			ASSERT_EQUAL(res.getForegroundColor(theme), "#83a598"_hex);

			END_TEST;
		}

		DEFINE_TEST(basicMatchingGetsAppropriateTheme_2)
		{
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::Panda);

			const std::vector<ScopedName> ancestorScopes = {
				ScopedName::from("variable.language.this")
			};

			auto res = theme->match(ancestorScopes);

			ASSERT_EQUAL(res.getFontStyle(), CssFontStyle::Normal);
			ASSERT_EQUAL(res.getForegroundColor(theme), "#FF9AC1"_hex);

			END_TEST;
		}

		DEFINE_TEST(basicMatchingGetsAppropriateTheme_3)
		{
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::Panda);

			const std::vector<ScopedName> ancestorScopes = {
				ScopedName::from("punctuation.definition.expression")
			};

			auto res = theme->match(ancestorScopes);

			ASSERT_EQUAL(res.getFontStyle(), CssFontStyle::Italic);
			ASSERT_EQUAL(res.getForegroundColor(theme), "#FFCC95"_hex);

			END_TEST;
		}

		DEFINE_TEST(inheritedThemesCanBeOverwritten)
		{
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::Gruvbox);
			
			const std::vector<ScopedName> ancestorScopes = {
				ScopedName::from("source.js"),
				ScopedName::from("comment.line.double-slash.js"),
				ScopedName::from("punctuation.definition.comment.js")
			};

			auto res = theme->match(ancestorScopes);

			ASSERT_EQUAL(res.getFontStyle(), CssFontStyle::Italic);
			ASSERT_EQUAL(res.getForegroundColor(theme), "#928374ff"_hex);

			END_TEST;
		}

		DEFINE_TEST(descendantSelectorHasPrecedenceOverNormalSelector)
		{
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::Gruvbox);

			const std::vector<ScopedName> ancestorScopes = {
				ScopedName::from("source.json"),
				ScopedName::from("meta.structure.dictionary.json"),
				ScopedName::from("support.type.property-name.json")
			};

			auto res = theme->match(ancestorScopes);

			ASSERT_EQUAL(res.getFontStyle(), CssFontStyle::Normal);
			ASSERT_EQUAL(res.getForegroundColor(theme), "#B8BB26FF"_hex);

			END_TEST;
		}

		DEFINE_TEST(descendantSelectorFailsWhenAncestorsDontMatch)
		{
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::Gruvbox);

			const std::vector<ScopedName> ancestorScopes = {
				ScopedName::from("source.json"),
				ScopedName::from("meta.structure.dictionary.cs"),
				ScopedName::from("support.type.property-name.json")
			};

			auto res = theme->match(ancestorScopes);

			ASSERT_EQUAL(res.getFontStyle(), CssFontStyle::Normal);
			ASSERT_EQUAL(res.getForegroundColor(theme), "#689D6AFF"_hex);

			END_TEST;
		}

		DEFINE_TEST(descendantSelectorSucceedsWithPartialMatches)
		{
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::Gruvbox);

			const std::vector<ScopedName> ancestorScopes = {
				ScopedName::from("source"),
				ScopedName::from("meta.structure"),
				ScopedName::from("support.type.property-name.json")
			};

			auto res = theme->match(ancestorScopes);

			ASSERT_EQUAL(res.getFontStyle(), CssFontStyle::Normal);
			ASSERT_EQUAL(res.getForegroundColor(theme), "#B8BB26FF"_hex);

			END_TEST;
		}

		DEFINE_TEST(descendantSelectorFailsWhenBottomNodeIsNotSpecific)
		{
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::Gruvbox);

			const std::vector<ScopedName> ancestorScopes = {
				ScopedName::from("source"),
				ScopedName::from("meta.structure"),
				ScopedName::from("support.type.property-name")
			};

			auto res = theme->match(ancestorScopes);

			ASSERT_EQUAL(res.getFontStyle(), CssFontStyle::Normal);
			ASSERT_EQUAL(res.getForegroundColor(theme), "#689d6a"_hex);

			END_TEST;
		}

		// -------------------- Public functions --------------------
		void setupTestSuite()
		{
			Tests::TestSuite& testSuite = Tests::addTestSuite("SynatxThemeTests");

			ADD_BEFORE_ALL(testSuite, beforeAll);
			ADD_AFTER_ALL(testSuite, afterAll);

			// Tests
			ADD_TEST(testSuite, basicMatchingGetsAppropriateTheme_1);
			ADD_TEST(testSuite, basicMatchingGetsAppropriateTheme_2);
			ADD_TEST(testSuite, basicMatchingGetsAppropriateTheme_3);

			ADD_TEST(testSuite, inheritedThemesCanBeOverwritten);
			ADD_TEST(testSuite, descendantSelectorHasPrecedenceOverNormalSelector);
			ADD_TEST(testSuite, descendantSelectorFailsWhenAncestorsDontMatch);
			ADD_TEST(testSuite, descendantSelectorSucceedsWithPartialMatches);
			ADD_TEST(testSuite, descendantSelectorFailsWhenBottomNodeIsNotSpecific);
		}
	}
}

#endif