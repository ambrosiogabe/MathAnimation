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

		// -------------------- Tests --------------------
		bool existsShouldReturnTrueForExistingItem();

		bool getShouldReturnItemIfExists();
		bool getShouldReturnNulloptIfItemNotExists();
		bool getShouldPromoteItemToNewestEntry();
		bool getShouldReassignPointersProperly();

		static LRUCache<uint32, DummyData> createCache();
		static void printCache(const LRUCache<uint32, DummyData>& cache);

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
		bool existsShouldReturnTrueForExistingItem()
		{
			auto cache = createCache();
			bool exists = cache.exists(KEY_ONE);

			ASSERT_TRUE(cache.exists(KEY_ONE));

			return true;
		}

		bool getShouldReturnItemIfExists()
		{
			auto cache = createCache();
			std::optional<DummyData> value = cache.get(KEY_ONE);

			ASSERT_TRUE(value.has_value());

			return true;
		}

		bool getShouldReturnNulloptIfItemNotExists()
		{
			auto cache = createCache();
			std::optional<DummyData> value = cache.get(UINT32_MAX - 1);

			ASSERT_FALSE(value.has_value());

			return true;
		}

		bool getShouldPromoteItemToNewestEntry()
		{
			auto cache = createCache();

			LRUCacheEntry<uint32, DummyData>* oldNewestEntry = cache.getNewest();
			std::optional<DummyData> result = cache.get(KEY_ONE);

			ASSERT_TRUE(result.has_value());
			ASSERT_TRUE(
				result.value() != oldNewestEntry->data && 
				result.value() == cache.getNewest()->data
			);

			return true;
		}

		bool getShouldReassignPointersProperly()
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

			ASSERT_TRUE(pointerTraversalSize == cache.size());

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

			ASSERT_TRUE(pointerTraversalSizeAfterGet == cache.size());
			ASSERT_TRUE(pointerTraversalSizeAfterGet == pointerTraversalSize);

			return true;
		}

		static LRUCache<uint32, DummyData> createCache()
		{
			auto cache = LRUCache<uint32, DummyData>();
			cache.insert(KEY_ONE, VALUE_ONE);
			for (size_t i = 0; i < 10; i++)
			{
				cache.insert(i + 1, DummyData{(int)i, (float)i * 3.14f});
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