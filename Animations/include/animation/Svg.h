#ifndef MATH_ANIM_SVG_OBJECT_H
#define MATH_ANIM_SVG_OBJECT_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	struct AnimObject;
	struct OrthoCamera;

	enum class CurveType : uint8
	{
		None = 0,
		Line,
		Bezier2,
		Bezier3
	};

	struct Line
	{
		Vec3 p1;
	};

	struct Bezier2
	{
		Vec3 p1;
		Vec3 p2;
	};

	struct Bezier3
	{
		Vec3 p1;
		Vec3 p2;
		Vec3 p3;
	};

	struct Curve
	{
		bool moveToP0;
		CurveType type;
		// Every curve has at least one point
		Vec3 p0;
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
		bool is3D;

		void calculateApproximatePerimeter();
		void render(NVGcontext* vg, const AnimObject* parent) const;
		void renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent, bool reverse = false) const;
		void free();
	};

	namespace Svg
	{
		SvgObject createDefault();
		
		void init(OrthoCamera& camera);

		void beginContour(SvgObject* object, const Vec3& firstPoint, bool clockwiseFill, bool is3D = false);
		void closeContour(SvgObject* object, bool lineToEndpoint = false);

		void moveTo(SvgObject* object, const Vec3& point, bool absolute = true);
		void lineTo(SvgObject* object, const Vec3& point, bool absolute = true);
		void hzLineTo(SvgObject* object, float xPoint, bool absolute = true);
		void vtLineTo(SvgObject* object, float yPoint, bool absolute = true);
		void bezier2To(SvgObject* object, const Vec3& control, const Vec3& dest, bool absolute = true);
		void bezier3To(SvgObject* object, const Vec3& control0, const Vec3& control1, const Vec3& dest, bool absolute = true);
		void smoothBezier3To(SvgObject* object, const Vec3& control1, const Vec3& dest, bool absolute = true);

		void copy(SvgObject* dest, const SvgObject* src);
		void renderInterpolation(NVGcontext* vg, const AnimObject* animObjectSrc, const SvgObject* interpolationSrc, const AnimObject* animObjectDst, const SvgObject* interpolationDst, float t);
	}
}

#endif 