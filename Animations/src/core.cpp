#include "core.h"

constexpr int hexToInt(char hexCode)
{
	switch (hexCode)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return hexCode - '0';
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		return hexCode - 'A' + 10;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
		return hexCode - 'a' + 10;
	}

	g_logger_error("Invalid hex character '{}'", hexCode);
	return -1;
}

constexpr float hexToFloat(char hexCode)
{
	return (float)hexToInt(hexCode);
}

MathAnim::Vec4 operator""_hex(const char* rawHexColor, size_t inputLength)
{
	return toHex(rawHexColor, inputLength);
}

MathAnim::Vec4 toHex(const std::string& str)
{
	return toHex(str.c_str(), str.length());
}

MathAnim::Vec4 toHex(const char* rawHexColor, size_t)
{
	g_logger_assert(rawHexColor != nullptr, "Invalid hex color. Cannot be null.");

	const char* hexColor = rawHexColor[0] == '#' ?
		rawHexColor + 1 :
		rawHexColor;
	size_t length = strlen(hexColor);
	g_logger_assert(length >= 6, "Invalid hex color '{}', hex color must have at least 6 digits.", rawHexColor);
	float color1 = (hexToFloat(hexColor[0]) * 16 + hexToFloat(hexColor[1])) / 255.0f;
	float color2 = (hexToFloat(hexColor[2]) * 16 + hexToFloat(hexColor[3])) / 255.0f;
	float color3 = (hexToFloat(hexColor[4]) * 16 + hexToFloat(hexColor[5])) / 255.0f;

	if (length == 8)
	{
		float color4 = (hexToFloat(hexColor[6]) * 16 + hexToFloat(hexColor[7])) / 255.0f;
		return MathAnim::Vec4{
			color1, color2, color3, color4
		};
	}

	// Default alpha to 1
	return MathAnim::Vec4{
		color1, color2, color3, 1.0f
	};
}

MathAnim::Vec4 toHex(const char* hex)
{
	return toHex(hex, std::strlen(hex));
}

void RawMemory::init(size_t initialSize)
{
	size = initialSize;
	data = (uint8*)g_memory_allocate(initialSize);
	offset = 0;
}

void RawMemory::free()
{
	if (data)
	{
		g_memory_free(data);
		size = 0;
		data = nullptr;
		offset = 0;
	}
}

void RawMemory::shrinkToFit()
{
	if (size != offset)
	{
		data = (uint8*)g_memory_realloc(data, offset);
		size = offset;
	}
}

void RawMemory::resetReadWriteCursor()
{
	offset = 0;
}

void RawMemory::setCursor(size_t inOffset)
{
	this->offset = inOffset;
}

void RawMemory::writeDangerous(const uint8* inData, size_t inDataSize)
{
	if (this->offset + inDataSize > this->size)
	{
		// Reallocate
		size_t newSize = (this->offset + inDataSize) * 2;
		uint8* newData = (uint8*)g_memory_realloc(this->data, newSize);
		g_logger_assert(newData != nullptr, "Failed to reallocate more memory.");
		this->data = newData;
		this->size = newSize;
	}

	g_memory_copyMem(this->data + this->offset, (uint8*)(inData), inDataSize);
	this->offset += inDataSize;
}

bool RawMemory::readDangerous(uint8* inData, size_t inDataSize)
{
	if (this->offset + inDataSize > this->size)
	{
		g_logger_error("Deserialized bad data. Read boundary out of bounds, cannot access '{}' bytes in memory of size '{}' bytes",
			this->offset + inDataSize,
			this->size);
		return false;
	}

	g_memory_copyMem((uint8*)inData, this->data + this->offset, inDataSize);
	this->offset += inDataSize;
	return true;
}