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
		bool compare(const glm::vec3& vec1, const glm::vec3& vec2, float epsilon = std::numeric_limits<float>::min());
		bool compare(const glm::vec2& vec1, const glm::vec2& vec2, float epsilon = std::numeric_limits<float>::min());
		bool compare(const glm::vec4& vec1, const glm::vec4& vec2, float epsilon = std::numeric_limits<float>::min());

		// Vector conversions
		glm::vec2 vector2From3(const glm::vec3& vec);
		glm::vec3 vector3From2(const glm::vec2& vec);

		// Math functions
		void rotate(glm::vec2& vec, float angleDeg, const glm::vec2& origin);
		void rotate(glm::vec3& vec, float angleDeg, const glm::vec3& origin);

		float toRadians(float degrees);
		float toDegrees(float radians);

		// Map Ranges
		float mapRange(float val, float inMin, float inMax, float outMin, float outMax);

		// Max, Min impls
		int max(int a, int b);
		int min(int a, int b);
		float saturate(float val);

		// Hash Strings
		uint32 hashString(const char* str);
	}
}

#endif
