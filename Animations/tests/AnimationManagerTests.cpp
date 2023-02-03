#ifdef _MATH_ANIM_TESTS
#include "AnimationManagerTests.h"
#include "core/Testing.h"

namespace MathAnim
{
	namespace AnimationManagerTests
	{
		// -------------------- Tests --------------------
		bool dummyOne();
		bool dummyTwo();

		void setupTestSuite()
		{
			TestSuite& testSuite = Tests::addTestSuite("AnimationManager");

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
			return true;
		}
	}
}

#endif