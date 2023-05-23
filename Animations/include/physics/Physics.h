#ifndef MATH_ANIM_PHYSICS_H
#define MATH_ANIM_PHYSICS_H
#include "core.h"

namespace MathAnim
{
	struct Ray
	{
		Vec3 origin;
		Vec3 direction;
		float length;
	};

	enum class RaycastHit : uint8
	{
		None = 0,
		HitEnter = 1 << 0,
		HitExit = 1 << 1,
		HitAll = 0b11
	};
	MATH_ANIM_ENUM_FLAG_OPS(RaycastHit);

	struct RaycastResult
	{
		RaycastHit hitFlags;
		Vec3 entry;
		Vec3 exit;
		float hitEntryDistance;
		float hitExitDistance;

		inline bool hit() { return hitFlags != RaycastHit::None; }
		inline bool hitEntry() { return (hitFlags & RaycastHit::HitEnter) == RaycastHit::HitEnter; }
		inline bool hitExit() { return (hitFlags & RaycastHit::HitExit) == RaycastHit::HitExit; }
	};

	struct AABB
	{
		Vec3 min;
		Vec3 max;
	};

	struct OBB
	{
		BBox bbox;
		glm::mat4 transformation;
	};

	struct Sphere
	{
		Vec3 center;
		float radius;
	};

	struct Torus
	{
		Vec3 center;
		Vec3 forward;
		Vec3 up;
		float innerRadius;
		float outerRadius;
	};

	namespace Physics
	{
		Ray createRay(const Vec3& start, const Vec3& end);
		AABB createAABB(const Vec3& center, const Vec3& size);

		inline Sphere createSphere(const Vec3& center, float radius) { return Sphere{ center, radius }; }
		inline Torus createTorus(const Vec3& center, const Vec3& forward, const Vec3& up, float innerRadius, float outerRadius)
		{
			return Torus{ center, forward, up, innerRadius, outerRadius };
		}

		RaycastResult rayIntersectsAABB(const Ray& ray, const AABB& aabb);
		RaycastResult rayIntersectsSphere(const Ray& ray, const Sphere& sphere);
		RaycastResult rayIntersectsTorus(const Ray& ray, const Torus& torus);
	}
}

#endif 