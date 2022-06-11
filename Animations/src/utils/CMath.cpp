#include "utils/CMath.h"

namespace MathAnim
{
	namespace CMath
	{
		// ------------------ Internal Values ------------------
		static float PI = 3.1415926535897932384626433832795028841971693993751058209749445923078164062f;

		// ------------------ Internal Functions ------------------
		static float easeInSine(float t);
		static float easeOutSine(float t);
		static float easeInOutSine(float t);

		static float easeInQuad(float t);
		static float easeOutQuad(float t);
		static float easeInOutQuad(float t);

		static float easeInCubic(float t);
		static float easeOutCubic(float t);
		static float easeInOutCubic(float t);

		static float easeInQuart(float t);
		static float easeOutQuart(float t);
		static float easeInOutQuart(float t);

		static float easeInQuint(float t);
		static float easeOutQuint(float t);
		static float easeInOutQuint(float t);

		static float easeInExpo(float t);
		static float easeOutExpo(float t);
		static float easeInOutExpo(float t);

		static float easeInCirc(float t);
		static float easeOutCirc(float t);
		static float easeInOutCirc(float t);

		static float easeInBack(float t);
		static float easeOutBack(float t);
		static float easeInOutBack(float t);

		static float easeInElastic(float t);
		static float easeOutElastic(float t);
		static float easeInOutElastic(float t);

		static float easeInBounce(float t);
		static float easeOutBounce(float t);
		static float easeInOutBounce(float t);

		// ------------------ Public Functions ------------------
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
			int length = (int)std::strlen(str);

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

		// Easing functions
		float ease(float t, EaseType type, EaseDirection direction)
		{
			if (type == EaseType::None || direction == EaseDirection::None)
			{
				g_logger_warning("Ease type or direction was set to none.");
				return t;
			}

			switch (type)
			{
			case EaseType::Linear:
				return t;
			case EaseType::Sine:
				return direction == EaseDirection::In
					? easeInSine(t)
					: direction == EaseDirection::Out
					? easeOutSine(t)
					: easeInOutSine(t);
			case EaseType::Quad:
				return direction == EaseDirection::In
					? easeInQuad(t)
					: direction == EaseDirection::Out
					? easeOutQuad(t)
					: easeInOutQuad(t);
			case EaseType::Cubic:
				return direction == EaseDirection::In
					? easeInCubic(t)
					: direction == EaseDirection::Out
					? easeOutCubic(t)
					: easeInOutCubic(t);
			case EaseType::Quart:
				return direction == EaseDirection::In
					? easeInQuart(t)
					: direction == EaseDirection::Out
					? easeOutQuart(t)
					: easeInOutQuart(t);
			case EaseType::Quint:
				return direction == EaseDirection::In
					? easeInQuint(t)
					: direction == EaseDirection::Out
					? easeOutQuint(t)
					: easeInOutQuint(t);
			case EaseType::Exponential:
				return direction == EaseDirection::In
					? easeInExpo(t)
					: direction == EaseDirection::Out
					? easeOutExpo(t)
					: easeInOutExpo(t);
			case EaseType::Circular:
				return direction == EaseDirection::In
					? easeInCirc(t)
					: direction == EaseDirection::Out
					? easeOutCirc(t)
					: easeInOutCirc(t);
			case EaseType::Back:
				return direction == EaseDirection::In
					? easeInBack(t)
					: direction == EaseDirection::Out
					? easeOutBack(t)
					: easeInOutBack(t);
			case EaseType::Elastic:
				return direction == EaseDirection::In
					? easeInElastic(t)
					: direction == EaseDirection::Out
					? easeOutElastic(t)
					: easeInOutElastic(t);
			case EaseType::Bounce:
				return direction == EaseDirection::In
					? easeInBounce(t)
					: direction == EaseDirection::Out
					? easeOutBounce(t)
					: easeInOutBounce(t);
			default:
				g_logger_error("Unknown easing %d and direction %d", (int)type, (int)direction);
			}

			return t;
		}

		// ------------------ Internal Functions ------------------
		// These are all taken from here https://easings.net
		static float easeInSine(float t)
		{
			return 1.0f - glm::cos((t * glm::pi<float>()) / 2.0f);
		}

		static float easeOutSine(float t)
		{
			return glm::sin((t * glm::pi<float>()) / 2.0f);
		}

		static float easeInOutSine(float t)
		{
			return -(glm::cos(glm::pi<float>() * t) - 1.0f) / 2.0f;
		}

		static float easeInQuad(float t)
		{
			return t * t;
		}

		static float easeOutQuad(float t)
		{
			return 1.0f - (1.0f - t) * (1.0f - t);
		}

		static float easeInOutQuad(float t)
		{
			return t < 0.5f 
				? 2.0f * t * t 
				: 1.0f - glm::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
		}

		static float easeInCubic(float t)
		{
			return t * t * t;
		}

		static float easeOutCubic(float t)
		{
			return 1.0f - glm::pow(1.0f - t, 3.0f);
		}

		static float easeInOutCubic(float t)
		{
			return t < 0.5f ? 4.0f * t * t * t : 1.0f - glm::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
		}

