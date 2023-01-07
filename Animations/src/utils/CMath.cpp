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
		bool isClockwise(const Vec2& p0, const Vec2& p1, const Vec2& p2)
		{
			return glm::determinant(
				glm::mat3(
					glm::vec3(p0.x, p0.y, 1.0f),
					glm::vec3(p1.x, p1.y, 1.0f),
					glm::vec3(p2.x, p2.y, 1.0f)
				)
			) < 0;
		}

		bool isClockwise(const Vec3& p0, const Vec3& p1, const Vec3& p2)
		{
			return glm::determinant(
				glm::mat3(
					glm::vec3(p0.x, p0.y, p0.z),
					glm::vec3(p1.x, p1.y, p1.z),
					glm::vec3(p2.x, p2.y, p2.z)
				)
			) < 0;
		}

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
			return Vec2{ vec.x, vec.y };
		}

		Vec3 vector3From2(const Vec2& vec)
		{
			return Vec3{ vec.x, vec.y, 0.0f };
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

		Vec2 max(const Vec2& a, const Vec2& b) 
		{
			return Vec2{ glm::max(a.x, b.x), glm::max(a.y, b.y) };
		}

		Vec2 min(const Vec2& a, const Vec2& b) 
		{
			return Vec2{ glm::min(a.x, b.x), glm::min(a.y, b.y) };
		}

		Vec3 max(const Vec3& a, const Vec3& b)
		{
			return Vec3{ glm::max(a.x, b.x), glm::max(a.y, b.y), glm::max(a.z, b.z) };
		}

		Vec3 min(const Vec3& a, const Vec3& b) 
		{
			return Vec3{ glm::min(a.x, b.x), glm::min(a.y, b.y), glm::min(a.z, b.z) };
		}

		Vec4 max(const Vec4& a, const Vec4& b)
		{
			return Vec4{ glm::max(a.x, b.x), glm::max(a.y, b.y), glm::max(a.z, b.z), glm::max(a.w, b.w) };
		}

		Vec4 min(const Vec4& a, const Vec4& b) 
		{
			return Vec4{ glm::min(a.x, b.x), glm::min(a.y, b.y), glm::min(a.z, b.z), glm::min(a.w, b.w) };
		}

		Vec2 rangeMaxMin(Vec2 range, float value)
		{
			return Vec2{ glm::min(range.min, value), glm::max(range.max, value) };
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
			return (1.0f - t) * ((1.0f - t) * p0 + t * p1) + t * ((1.0f - t) * p1 + t * p2);
		}

		Vec2 bezier3(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, float t)
		{
			return
				glm::pow(1.0f - t, 3.0f) * p0 +
				3.0f * (1.0f - t) * (1.0f - t) * t * p1 +
				(3.0f * (1.0f - t) * t * t) * p2 +
				t * t * t * p3;
		}

		Vec3 bezier1(const Vec3& p0, const Vec3& p1, float t)
		{
			return (1.0f - t) * p0 + t * p1;
		}

		Vec3 bezier2(const Vec3& p0, const Vec3& p1, const Vec3& p2, float t)
		{
			return (1.0f - t) * ((1.0f - t) * p0 + t * p1) + t * ((1.0f - t) * p1 + t * p2);
		}

		Vec3 bezier3(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t)
		{
			return
				glm::pow(1.0f - t, 3.0f) * p0 +
				3.0f * (1.0f - t) * (1.0f - t) * t * p1 +
				(3.0f * (1.0f - t) * t * t) * p2 +
				t * t * t * p3;
		}

		Vec2 bezier1Normal(const Vec2& p0, const Vec2& p1, float t)
		{
			return normalize(p1 - p0);
		}

		Vec2 bezier2Normal(const Vec2& p0, const Vec2& p1, const Vec2& p2, float t)
		{
			// Derivative taken from https://en.wikipedia.org/wiki/Bézier_curve#Quadratic_Bézier_curves
			// Just return the normalized derivative at point t
			return normalize(
				(2.0f * (1.0f - t) * (p1 - p0)) + 
				(2.0f * t * (p2 - p1))
			);
		}

		Vec2 bezier3Normal(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, float t)
		{
			// Derivative taken from https://en.wikipedia.org/wiki/Bézier_curve#Cubic_Bézier_curves
			// Just return the normalized derivative at point t
			return normalize(
				(3.0f * (1.0f - t) * (1.0f - t) * (p1 - p0)) +
				(6.0f * (1.0f - t) * t * (p2 - p1)) + 
				(3.0f * t * t * (p3 - p2))
			);
		}

		Vec3 bezier1Normal(const Vec3& p0, const Vec3& p1, float t)
		{
			return normalize(p1 -  p0);
		}

		Vec3 bezier2Normal(const Vec3& p0, const Vec3& p1, const Vec3& p2, float t)
		{
			// Derivative taken from https://en.wikipedia.org/wiki/Bézier_curve#Quadratic_Bézier_curves
			// Just return the normalized derivative at point t
			return normalize(
				(2.0f * (1.0f - t) * (p1 - p0)) +
				(2.0f * t * (p2 - p1))
			);
		}

		Vec3 bezier3Normal(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t)
		{
			// Derivative taken from https://en.wikipedia.org/wiki/Bézier_curve#Cubic_Bézier_curves
			// Just return the normalized derivative at point t
			return normalize(
				(3.0f * (1.0f - t) * (1.0f - t) * (p1 - p0)) +
				(6.0f * (1.0f - t) * t * (p2 - p1)) +
				(3.0f * t * t * (p3 - p2))
			);
		}

		Vec2 tRootBezier2(const Vec2& p0, const Vec2& p1, const Vec2& p2)
		{
			Vec2 w0 = 2.0f * (p1 - p0);
			Vec2 w1 = 2.0f * (p2 - p1);
			Vec2 tValues;

			// If the denominator is 0, then return invalid t-value
			tValues.x = CMath::compare(w1.x - w0.x, 0.0f)
				? -1.0f 
				: (-w0.x) / (w1.x - w0.x);
			tValues.y = CMath::compare(w1.y - w0.y, 0.0f)
				? -1.0f
				: (-w0.y) / (w1.y - w0.y);

			return tValues;
		}

		Vec4 tRootsBezier3(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3)
		{
			Vec2 v0 = 3.0f * (p1 - p0);
			Vec2 v1 = 3.0f * (p2 - p1);
			Vec2 v2 = 3.0f * (p3 - p2);

			Vec2 a = v0 - (2.0f * v1) + v2;
			Vec2 b = 2.0f * (v1 - v0);
			Vec2 c = v0;

			Vec4 res;
			// res[0] is + case in quadratic curve
			// res[1] is - case in quadratic curve
			if (CMath::compare(a.x, 0.0f))
			{
				res.values[0] = -1.0f;
				res.values[2] = -1.0f;
			}
			else
			{
				// Solve x cases
				res.values[0] = quadraticFormulaPos(a.x, b.x, c.x);
				res.values[2] = quadraticFormulaNeg(a.x, b.x, c.x);
			}

			if (CMath::compare(a.y, 0.0f))
			{
				res.values[1] = -1.0f;
				res.values[3] = -1.0f;
			}
			else
			{
				res.values[1] = quadraticFormulaPos(a.y, b.y, c.y);
				res.values[3] = quadraticFormulaNeg(a.y, b.y, c.y);
			}

			return res;
		}

		BBox bezier1BBox(const Vec2& p0, const Vec2& p1)
		{
			BBox res;
			res.min = min(p0, p1);
			res.max = max(p0, p1);
			return res;
		}

		BBox bezier2BBox(const Vec2& p0, const Vec2& p1, const Vec2& p2)
		{
			// Find extremities then return min max extremities
			// Initialize it to the min/max of the endpoints
			BBox res;
			res.min = min(p0, p2);
			res.max = max(p0, p2);

			Vec2 roots = tRootBezier2(p0, p1, p2);
			for (int i = 0; i < 2; i++)
			{
				// Root is in range of the bezier curve
				if (roots.values[i] > 0.0f && roots.values[i] < 1.0f)
				{
					Vec2 pos = bezier2(p0, p1, p2, roots.values[i]);
					res.min = min(res.min, pos);
					res.max = max(res.max, pos);
				}
			}

			return res;
		}

		BBox bezier3BBox(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3)
		{
			// Find extremities then return min max extremities
			// Initialize it to the min/max of the endpoints
			BBox res;
			res.min = min(p0, p3);
			res.max = max(p0, p3);

			Vec4 roots = tRootsBezier3(p0, p1, p2, p3);
			for (int i = 0; i < 4; i++)
			{
				// Root is in range of the bezier curve
				if (roots.values[i] > 0.0f && roots.values[i] < 1.0f)
				{
					Vec2 pos = bezier2(p0, p1, p2, roots.values[i]);
					res.min = min(res.min, pos);
					res.max = max(res.max, pos);
				}
			}

			return res;
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
			case EaseType::Length:
			case EaseType::None:
				break;
			}

			return t;
		}

		// Animation functions
		Vec4 interpolate(float t, const Vec4& src, const Vec4& target)
		{
			return Vec4{
				(target.x - src.x) * t + src.x,
				(target.y - src.y) * t + src.y,
				(target.z - src.z) * t + src.z,
				(target.w - src.w) * t + src.w
			};
		}

		Vec3 interpolate(float t, const Vec3& src, const Vec3& target)
		{
			return Vec3{
				(target.x - src.x) * t + src.x,
				(target.y - src.y) * t + src.y,
				(target.z - src.z) * t + src.z
			};
		}

		Vec2 interpolate(float t, const Vec2& src, const Vec2& target)
		{
			return Vec2{
				(target.x - src.x) * t + src.x,
				(target.y - src.y) * t + src.y
			};
		}

		glm::u8vec4 interpolate(float t, const glm::u8vec4& src, const glm::u8vec4& target)
		{
			glm::vec4 normalSrc = glm::vec4{
				(float)src.r / 255.0f,
				(float)src.g / 255.0f,
				(float)src.b / 255.0f,
				(float)src.a / 255.0f
			};
			glm::vec4 normalTarget = glm::vec4{
				(float)target.r / 255.0f,
				(float)target.g / 255.0f,
				(float)target.b / 255.0f,
				(float)target.a / 255.0f
			};
			glm::vec4 res = (normalTarget - normalSrc) * t + normalSrc;

			return glm::u8vec4(
				(uint8)(res.r * 255.0f),
				(uint8)(res.g * 255.0f),
				(uint8)(res.b * 255.0f),
				(uint8)(res.a * 255.0f)
			);
		}

		float interpolate(float t, float src, float target)
		{
			return (target - src) * t + src;
		}

		// (de)Serialization functions
		void serialize(RawMemory& memory, const Vec4& vec)
		{
			// Target
			//   X    -> float
			//   Y    -> float
			//   Z    -> float
			//   W    -> float
			memory.write<float>(&vec.x);
			memory.write<float>(&vec.y);
			memory.write<float>(&vec.z);
			memory.write<float>(&vec.w);
		}

		void serialize(RawMemory& memory, const Vec3& vec)
		{
			// Target
			//   X    -> float
			//   Y    -> float
			//   Z    -> float
			memory.write<float>(&vec.x);
			memory.write<float>(&vec.y);
			memory.write<float>(&vec.z);
		}

		void serialize(RawMemory& memory, const Vec2& vec)
		{
			// Target
			//   X    -> float
			//   Y    -> float
			memory.write<float>(&vec.x);
			memory.write<float>(&vec.y);
		}

		void serialize(RawMemory& memory, const Vec4i& vec)
		{
			// Target
			//   X    -> i32
			//   Y    -> i32
			//   Z    -> i32
			//   W    -> i32
			memory.write<int32>(&vec.x);
			memory.write<int32>(&vec.y);
			memory.write<int32>(&vec.z);
			memory.write<int32>(&vec.w);
		}

		void serialize(RawMemory& memory, const Vec3i& vec)
		{
			// Target
			//   X    -> i32
			//   Y    -> i32
			//   Z    -> i32
			memory.write<int32>(&vec.x);
			memory.write<int32>(&vec.y);
			memory.write<int32>(&vec.z);
		}

		void serialize(RawMemory& memory, const Vec2i& vec)
		{
			// Target
			//   X    -> i32
			//   Y    -> i32
			memory.write<int32>(&vec.x);
			memory.write<int32>(&vec.y);
		}

		void serialize(RawMemory& memory, const glm::u8vec4& vec)
		{
			// Target 
			//  R -> u8
			//  G -> u8
			//  B -> u8
			//  A -> u8
			memory.write<uint8>(&vec.r);
			memory.write<uint8>(&vec.g);
			memory.write<uint8>(&vec.b);
			memory.write<uint8>(&vec.a);
		}

		Vec4 deserializeVec4(RawMemory& memory)
		{
			// Target
			//   X    -> float
			//   Y    -> float
			//   Z    -> float
			//   W    -> float
			Vec4 res;
			memory.read<float>(&res.x);
			memory.read<float>(&res.y);
			memory.read<float>(&res.z);
			memory.read<float>(&res.w);
			return res;
		}

		Vec3 deserializeVec3(RawMemory& memory)
		{
			// Target
			//   X    -> float
			//   Y    -> float
			//   Z    -> float
			Vec3 res;
			memory.read<float>(&res.x);
			memory.read<float>(&res.y);
			memory.read<float>(&res.z);
			return res;
		}

		Vec2 deserializeVec2(RawMemory& memory)
		{
			// Target
			//   X    -> float
			//   Y    -> float
			Vec2 res;
			memory.read<float>(&res.x);
			memory.read<float>(&res.y);
			return res;
		}

		Vec4i deserializeVec4i(RawMemory& memory)
		{
			// Target
			//   X    -> i32
			//   Y    -> i32
			//   Z    -> i32
			//   W    -> i32
			Vec4i res;
			memory.read<int32>(&res.x);
			memory.read<int32>(&res.y);
			memory.read<int32>(&res.z);
			memory.read<int32>(&res.w);
			return res;
		}

		Vec3i deserializeVec3i(RawMemory& memory)
		{
			// Target
			//   X    -> i32
			//   Y    -> i32
			//   Z    -> i32
			Vec3i res;
			memory.read<int32>(&res.x);
			memory.read<int32>(&res.y);
			memory.read<int32>(&res.z);
			return res;
		}

		Vec2i deserializeVec2i(RawMemory& memory)
		{
			// Target
			//   X    -> i32
			//   Y    -> i32
			Vec2i res;
			memory.read<int32>(&res.x);
			memory.read<int32>(&res.y);
			return res;
		}

		glm::u8vec4 deserializeU8Vec4(RawMemory& memory)
		{
			// Target 
			//  R -> u8
			//  G -> u8
			//  B -> u8
			//  A -> u8
			glm::u8vec4 res;
			memory.read<uint8>(&res.r);
			memory.read<uint8>(&res.g);
			memory.read<uint8>(&res.b);
			memory.read<uint8>(&res.a);
			return res;
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
				t -= (1.5f / d1);
				return n1 * t * t + .75f;
			}
			else if (t < 2.5f / d1)
			{
				t -= (2.25f / d1);
				return n1 * t * t + .9375f;
			}
			else
			{
				t -= (2.625f / d1);
				return n1 * t * t + .984375f;
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