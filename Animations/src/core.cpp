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

	g_logger_error("Invalid hex character '%c'", hexCode);
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

MathAnim::Vec4 toHex(const char* rawHexColor, size_t inputLength)
{
	g_logger_assert(rawHexColor != nullptr, "Invalid hex color. Cannot be null.");

	const char* hexColor = rawHexColor[0] == '#' ?
		rawHexColor + 1 :
		rawHexColor;
	size_t length = strlen(hexColor);
	g_logger_assert(length >= 6, "Invalid hex color '%s', hex color must have at least 6 digits.");
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
	data = (uint8*)g_memory_realloc(data, offset);
	size = offset;
}

void RawMemory::resetReadWriteCursor()
{
	offset = 0;
}

void RawMemory::setCursor(size_t offset)
{
	this->offset = offset;
}

void RawMemory::writeDangerous(const uint8* data, size_t dataSize)
{
	if (this->offset + dataSize >= this->size)
	{
		// Reallocate
		size_t newSize = (this->offset + dataSize) * 2;
		uint8* newData = (uint8*)g_memory_realloc(this->data, newSize);
		g_logger_assert(newData != nullptr, "Failed to reallocate more memory.");
		this->data = newData;
		this->size = newSize;
	}

	g_memory_copyMem(this->data + this->offset, (uint8*)(data), dataSize);
	this->offset += dataSize;
}

bool RawMemory::readDangerous(uint8* data, size_t dataSize)
{
	if (this->offset + dataSize > this->size)
	{
		g_logger_error("Deserialized bad data. Read boundary out of bounds, cannot access '%zu' bytes in memory of size '%zu' bytes",
			this->offset + dataSize,
			this->size);
		return false;
	}

	g_memory_copyMem((uint8*)data, this->data + this->offset, dataSize);
	this->offset += dataSize;
	return true;
}