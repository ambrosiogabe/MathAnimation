#ifndef MATH_ANIM_DATA_STRUCTURES_H
#define MATH_ANIM_DATA_STRUCTURES_H

#include "cppUtils/cppUtils.hpp"

namespace MathAnim
{

	struct Vec2
	{
		float x;
		float y;
	};

	struct Vec3
	{
		union
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
	};

	struct Vec4
	{
		union
		{
			float values[4];
			struct
			{
				float w;
				float x;
				float y;
				float z;
			};
			struct
			{
				float r;
				float g;
				float b;
				float a;
			};
		};
	};

	struct Vec2i
	{
		union
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
	};

	struct Vec3i
	{
		union
		{
			int32 values[3];
			struct
			{
				int32 x;
				int32 y;
				int32 z;
			};
		};
	};

	struct Vec4i
	{
		union
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
	};

	Vec2 operator+(const Vec2& a, const Vec2& b);
	Vec2 operator-(const Vec2& a, const Vec2& b);
	Vec2 operator*(const Vec2& a, float scale);
	Vec2 operator/(const Vec2& a, float scale);
	Vec2 operator*(float scale, const Vec2& a);
	Vec2 operator/(float scale, const Vec2& a);
	Vec2 operator*=(const Vec2& a, float scale);
	Vec2 operator/=(const Vec2& a, float scale);
	Vec2 operator+=(const Vec2& a, const Vec2& b);
	Vec2 operator-=(const Vec2& a, const Vec2& b);

	Vec3 operator+(const Vec3& a, const Vec3& b);
	Vec3 operator-(const Vec3& a, const Vec3& b);
	Vec3 operator*(const Vec3& a, float scale);
	Vec3 operator/(const Vec3& a, float scale);
	Vec3 operator*(float scale, const Vec3& a);
	Vec3 operator/(float scale, const Vec3& a);
	Vec3 operator*=(const Vec3& a, float scale);
	Vec3 operator/=(const Vec3& a, float scale);
	Vec3 operator+=(const Vec3& a, const Vec3& b);
	Vec3 operator-=(const Vec3& a, const Vec3& b);

	Vec4 operator+(const Vec4& a, const Vec4& b);
	Vec4 operator-(const Vec4& a, const Vec4& b);
	Vec4 operator*(const Vec4& a, float scale);
	Vec4 operator/(const Vec4& a, float scale);
	Vec4 operator*(float scale, const Vec4& a);
	Vec4 operator/(float scale, const Vec4& a);
	Vec4 operator*=(const Vec4& a, float scale);
	Vec4 operator/=(const Vec4& a, float scale);
	Vec4 operator+=(const Vec4& a, const Vec4& b);
	Vec4 operator-=(const Vec4& a, const Vec4& b);

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
	Vec2i operator*=(const Vec2i& a, int32 scale);
	Vec2i operator/=(const Vec2i& a, int32 scale);
	Vec2i operator+=(const Vec2i& a, const Vec2i& b);
	Vec2i operator-=(const Vec2i& a, const Vec2i& b);

	Vec3i operator+(const Vec3i& a, const Vec3i& b);
	Vec3i operator-(const Vec3i& a, const Vec3i& b);
	Vec3i operator*(const Vec3i& a, int32 scale);
	Vec3i operator/(const Vec3i& a, int32 scale);
	Vec3i operator*(int32 scale, const Vec3i& a);
	Vec3i operator/(int32 scale, const Vec3i& a);
	Vec3i operator*=(const Vec3i& a, int32 scale);
	Vec3i operator/=(const Vec3i& a, int32 scale);
	Vec3i operator+=(const Vec3i& a, const Vec3i& b);
	Vec3i operator-=(const Vec3i& a, const Vec3i& b);

	Vec4i operator+(const Vec4i& a, const Vec4i& b);
	Vec4i operator-(const Vec4i& a, const Vec4i& b);
	Vec4i operator*(const Vec4i& a, int32 scale);
	Vec4i operator/(const Vec4i& a, int32 scale);
	Vec4i operator*(int32 scale, const Vec4i& a);
	Vec4i operator/(int32 scale, const Vec4i& a);
	Vec4i operator*=(const Vec4i& a, int32 scale);
	Vec4i operator/=(const Vec4i& a, int32 scale);
	Vec4i operator+=(const Vec4i& a, const Vec4i& b);
	Vec4i operator-=(const Vec4i& a, const Vec4i& b);

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

#endif