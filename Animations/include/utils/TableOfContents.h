#ifndef MATH_ANIM_TABLE_OF_CONTENTS
#define MATH_ANIM_TABLE_OF_CONTENTS
#include "core.h"

namespace MathAnim
{
	struct TableOfContents
	{
		uint32 numEntries;

		RawMemory tocEntries;
		RawMemory data;

		void init();
		void free();

		void addEntry(const RawMemory& memory, const char* entryName, size_t entryNameLength = 0);
		RawMemory getEntry(const char* entryName, size_t entryNameLength = 0);

		void serialize(const char* filepath);
		static TableOfContents deserialize(RawMemory& memory);
	};
}

#endif 