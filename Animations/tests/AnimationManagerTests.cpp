#ifdef _MATH_ANIM_TESTS
#include "AnimationManagerTests.h"
#include "core/Testing.h"

namespace MathAnim
{
	namespace AnimationManagerTests
	{
		// -------------------- Tests --------------------
		DEFINE_TEST(dummyOne)
		{
			END_TEST;
		}

		DEFINE_TEST(dummyTwo)
		{
			END_TEST;
		}

		void setupTestSuite()
		{
			TestSuite& testSuite = Tests::addTestSuite("AnimationManager");

			ADD_TEST(testSuite, dummyOne);
			ADD_TEST(testSuite, dummyTwo);
		}
	}
}

#endif