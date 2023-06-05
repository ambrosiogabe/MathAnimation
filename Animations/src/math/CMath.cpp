#include "math/CMath.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <complex>

typedef std::complex<double> complex;

namespace MathAnim
{
	namespace CMath
	{
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

		bool compare(std::complex<double> xComplex, std::complex<double> yComplex, double epsilon)
		{
			double x = xComplex.real();
			double y = yComplex.real();
			return std::abs(x - y) <= epsilon * std::max(1.0, std::max(std::abs(x), std::abs(y)));
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

		float angleBetween(const Vec3& a, const Vec3& b, const Vec3& planeNormal)
		{
			float dp = dot(a, b);
			if (compare(dp, 0.0f))
			{
				// Vectors are parallel
				return 0.0f;
			}

			float lengthMultiplied = length(a) * length(b);
			if (compare(lengthMultiplied, 0.0f))
			{
				// Undefined
				return 0.0f;
			}

			float cosTheta = glm::clamp(dp / lengthMultiplied,  -1.0f, 1.0f);
			float angle = glm::acos(cosTheta);

			// Reverse the angle if needed
			Vec3 crossProduct = cross(a, b);
			if (dot(planeNormal, crossProduct) > 0)
			{
				angle = -angle;
			}

			return angle;
		}

		Vec3 normalizeAxisAngles(const Vec3& rotation)
		{
			Vec3 res = rotation;
			for (int i = 0; i < 3; i++)
			{
				while (res.values[i] > 360.0f)
				{
					res.values[i] -= 360.0f;
				}
				while (res.values[i] < 0.0f)
				{
					res.values[i] += 360.0f;
				}
			}

			return res;
		}

		float mapRange(float val, float inMin, float inMax, float outMin, float outMax)
		{
			return (val - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
		}

		float mapRange(const Vec2& inputRange, const Vec2& outputRange, float value)
		{
			return (value - inputRange.x) / (inputRange.y - inputRange.x) * (outputRange.y - outputRange.x) + outputRange.x;
		}

		// Returns n=0..numComplexValues, and fills in outArray with the n values from
		// inArray that are real-valued (i.e., whose imaginary parts are within TOLERANCE of 0.)
		// outArray must be large enough to receive numComplexValues values.
		static int filterRealNumbers(int numComplexValues, const complex inArray[], double outArray[])
		{
			int numRealValues = 0;
			for (int i = 0; i < numComplexValues; ++i)
			{
				if (fabs(inArray[i].imag()) < 0.0001f)
				{
					outArray[numRealValues++] = inArray[i].real();
				}
			}
			return numRealValues;
		}

		int solveQuadraticEquation(double a, double b, double c, double roots[2])
		{
			roots[0] = NAN;
			roots[1] = NAN;

			if (CMath::compare((float)a, 0.0f))
			{
				if (CMath::compare((float)b, 0.0f))
				{
					// The equation devolves to: c = 0, where the variable x has vanished!
					return 0;   // cannot divide by zero, so there is no solution.
				}
				else
				{
					// Simple linear equation: bx + c = 0, so x = -c/b.
					roots[0] = -c / b;
					return 1;   // there is a single solution.
				}
			}
			else
			{
				const double radicand = b * b - 4.0 * a * a;
				if (CMath::compare((float)radicand, 0.0f))
				{
					// Both roots have the same value: -b / 2a.
					roots[0] = -b / (2.0 * a);
					return 1;
				}
				else
				{
					// There are two distinct real roots.
					const double r = glm::sqrt(radicand);
					const double d = 2.0 * a;

					roots[0] = (-b + r) / d;
					roots[1] = (-b - r) / d;
					return 2;
				}
			}
		}

		static complex cbrt(complex a, int n)
		{
			/*
				This function returns one of the 3 complex cube roots of the complex number 'a'.
				The value of n=0..2 selects which root is returned.
			*/

			const double TWOPI = 2.0f * 3.141592653589793238462643383279502884;

			double rho = pow(abs(a), 1.0 / 3.0);
			double theta = ((TWOPI * n) + arg(a)) / 3.0;
			return complex(rho * cos(theta), rho * sin(theta));
		}

		int solveCubicEquation(double aReal, double bReal, double cReal, double dReal, double roots[3])
		{
			roots[0] = NAN;
			roots[1] = NAN;
			roots[2] = NAN;

			complex a = complex(aReal);
			complex b = complex(bReal);
			complex c = complex(cReal);
			complex d = complex(dReal);

			if (CMath::compare(a, 0.0))
			{
				return solveQuadraticEquation(bReal, cReal, dReal, roots);
			}

			b /= a;
			c /= a;
			d /= a;

			complex S = b / 3.0;
			complex D = c / 3.0 - S * S;
			complex E = S * S * S + (d - S * c) / 2.0;
			complex Froot = sqrt(E * E + D * D * D);
			complex F = -Froot - E;

			if (CMath::compare(F, 0.0))
			{
				F = Froot - E;
			}

			complex complexRoots[3];
			for (int i = 0; i < 3; ++i)
			{
				const complex G = cbrt(F, i);
				complexRoots[i] = G - D / G - S;
			}

			return filterRealNumbers(3, complexRoots, roots);
		}

		int solveQuarticEquation(double aReal, double bReal, double cReal, double dReal, double eReal , double roots[4])
		{
			roots[0] = NAN;
			roots[1] = NAN;
			roots[2] = NAN;
			roots[3] = NAN;

			if (compare((float)aReal, 0.0f))
			{
				return solveCubicEquation(bReal, cReal, dReal, eReal, roots);
			}

			complex a = complex(aReal);
			complex b = complex(bReal);
			complex c = complex(cReal);
			complex d = complex(dReal);
			complex e = complex(eReal);

			// See "Summary of Ferrari's Method" in http://en.wikipedia.org/wiki/Quartic_function

			// Without loss of generality, we can divide through by 'a'.
			// Anywhere 'a' appears in the equations, we can assume a = 1.
			b /= a;
			c /= a;
			d /= a;
			e /= a;

			complex b2 = b * b;
			complex b3 = b * b2;
			complex b4 = b2 * b2;

			complex alpha = (-3.0 / 8.0) * b2 + c;
			complex beta = b3 / 8.0 - b * c / 2.0 + d;
			complex gamma = (-3.0 / 256.0) * b4 + b2 * c / 16.0 - b * d / 4.0 + e;

			complex alpha2 = alpha * alpha;
			complex t = -b / 4.0;

			complex complexRoots[4];

			if (CMath::compare(beta, 0.0))
			{
				complex rad = sqrt(alpha2 - 4.0 * gamma);
				complex r1 = sqrt((-alpha + rad) / 2.0);
				complex r2 = sqrt((-alpha - rad) / 2.0);

				complexRoots[0] = t + r1;
				complexRoots[1] = t - r1;
				complexRoots[2] = t + r2;
				complexRoots[3] = t - r2;
			}
			else
			{
				complex alpha3 = alpha * alpha2;
				complex P = -(alpha2 / 12.0 + gamma);
				complex Q = -alpha3 / 108.0 + alpha * gamma / 3.0 - beta * beta / 8.0;
				complex R = -Q / 2.0 + sqrt(Q * Q / 4.0 + P * P * P / 27.0);
				complex U = cbrt(R, 0);
				complex y = (-5.0 / 6.0) * alpha + U;
				if (CMath::compare(U, 0.0))
				{
					y -= cbrt(Q, 0);
				}
				else
				{
					y -= P / (3.0 * U);
				}
				complex W = sqrt(alpha + 2.0 * y);

				complex r1 = sqrt(-(3.0 * alpha + 2.0 * y + 2.0 * beta / W));
				complex r2 = sqrt(-(3.0 * alpha + 2.0 * y - 2.0 * beta / W));

				complexRoots[0] = t + (W - r1) / 2.0;
				complexRoots[1] = t + (W + r1) / 2.0;
				complexRoots[2] = t + (-W - r2) / 2.0;
				complexRoots[3] = t + (-W + r2) / 2.0;
			}

			return filterRealNumbers(4, complexRoots, roots);
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

		Vec2 bezier1Normal(const Vec2& p0, const Vec2& p1, float)
		{
			return normalize(p1 - p0);
		}

		Vec2 bezier2Normal(const Vec2& p0, const Vec2& p1, const Vec2& p2, float t)
		{
			// Derivative taken from https://en.wikipedia.org/wiki/B�zier_curve#Quadratic_B�zier_curves
			// Just return the normalized derivative at point t
			return normalize(
				(2.0f * (1.0f - t) * (p1 - p0)) +
				(2.0f * t * (p2 - p1))
			);
		}

		Vec2 bezier3Normal(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, float t)
		{
			// Derivative taken from https://en.wikipedia.org/wiki/B�zier_curve#Cubic_B�zier_curves
			// Just return the normalized derivative at point t
			return normalize(
				(3.0f * (1.0f - t) * (1.0f - t) * (p1 - p0)) +
				(6.0f * (1.0f - t) * t * (p2 - p1)) +
				(3.0f * t * t * (p3 - p2))
			);
		}

		Vec3 bezier1Normal(const Vec3& p0, const Vec3& p1, float)
		{
			return normalize(p1 - p0);
		}

		Vec3 bezier2Normal(const Vec3& p0, const Vec3& p1, const Vec3& p2, float t)
		{
			// Derivative taken from https://en.wikipedia.org/wiki/B�zier_curve#Quadratic_B�zier_curves
			// Just return the normalized derivative at point t
			return normalize(
				(2.0f * (1.0f - t) * (p1 - p0)) +
				(2.0f * t * (p2 - p1))
			);
		}

		Vec3 bezier3Normal(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t)
		{
			// Derivative taken from https://en.wikipedia.org/wiki/B�zier_curve#Cubic_B�zier_curves
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

		glm::mat4 transformationFrom(const Vec3& forward, const Vec3& up, const Vec3& position)
		{
			Vec3 right = CMath::cross(up, forward);
			return glm::transpose(glm::mat4(
				glm::vec4(right.x, up.x, forward.x, 0.0f),
				glm::vec4(right.y, up.y, forward.y, 0.0f),
				glm::vec4(right.z, up.z, forward.z, 0.0f),
				glm::vec4(position.x, position.y, position.z, 1.0f)
			));
		}

		// Transformation helpers
		glm::mat4 calculateTransform(const Vec3& eulerAnglesRotation, const Vec3& scale, const Vec3& position)
		{
			glm::quat xRotation = glm::angleAxis(glm::radians(eulerAnglesRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::quat yRotation = glm::angleAxis(glm::radians(eulerAnglesRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::quat zRotation = glm::angleAxis(glm::radians(eulerAnglesRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			glm::quat finalRotation = xRotation;
			finalRotation = yRotation * finalRotation;
			finalRotation = zRotation * finalRotation;

			glm::mat4 rotation = glm::toMat4(finalRotation);
			glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), CMath::convert(scale));
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), CMath::convert(position));

			return translation * rotation * scaleMatrix;
		}

		Vec3 extractPosition(const glm::mat4& transformation)
		{
			return Vec3{
				transformation[3][0],
				transformation[3][1],
				transformation[3][2]
			};
		}

		// (de)Serialization functions
		void serialize(nlohmann::json& j, const char* propertyName, const Vec4& vec)
		{
			j[propertyName]["X"] = vec.x;
			j[propertyName]["Y"] = vec.y;
			j[propertyName]["Z"] = vec.z;
			j[propertyName]["W"] = vec.w;
		}

		void serialize(nlohmann::json& j, const char* propertyName, const Vec3& vec)
		{
			j[propertyName]["X"] = vec.x;
			j[propertyName]["Y"] = vec.y;
			j[propertyName]["Z"] = vec.z;
		}

		void serialize(nlohmann::json& j, const char* propertyName, const Vec2& vec)
		{
			j[propertyName]["X"] = vec.x;
			j[propertyName]["Y"] = vec.y;
		}

		void serialize(nlohmann::json& j, const char* propertyName, const Vec4i& vec)
		{
			j[propertyName]["X"] = vec.x;
			j[propertyName]["Y"] = vec.y;
			j[propertyName]["Z"] = vec.z;
			j[propertyName]["W"] = vec.w;
		}

		void serialize(nlohmann::json& j, const char* propertyName, const Vec3i& vec)
		{
			j[propertyName]["X"] = vec.x;
			j[propertyName]["Y"] = vec.y;
			j[propertyName]["Z"] = vec.z;
		}

		void serialize(nlohmann::json& j, const char* propertyName, const Vec2i& vec)
		{
			j[propertyName]["X"] = vec.x;
			j[propertyName]["Y"] = vec.y;
		}

		void serialize(nlohmann::json& j, const char* propertyName, const glm::u8vec4& vec)
		{
			j[propertyName]["R"] = vec.r;
			j[propertyName]["G"] = vec.g;
			j[propertyName]["B"] = vec.b;
			j[propertyName]["A"] = vec.a;
		}

		void serialize(nlohmann::json& j, const char* propertyName, const glm::quat& quat)
		{
			j[propertyName]["W"] = quat.w;
			j[propertyName]["X"] = quat.x;
			j[propertyName]["Y"] = quat.y;
			j[propertyName]["Z"] = quat.z;
		}

		Vec4 deserializeVec4(const nlohmann::json& j, const Vec4& defaultValue)
		{
			Vec4 res = defaultValue;
			if (j.contains("X"))
			{
				res.x = j["X"];
			}
			if (j.contains("Y"))
			{
				res.y = j["Y"];
			}
			if (j.contains("Z"))
			{
				res.z = j["Z"];
			}
			if (j.contains("W"))
			{
				res.w = j["W"];
			}
			return res;
		}

		Vec3 deserializeVec3(const nlohmann::json& j, const Vec3& defaultValue)
		{
			Vec3 res = defaultValue;
			if (j.contains("X"))
			{
				res.x = j["X"];
			}
			if (j.contains("Y"))
			{
				res.y = j["Y"];
			}
			if (j.contains("Z"))
			{
				res.z = j["Z"];
			}
			return res;
		}

		Vec2 deserializeVec2(const nlohmann::json& j, const Vec2& defaultValue)
		{
			Vec2 res = defaultValue;
			if (j.contains("X"))
			{
				res.x = j["X"];
			}
			if (j.contains("Y"))
			{
				res.y = j["Y"];
			}
			return res;
		}

		Vec4i deserializeVec4i(const nlohmann::json& j, const Vec4i& defaultValue)
		{
			Vec4i res = defaultValue;
			if (j.contains("X"))
			{
				res.x = j["X"];
			}
			if (j.contains("Y"))
			{
				res.y = j["Y"];
			}
			if (j.contains("Z"))
			{
				res.z = j["Z"];
			}
			if (j.contains("W"))
			{
				res.w = j["W"];
			}
			return res;
		}

		Vec3i deserializeVec3i(const nlohmann::json& j, const Vec3i& defaultValue)
		{
			Vec3i res = defaultValue;
			if (j.contains("X"))
			{
				res.x = j["X"];
			}
			if (j.contains("Y"))
			{
				res.y = j["Y"];
			}
			if (j.contains("Z"))
			{
				res.z = j["Z"];
			}
			return res;
		}

		Vec2i deserializeVec2i(const nlohmann::json& j, const Vec2i& defaultValue)
		{
			Vec2i res = defaultValue;
			if (j.contains("X"))
			{
				res.x = j["X"];
			}
			if (j.contains("Y"))
			{
				res.y = j["Y"];
			}
			return res;
		}

		glm::u8vec4 deserializeU8Vec4(const nlohmann::json& j, const glm::u8vec4& defaultValue)
		{
			glm::u8vec4 res = defaultValue;
			if (j.contains("R"))
			{
				res.x = j["R"];
			}
			if (j.contains("G"))
			{
				res.y = j["G"];
			}
			if (j.contains("B"))
			{
				res.z = j["B"];
			}
			if (j.contains("A"))
			{
				res.w = j["A"];
			}
			return res;
		}

		glm::quat deserializeQuat(const nlohmann::json& j, const glm::quat& defaultValue)
		{
			glm::quat res = defaultValue;
			if (j.contains("W"))
			{
				res.w = j["W"];
			}
			if (j.contains("X"))
			{
				res.x = j["X"];
			}
			if (j.contains("Y"))
			{
				res.y = j["Y"];
			}
			if (j.contains("Z"))
			{
				res.z = j["Z"];
			}
			return res;
		}

		// ------------------ DEPRECATED BEGIN ------------------
		Vec4 legacy_deserializeVec4(RawMemory& memory)
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

		Vec3 legacy_deserializeVec3(RawMemory& memory)
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

		Vec2 legacy_deserializeVec2(RawMemory& memory)
		{
			// Target
			//   X    -> float
			//   Y    -> float
			Vec2 res;
			memory.read<float>(&res.x);
			memory.read<float>(&res.y);
			return res;
		}

		Vec4i legacy_deserializeVec4i(RawMemory& memory)
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

		Vec3i legacy_deserializeVec3i(RawMemory& memory)
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

		Vec2i legacy_deserializeVec2i(RawMemory& memory)
		{
			// Target
			//   X    -> i32
			//   Y    -> i32
			Vec2i res;
			memory.read<int32>(&res.x);
			memory.read<int32>(&res.y);
			return res;
		}

		glm::u8vec4 legacy_deserializeU8Vec4(RawMemory& memory)
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
		// ------------------ DEPRECATED END ------------------

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
				return n1 * (t -= (1.5f / d1)) * t + .75f;
			}
			else if (t < 2.5f / d1)
			{
				return n1 * (t -= (2.25f / d1)) * t + .9375f;
			}
			else
			{
				return n1 * (t -= (2.625f / d1)) * t + .984375f;
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