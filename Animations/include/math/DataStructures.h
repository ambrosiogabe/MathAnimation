#ifndef MATH_ANIM_DATA_STRUCTURES_H
#define MATH_ANIM_DATA_STRUCTURES_H

#include <cppUtils/cppPrint.hpp>
#include <cppUtils/cppUtils.hpp>

#pragma warning( push )
#pragma warning( disable : 4201 )

namespace MathAnim
{
	union Vec2
	{
		float values[2];
		struct
		{
			float x;
			float y;
		};
		struct
		{
			float min;
			float max;
		};
	};

	union Vec3
	{
		float values[3];
		struct
		{
			float x;
			float y;
			float z;
		};
		struct
		{
			float r;
			float g;
			float b;
		};
	};

	union Vec4
	{
		float values[4];
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
		struct
		{
			float r;
			float g;
			float b;
			float a;
		};
	};

	union Vec2i
	{
		int32 values[2];
		struct
		{
			int32 x;
			int32 y;
		};
		struct
		{
			int32 min;
			int32 max;
		};
	};

	union Vec3i
	{
		int32 values[3];
		struct
		{
			int32 x;
			int32 y;
			int32 z;
		};
	};

	union Vec4i
	{
		int32 values[4];
		struct
		{
			int32 w;
			int32 x;
			int32 y;
			int32 z;
		};
	};

	struct BBox
	{
		Vec2 min;
		Vec2 max;
	};

	struct BBoxi
	{
		Vec2i min;
		Vec2i max;
	};

	namespace Vector3
	{
		constexpr Vec3 Right = Vec3{ 1.0f, 0.0f, 0.0f };
		constexpr Vec3 Forward = Vec3{ 0.0f, 0.0f, 1.0f };
		constexpr Vec3 Up = Vec3{ 0.0f, 1.0f, 0.0f };

		constexpr Vec3 Left = Vec3{ -1.0f, 0.0f, 0.0f };
		constexpr Vec3 Back = Vec3{ 0.0f, 0.0f, -1.0f };
		constexpr Vec3 Down = Vec3{ 0.0f, -1.0f, 0.0f };
	}

	namespace Vector2
	{
		constexpr Vec2 Right = Vec2{ 1.0f, 0.0f };
		constexpr Vec2 Up = Vec2{ 0.0f, 1.0f };

		constexpr Vec2 Left = Vec2{ -1.0f, 0.0f };
		constexpr Vec2 Down = Vec2{ 0.0f, -1.0f };
	}

	Vec2 operator+(const Vec2& a, const Vec2& b);
	Vec2 operator-(const Vec2& a, const Vec2& b);
	Vec2 operator*(const Vec2& a, float scale);
	Vec2 operator/(const Vec2& a, float scale);
	Vec2 operator*(float scale, const Vec2& a);
	Vec2 operator/(float scale, const Vec2& a);
	Vec2 operator*(const Vec2& a, const Vec2& scale);
	Vec2 operator/(const Vec2& a, const Vec2& scale);
	Vec2& operator*=(Vec2& a, float scale);
	Vec2& operator/=(Vec2& a, float scale);
	Vec2& operator*=(Vec2& a, const Vec2& scale);
	Vec2& operator/=(Vec2& a, const Vec2& scale);
	Vec2& operator+=(Vec2& a, const Vec2& b);
	Vec2& operator-=(Vec2& a, const Vec2& b);

	Vec3 operator+(const Vec3& a, const Vec3& b);
	Vec3 operator-(const Vec3& a, const Vec3& b);
	Vec3 operator*(const Vec3& a, float scale);
	Vec3 operator/(const Vec3& a, float scale);
	Vec3 operator*(float scale, const Vec3& a);
	Vec3 operator/(float scale, const Vec3& a);
	Vec3 operator*(const Vec3& a, const Vec3& scale);
	Vec3 operator/(const Vec3& a, const Vec3& scale);
	Vec3& operator*=(Vec3& a, float scale);
	Vec3& operator/=(Vec3& a, float scale);
	Vec3& operator*=(Vec3& a, const Vec3& scale);
	Vec3& operator/=(Vec3& a, const Vec3& scale);
	Vec3& operator+=(Vec3& a, const Vec3& b);
	Vec3& operator-=(Vec3& a, const Vec3& b);

