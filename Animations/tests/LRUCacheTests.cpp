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

		bool operator==(const DummyData& b) const
		{
			return i == b.i && f == b.f;
		}

		bool operator!=(const DummyData& b) const
		{
			return !(*this == b);
		}
	};
}

namespace std
{

	template <>
	struct hash<MathAnim::DummyData>
	{
		std::size_t operator()(const MathAnim::DummyData& k) const
		{
			using std::size_t;
			using std::hash;
			using std::string;

			// Compute individual hash values for first,
			// second and third and combine them using XOR
			// and bit shifting:

			return ((hash<int>()(k.i)
				^ (hash<float>()(k.f) << 1)) >> 1);
		}
	};

}

namespace MathAnim
{
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

			ASSERT_TRUE(cache.exists(KEY_ONE));

			END_TEST;
		};

		// -------------- Test get function --------------
		DEFINE_TEST(getOnEmptyCacheShouldReturnNullopt)
		{
			LRUCache<uint32, DummyData> cache = {};

			std::optional<DummyData> res = cache.get(5);

			ASSERT_FALSE(res.has_value());

			END_TEST;
		}

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

		DEFINE_TEST(getShouldNotModifyAnyData)
		{
			auto cache = createCache();

			std::unordered_map<DummyData, uint32> dataBeforeGet = {};
			{
				auto* oldest = cache.getOldest();
				while (oldest != nullptr)
				{
					if (dataBeforeGet.find(oldest->data) != dataBeforeGet.end())
					{
						dataBeforeGet[oldest->data] = dataBeforeGet[oldest->data] + 1;
					}
					else
					{
						dataBeforeGet[oldest->data] = 1;
					}

					oldest = oldest->next;
				}
			}

			// Promote entry to the new newest
			std::optional<DummyData> result = cache.get(KEY_ONE);
			ASSERT_TRUE(result.has_value());

			// Make sure data didn't change for some reason during get
			std::unordered_map<DummyData, uint32> dataAfterGet = {};
			{
				auto* oldest = cache.getOldest();
				while (oldest != nullptr)
				{
					if (dataAfterGet.find(oldest->data) != dataAfterGet.end())
					{
						dataAfterGet[oldest->data] = dataAfterGet[oldest->data] + 1;
					}
					else
					{
						dataAfterGet[oldest->data] = 1;
					}

					oldest = oldest->next;
				}
			}

			// Make sure same counts of data before and after
			ASSERT_EQUAL(dataBeforeGet.size(), dataAfterGet.size());

			// Make sure same number of entries for each piece of data before and after
			for (auto& [k, beforeCount] : dataBeforeGet)
			{
				ASSERT_TRUE(dataAfterGet.find(k) != dataAfterGet.end());

				uint32 afterCount = dataAfterGet[k];
				ASSERT_EQUAL(afterCount, beforeCount);
				ASSERT_EQUAL(dataAfterGet.find(k)->first, k);
			}

			END_TEST;
		}

		// -------------- Test insert function --------------
		DEFINE_TEST(insertShouldInsertItem)
		{
			auto cache = createCache();

			cache.insert(777, { 25, 0.2f });
			std::optional<DummyData> res = cache.get(777);

			ASSERT_TRUE(res.has_value());

			DummyData shouldBe = { 25, 0.2f };
			ASSERT_EQUAL(res.value(), shouldBe);

			END_TEST;
		}

		DEFINE_TEST(insertShouldSetNewestEntryToNewItem)
		{
			auto cache = createCache();

			cache.insert(777, { 25, 0.2f });
			std::optional<DummyData> res = cache.get(777);

			ASSERT_TRUE(res.has_value());
			ASSERT_EQUAL(res.value(), cache.getNewest()->data);

			END_TEST;
		}

		DEFINE_TEST(insertOnEmptyListShouldSetOldestAndNewestEntry)
		{
			auto cache = LRUCache<uint32, DummyData>();

			cache.insert(777, { 25, 0.2f });
			std::optional<DummyData> res = cache.get(777);

			ASSERT_TRUE(res.has_value());
			ASSERT_EQUAL(res.value(), cache.getNewest()->data);
			ASSERT_EQUAL(res.value(), cache.getOldest()->data);

			END_TEST;
		}

		DEFINE_TEST(insertShouldUpdateNewestPointers)
		{
			auto cache = createCache();

			LRUCacheEntry<uint32, DummyData>* oldNewestEntry = cache.getNewest();

			cache.insert(777, { 25, 0.2f });
			std::optional<DummyData> res = cache.get(777);

			ASSERT_TRUE(res.has_value());

			ASSERT_NOT_NULL(oldNewestEntry->next);
			ASSERT_EQUAL(oldNewestEntry->next->data, res.value());

			END_TEST;
		}

		// -------------- Test evict function --------------
		DEFINE_TEST(evictShouldEvictAnExistingItem)
		{
			auto cache = createCache();

			bool res = cache.evict(KEY_ONE);
			ASSERT_TRUE(res);
			ASSERT_FALSE(cache.exists(KEY_ONE));

			END_TEST;
		}

		DEFINE_TEST(evictShouldReturnFalseWhenEvictingNonExistentItem)
		{
			auto cache = createCache();

			uint32 nonExistentKey = 81'234;
			bool res = cache.evict(nonExistentKey);
			ASSERT_FALSE(res);
			ASSERT_FALSE(cache.exists(nonExistentKey));

			END_TEST;
		}

		DEFINE_TEST(evictOldestEntryShouldSetOldestToNextEntry)
		{
			auto cache = createCache();

			uint32 oldestKey = cache.getOldest()->key;
			uint32 nextKey = cache.getOldest()->next->key;
			cache.evict(oldestKey);

			ASSERT_FALSE(cache.exists(oldestKey));
			ASSERT_EQUAL(cache.getOldest()->key, nextKey);
			ASSERT_NULL(cache.getOldest()->prev);

			END_TEST;
		}

		DEFINE_TEST(evictNewestEntryShouldSetNewestToPrevEntry)
		{
			auto cache = createCache();

			uint32 newestKey = cache.getNewest()->key;
			uint32 prevKey = cache.getNewest()->prev->key;
			cache.evict(newestKey);

			ASSERT_FALSE(cache.exists(newestKey));
			ASSERT_EQUAL(cache.getNewest()->key, prevKey);
			ASSERT_NULL(cache.getNewest()->next);

			END_TEST;
		}

		DEFINE_TEST(evictOnEmptyCacheShouldReturnFalse)
		{
			LRUCache<uint32, DummyData> cache = {};

			bool res = cache.evict(0);

			ASSERT_FALSE(res);

			END_TEST;
		}

		// -------------- Test clear function --------------
		DEFINE_TEST(clearShouldClearAllEntries)
		{
			auto cache = createCache();

			ASSERT_TRUE(cache.size() > 0);
			ASSERT_NOT_NULL(cache.getNewest());
			ASSERT_NOT_NULL(cache.getOldest());
			cache.clear();
			ASSERT_TRUE(cache.size() == 0);
			ASSERT_NULL(cache.getNewest());
			ASSERT_NULL(cache.getOldest());

			END_TEST;
		}

		void setupTestSuite()
		{
			TestSuite& testSuite = Tests::addTestSuite("LRUCache");

			ADD_TEST(testSuite, existsShouldReturnTrueForExistingItem);

			// -------------- Test get function --------------
			ADD_TEST(testSuite, getOnEmptyCacheShouldReturnNullopt);
			ADD_TEST(testSuite, getShouldReturnItemIfExists);
			ADD_TEST(testSuite, getShouldReturnNulloptIfItemNotExists);
			ADD_TEST(testSuite, getShouldPromoteItemToNewestEntry);
			ADD_TEST(testSuite, getShouldReassignPointersProperly);
			ADD_TEST(testSuite, getShouldNotModifyAnyData);

			// -------------- Test insert function --------------
			ADD_TEST(testSuite, insertShouldInsertItem);
			ADD_TEST(testSuite, insertShouldSetNewestEntryToNewItem);
			ADD_TEST(testSuite, insertOnEmptyListShouldSetOldestAndNewestEntry);
			ADD_TEST(testSuite, insertShouldUpdateNewestPointers);

			// -------------- Test evict function --------------
			ADD_TEST(testSuite, evictShouldEvictAnExistingItem);
			ADD_TEST(testSuite, evictShouldReturnFalseWhenEvictingNonExistentItem);
			ADD_TEST(testSuite, evictOldestEntryShouldSetOldestToNextEntry);
			ADD_TEST(testSuite, evictNewestEntryShouldSetNewestToPrevEntry);
			ADD_TEST(testSuite, evictOnEmptyCacheShouldReturnFalse);

			// -------------- Test clear function --------------
			ADD_TEST(testSuite, clearShouldClearAllEntries);
		}

		// -------------------- Private functions --------------------
		static LRUCache<uint32, DummyData> createCache()
		{
			auto cache = LRUCache<uint32, DummyData>();
			cache.insert(KEY_ONE, VALUE_ONE);
			for (size_t i = 0; i < 10; i++)
			{
				cache.insert((uint32)i + 1, DummyData{ (int)i, (float)i * 3.14f });
			}

			return cache;
		}

#pragma warning( push )
#pragma warning( disable : 4505 )
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

			g_logger_info("{}", stringBuffer);
		}
	}
}

#pragma warning( pop )
#endif