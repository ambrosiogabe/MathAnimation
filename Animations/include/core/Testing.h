#ifdef _MATH_ANIM_TESTS
#ifndef MATH_ANIM_TESTING_H
#define MATH_ANIM_TESTING_H
#include "core.h"

#define ADD_TEST(testSuite, testName) Tests::addTest(testSuite, #testName, testName)

#define ASSERT_TRUE(val) { if (!(val)) return "ASSERT_TRUE("#val")"; }
#define ASSERT_FALSE(val) { if (val) return "ASSERT_FALSE("#val")"; }

#define ASSERT_EQUAL(a, b) { if (a != b) return "ASSERT_EQUAL("#a", "#b")"; }
#define ASSERT_NOT_EQUAL(a, b) { if (a == b) return "ASSERT_NOT_EQUAL("#a", "#b")"; }

#define DEFINE_TEST(fnName) const char* fnName()
#define END_TEST return nullptr

namespace MathAnim
{
	struct TestSuite;

	typedef const char* (*TestFn)();

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