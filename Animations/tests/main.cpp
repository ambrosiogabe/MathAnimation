#ifdef _MATH_ANIM_TESTS
#include "core/Testing.h"
#include "LRUCacheTests.h"
#include "AnimationManagerTests.h"

int main()
{
	using namespace MathAnim;

	g_logger_init();
	g_memory_init_padding(true, 5);

	LRUCacheTests::setupTestSuite();
	AnimationManagerTests::setupTestSuite();

	Tests::runTests();
	Tests::free();

	return 0;
}

#endif