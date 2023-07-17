#ifdef _MATH_ANIM_TESTS
#include "LRUCacheTests.h"
#include "AnimationManagerTests.h"
#include "SyntaxHighlighterTests.h"

#include <cppUtils/cppTests.hpp>
#include <cppUtils/cppUtils.hpp>

using namespace CppUtils;

int main()
{
	using namespace MathAnim;

	g_logger_init();
	g_memory_init_padding(true, 5);

	LRUCacheTests::setupTestSuite();
	AnimationManagerTests::setupTestSuite();
	SyntaxHighlighterTests::setupTestSuite();

	Tests::runTests();
	Tests::free();

	return 0;
}

#endif