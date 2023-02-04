#ifdef _MATH_ANIM_TESTS
#include "LRUCacheTests.h"
#include "core/Testing.h"
#include "utils/LRUCache.hpp"

namespace MathAnim
{
	struct DummyData
	{
		int i;
		float f;

		bool operator==(const DummyData& b)
		{
			return i == b.i && f == b.f;
		}

		bool operator!=(const DummyData& b)
		{
			return !(*this == b);
		}
	};

	namespace LRUCacheTests
	{
		// -------------------- Constants --------------------
		constexpr int KEY_ONE = 21;
		constexpr DummyData VALUE_ONE = {
			22,
			3.14f
		};

		// -------------------- Private functions --------------------
		static LRUCache<uint32, DummyData> createCache();
		static void printCache(const LRUCache<uint32, DummyData>& cache);

		// -------------------- Tests --------------------
		DEFINE_TEST(existsShouldReturnTrueForExistingItem)
		{
			auto cache = createCache();
			bool exists = cache.exists(KEY_ONE);

			ASSERT_TRUE(cache.exists(KEY_ONE));

			END_TEST;
		};

		DEFINE_TEST(getShouldReturnItemIfExists)
		{
			auto cache = createCache();
			std::optional<DummyData> value = cache.get(KEY_ONE);

			ASSERT_TRUE(value.has_value());

			END_TEST;
		};

		DEFINE_TEST(getShouldReturnNulloptIfItemNotExists)
		{
			auto cache = createCache();
			std::optional<DummyData> value = cache.get(UINT32_MAX - 1);

			ASSERT_FALSE(value.has_value());

			END_TEST;
		};

		DEFINE_TEST(getShouldPromoteItemToNewestEntry)
		{
			auto cache = createCache();

			LRUCacheEntry<uint32, DummyData>* oldNewestEntry = cache.getNewest();
			std::optional<DummyData> result = cache.get(KEY_ONE);

			ASSERT_TRUE(result.has_value());
			ASSERT_NOT_EQUAL(result.value(), oldNewestEntry->data);
			ASSERT_EQUAL(result.value(), cache.getNewest()->data);

			END_TEST;
		}

		DEFINE_TEST(getShouldReassignPointersProperly)
		{
			auto cache = createCache();

			size_t pointerTraversalSize = 0;
			{
				auto* oldest = cache.getOldest();
				while (oldest != nullptr)
				{
					oldest = oldest->next;
					pointerTraversalSize++;
				}
			}

			ASSERT_EQUAL(pointerTraversalSize, cache.size());

			// Promote entry to the new newest
			std::optional<DummyData> result = cache.get(KEY_ONE);
			ASSERT_TRUE(result.has_value());

			// Make sure pointer traversal size still matches
			size_t pointerTraversalSizeAfterGet = 0;
			{
				auto* oldest = cache.getOldest();
				while (oldest != nullptr)
				{
					oldest = oldest->next;
					pointerTraversalSizeAfterGet++;
				}
			}

			ASSERT_EQUAL(pointerTraversalSizeAfterGet, cache.size());
			ASSERT_EQUAL(pointerTraversalSizeAfterGet, pointerTraversalSize);

			END_TEST;
		}


		void setupTestSuite()
		{
			TestSuite& testSuite = Tests::addTestSuite("LRUCache");

			ADD_TEST(testSuite, existsShouldReturnTrueForExistingItem);

			ADD_TEST(testSuite, getShouldReturnItemIfExists);
			ADD_TEST(testSuite, getShouldReturnNulloptIfItemNotExists);
			ADD_TEST(testSuite, getShouldPromoteItemToNewestEntry);
			ADD_TEST(testSuite, getShouldReassignPointersProperly);
		}

		// -------------------- Tests --------------------
		static LRUCache<uint32, DummyData> createCache()
		{
			auto cache = LRUCache<uint32, DummyData>();
			cache.insert(KEY_ONE, VALUE_ONE);
			for (size_t i = 0; i < 10; i++)
			{
				cache.insert(i + 1, DummyData{ (int)i, (float)i * 3.14f });
			}

			return cache;
		}

		static void printCache(const LRUCache<uint32, DummyData>& cache)
		{
			std::string stringBuffer = "null <- ";
			{
				const auto* oldest = cache.getOldestConst();
				while (oldest != nullptr)
				{
					stringBuffer += std::to_string(oldest->key);
					if (oldest->next)
					{
						stringBuffer += " <-> ";
					}
					else
					{
						stringBuffer += " -> null";
					}
					oldest = oldest->next;
				}
			}

			g_logger_info("%s", stringBuffer.c_str());
		}
	}
}

#endif