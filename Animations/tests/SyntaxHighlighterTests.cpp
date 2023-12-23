#ifdef _MATH_ANIM_TESTS
#include "SyntaxHighlighterTests.h"
#include "SyntaxHighlighterTestCases.h"

#include "parsers/SyntaxHighlighter.h"
#include "parsers/SyntaxTheme.h"
#include "parsers/Grammar.h"

using namespace CppUtils;

namespace MathAnim
{
	namespace SyntaxHighlighterTests
	{
		// -------------------- Constants --------------------
		static const char* luaGrammar = "./assets/customGrammars/lua.grammar.json";

		// -------------------- Private functions --------------------

		// -------------------- Init/Teardown --------------------
		DEFINE_BEFORE_ALL(beforeAll)
		{
			g_logger_level oldLevel = g_logger_get_level();
			g_logger_set_level(g_logger_level_None);
			Highlighters::init();
			Highlighters::importGrammar(luaGrammar);
			g_logger_set_level(oldLevel);
			END_BEFORE_ALL;
		}

		DEFINE_AFTER_ALL(afterAll)
		{
			Highlighters::free();
			END_AFTER_ALL;
		}

		// -------------------- Tests --------------------
		DEFINE_TEST(withCppLang_CppHelloWorldParsesCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Cpp);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(CPP_MAIN_TEST_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, CPP_MAIN_TEST_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withGlslLang_CppHelloWorldParsesCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Glsl);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(CPP_MAIN_TEST_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, CPP_MAIN_TEST_WITH_GLSL_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJsLang_JavaScriptNumberLiteralParsesCorrectly_NestedCaptureTest)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_NUMBER_LITERAL_TEST_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, JS_NUMBER_LITERAL_TEST_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withCpp_strayBracketParsesCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Cpp);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(CPP_STRAY_BRACKET_TEST_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, CPP_STRAY_BRACKET_TEST_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withCpp_singleLineCommentParsesCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Cpp);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(CPP_SINGLE_LINE_COMMENT_TEST_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, CPP_SINGLE_LINE_COMMENT_TEST_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJs_basicArrowFunctionParsesCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_BASIC_ARROW_FN_TEST_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, JS_BASIC_ARROW_FN_TEST_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJs_matchesWithAnchorsParseCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_ANCHORED_MATCHES_SRC, *theme);

			g_logger_info("stringified tree:\n{}", stringifiedParseTree);

			ASSERT_EQUAL(stringifiedParseTree, JS_ANCHORED_MATCHES_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJs_allowsBeginEndCaptureShorthandInGrammar)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_INTERPOLATED_STRING_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, JS_INTERPOLATED_STRING_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJs_allowsCaptureToExtendBeyondMatch)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_TEST_BEGIN_CAPTURE_EXTENDING_BEYOND_MATCH_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, JS_TEST_BEGIN_CAPTURE_EXTENDING_BEYOND_MATCH_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJs_forKindaSimpleLoopParsesCorrect)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_FOR_LOOP_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, JS_FOR_LOOP_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJs_capturesInCapturesWorkCorrect)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_CAPTURE_IN_CAPTURE_SRC, *theme);

			ASSERT_EQUAL(stringifiedParseTree, JS_CAPTURE_IN_CAPTURE_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withLua_endBlockDoesNotExceedWhenItsStoppedOnANewline)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getImportedHighlighter(luaGrammar);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(LUA_NEWLINE_END_BLOCK_THING, *theme);

			ASSERT_EQUAL(stringifiedParseTree, LUA_NEWLINE_END_BLOCK_THING_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withLua_backreferencesInEndBlocksWork)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getImportedHighlighter(luaGrammar);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(LUA_BACKREFERENCE_TEST, *theme);

			ASSERT_EQUAL(stringifiedParseTree, LUA_BACKREFERENCE_TEST_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withLua_backreferenceWith0SizedMatchWorks)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getImportedHighlighter(luaGrammar);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(LUA_BACKREFERENCE_0_SIZE, *theme);

			ASSERT_EQUAL(stringifiedParseTree, LUA_BACKREFERENCE_0_SIZE_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withLua_backreferenceWithNoEndMatchParsesUntilTheEnd)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getImportedHighlighter(luaGrammar);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(LUA_BACKREFERENCE_MISMATCH, *theme);

			ASSERT_EQUAL(stringifiedParseTree, LUA_BACKREFERENCE_MISMATCH_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJs_scopeCaptureWithExtraTextGetsSetCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			const SyntaxTheme* theme = Highlighters::getTheme(HighlighterTheme::OneDark);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_COMPLEX_SCOPE_CAPTURE, *theme);

			ASSERT_EQUAL(stringifiedParseTree, JS_COMPLEX_SCOPE_CAPTURE_EXPECTED);

			END_TEST;
		}

		void setupTestSuite()
		{
			Tests::TestSuite& testSuite = Tests::addTestSuite("SyntaxHighlighterTests");

			ADD_BEFORE_ALL(testSuite, beforeAll);
			ADD_AFTER_ALL(testSuite, afterAll);

			// -------------- Test get function --------------
			ADD_TEST(testSuite, withCppLang_CppHelloWorldParsesCorrectly);
			ADD_TEST(testSuite, withGlslLang_CppHelloWorldParsesCorrectly);
			ADD_TEST(testSuite, withJsLang_JavaScriptNumberLiteralParsesCorrectly_NestedCaptureTest);
			ADD_TEST(testSuite, withCpp_strayBracketParsesCorrectly);
			ADD_TEST(testSuite, withCpp_singleLineCommentParsesCorrectly);
			ADD_TEST(testSuite, withJs_basicArrowFunctionParsesCorrectly);

			//ADD_TEST(testSuite, withJs_matchesWithAnchorsParseCorrectly);
			//ADD_TEST(testSuite, withJs_allowsBeginEndCaptureShorthandInGrammar);
			//ADD_TEST(testSuite, withJs_allowsCaptureToExtendBeyondMatch);
			//ADD_TEST(testSuite, withJs_forKindaSimpleLoopParsesCorrect);
			//ADD_TEST(testSuite, withJs_capturesInCapturesWorkCorrect);
			//ADD_TEST(testSuite, withLua_endBlockDoesNotExceedWhenItsStoppedOnANewline);
			//ADD_TEST(testSuite, withLua_backreferencesInEndBlocksWork);
			//ADD_TEST(testSuite, withLua_backreferenceWith0SizedMatchWorks);
			//ADD_TEST(testSuite, withLua_backreferenceWithNoEndMatchParsesUntilTheEnd);
			//ADD_TEST(testSuite, withJs_scopeCaptureWithExtraTextGetsSetCorrectly);
		}

		// -------------------- Private functions --------------------
	}
}
#endif