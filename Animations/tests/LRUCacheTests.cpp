#ifdef _MATH_ANIM_TESTS
#include "LRUCacheTests.h"
#include "core/Testing.h"

namespace MathAnim
{
	namespace LRUCacheTests
	{
		// -------------------- Tests --------------------
		bool dummyOne();
		bool dummyTwo();

		void setupTestSuite()
		{
			TestSuite& testSuite = Tests::addTestSuite("LRUCache");

			ADD_TEST(testSuite, dummyOne);
			ADD_TEST(testSuite, dummyTwo);
		}

		// -------------------- Tests --------------------
		bool dummyOne()
		{
			return true;
		}

		bool dummyTwo()
		{
			return false;
		}
	}
}

#endif