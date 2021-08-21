#ifndef MATH_ANIM_DATA_STRUCTURES_H
#define MATH_ANIM_DATA_STRUCTURES_H

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

	namespace CMath
	{
		float lengthSquared(const Vec2& vec);
		float lengthSquared(const Vec3& vec);
		float lengthSquared(const Vec4& vec);

		float length(const Vec2& vec);
		float length(const Vec3& vec);
		float length(const Vec4& vec);

		Vec2 normalize(const Vec2& vec);
		Vec3 normalize(const Vec3& vec);
		Vec4 normalize(const Vec4& vec);
	}
}

#endif