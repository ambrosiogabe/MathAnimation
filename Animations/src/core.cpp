#include "core.h"

constexpr char u4ToHex(uint8 val)
{
	switch (val)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		return (char)(val + '0');
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		return (char)((val - 10) + 'A');
	}

	return '¿';
}

std::string u8ToHex(uint8 val)
{
	uint8 low = val & 0xF;
	uint8 high = (val >> 4) & 0xF;
	return std::string() + u4ToHex(high) + u4ToHex(low);
}

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
	g_logger_assert(length >= 3, "Invalid hex color '{}', hex color must have at least 6 digits.", rawHexColor);

	// Shorthand like #fc0 -> #ffcc00
	if (length == 3)
	{
		float color1 = (hexToFloat(hexColor[0]) * 16 + hexToFloat(hexColor[0])) / 255.0f;
		float color2 = (hexToFloat(hexColor[1]) * 16 + hexToFloat(hexColor[1])) / 255.0f;
		float color3 = (hexToFloat(hexColor[2]) * 16 + hexToFloat(hexColor[2])) / 255.0f;
		return MathAnim::Vec4{
			color1, color2, color3, 1.0f
		};
	}

	// Shorthand like #fc2e -> #ffcc22ee
	if (length == 4)
	{
		float color1 = (hexToFloat(hexColor[0]) * 16 + hexToFloat(hexColor[0])) / 255.0f;
		float color2 = (hexToFloat(hexColor[1]) * 16 + hexToFloat(hexColor[1])) / 255.0f;
		float color3 = (hexToFloat(hexColor[2]) * 16 + hexToFloat(hexColor[2])) / 255.0f;
		float color4 = (hexToFloat(hexColor[3]) * 16 + hexToFloat(hexColor[3])) / 255.0f;
		return MathAnim::Vec4{
			color1, color2, color3, color4
		};
	}

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

std::string toHexString(const MathAnim::Vec4& color)
{
	std::string hexString = "#";
	uint8 r = (uint8)(color.r * 255.0f);
	uint8 g = (uint8)(color.g * 255.0f);
	uint8 b = (uint8)(color.b * 255.0f);
	uint8 a = (uint8)(color.a * 255.0f);

	hexString += u8ToHex(r);
	hexString += u8ToHex(g);
	hexString += u8ToHex(b);
	hexString += u8ToHex(a);

	return hexString;
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

	g_memory_copyMem(this->data + this->offset, this->size, (uint8*)(inData), inDataSize);
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

	g_memory_copyMem((uint8*)inData, this->size, this->data + this->offset, inDataSize);
	this->offset += inDataSize;
	return true;
}