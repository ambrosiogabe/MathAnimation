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

		// Other stuff
		Vec2 bezier1(const Vec2& p0, const Vec2& p1, float t);
		Vec2 bezier2(const Vec2& p0, const Vec2& p1, const Vec2& p2, float t);

		// Easing functions
		float easeInOutCubic(float t);
	}
}

#endif
