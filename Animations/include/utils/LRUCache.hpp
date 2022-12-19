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
		LRUCacheEntry* next;
		LRUCacheEntry* prev;
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
				LRUCacheEntry<Key, Value>* entry = iter->second;

				// If this is already the newest entry, no need to promote it
				if (entry == newestEntry)
				{
					return entry->data;
				}

				// Update the surrounding nodes in the doubly linked list
				if (entry->next)
				{
					entry->next->prev = entry->prev;
				}

				if (entry->prev)
				{
					entry->prev->next = entry->next;
				}

				// Set the new newest to have no older entry just in case there is none
				entry->prev = nullptr;
				entry->next = nullptr;

				// If there is a newest, update it's next pointer
				if (newestEntry)
				{
					newestEntry->next = entry;
					// Update the new newest older entry... confusing stuff
					entry->prev = newestEntry;
				}

				// Move this to the front of the list since this is the new "newest"
				newestEntry = entry;

				return entry->data;
			}

			return std::nullopt;
		}

		void insert(const Key& key, const Value& value)
		{
			LRUCacheEntry<Key, Value>* newEntry = (LRUCacheEntry<Key, Value>*)g_memory_allocate(sizeof(LRUCacheEntry<Key, Value>));
			*newEntry = {};
			newEntry->data = value;
			newEntry->key = key; 
			newEntry->next = nullptr;
			newEntry->prev = nullptr;

			// Update the newest entry if it exists
			if (newestEntry)
			{
				newestEntry->next = newEntry;
				newEntry->prev = newestEntry;
			}
			else
			{
				// Otherwise, this node is both the newest entry and the oldest entry
				g_logger_assert(indexLookup.size() == 0, "We should only be assigning this as the oldest when it's the first node in the LRU.");
				oldestEntry = newEntry;
			}

			newestEntry = newEntry;
			indexLookup[key] = newEntry;
		}

		bool evict(const Key& key)
		{
			if (exists(key))
			{
				auto entryToEvictIter = indexLookup.find(key);
				g_logger_assert(entryToEvictIter != indexLookup.end(), "How did this happen. Somehow key without value exists in our lookup.");
				LRUCacheEntry<Key, Value>* entryToEvict = entryToEvictIter->second;

				// Update the surrounding nodes next/prev pointers
				if (entryToEvict->prev)
				{
					entryToEvict->prev->next = entryToEvict->next;
				}

				if (entryToEvict->next)
				{
					entryToEvict->next->prev = entryToEvict->prev;
				}

				// Update oldest and/or newest entry if we're evicting them
				if (oldestEntry == entryToEvict)
				{
					// Update the oldest entry
					oldestEntry = oldestEntry->next;
				}

				if (newestEntry == entryToEvict)
				{
					newestEntry = newestEntry->prev;
				}

				// Free the memory now and get rid of this entry
				indexLookup.erase(entryToEvictIter);
				g_memory_free(entryToEvict);
				return true;
			}
			else
			{
				return false;
			}
		}

		inline LRUCacheEntry<Key, Value>* getOldest() { return oldestEntry; }
		inline LRUCacheEntry<Key, Value>* getNewest() { return newestEntry; }

		void clear()
		{
			// First free all the nodes
			for (auto k = indexLookup.begin(); k != indexLookup.end(); k++)
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