		static float easeInQuart(float t)
		{
			return t * t * t * t;
		}

		static float easeOutQuart(float t)
		{
			return 1.0f - glm::pow(1.0f - t, 4.0f);
		}

		static float easeInOutQuart(float t)
		{
			return t < 0.5f 
				? 8.0f * t * t * t * t 
				: 1.0f - glm::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
		}

		static float easeInQuint(float t)
		{
			return t * t * t * t * t;
		}

		static float easeOutQuint(float t)
		{
			return 1.0f - glm::pow(1.0f - t, 5.0f);
		}

		static float easeInOutQuint(float t)
		{
			return t < 0.5f 
				? 16.0f * t * t * t * t * t 
				: 1.0f - glm::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;
		}

		static float easeInExpo(float t)
		{
			return glm::epsilonEqual(t, 0.0f, 0.01f)
				? 0.0f 
				: glm::pow(2.0f, 10.0f * t - 10.0f);
		}

		static float easeOutExpo(float t)
		{
			return glm::epsilonEqual(t, 1.0f, 0.01f)
				? 1.0f 
				: 1.0f - glm::pow(2.0f, -10.0f * t);
		}

		static float easeInOutExpo(float t)
		{
			return glm::epsilonEqual(t, 0.0f, 0.01f)
				? 0.0f
				: glm::epsilonEqual(t, 1.0f, 0.01f)
				? 1.0f
				: t < 0.5f 
					? glm::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
					: (2.0f - glm::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
		}

		static float easeInCirc(float t)
		{
			return 1.0f - glm::sqrt(1.0f - glm::pow(t, 2.0f));
		}

		static float easeOutCirc(float t)
		{
			return glm::sqrt(1.0f - glm::pow(t - 1.0f, 2.0f));
		}

		static float easeInOutCirc(float t)
		{
			return t < 0.5f
				? (1.0f - glm::sqrt(1.0f - glm::pow(2.0f * t, 2.0f))) / 2.0f
				: (glm::sqrt(1.0f - glm::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
		}

		static float easeInBack(float t)
		{
			constexpr float c1 = 1.70158f;
			constexpr float c3 = c1 + 1.0f;

			return c3 * t * t * t - c1 * t * t;
		}

		static float easeOutBack(float t)
		{
			constexpr float c1 = 1.70158f;
			constexpr float c3 = c1 + 1.0f;

			return 1.0f + c3 * glm::pow(t - 1.0f, 3.0f) + c1 * glm::pow(t - 1.0f, 2.0f);
		}

		static float easeInOutBack(float t)
		{
			constexpr float c1 = 1.70158f;
			constexpr float c2 = c1 * 1.525f;

			return t < 0.5f
				? (glm::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
				: (glm::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
		}

		static float easeInElastic(float t)
		{
			constexpr float c4 = (2.0f * glm::pi<float>()) / 3.0f;

			return glm::epsilonEqual(t, 0.0f, 0.01f)
				? 0.0f
				: glm::epsilonEqual(t, 1.0f, 0.01f)
				? 1.0f
				: -glm::pow(2.0f, 10.0f * t - 10.0f) * glm::sin((t * 10.0f - 10.75f) * c4);
		}

		static float easeOutElastic(float t)
		{
			constexpr float c4 = (2.0f * glm::pi<float>()) / 3.0f;

			return glm::epsilonEqual(t, 0.0f, 0.01f)
				? 0.0f
				: glm::epsilonEqual(t, 1.0f, 0.01f)
				? 1.0f
				: glm::pow(2.0f, -10.0f * t) * glm::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
		}

		static float easeInOutElastic(float t)
		{
			constexpr float c5 = (2.0f * glm::pi<float>()) / 4.5f;

			return glm::epsilonEqual(t, 0.0f, 0.01f)
				? 0.0f
				: glm::epsilonEqual(t, 1.0f, 0.01f)
				? 1.0f
				: t < 0.5f
				? -(glm::pow(2.0f, 20.0f * t - 10.0f) * sin((20.0f * t - 11.125f) * c5)) / 2.0f
				: (glm::pow(2.0f, -20.0f * t + 10.0f) * sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
		}

		static float easeInBounce(float t)
		{
			return 1.0f - easeOutBounce(1.0f - t);
		}

		static float easeOutBounce(float t)
		{
			constexpr float n1 = 7.5625f;
			constexpr float d1 = 2.75f;

			if (t < 1.0f / d1)
			{
				return n1 * t * t;
			}
			else if (t < 2.0f / d1)
			{
				return n1 * (t -= (1.5f / d1)) * t + .75f;
			}
			else if (t < 2.5f / d1)
			{
				return n1 * (t -= (2.25f / d1)) * t + .9375f;
			}
			else
			{
				return n1 * (t -= (2.625f / d1)) * t + .984375f;
			}
		}

		static float easeInOutBounce(float t)
		{
			return t < 0.5f
				? (1.0f - easeOutBounce(1.0f - 2.0f * t)) / 2.0f
				: (1.0f + easeOutBounce(2.0f * t - 1.0f)) / 2.0f;
		}
	}
}