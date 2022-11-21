#ifndef MATH_ANIM_C_MATH_H
#define MATH_ANIM_C_MATH_H

#include "core.h"

// TODO: Find who's leaking min, max macros...
#ifdef min 
#undef min
#endif
#ifdef max
#undef max
#endif

namespace MathAnim
{
	enum class EaseType : uint8
	{
		None,
		Linear,
		Sine,
		Quad,
		Cubic,
		Quart,
		Quint,
		Exponential,
		Circular,
		Back,
		Elastic,
		Bounce,
		Length
	};

	constexpr const char* easeTypeNames[(uint8)EaseType::Length] = {
		"None",
		"Linear",
		"Sine",
		"Quad",
		"Cubic",
		"Quart",
		"Quint",
		"Exponential",
		"Circular",
		"Back",
		"Elastic",
		"Bounce"
	};

	enum class EaseDirection : uint8
	{
		None,
		In,
		Out,
		InOut,
		Length
	};

	constexpr const char* easeDirectionNames[(uint8)EaseDirection::Length] = {
		"None",
		"In",
		"Out",
		"In-Out"
	};

	namespace CMath
	{
		inline float quadraticFormulaPos(float a, float b, float c)
		{
			return (-b + glm::sqrt(b * b - 4 * a * c)) / (2.0f * a);
		}

		inline float quadraticFormulaNeg(float a, float b, float c)
		{
			return (-b - glm::sqrt(b * b - 4 * a * c)) / (2.0f * a);
		}

		// Implicit conversions from Vec to glm::vec
		inline glm::vec2 convert(const Vec2& vec)
		{
			return glm::vec2(vec.x, vec.y);
		}

		inline glm::vec3 convert(const Vec3& vec)
		{
			return glm::vec3(vec.x, vec.y, vec.z);
		}

		inline glm::vec4 convert(const Vec4& vec)
		{
			return glm::vec4(vec.x, vec.y, vec.z, vec.w);
		}

		// Winding order stuff
		bool isClockwise(const Vec2& p0, const Vec2& p1, const Vec2& p2);
		inline bool isCounterClockwise(const Vec2& p0, const Vec2& p1, const Vec2& p2) { return !isClockwise(p0, p1, p2); }

		bool isClockwise(const Vec3& p0, const Vec3& p1, const Vec3& p2);
		inline bool isCounterClockwise(const Vec3& p0, const Vec3& p1, const Vec3& p2) { return !isClockwise(p0, p1, p2); }

		// Float Comparison functions, using custom epsilon
		bool compare(float x, float y, float epsilon = std::numeric_limits<float>::min());
		bool compare(const Vec3& vec1, const Vec3& vec2, float epsilon = std::numeric_limits<float>::min());
		bool compare(const Vec2& vec1, const Vec2& vec2, float epsilon = std::numeric_limits<float>::min());
		bool compare(const Vec4& vec1, const Vec4& vec2, float epsilon = std::numeric_limits<float>::min());

		template<typename T>
		inline uint64 combineHash(const T& t, uint64 hash)
		{
			// Taken from https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values
			return (hash ^ (std::hash<T>(t) + 0x9e3779b9 + (hash << 6) + (hash >> 2)));
		}

		template<>
		inline uint64 combineHash(const float& t, uint64 hash)
		{
			// Taken from https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values
			return (hash ^ ((int32)t + 0x9e3779b9 + (hash << 6) + (hash >> 2)));
		}

		template<>
		inline uint64 combineHash(const int& t, uint64 hash)
		{
			// Taken from https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values
			return (hash ^ ((int32)t + 0x9e3779b9 + (hash << 6) + (hash >> 2)));
		}

		// Vector conversions
		Vec2 vector2From3(const Vec3& vec);
		Vec3 vector3From2(const Vec2& vec);

		// Math functions
		void rotate(Vec2& vec, float angleDeg, const Vec2& origin);
		void rotate(Vec3& vec, float angleDeg, const Vec3& origin);

		float toRadians(float degrees);
		float toDegrees(float radians);

		// Map Ranges
		float mapRange(float val, float inMin, float inMax, float outMin, float outMax);
		float mapRange(const Vec2& inputRange, const Vec2& outputRange, float value);

