#include "utils/TableOfContents.h"

namespace MathAnim
{
	void TableOfContents::init() 
	{
		// Initialize with some capacity
		tocEntries.init(8);
		data.init(8);
		numEntries = 0;
	}

	void TableOfContents::free() 
	{
		tocEntries.free();
		data.free();
		numEntries = 0;
	}

	void TableOfContents::addEntry(const RawMemory& memory, const char* entryName, size_t entryNameLength)
	{
		// Each TOC entry looks like
		// entryNameLength    -> uint32
		// entryName          -> uint8[entryNameLength + 1]
		// dataOffset         -> size_t
		// dataSize           -> size_t
		if (entryNameLength == 0)
		{
			entryNameLength = std::strlen(entryName);
		}

		// Write out the entry into the TOC
		g_logger_assert(entryNameLength < UINT32_MAX, "Invalid entry name. Has length > UINT32_MAX. Entry name: '%s'", entryName);
		uint32 entryNameLengthU32 = (uint32)entryNameLength; 
		tocEntries.write<uint32>(&entryNameLengthU32);
		tocEntries.writeDangerous((const uint8*)entryName, entryNameLength);
		uint8 nullByte = '\0';
		tocEntries.write<uint8>(&nullByte);

		tocEntries.write<size_t>(&data.offset);
		tocEntries.write<size_t>(&memory.size);

		// Then write the actual data into the memory blob
		data.writeDangerous(memory.data, memory.size);
		numEntries++;
	}

	RawMemory TableOfContents::getEntry(const char* entryName, size_t entryNameLength)
	{
		// Each TOC entry looks like
		// entryNameLength    -> uint32
		// entryName          -> uint8[entryNameLength + 1]
		// dataOffset         -> size_t
		// dataSize           -> size_t
		if (entryNameLength == 0)
		{
			entryNameLength = std::strlen(entryName);
		}

		RawMemory res = {};

		tocEntries.setCursor(0);
		for (uint32 entryIndex = 0; entryIndex < numEntries; entryIndex++)
		{
			uint32 currentEntryNameLength;
			tocEntries.read<uint32>(&currentEntryNameLength);
			if (entryNameLength >= UINT32_MAX) 
			{
				g_logger_error("Corrupted TableOfContents. Invalid entry name. Has length > UINT32_MAX. Entry name: '%s'", entryName);
				return res;
			}

			if (currentEntryNameLength != entryNameLength)
			{
				// Skip this entry since they're definitely not the same
				tocEntries.setCursor(tocEntries.offset + (currentEntryNameLength + 1));
				tocEntries.setCursor(tocEntries.offset + (sizeof(size_t) * 2));
			}
			else
			{
				// Compare the strings
				int strCompare = g_memory_compareMem(&tocEntries.data[tocEntries.offset], (void*)entryName, sizeof(uint8) * currentEntryNameLength);
				if (strCompare == 0)
				{
					// Return this entry data
					tocEntries.setCursor(tocEntries.offset + (currentEntryNameLength + 1));
					size_t dataOffset, dataSize;
					tocEntries.read<size_t>(&dataOffset);
					tocEntries.read<size_t>(&dataSize);
					if (dataOffset + dataSize > data.size) 
					{
						g_logger_error("Corrupted TableOfContents. Data Entry '%s' exceeds available size of data with [offset, size]: [%d, %d]", entryName, dataOffset, dataSize);
						return res;
					}

					res.init(dataSize);
					data.setCursor(dataOffset);
					data.readDangerous(res.data, dataSize);
					return res;
				}
			}
		}

		g_logger_warning("No entry found in TableOfContents with name '%s'.", entryName);
		return res;
	}

	void TableOfContents::serialize(const char* filepath)
	{
		tocEntries.shrinkToFit();
		data.shrinkToFit();

		FILE* fp = fopen(filepath, "wb");

		// Write metadata
		// tocSize     -> size_t
		// dataSize    -> size_t
		// numEntries  -> uint32
		// magicNumber -> uint32
		uint32 magicNumber = 0xC001CAFE;
		SizedMemory tocMetadata = pack<
			size_t, 
			size_t,
			uint32, 
			uint32
		>(
			tocEntries.size,
			data.size,
			numEntries, 
			magicNumber
		);
		fwrite(tocMetadata.memory, tocMetadata.size, 1, fp);
		g_memory_free(tocMetadata.memory);

		// Write TOC offsets and actual data
		fwrite(tocEntries.data, tocEntries.size, 1, fp);
		fwrite(data.data, data.size, 1, fp);
		fclose(fp);
	}

	TableOfContents TableOfContents::deserialize(RawMemory& memory)
	{
		// Read metadata
		// tocSize     -> size_t
		// dataSize    -> size_t
		// numEntries  -> uint32
		// magicNumber -> uint32

		TableOfContents res;
		res.init();

		size_t tocSize;
		memory.read<size_t>(&tocSize);
		size_t dataSize;
		memory.read<size_t>(&dataSize);
		memory.read<uint32>(&res.numEntries);
		uint32 magicNumber = 0;
		memory.read<uint32>(&magicNumber);

		if (magicNumber != 0xC001CAFE)
		{
			g_logger_error("Corrupted project file. Expected magic number '0xC001CAFE' but got '0x%8x'.", magicNumber);

			res.numEntries = 0;
			return res;
		}

		res.tocEntries.free();
		res.data.free();

		res.tocEntries.init(tocSize);
		memory.readDangerous(res.tocEntries.data, tocSize);

		res.data.init(dataSize);
		memory.readDangerous(res.data.data, dataSize);

		return res;
	}
}