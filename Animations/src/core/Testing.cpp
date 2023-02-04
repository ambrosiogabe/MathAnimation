#ifdef _MATH_ANIM_TESTS
#include "core/Testing.h"
#include "multithreading/GlobalThreadPool.h"

// See here for more escape code colors https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
#define ANSI_COLOR_RED     "\u001b[38;5;167m"
#define ANSI_COLOR_GREEN   "\u001b[38;5;84m"
#define ANSI_COLOR_YELLOW  "\u001b[38;5;220m"
#define ANSI_COLOR_BLUE    "\u001b[34m"
#define ANSI_COLOR_MAGENTA "\u001b[35m"
#define ANSI_COLOR_CYAN    "\u001b[36m"
#define ANSI_COLOR_RESET   "\u001b[0m"
 
namespace MathAnim
{
	// ----------------- Internal structures -----------------
	struct TestPrototype
	{
		uint8* name;
		size_t nameLength;
		TestFn fn;
	};

	struct TestSuite
	{
		const char* name;
		bool* testResults;
		TestPrototype* tests;
		size_t testsLength;
		size_t numTestsPassed;
	};

	namespace Tests
	{
		// ----------------- Internal variables -----------------
		static std::vector<TestSuite> testSuites = {};

		// ----------------- Internal functions -----------------
		static void runTestSuite(void* testSuiteRaw, size_t testSuiteSize);
		static void printTestSuiteResCallback(void* testSuiteRaw, size_t testSuiteSize);

		TestSuite& addTestSuite(const char* testSuiteName)
		{
			TestSuite res = {};
			res.name = testSuiteName;
			res.numTestsPassed = 0;
			res.tests = nullptr;
			res.testResults = nullptr;
			res.testsLength = 0;
			testSuites.push_back(res);

			return testSuites[testSuites.size() - 1];
		}

		void addTest(TestSuite& testSuite, const char* testName, TestFn fn)
		{
			testSuite.testsLength++;
			testSuite.tests = (TestPrototype*)g_memory_realloc(
				testSuite.tests,
				sizeof(TestPrototype) * testSuite.testsLength
			);
			testSuite.testResults = (bool*)g_memory_realloc(
				testSuite.testResults,
				sizeof(bool) * testSuite.testsLength
			);

			TestPrototype test = {};
			test.fn = fn;
			test.nameLength = std::strlen(testName);
			test.name = (uint8*)g_memory_allocate(sizeof(uint8) * (test.nameLength + 1));
			g_memory_copyMem(test.name, (void*)testName, sizeof(uint8) * test.nameLength);
			test.name[test.nameLength] = '\0';

			testSuite.tests[testSuite.testsLength - 1] = test;
			testSuite.testResults[testSuite.testsLength - 1] = false;
		}

		void runTests()
		{
			GlobalThreadPool threadPool = GlobalThreadPool(std::thread::hardware_concurrency());

			for (const auto& testSuite : testSuites)
			{
				threadPool.queueTask(
					runTestSuite,
					testSuite.name,
					(void*)&testSuite,
					sizeof(TestSuite),
					MathAnim::Priority::None,
					printTestSuiteResCallback
				);
			}

			threadPool.beginWork();

			// Should join all threads and wait for tasks to finish
			threadPool.free();

			size_t numTestSuitesPassed = 0;
			for (const auto& testSuite : testSuites)
			{
				if (testSuite.numTestsPassed >= testSuite.testsLength)
				{
					numTestSuitesPassed++;
				}
			}

			printf("\n  Number of Test Suites Passed "
				ANSI_COLOR_YELLOW
				"%llu/%llu\n\n"
				ANSI_COLOR_RESET,
				numTestSuitesPassed,
				testSuites.size()
			);
		}

		void free()
		{
			for (auto& testSuite : testSuites)
			{
				for (size_t i = 0; i < testSuite.testsLength; i++)
				{
					if (testSuite.tests[i].name)
					{
						g_memory_free(testSuite.tests[i].name);
					}

					testSuite.tests[i].name = nullptr;
					testSuite.tests[i].nameLength = 0;
					testSuite.tests[i].fn = nullptr;
				}

				if (testSuite.tests)
				{
					g_memory_free(testSuite.tests);
				}

				if (testSuite.testResults)
				{
					g_memory_free(testSuite.testResults);
				}

				testSuite.tests = nullptr;
				testSuite.testResults = nullptr;
				testSuite.numTestsPassed = 0;
				testSuite.name = nullptr;
				testSuite.testsLength = 0;
			}

			testSuites.clear();
		}

		// ----------------- Internal functions -----------------
		static void runTestSuite(void* testSuiteRaw, size_t testSuiteSize)
		{
			g_logger_assert(testSuiteSize == sizeof(TestSuite), "Invalid data passed to runTestSuite");
			TestSuite* testSuite = (TestSuite*)testSuiteRaw;

			for (size_t i = 0; i < testSuite->testsLength; i++)
			{
				testSuite->testResults[i] = testSuite->tests[i].fn();
				if (testSuite->testResults[i])
				{
					testSuite->numTestsPassed++;
				}
			}
		}

		static void printTestSuiteResCallback(void* testSuiteRaw, size_t testSuiteSize)
		{
			g_logger_assert(testSuiteSize == sizeof(TestSuite), "Invalid data passed to printTestSuiteResCallback");
			TestSuite* testSuite = (TestSuite*)testSuiteRaw;

			printf("  Test Suite '%s' Results...\n\n", testSuite->name);

			for (size_t i = 0; i < testSuite->testsLength; i++)
			{
				if (testSuite->testResults[i])
				{
					printf(
						ANSI_COLOR_GREEN
						"      + Success "
						ANSI_COLOR_RESET
						"'%s::%s'\n",
						testSuite->name,
						testSuite->tests[i].name
					);
				}
				else
				{
					printf(
						ANSI_COLOR_RED
						"      + Fail    "
						ANSI_COLOR_RESET
						"'%s::%s'\n",
						testSuite->name,
						testSuite->tests[i].name
					);
				}
			}

			if (testSuite->numTestsPassed < testSuite->testsLength)
			{
				printf(
					ANSI_COLOR_RED
					"\n    Suite Fail "
					ANSI_COLOR_RESET
					"'%s'",
					testSuite->name
				);
			}
			else
			{
				printf(
					ANSI_COLOR_GREEN
					"\n    Suite Success "
					ANSI_COLOR_RESET
					"'%s'",
					testSuite->name
				);
			}

			printf("\n    Number of Tests Passed "
				ANSI_COLOR_YELLOW
				"%llu/%llu\n\n"
				ANSI_COLOR_RESET,
				testSuite->numTestsPassed,
				testSuite->testsLength
			);
		}
	}
}

#endif