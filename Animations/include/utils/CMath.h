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
		// Float Comparison functions, using custom epsilon
		bool compare(float x, float y, float epsilon = std::numeric_limits<float>::min());
		bool compare(const Vec3& vec1, const Vec3& vec2, float epsilon = std::numeric_limits<float>::min());
		bool compare(const Vec2& vec1, const Vec2& vec2, float epsilon = std::numeric_limits<float>::min());
		bool compare(const Vec4& vec1, const Vec4& vec2, float epsilon = std::numeric_limits<float>::min());

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

		// Hash Strings
		uint32 hashString(const char* str);

		// Bezier stuff
		Vec2 bezier1(const Vec2& p0, const Vec2& p1, float t);
		Vec2 bezier2(const Vec2& p0, const Vec2& p1, const Vec2& p2, float t);
		Vec2 bezier3(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, float t);
		Vec3 bezier1(const Vec3& p0, const Vec3& p1, float t);
		Vec3 bezier2(const Vec3& p0, const Vec3& p1, const Vec3& p2, float t);
		Vec3 bezier3(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t);

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
