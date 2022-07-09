#ifndef MATH_ANIM_CORE_H
#define MATH_ANIM_CORE_H

// Glm
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/euler_angles.hpp>

// My stuff
#include <cppUtils/cppUtils.hpp>

// Standard
#include <filesystem>
#include <cstring>
#include <iostream>
#include <fstream>
#include <array>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <random>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>

// GLFW/glad
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// stb
#include <stb/stb_image.h>
#include <stb/stb_write.h>

// Freetype
#include <ft2build.h>
#include FT_FREETYPE_H

// Core library stuff
#include "math/DataStructures.h"

// User defined literals
MathAnim::Vec4 operator "" _hex(const char* hexColor, size_t length);

struct RawMemory
{
	uint8* data;
	size_t size;
	size_t offset;

	void init(size_t initialSize);
	void free();
	void shrinkToFit();
	void resetReadWriteCursor();

	void writeDangerous(const uint8* data, size_t dataSize);
	void readDangerous(uint8* data, size_t dataSize);

	template<typename T>
	void write(const T* data)
	{
		writeDangerous((uint8*)data, sizeof(T));
	}

	template<typename T>
	void read(T* data)
	{
		readDangerous((uint8*)data, sizeof(T));
	}

	void setCursor(size_t offset);
};

struct SizedMemory
{
	uint8* memory;
	size_t size;
};

namespace MemoryHelper
{
	template<typename T>
	size_t copyDataByType(uint8* dst, size_t offset, const T& data)
	{
		uint8* dstData = dst + offset;
		*(T*)dstData = data;
		return sizeof(T);
	}

	template<typename First, typename... Rest>
	void copyDataToType(uint8* dst, size_t offset, const First& data, Rest... rest)
	{
		static_assert(std::is_pod<First>(), "Cannot accept non-POD values for dynamic memory packing.");
		offset += copyDataByType<First>(dst, offset, data);
		if constexpr (sizeof...(Rest) != 0)
		{
			copyDataToType<Rest...>(dst, offset, rest...);
		}
	}

	template <typename First, typename... Rest>
	void unpackData(const SizedMemory& memory, size_t offset, First* first, Rest*... rest)
	{
#ifdef _DEBUG
		g_logger_assert(offset + sizeof(First) <= memory.size, "Cannot unpack this memory. Would result in a buffer overrun.");
#endif
		static_assert(std::is_pod<First>(), "Cannot accept non-POD values for dynamic memory unpacking");
		uint8* data = memory.memory + offset;
		*first = *(First*)data;

		if constexpr (sizeof...(Rest) != 0)
		{
			unpackData<Rest...>(memory, offset + sizeof(First), rest...);
		}
	}
}

template<typename First, typename... Rest>
size_t sizeOfTypes()
{
	if constexpr (sizeof...(Rest) == 0)
	{
		return sizeof(First);
	}
	else
	{
		return sizeof(First) + sizeOfTypes<Rest...>();
	}
}

template<typename... Types>
SizedMemory pack(const Types&... data)
{
	size_t typesSize = sizeOfTypes<Types...>();
	uint8* result = (uint8*)g_memory_allocate(typesSize);
	MemoryHelper::copyDataToType<Types...>(result, 0, data...);
	return { result, typesSize };
}

template<typename... Types>
void unpack(const SizedMemory& memory, Types*... data)
{
	MemoryHelper::unpackData<Types...>(memory, 0, data...);
}

#endif