		// Max, Min impls
		int max(int a, int b);
		int min(int a, int b);
		float saturate(float val);

		inline float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
		inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

		inline float abs(float a) { return a > 0 ? a : -a; }
		inline Vec2 abs(const Vec2& a) { return Vec2{ abs(a.x), abs(a.y) }; }
		inline Vec3 abs(const Vec3& a) { return Vec3{ abs(a.x), abs(a.y), abs(a.z)}; }

		// Max, Min helpers
		Vec2 max(const Vec2& a, const Vec2& b);
		Vec2 min(const Vec2& a, const Vec2& b);
		Vec3 max(const Vec3& a, const Vec3& b);
		Vec3 min(const Vec3& a, const Vec3& b);
		Vec4 max(const Vec4& a, const Vec4& b);
		Vec4 min(const Vec4& a, const Vec4& b);

		// Range max, min helpers
		Vec2 rangeMaxMin(Vec2 range, float value);

		// Hash Strings
		uint32 hashString(const char* str);

		// Bezier stuff
		Vec2 bezier1(const Vec2& p0, const Vec2& p1, float t);
		Vec2 bezier2(const Vec2& p0, const Vec2& p1, const Vec2& p2, float t);
		Vec2 bezier3(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, float t);

		Vec3 bezier1(const Vec3& p0, const Vec3& p1, float t);
		Vec3 bezier2(const Vec3& p0, const Vec3& p1, const Vec3& p2, float t);
		Vec3 bezier3(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t);

		Vec2 bezier1Normal(const Vec2& p0, const Vec2& p1, float t);
		Vec2 bezier2Normal(const Vec2& p0, const Vec2& p1, const Vec2& p2, float t);
		Vec2 bezier3Normal(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, float t);

		Vec3 bezier1Normal(const Vec3& p0, const Vec3& p1, float t);
		Vec3 bezier2Normal(const Vec3& p0, const Vec3& p1, const Vec3& p2, float t);
		Vec3 bezier3Normal(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t);

		// Bezier extremities
		// Returns pair <xRoot, yRoot> in tValues
		// -1.0f indicates an invalid root
		Vec2 tRootBezier2(const Vec2& p0, const Vec2& p1, const Vec2& p2);
		// Returns pairs <xRootPos, yRootPos, xRootNeg, yRootNeg> in tValues
		// -1.0f indicates an invalid root
		Vec4 tRootsBezier3(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3);

		BBox bezier1BBox(const Vec2& p0, const Vec2& p1);
		BBox bezier2BBox(const Vec2& p0, const Vec2& p1, const Vec2& p2);
		BBox bezier3BBox(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3);

		// Easing functions
		float ease(float t, EaseType type, EaseDirection direction);

		// Animation functions
		Vec4 interpolate(float t, const Vec4& src, const Vec4& target);
		Vec3 interpolate(float t, const Vec3& src, const Vec3& target);
		Vec2 interpolate(float t, const Vec2& src, const Vec2& target);
		glm::u8vec4 interpolate(float t, const glm::u8vec4& src, const glm::u8vec4& target);
		float interpolate(float t, float src, float target);

		// (de)Serialization functions
		void serialize(RawMemory& memory, const Vec4& vec);
		void serialize(RawMemory& memory, const Vec3& vec);
		void serialize(RawMemory& memory, const Vec2& vec);

		void serialize(RawMemory& memory, const Vec4i& vec);
		void serialize(RawMemory& memory, const Vec3i& vec);
		void serialize(RawMemory& memory, const Vec2i& vec);

		void serialize(RawMemory& memory, const glm::u8vec4& vec);

		Vec4 deserializeVec4(RawMemory& memory);
		Vec3 deserializeVec3(RawMemory& memory);
		Vec2 deserializeVec2(RawMemory& memory);

		Vec4i deserializeVec4i(RawMemory& memory);
		Vec3i deserializeVec3i(RawMemory& memory);
		Vec2i deserializeVec2i(RawMemory& memory);

		glm::u8vec4 deserializeU8Vec4(RawMemory& memory);
	}
}

#endif
