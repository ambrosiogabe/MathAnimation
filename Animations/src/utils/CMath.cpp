#include "utils/CMath.h"

namespace MathAnim
{
	namespace CMath
	{
		static float PI = 3.1415926535897932384626433832795028841971693993751058209749445923078164062f;

		bool compare(float x, float y, float epsilon)
		{
			return abs(x - y) <= epsilon * std::max(1.0f, std::max(abs(x), abs(y)));
		}

		bool compare(const Vec2& vec1, const Vec2& vec2, float epsilon)
		{
			return compare(vec1.x, vec2.x, epsilon) && compare(vec1.y, vec2.y, epsilon);
		}

		bool compare(const Vec3& vec1, const Vec3& vec2, float epsilon)
		{
			return compare(vec1.x, vec2.x, epsilon) && compare(vec1.y, vec2.y, epsilon) && compare(vec1.z, vec2.z, epsilon);
		}

		bool compare(const Vec4& vec1, const Vec4& vec2, float epsilon)
		{
			return compare(vec1.x, vec2.x, epsilon) && compare(vec1.y, vec2.y, epsilon) && compare(vec1.z, vec2.z, epsilon) && compare(vec1.w, vec2.w, epsilon);
		}

		Vec2 vector2From3(const Vec3& vec)
		{
			return Vec2{vec.x, vec.y};
		}

		Vec3 vector3From2(const Vec2& vec)
		{
			return Vec3{vec.x, vec.y, 0.0f};
		}

		float toRadians(float degrees)
		{
			return degrees * PI / 180.0f;
		}

		float toDegrees(float radians)
		{
			return radians * 180.0f / PI;
		}

		void rotate(Vec2& vec, float angleDeg, const Vec2& origin)
		{
			float x = vec.x - origin.x;
			float y = vec.y - origin.y;

			float xPrime = origin.x + ((x * (float)cos(toRadians(angleDeg))) - (y * (float)sin(toRadians(angleDeg))));
			float yPrime = origin.y + ((x * (float)sin(toRadians(angleDeg))) + (y * (float)cos(toRadians(angleDeg))));

			vec.x = xPrime;
			vec.y = yPrime;
		}

		void rotate(Vec3& vec, float angleDeg, const Vec3& origin)
		{
			// This function ignores Z values
			float x = vec.x - origin.x;
			float y = vec.y - origin.y;

			float xPrime = origin.x + ((x * (float)cos(toRadians(angleDeg))) - (y * (float)sin(toRadians(angleDeg))));
			float yPrime = origin.y + ((x * (float)sin(toRadians(angleDeg))) + (y * (float)cos(toRadians(angleDeg))));

			vec.x = xPrime;
			vec.y = yPrime;
		}

		float mapRange(float val, float inMin, float inMax, float outMin, float outMax)
		{
			return (val - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
		}

		float mapRange(const Vec2& inputRange, const Vec2& outputRange, float value)
		{
			return (value - inputRange.x) / (inputRange.y - inputRange.x) * (outputRange.y - outputRange.x) + outputRange.x;
		}

		int max(int a, int b)
		{
			return a > b ? a : b;
		}

		int min(int a, int b)
		{
			return a < b ? a : b;
		}

		float saturate(float val)
		{
			return val < 0 ? 0 :
				val > 1 ? 1 :
				val;
		}

		uint32 hashString(const char* str)
		{
			uint32 hash = 2166136261u;
			int length = strlen(str);

			for (int i = 0; i < length; i++)
			{
				hash ^= str[i];
				hash *= 16777619;
			}

			return hash;
		}

		Vec2 bezier1(const Vec2& p0, const Vec2& p1, float t)
		{
			return (1.0f - t) * p0 + t * p1;
		}

		Vec2 bezier2(const Vec2& p0, const Vec2& p1, const Vec2& p2, float t)
		{
			return (1 - t) * ((1 - t) * p0 + t * p1) + t * ((1 - t) * p1 + t * p2);
		}
	}
}