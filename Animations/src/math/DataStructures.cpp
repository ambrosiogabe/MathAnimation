#include "math/DataStructures.h"
#include "core.h"

namespace MathAnim
{
	Vec2 operator+(const Vec2& a, const Vec2& b)
	{
		return {
			a.x + b.x,
			a.y + b.y
		};
	}

	Vec2 operator-(const Vec2& a, const Vec2& b)
	{
		return {
			a.x - b.x,
			a.y - b.y
		};
	}

	Vec2 operator*(const Vec2& a, float scale)
	{
		return {
			a.x * scale,
			a.y * scale
		};
	}

	Vec2 operator/(const Vec2& a, float scale)
	{
		return {
			a.x / scale,
			a.y / scale
		};
	}

	Vec2 operator*(float scale, const Vec2& a) 
	{
		return a * scale;
	}

	Vec2 operator/(float scale, const Vec2& a) 
	{
		return a / scale;
	}

	Vec2 operator*=(const Vec2& a, float scale)
	{
		return a * scale;
	}

	Vec2 operator/=(const Vec2& a, float scale)
	{
		return a / scale;
	}

	Vec2 operator+=(const Vec2& a, const Vec2& b) 
	{
		return a + b;
	}

	Vec2 operator-=(const Vec2& a, const Vec2& b) 
	{
		return a - b;
	}


	Vec3 operator+(const Vec3& a, const Vec3& b)
	{
		return {
			a.x + b.x,
			a.y + b.y,
			a.z + b.z
		};
	}

	Vec3 operator-(const Vec3& a, const Vec3& b)
	{
		return {
			a.x - b.x,
			a.y - b.y,
			a.z - b.z
		};
	}

	Vec3 operator*(const Vec3& a, float scale)
	{
		return {
			a.x * scale,
			a.y * scale,
			a.z * scale
		};
	}

	Vec3 operator/(const Vec3& a, float scale)
	{
		return {
			a.x / scale,
			a.y / scale,
			a.z / scale
		};
	}

	Vec3 operator*=(const Vec3& a, float scale)
	{
		return a * scale;
	}

	Vec3 operator*(float scale, const Vec3& a)
	{
		return a * scale;
	}

	Vec3 operator/(float scale, const Vec3& a)
	{
		return a / scale;
	}

	Vec3 operator/=(const Vec3& a, float scale)
	{
		return a / scale;
	}

	Vec3 operator+=(const Vec3& a, const Vec3& b) 
	{
		return a + b;
	}

	Vec3 operator-=(const Vec3& a, const Vec3& b) 
	{
		return a - b;
	}


	Vec4 operator+(const Vec4& a, const Vec4& b)
	{
		return {
			a.w + b.w,
			a.x + b.x,
			a.y + b.y,
			a.z + b.z
		};
	}

	Vec4 operator-(const Vec4& a, const Vec4& b)
	{
		return {
			a.w - b.w,
			a.x - b.x,
			a.y - b.y,
			a.z - b.z
		};
	}

	Vec4 operator*(const Vec4& a, float scale)
	{
		return {
			a.w * scale,
			a.x * scale,
			a.y * scale,
			a.z * scale
		};
	}

	Vec4 operator/(const Vec4& a, float scale)
	{
		return {
			a.w / scale,
			a.x / scale,
			a.y / scale,
			a.z / scale
		};
	}

	Vec4 operator*(float scale, const Vec4& a)
	{
		return a * scale;
	}

	Vec4 operator/(float scale, const Vec4& a)
	{
		return a / scale;
	}

	Vec4 operator*=(const Vec4& a, float scale)
	{
		return a * scale;
	}

	Vec4 operator/=(const Vec4& a, float scale)
	{
		return a / scale;
	}

	Vec4 operator+=(const Vec4& a, const Vec4& b) 
	{
		return a + b;
	}

	Vec4 operator-=(const Vec4& a, const Vec4& b) 
	{
		return a - b;
	}

	bool operator==(const Vec4& a, const Vec4& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
	}

