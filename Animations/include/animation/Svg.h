#ifndef MATH_ANIM_SVG_OBJECT_H
#define MATH_ANIM_SVG_OBJECT_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	struct AnimObject;

	enum class CurveType : uint8
	{
		None = 0,
		Line,
		Bezier2,
		Bezier3
	};

	struct Line
	{
		Vec2 p1;
	};

	struct Bezier2
	{
		Vec2 p1;
		Vec2 p2;
	};

	struct Bezier3
	{
		Vec2 p1;
		Vec2 p2;
		Vec2 p3;
	};

	struct Curve
	{
		CurveType type;
		// Every curve has at least one point
		Vec2 p0;
		union
		{
			Line line;
			Bezier2 bezier2;
			Bezier3 bezier3;
		} as;
	};

	struct Contour
	{
		Curve* curves;
		int numCurves;
		int maxCapacity;
		bool clockwiseFill;
	};

	struct SvgObject
	{
		Contour* contours;
		int numContours;
		float approximatePerimeter;

		void calculateApproximatePerimeter();
		void render(NVGcontext* vg, const AnimObject* parent) const;
		void renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent) const;
		void free();
	};

	namespace Svg
	{
		SvgObject createDefault();

		void beginContour(SvgObject* object, const Vec2& firstPoint, bool clockwiseFill);
		void closeContour(SvgObject* object);

		void lineTo(SvgObject* object, const Vec2& point);
		void bezier2To(SvgObject* object, const Vec2& control, const Vec2& dest);
		void bezier3To(SvgObject* object, const Vec2& control0, const Vec2& control1, const Vec2& dest);

		void copy(SvgObject* dest, const SvgObject* src);
		void renderInterpolation(NVGcontext* vg, const Vec2& srcPos, const SvgObject* interpolationSrc, const Vec2& dstPos, const SvgObject* interpolationDst, float t);
	}
}

#endif 