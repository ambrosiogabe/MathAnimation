#include "physics/Physics.h"
#include "math/CMath.h"

#include "renderer/Renderer.h"
#include "renderer/Colors.h"

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

		RaycastResult rayIntersectsTorus(const Ray& rayGlobal, const Torus& torus)
		{
			// Transform the global ray into the torus local space
			glm::mat4 transformation = CMath::transformationFrom(
				torus.forward,
				torus.up,
				Vec3{0, 0, 0} // We'll translate the point manually
			);
			glm::mat4 inverseTransform = glm::inverse(transformation);

			// Translate the origin into local space
			glm::vec4 rayLocalOrigin = glm::vec4(CMath::convert(rayGlobal.origin), 1.0f) - glm::vec4(CMath::convert(torus.center), 0.0f);
			// Rotate the origin into local space
			rayLocalOrigin = inverseTransform * rayLocalOrigin;
			// Rotate the direction into local space
			glm::vec4 rayLocalDirection = glm::normalize(inverseTransform * glm::vec4(CMath::convert(rayGlobal.direction), 1.0f));
			Ray ray = createRay(
				CMath::vector3From4(CMath::convert(rayLocalOrigin)),
				CMath::vector3From4(CMath::convert(rayLocalOrigin + rayLocalDirection * rayGlobal.length))
			);

			RaycastResult res = {};
			res.hitFlags = RaycastHit::None;
			res.hitEntryDistance = FLT_MAX;
			res.hitExitDistance = FLT_MAX;

			// NOTE: The following is adapted from https://github.com/marcin-chwedczuk/ray_tracing_torus_js/blob/master/app/scripts/Torus.js
			// NOTE: Article is here http://blog.marcinchwedczuk.pl/ray-tracing-torus

			// Set up the coefficients of a quartic equation for the intersection
			// of the parametric equation P = vantage + u*direction and the 
			// surface of the torus.
			// There is far too much algebra to explain here.
			// See the text of the tutorial for derivation.

			double uArray[4];

			double tubeRadius = (torus.outerRadius - torus.innerRadius) / 2.0f;
			double sweptRadius = torus.innerRadius + tubeRadius;

			const double T = 4.0 * sweptRadius * sweptRadius;
			const double G = T * (ray.direction.x * ray.direction.x + ray.direction.y * ray.direction.y);
			const double H = 2.0 * T * (ray.origin.x * ray.direction.x + ray.origin.y * ray.direction.y);
			const double I = T * (ray.origin.x * ray.origin.x + ray.origin.y * ray.origin.y);
			const double J = CMath::lengthSquared(ray.direction);
			const double K = 2.0 * CMath::dot(ray.origin, ray.direction);
			const double L = CMath::lengthSquared(ray.origin) + sweptRadius * sweptRadius - tubeRadius * tubeRadius;

			const int numRealRoots = CMath::solveQuarticEquation(
				J * J,                     // coefficient of u^4
				2.0 * J * K,                // coefficient of u^3
				2.0 * J * L + K * K - G,      // coefficient of u^2
				2.0 * K * L - H,            // coefficient of u^1 = u
				L * L - I,                 // coefficient of u^0 = constant term
				uArray                   // receives 0..4 real solutions
			);

			// Find smallest and largest roots for exit and entrance
			const double SURFACE_TOLERANCE = 1.0e-4;
			// Initialize roots to out of bounds
			double minT = ray.length + 1.0;
			double maxT = -1.0;
			for (int i = 0; i < numRealRoots; ++i)
			{
				// Compact the array...
				if (uArray[i] > SURFACE_TOLERANCE && uArray[i] < minT)
				{
					minT = uArray[i];
				}

				if (uArray[i] > SURFACE_TOLERANCE && uArray[i] > maxT)
				{
					maxT = uArray[i];
				}
			}

			addTMinToRaycastResult(res, ray, (float)minT);
			addTMaxToRaycastResult(res, ray, (float)maxT);

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