	bool operator==(const Vec3& a, const Vec3& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	bool operator==(const Vec2& a, const Vec2& b)
	{
		return a.x == b.x && a.y == b.y;
	}

	bool operator!=(const Vec4& a, const Vec4& b)
	{
		return !(a == b);
	}

	bool operator!=(const Vec3& a, const Vec3& b)
	{
		return !(a == b);
	}

	bool operator!=(const Vec2& a, const Vec2& b)
	{
		return !(a == b);
	}

	Vec2i operator+(const Vec2i& a, const Vec2i& b)
	{
		return Vec2i{ a.x + b.x, a.y + b.y };
	}

	Vec2i operator-(const Vec2i& a, const Vec2i& b)
	{
		return Vec2i{ a.x - b.x, a.y - b.y };
	}

	Vec2i operator*(const Vec2i& a, int32 scale)
	{
		return Vec2i{ a.x * scale , a.y * scale };
	}

	Vec2i operator/(const Vec2i& a, int32 scale)
	{
		return Vec2i{ a.x / scale, a.y / scale };
	}

	Vec2i operator*(int32 scale, const Vec2i& a) 
	{
		return Vec2i{ a.x * scale , a.y * scale };
	}

	Vec2i operator/(int32 scale, const Vec2i& a) 
	{
		return Vec2i{ a.x / scale, a.y / scale };
	}

	Vec2i operator*=(const Vec2i& a, int32 scale)
	{
		return a * scale;
	}

	Vec2i operator/=(const Vec2i& a, int32 scale)
	{
		return a / scale;
	}

	Vec2i operator+=(const Vec2i& a, const Vec2i& b)
	{
		return a + b;
	}

	Vec2i operator-=(const Vec2i& a, const Vec2i& b)
	{
		return a - b;
	}

	Vec3i operator+(const Vec3i& a, const Vec3i& b)
	{
		return Vec3i{ a.x + b.x, a.y + b.y, a.z + b.z };
	}

	Vec3i operator-(const Vec3i& a, const Vec3i& b)
	{
		return Vec3i{ a.x - b.x, a.y - b.y, a.z - b.z };
	}

	Vec3i operator*(const Vec3i& a, int32 scale)
	{
		return Vec3i{ a.x * scale, a.y * scale, a.z * scale };
	}

	Vec3i operator/(const Vec3i& a, int32 scale)
	{
		return Vec3i{ a.x / scale, a.y / scale, a.z / scale };
	}

	Vec3i operator*(int32 scale, const Vec3i& a)
	{
		return a * scale;
	}

	Vec3i operator/(int32 scale, const Vec3i& a)
	{
		return a / scale;
	}

	Vec3i operator*=(const Vec3i& a, int32 scale)
	{
		return a * scale;
	}

	Vec3i operator/=(const Vec3i& a, int32 scale)
	{
		return a / scale;
	}

	Vec3i operator+=(const Vec3i& a, const Vec3i& b)
	{
		return a + b;
	}

	Vec3i operator-=(const Vec3i& a, const Vec3i& b) 
	{
		return a - b;
	}

	Vec4i operator+(const Vec4i& a, const Vec4i& b)
	{
		return Vec4i{ a.w + b.w, a.x + b.x, a.y + b.y, a.z + b.z };
	}

	Vec4i operator-(const Vec4i& a, const Vec4i& b)
	{
		return Vec4i{ a.w - b.w, a.x - b.x, a.y - b.y, a.z - b.z };
	}

	Vec4i operator*(const Vec4i& a, int32 scale)
	{
		return Vec4i{ a.w * scale, a.x * scale, a.y * scale, a.z * scale };
	}

	Vec4i operator/(const Vec4i& a, int32 scale)
	{
		return Vec4i{ a.w / scale, a.x / scale, a.y / scale, a.z / scale };
	}

	Vec4i operator*(int32 scale, const Vec4i& a)
	{
		return a * scale;
	}

	Vec4i operator/(int32 scale, const Vec4i& a)
	{
		return a / scale;
	}

	Vec4i operator*=(const Vec4i& a, int32 scale)
	{
		return a * scale;
	}

	Vec4i operator/=(const Vec4i& a, int32 scale)
	{
		return a / scale;
	}

	Vec4i operator+=(const Vec4i& a, const Vec4i& b)
	{
		return a + b;
	}

	Vec4i operator-=(const Vec4i& a, const Vec4i& b)
	{
		return a - b;
	}

	bool operator==(const Vec4i& a, const Vec4i& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
	}

	bool operator==(const Vec3i& a, const Vec3i& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	bool operator==(const Vec2i& a, const Vec2i& b)
	{
		return a.x == b.x && a.y == b.y;
	}

	bool operator!=(const Vec4i& a, const Vec4i& b)
	{
		return !(a == b);
	}

	bool operator!=(const Vec3i& a, const Vec3i& b)
	{
		return !(a == b);
	}

	bool operator!=(const Vec2i& a, const Vec2i& b)
	{
		return !(a == b);
	}

	namespace CMath
	{
		float lengthSquared(const Vec2& vec) 
		{
			return vec.x * vec.x + vec.y * vec.y;
		}

		float lengthSquared(const Vec3& vec) 
		{
			return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
		}

		float lengthSquared(const Vec4& vec) 
		{
			return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
		}

		int lengthSquared(const Vec2i& vec)
		{
			return vec.x * vec.x + vec.y * vec.y;
		}

		int lengthSquared(const Vec3i& vec)
		{
			return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
		}

		int lengthSquared(const Vec4i& vec)
		{
			return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
		}

		float length(const Vec2& vec) 
		{
			return glm::sqrt(vec.x * vec.x + vec.y * vec.y);
		}

		float length(const Vec3& vec) 
		{
			return glm::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
		}

		float length(const Vec4& vec) 
		{
			return glm::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w);
		}

		float length(const Vec2i& vec)
		{
			return glm::sqrt((float)(vec.x * vec.x + vec.y * vec.y));
		}

		float length(const Vec3i& vec)
		{
			return glm::sqrt((float)(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z));
		}

		float length(const Vec4i& vec)
		{
			return glm::sqrt((float)(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w));
		}

		Vec2 normalize(const Vec2& vec) 
		{
			return vec * glm::inversesqrt(lengthSquared(vec));
		}

		Vec3 normalize(const Vec3& vec) 
		{
			return vec * glm::inversesqrt(lengthSquared(vec));
		}

		Vec4 normalize(const Vec4& vec) 
		{
			return vec * glm::inversesqrt(lengthSquared(vec));
		}
	}
}