	Vec4 operator+(const Vec4& a, const Vec4& b);
	Vec4 operator-(const Vec4& a, const Vec4& b);
	Vec4 operator*(const Vec4& a, float scale);
	Vec4 operator/(const Vec4& a, float scale);
	Vec4 operator*(float scale, const Vec4& a);
	Vec4 operator/(float scale, const Vec4& a);
	Vec4 operator*(const Vec4& a, const Vec4& scale);
	Vec4 operator/(const Vec4& a, const Vec4& scale);
	Vec4& operator*=(Vec4& a, float scale);
	Vec4& operator/=(Vec4& a, float scale);
	Vec4& operator*=(Vec4& a, const Vec4& scale);
	Vec4& operator/=(Vec4& a, const Vec4& scale);
	Vec4& operator+=(Vec4& a, const Vec4& b);
	Vec4& operator-=(Vec4& a, const Vec4& b);

	bool operator==(const Vec4& a, const Vec4& b);
	bool operator==(const Vec3& a, const Vec3& b);
	bool operator==(const Vec2& a, const Vec2& b);
	bool operator!=(const Vec4& a, const Vec4& b);
	bool operator!=(const Vec3& a, const Vec3& b);
	bool operator!=(const Vec2& a, const Vec2& b);

	Vec2i operator+(const Vec2i& a, const Vec2i& b);
	Vec2i operator-(const Vec2i& a, const Vec2i& b);
	Vec2i operator*(const Vec2i& a, int32 scale);
	Vec2i operator/(const Vec2i& a, int32 scale);
	Vec2i operator*(int32 scale, const Vec2i& a);
	Vec2i operator/(int32 scale, const Vec2i& a);
	Vec2i& operator*=(Vec2i& a, int32 scale);
	Vec2i& operator/=(Vec2i& a, int32 scale);
	Vec2i& operator+=(Vec2i& a, const Vec2i& b);
	Vec2i& operator-=(Vec2i& a, const Vec2i& b);

	Vec3i operator+(const Vec3i& a, const Vec3i& b);
	Vec3i operator-(const Vec3i& a, const Vec3i& b);
	Vec3i operator*(const Vec3i& a, int32 scale);
	Vec3i operator/(const Vec3i& a, int32 scale);
	Vec3i operator*(int32 scale, const Vec3i& a);
	Vec3i operator/(int32 scale, const Vec3i& a);
	Vec3i& operator*=(Vec3i& a, int32 scale);
	Vec3i& operator/=(Vec3i& a, int32 scale);
	Vec3i& operator+=(Vec3i& a, const Vec3i& b);
	Vec3i& operator-=(Vec3i& a, const Vec3i& b);

	Vec4i operator+(const Vec4i& a, const Vec4i& b);
	Vec4i operator-(const Vec4i& a, const Vec4i& b);
	Vec4i operator*(const Vec4i& a, int32 scale);
	Vec4i operator/(const Vec4i& a, int32 scale);
	Vec4i operator*(int32 scale, const Vec4i& a);
	Vec4i operator/(int32 scale, const Vec4i& a);
	Vec4i& operator*=(Vec4i& a, int32 scale);
	Vec4i& operator/=(Vec4i& a, int32 scale);
	Vec4i& operator+=(Vec4i& a, const Vec4i& b);
	Vec4i& operator-=(Vec4i& a, const Vec4i& b);

	bool operator==(const Vec4i& a, const Vec4i& b);
	bool operator==(const Vec3i& a, const Vec3i& b);
	bool operator==(const Vec2i& a, const Vec2i& b);
	bool operator!=(const Vec4i& a, const Vec4i& b);
	bool operator!=(const Vec3i& a, const Vec3i& b);
	bool operator!=(const Vec2i& a, const Vec2i& b);

	namespace CMath
	{
		float lengthSquared(const Vec2& vec);
		float lengthSquared(const Vec3& vec);
		float lengthSquared(const Vec4& vec);

		int lengthSquared(const Vec2i& vec);
		int lengthSquared(const Vec3i& vec);
		int lengthSquared(const Vec4i& vec);

		float length(const Vec2& vec);
		float length(const Vec3& vec);
		float length(const Vec4& vec);

		float length(const Vec2i& vec);
		float length(const Vec3i& vec);
		float length(const Vec4i& vec);

		Vec2 normalize(const Vec2& vec);
		Vec3 normalize(const Vec3& vec);
		Vec4 normalize(const Vec4& vec);
	}
}

// Print functions
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::Vec2& vec2);
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::Vec3& vec3);
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::Vec4& vec4);

CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::Vec2i& vec2);
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::Vec3i& vec3);
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::Vec4i& vec4);

CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::BBox& bbox);
CppUtils::Stream& operator<<(CppUtils::Stream& ostream, const MathAnim::BBoxi& bbox);

#pragma warning( pop )

#endif