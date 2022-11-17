#ifndef MATH_ANIM_LRU_CACHE_H
#define MATH_ANIM_LRU_CACHE_H
#include "core.h"

namespace MathAnim
{
	template<typename Key, typename Value>
	struct LRUCacheEntry
	{
		Key key;
		Value data;
		LRUCacheEntry* newerEntry;
		LRUCacheEntry* olderEntry;
	};

	template<typename Key, typename Value>
	class LRUCache
	{
	public:
		bool exists(const Key& key)
		{
			return indexLookup.find(key) != indexLookup.end();
		}

		std::optional<Value> get(const Key& key)
		{
			if (exists(key))
			{
				auto iter = indexLookup.find(key);

				// Update the surrounding nodes in the doubly linked list
				if (iter->second->newerEntry != nullptr)
				{
					iter->second->newerEntry->olderEntry = iter->second->olderEntry;
				}

				if (iter->second->olderEntry != nullptr)
				{
					iter->second->olderEntry->newerEntry = iter->second->newerEntry;
				}

				// Set the new newest to have no older entry just in case there is none
				iter->second->olderEntry = nullptr;

				// If there is a newest, update it's next pointer
				if (newestEntry)
				{
					newestEntry->newerEntry = iter->second;
					// Update the new newest older entry... confusing stuff
					iter->second->olderEntry = newestEntry;
				}

				// Move this to the front of the list since this is the new "newest"
				newestEntry = iter->second;

				return iter->second->data;
			}

			return std::nullopt;
		}

		void insert(const Key& key, const Value& value)
		{
			LRUCacheEntry<Key, Value>* newEntry = (LRUCacheEntry<Key, Value>*)g_memory_allocate(sizeof(LRUCacheEntry<Key, Value>));
			newEntry->data = value;
			newEntry->newerEntry = nullptr;
			newEntry->olderEntry = nullptr;

			// Update the newest entry if it exists
			if (newestEntry)
			{
				newestEntry->newerEntry = newEntry;
				newEntry->olderEntry = newestEntry;
			}
			else
			{
				// Otherwise, this node is both the newest entry and the oldest entry
				oldestEntry = newEntry;
			}

			newestEntry = newEntry;
			indexLookup[key] = newEntry;
		}

		void evict(const Key& key)
		{
			if (exists(key))
			{
				auto entryToEvictIter = indexLookup.find(key);
				g_logger_assert(entryToEvictIter != indexLookup.end(), "How did this happen. Somehow key without value exists in our lookup.");
				LRUCacheEntry<Key, Value>* entryToEvict = entryToEvictIter->second;

				// Update the surrounding nodes next/prev pointers
				if (entryToEvict->olderEntry)
				{
					entryToEvict->olderEntry->newerEntry = entryToEvict->newerEntry;
				}

				if (entryToEvict->newerEntry)
				{
					entryToEvict->newerEntry->olderEntry = entryToEvict->olderEntry;
				}

				// Update oldest and/or newest entry if we're evicting them
				if (oldestEntry == entryToEvict)
				{
					// Update the oldest entry
					oldestEntry = oldestEntry->newerEntry;
				}

				if (newestEntry == entryToEvict)
				{
					newestEntry = newestEntry->olderEntry;
				}

				// Free the memory now and get rid of this entry
				indexLookup.erase(entryToEvictIter);
				g_memory_free(entryToEvict);
			}
		}

		inline LRUCacheEntry<Key, Value>* getOldest() { return oldestEntry; }
		inline LRUCacheEntry<Key, Value>* getNewest() { return newestEntry; }

		void clear()
		{
			// First free all the nodes
			for (auto& k = indexLookup.begin(); k != indexLookup.end(); k++)
			{
				g_memory_free(k->second);
			}

			// Then clear the index lookup and stuff
			indexLookup.clear();
			oldestEntry = nullptr;
			newestEntry = nullptr;
		}

	private:
		std::unordered_map<Key, LRUCacheEntry<Key, Value>*> indexLookup;
		LRUCacheEntry<Key, Value>* oldestEntry;
		LRUCacheEntry<Key, Value>* newestEntry;
	};
}

#endif 