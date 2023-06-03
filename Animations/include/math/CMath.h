#ifndef MATH_ANIM_C_MATH_H
#define MATH_ANIM_C_MATH_H

#include "core.h"

#include <nlohmann/json_fwd.hpp>
#include <complex>

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

	constexpr auto easeTypeNames = fixedSizeArray<const char*, (size_t)EaseType::Length>(
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
	);

	enum class EaseDirection : uint8
	{
		None,
		In,
		Out,
		InOut,
		Length
	};

	constexpr auto easeDirectionNames = fixedSizeArray<const char*, (size_t)EaseDirection::Length>(
		"None",
		"In",
		"Out",
		"In-Out"
	);

	namespace CMath
	{
		constexpr float PI = 3.1415926535897932384626433832795028841971693993751058209749445923078164062f;

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

		// Implicit conversions from glm::vec to Vec
		inline Vec2 convert(const glm::vec2& vec)
		{
			return Vec2{vec.x, vec.y};
		}

		inline Vec3 convert(const glm::vec3& vec)
		{
			return Vec3{vec.x, vec.y, vec.z};
		}

		inline Vec4 convert(const glm::vec4& vec)
		{
			return Vec4{vec.x, vec.y, vec.z, vec.w};
		}

		// Winding order stuff
		bool isClockwise(const Vec2& p0, const Vec2& p1, const Vec2& p2);
		inline bool isCounterClockwise(const Vec2& p0, const Vec2& p1, const Vec2& p2) { return !isClockwise(p0, p1, p2); }

		bool isClockwise(const Vec3& p0, const Vec3& p1, const Vec3& p2);
		inline bool isCounterClockwise(const Vec3& p0, const Vec3& p1, const Vec3& p2) { return !isClockwise(p0, p1, p2); }

		// Float Comparison functions, using custom epsilon
		bool compare(float x, float y, float epsilon = std::numeric_limits<float>::min());
		bool compare(std::complex<double> x, std::complex<double> y, double epsilon = std::numeric_limits<double>::min());
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

		template<>
		inline uint64 combineHash(const uint64& t, uint64 hash)
		{
			// Taken from https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values
			return (hash ^ ((uint64)t + 0x9e3779b9 + (hash << 6) + (hash >> 2)));
		}

		// Vector conversions
		inline Vec2 vector2From3(const Vec3& vec) { return Vec2{ vec.x, vec.y }; }
		inline Vec3 vector3From2(const Vec2& vec) { return Vec3{ vec.x, vec.y, 0.0f }; }
		inline Vec3 vector3From4(const Vec4& vec) { return Vec3{ vec.x, vec.y, vec.z }; }

		// ----------- Geometry helpers -----------

		void rotate(Vec2& vec, float angleDeg, const Vec2& origin);
		void rotate(Vec3& vec, float angleDeg, const Vec3& origin);

		/**
		 * @brief Calculates the angle between vectors a and b
		 * @param a 
		 * @param b 
		 * @return Angle in radians between a and b
		*/
		float angleBetween(const Vec3& a, const Vec3& b);

		constexpr inline float toRadians(float degrees) { return degrees * PI / 180.0f; }
		constexpr inline float toDegrees(float radians) { return radians * 180.0f / PI; }

		// ----------- Linear transformation helpers -----------
		
		float mapRange(float val, float inMin, float inMax, float outMin, float outMax);
		float mapRange(const Vec2& inputRange, const Vec2& outputRange, float value);

		inline float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
		inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

		inline float abs(float a) { return a > 0 ? a : -a; }
		inline Vec2 abs(const Vec2& a) { return Vec2{ abs(a.x), abs(a.y) }; }
		inline Vec3 abs(const Vec3& a) { return Vec3{ abs(a.x), abs(a.y), abs(a.z)}; }

		/**
		* @brief Solves a quadratic equation of the form aX ^ 2 + bY + c = 0
		*
		* @param a
		* @param b
		* @param c
		* @param roots Contains the real roots of the equation. Values are initialized to NaN.
		* @return Returns the number of real roots
		*/
		int solveQuadraticEquation(double a, double b, double c, double roots[2]);

		/**
		* @brief Solves a cubic equation of the form aX ^ 3 + bY ^ 2 + cZ ^ 1 + d = 0
		*
		* @param a 
		* @param b 
		* @param c 
		* @param d 
		* @param roots Contains the real roots of the equation. Values are initialized to NaN.
		* @return Returns the number of real roots
		*/
		int solveCubicEquation(double a, double b, double c, double d, double roots[3]);

		/** 
		* @brief Solves a quartic equation of the form aX ^ 4 + bY ^ 3 + cZ ^ 2 + dW ^ 1 + e = 0
		* 
		* @param a The coefficient a
		* @param b The coefficient b
		* @param c The coefficient c
		* @param d The coefficient d
		* @param e The coefficient e
		* @param roots Contains the real roots of the equation. Values are initialized to NaN.
		* @return Returns the number of real roots
		*/
		int solveQuarticEquation(double a, double b, double c, double d, double e, double roots[4]);

		/**
		* @brief Solves cross product between vectors a x b. Follows the traditional definition of a 
		* mathematical cross product.
		*
		* @param a The vector a
		* @param b The vector b
		* @return Returns the result of a x b
		*/
		inline Vec3 cross(const Vec3& a, const Vec3& b)
		{
			return Vec3{
				(a.values[1] * b.values[2]) - (a.values[2] * b.values[1]),
				(a.values[2] * b.values[0]) - (a.values[0] * b.values[2]),
				(a.values[0] * b.values[1]) - (a.values[1] * b.values[0]),
			};
		}

		// ----------- Max/min helpers -----------

		/**
		 * @param a 
		 * @param b 
		 * @return Returns maximum of a and b
		*/
		inline int max(int a, int b) { return a > b ? a : b; }
		/**
		 * @param a 
		 * @param b 
		 * @return Returns minimum of a and b
		*/
		inline int min(int a, int b) { return a < b ? a : b; }

		/**
		 * @brief Returns the component-wise max of a and b
		 * @param a Vector a
		 * @param b Vector b
		 * @return Vec2{max(a.x, b.x), max(a.y, b.y)}
		*/
		Vec2 max(const Vec2& a, const Vec2& b);
		/**
		 * @brief Returns the component-wise min of a and b
		 * @param a Vector a
		 * @param b Vector b
		 * @return Vec2{min(a.x, b.x), min(a.y, b.y)}
		*/
		Vec2 min(const Vec2& a, const Vec2& b);
		/**
		 * @brief Returns the component-wise max of a and b
		 * @param a Vector a
		 * @param b Vector b
		 * @return Vec3{max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)}
		*/
		Vec3 max(const Vec3& a, const Vec3& b);
		/**
		 * @brief Returns the component-wise min of a and b
		 * @param a Vector a
		 * @param b Vector b
		 * @return Vec3{min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)}
		*/
		Vec3 min(const Vec3& a, const Vec3& b);
		/**
		 * @brief Returns the component-wise max of a and b
		 * @param a Vector a
		 * @param b Vector b
		 * @return Vec4{max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)}
		*/
		Vec4 max(const Vec4& a, const Vec4& b);
		/**
		 * @brief Returns the component-wise min of a and b
		 * @param a Vector a
		 * @param b Vector b
		 * @return Vec4{min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)}
		*/
		Vec4 min(const Vec4& a, const Vec4& b);

		// ----------- String hashing -----------

		/**
		 * @brief Returns a 32-bit hash of the null-terminated c string.
		 * @param str The string to hash
		 * @return 32-bit hash
		*/
		uint32 hashString(const char* str);

		// ----------- Bezier Helpers -----------

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
		
		/**
		 * @brief Finds bezier extremities for a quadratic bezier curve
		 * @param p0 Start point quadratic curve
		 * @param p1 Handle point
		 * @param p2 End point quadratic curve
		 * @return Pair Vec2{ xRoot, yRoot } in tValues. -1.0f indicates an invalid root.
		*/
		Vec2 tRootBezier2(const Vec2& p0, const Vec2& p1, const Vec2& p2);

		/**
		 * @brief Finds bezier extremities for a cubic bezier curve
		 * @param p0 Start point cubic curve
		 * @param p1 First handle point
		 * @param p2 Second handle point
		 * @param p3 End point cubic curve
		 * @return Pair Vec4{ xRoot, yRoot, xRootNeg, yRootNeg } in tValues. -1.0f indicates an invalid root.
		*/
		Vec4 tRootsBezier3(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3);

		BBox bezier1BBox(const Vec2& p0, const Vec2& p1);
		BBox bezier2BBox(const Vec2& p0, const Vec2& p1, const Vec2& p2);
		BBox bezier3BBox(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3);

		// ----------- Easing Functions -----------

		float ease(float t, EaseType type, EaseDirection direction);

		
		// ----------- Interpolation Functions -----------

		Vec4 interpolate(float t, const Vec4& src, const Vec4& target);
		Vec3 interpolate(float t, const Vec3& src, const Vec3& target);
		Vec2 interpolate(float t, const Vec2& src, const Vec2& target);
		glm::u8vec4 interpolate(float t, const glm::u8vec4& src, const glm::u8vec4& target);
		float interpolate(float t, float src, float target);

		// ----------- Transformation Helpers -----------

		glm::mat4 transformationFrom(const Vec3& forward, const Vec3& up, const Vec3& position);
		glm::mat4 calculateTransform(const Vec3& eulerAnglesRotation, const Vec3& scale, const Vec3& position);
		Vec3 extractPosition(const glm::mat4& transformation);

		// ----------- (De)Serialization -----------

		void serialize(nlohmann::json& j, const char* propertyName, const Vec4& vec);
		void serialize(nlohmann::json& j, const char* propertyName, const Vec3& vec);
		void serialize(nlohmann::json& j, const char* propertyName, const Vec2& vec);

		void serialize(nlohmann::json& j, const char* propertyName, const Vec4i& vec);
		void serialize(nlohmann::json& j, const char* propertyName, const Vec3i& vec);
		void serialize(nlohmann::json& j, const char* propertyName, const Vec2i& vec);

		void serialize(nlohmann::json& j, const char* propertyName, const glm::u8vec4& vec);
		void serialize(nlohmann::json& j, const char* propertyName, const glm::quat& quat);

		Vec4 deserializeVec4(const nlohmann::json& j, const Vec4& defaultValue);
		Vec3 deserializeVec3(const nlohmann::json& j, const Vec3& defaultValue);
		Vec2 deserializeVec2(const nlohmann::json& j, const Vec2& defaultValue);

		Vec4i deserializeVec4i(const nlohmann::json& j, const Vec4i& defaultValue);
		Vec3i deserializeVec3i(const nlohmann::json& j, const Vec3i& defaultValue);
		Vec2i deserializeVec2i(const nlohmann::json& j, const Vec2i& defaultValue);

		glm::u8vec4 deserializeU8Vec4(const nlohmann::json& memory, const glm::u8vec4& defaultValue);
		glm::quat deserializeQuat(const nlohmann::json& j, const glm::quat& defaultValue);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		Vec4 legacy_deserializeVec4(RawMemory& memory);
		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		Vec3 legacy_deserializeVec3(RawMemory& memory);
		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		Vec2 legacy_deserializeVec2(RawMemory& memory);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		Vec4i legacy_deserializeVec4i(RawMemory& memory);
		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		Vec3i legacy_deserializeVec3i(RawMemory& memory);
		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		Vec2i legacy_deserializeVec2i(RawMemory& memory);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		glm::u8vec4 legacy_deserializeU8Vec4(RawMemory& memory);
	}
}

#endif
