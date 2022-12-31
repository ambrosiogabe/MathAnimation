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
#include <glm/gtx/matrix_decompose.hpp>

// My stuff
#include <cppUtils/cppUtils.h>

// Standard
#include <filesystem>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
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
#include <set>
#include <unordered_set>
#include <regex>

// GLFW/glad
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// stb
#include <stb/stb_image.h>
#include <stb/stb_write.h>
#include <stb/stb_image_resize.h>

// Freetype
#include <ft2build.h>
#include FT_FREETYPE_H

// Core library stuff
#include "math/DataStructures.h"

#define IMGUI_USER_CONFIG "core/InternalImGuiConfig.h"
#include <imgui.h>

// Regex Library for textmate grammar
#include <oniguruma.h>

// User defined literals
MathAnim::Vec4 operator""_hex(const char* hexColor, size_t length);
MathAnim::Vec4 toHex(const std::string& str);
MathAnim::Vec4 toHex(const char* hex, size_t length);
MathAnim::Vec4 toHex(const char* hex);
MathAnim::Vec4 fromCssColor(const char* cssColorStr);
MathAnim::Vec4 fromCssColor(const std::string& cssColorStr);

// SIMD intrinsics
#include <xmmintrin.h>

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
	bool readDangerous(uint8* data, size_t dataSize);

	template<typename T>
	void write(const T* data)
	{
		writeDangerous((uint8*)data, sizeof(T));
	}

	template<typename T>
	bool read(T* data)
	{
		return readDangerous((uint8*)data, sizeof(T));
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

// Array helpers taken from https://stackoverflow.com/a/57524328
template <typename T, std::size_t N, class ...Args>
constexpr std::array<T, N> fixedSizeArray(Args&&... values)
{
	static_assert(sizeof...(values) == N);
	return std::array<T, N>{values...};
}

typedef uint64 AnimObjId;
typedef uint64 AnimId;

namespace MathAnim
{
	constexpr AnimObjId NULL_ANIM_OBJECT = UINT64_MAX;
	constexpr AnimId NULL_ANIM = UINT64_MAX;
}

#endif
