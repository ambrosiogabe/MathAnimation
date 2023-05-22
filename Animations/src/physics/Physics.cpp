#include "physics/Physics.h"
#include "math/CMath.h"

namespace MathAnim
{
	namespace Physics
	{
		// ------------- Internal Functions -------------
		static void addTMinToRaycastResult(RaycastResult& res, const Ray& ray, float tmin);
		static void addTMaxToRaycastResult(RaycastResult& res, const Ray& ray, float tmax);

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
			res.hitEntryDistance = FLT_MAX;
			res.hitExitDistance = FLT_MAX;

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
			addTMinToRaycastResult(res, ray, tmin);
			addTMaxToRaycastResult(res, ray, tmax);

			return res;
		}

		RaycastResult rayIntersectsSphere(const Ray& ray, const Sphere& sphere)
		{
			RaycastResult res = {};
			res.hitFlags = RaycastHit::None;
			res.hitEntryDistance = FLT_MAX;
			res.hitExitDistance = FLT_MAX;

			// For a diagram illustrating what all these variable names mean, see this linked image:
			// https://github.com/ambrosiogabe/MathAnimation/tree/master/.github/images/sphereRaycastDiagram.PNG
			Vec3 cVec = sphere.center - ray.origin;
			float a = CMath::dot(cVec, ray.direction);
			if (a < 0.0f) return res;

			float c = CMath::length(cVec);
			float b = glm::sqrt((c * c) - (a * a));
			if (b > sphere.radius) return res;

			float dt = glm::sqrt((sphere.radius * sphere.radius) - (b * b));

			float tmin = a - dt;
			float tmax = a + dt;

			// Swap if needed
			if (tmin > tmax)
			{
				float tmp = tmin;
				tmin = tmax;
				tmax = tmp;
			}

			addTMinToRaycastResult(res, ray, tmin);
			addTMaxToRaycastResult(res, ray, tmax);

			return res;
		}

		RaycastResult rayIntersectsTorus(const Ray& ray, const Torus& torus)
		{
			RaycastResult res = {};
			res.hitFlags = RaycastHit::None;
			res.hitEntryDistance = FLT_MAX;
			res.hitExitDistance = FLT_MAX;

			// For a diagram illustrating what all these variable names mean, see this linked image:
			// https://github.com/ambrosiogabe/MathAnimation/tree/master/.github/images/sphereRaycastDiagram.PNG
			Vec3 cVec = torus.center - ray.origin;
			float a = CMath::dot(cVec, ray.direction);
			if (a < 0.0f) return res;

			float c = CMath::length(cVec);
			float b = glm::sqrt((c * c) - (a * a));
			if (b > torus.outerRadius || b < torus.innerRadius) return res;

			// TODO: This needs to be modified, definitely wrong
			float dt = glm::sqrt((torus.outerRadius * torus.outerRadius) - (b * b));

			float tmin = a - dt;
			float tmax = a + dt;

			// Swap if needed
			if (tmin > tmax)
			{
				float tmp = tmin;
				tmin = tmax;
				tmax = tmp;
			}

			addTMinToRaycastResult(res, ray, tmin);
			addTMaxToRaycastResult(res, ray, tmax);

			return res;
		}

		// ------------- Internal Functions -------------
		static void addTMinToRaycastResult(RaycastResult& res, const Ray& ray, float tmin)
		{
			if (tmin >= 0.0f && tmin <= ray.length)
			{
				res.hitFlags = res.hitFlags | RaycastHit::HitEnter;
				res.entry = ray.origin + (ray.direction * tmin);
				res.hitEntryDistance = tmin;
			}
		}

		static void addTMaxToRaycastResult(RaycastResult& res, const Ray& ray, float tmax)
		{
			if (tmax >= 0.0f && tmax <= ray.length)
			{
				res.hitFlags = res.hitFlags | RaycastHit::HitExit;
				res.exit = ray.origin + (ray.direction * tmax);
				res.hitExitDistance = tmax;
			}
		}
	}
}
