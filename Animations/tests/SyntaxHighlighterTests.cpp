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

		// -------------------- Private functions --------------------

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
		DEFINE_TEST(withCppLang_CppHelloWorldParsesCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Cpp);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(CPP_MAIN_TEST_SRC);

			ASSERT_EQUAL(stringifiedParseTree, CPP_MAIN_TEST_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withGlslLang_CppHelloWorldParsesCorrectly)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Glsl);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(CPP_MAIN_TEST_SRC);

			ASSERT_EQUAL(stringifiedParseTree, CPP_MAIN_TEST_WITH_GLSL_EXPECTED);

			END_TEST;
		}

		DEFINE_TEST(withJsLang_JavaScriptNumberLiteralParsesCorrectly_NestedCaptureTest)
		{
			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(HighlighterLanguage::Javascript);
			std::string stringifiedParseTree = highlighter->getStringifiedParseTreeFor(JS_NUMBER_LITERAL_TEST_SRC);

			ASSERT_EQUAL(stringifiedParseTree, JS_NUMBER_LITERAL_TEST_EXPECTED);

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
		}

		// -------------------- Private functions --------------------
	}
}
#endif