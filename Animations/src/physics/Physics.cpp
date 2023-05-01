#include "physics/Physics.h"
#include "math/CMath.h"

namespace MathAnim
{
	namespace Physics
	{
		Ray createRay(const Vec3& start, const Vec3& end)
		{
			Ray res = {};
			res.origin = start;
			res.direction = CMath::normalize(end - start);
			res.length = CMath::length(start - end);

			return res;
		}

		AABB createAABB(const Vec3& center, const Vec3& size)
		{
			AABB res = {};
			res.min = center - (size / 2.0f);
			res.max = center + (size / 2.0f);

			return res;
		}

		RaycastResult rayIntersectsAABB(const Ray& ray, const AABB& aabb)
		{
			RaycastResult res = {};
			res.hitFlags = RaycastHit::None;

			float tmin = (aabb.min.x - ray.origin.x) / ray.direction.x;
			float tmax = (aabb.max.x - ray.origin.x) / ray.direction.x;

			if (tmin > tmax)
			{
				// Swap
				float tmp = tmin;
				tmin = tmax;
				tmax = tmp;
			}

			float tymin = (aabb.min.y - ray.origin.y) / ray.direction.y;
			float tymax = (aabb.max.y - ray.origin.y) / ray.direction.y;

			if (tymin > tymax)
			{
				// Swap 
				float tmp = tymin;
				tymin = tymax;
				tymax = tmp;
			}

			if ((tmin > tymax) || (tymin > tmax))
			{
				return res;
			}

			if (tymin > tmin)
			{
				tmin = tymin;
			}

			if (tymax < tmax)
			{
				tmax = tymax;
			}

			float tzmin = (aabb.min.z - ray.origin.z) / ray.direction.z;
			float tzmax = (aabb.max.z - ray.origin.z) / ray.direction.z;

			if (tzmin > tzmax)
			{
				// Swap
				float tmp = tzmin;
				tzmin = tzmax;
				tzmax = tmp;
			}

			if ((tmin > tzmax) || (tzmin > tmax))
			{
				return res;
			}

			if (tzmin > tmin)
			{
				tmin = tzmin;
			}

			if (tzmax < tmax)
			{
				tmax = tzmax;
			}

			// NOTE: tmin and tmax now hold the entry and exit points
			if (tmin >= 0.0f && tmin <= ray.length)
			{
				res.hitFlags = res.hitFlags | RaycastHit::HitEnter;
				res.entry = ray.origin + (ray.direction * tmin);
			}

			if (tmax >= 0.0f && tmax <= ray.length)
			{
				res.hitFlags = res.hitFlags | RaycastHit::HitExit;
				res.exit = ray.origin + (ray.direction * tmax);
			}

			return res;
		}
	}
}
