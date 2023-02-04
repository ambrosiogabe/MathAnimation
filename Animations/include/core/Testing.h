#ifdef _MATH_ANIM_TESTS
#ifndef MATH_ANIM_TESTING_H
#define MATH_ANIM_TESTING_H
#include "core.h"

#define ADD_TEST(testSuite, testName) Tests::addTest(testSuite, #testName, testName)

#define ASSERT_TRUE(val) { if (!(val)) return false; }
#define ASSERT_FALSE(val) { if (val) return false; }

namespace MathAnim
{
	struct TestSuite;

	typedef bool (*TestFn)();

	namespace Tests
	{
		TestSuite& addTestSuite(const char* testSuiteName);

		void addTest(TestSuite& testSuite, const char* testName, TestFn fn);

		void runTests();

		void free();
	}
}

#endif // MATH_ANIM_TESTING_H
#endif // _MATH_ANIM_TESTS