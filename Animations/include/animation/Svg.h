#ifndef MATH_ANIM_SVG_OBJECT_H
#define MATH_ANIM_SVG_OBJECT_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	struct AnimObject;
	struct OrthoCamera;
	struct PerspectiveCamera;
	struct Texture;

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
		bool moveToP0;
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
	};

	struct SvgObject
	{
		// TODO: Have a Path struct which contains SubPath structs which are the holes...(possibly)
		Contour* contours;
		int numContours;
		float approximatePerimeter;
		bool is3D;
		BBox bbox;
		
		void normalize(const Vec2& min = Vec2{ FLT_MAX, FLT_MAX }, const Vec2& max = Vec2{ FLT_MIN, FLT_MIN });
		void calculateApproximatePerimeter();
		void calculateBBox();
		void render(NVGcontext* vg, const AnimObject* parent, const Vec2& offset = Vec2{0, 0}, bool renderBBoxes = false) const;
		void renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent, const Vec2& offset = Vec2{0, 0}, bool reverse = false, bool isSvgGroup = false, bool renderBBoxes = false) const;
		void free();
	};

	struct SvgGroup
	{
		char** uniqueObjectNames;
		SvgObject* uniqueObjects;
		int numUniqueObjects;

		SvgObject* objects;
		Vec2* objectOffsets;
		int numObjects;
		Vec4 viewbox;
		BBox bbox;

		void normalize();
		void calculateBBox();
		void render(NVGcontext* vg, AnimObject* parent, bool renderBBoxes = false) const;
		void renderCreateAnimation(NVGcontext* vg, float t, AnimObject* parent, bool reverse = false, bool renderBBoxes = false) const;
		void free();
	};

	namespace Svg
	{
		SvgObject createDefault();
		SvgGroup createDefaultGroup();
		
		void init(OrthoCamera& sceneCamera2d, PerspectiveCamera& sceneCamera3d);
		void free();

		void endFrame();

		const Texture& getSvgCache();

		void beginSvgGroup(SvgGroup* group, const Vec4& viewbox);
		void pushSvgToGroup(SvgGroup* group, const SvgObject& obj, const std::string& id, const Vec2& offset);
		void endSvgGroup(SvgGroup* group);

		void beginContour(SvgObject* object, const Vec2& firstPoint, bool is3D = false);
		void closeContour(SvgObject* object, bool lineToEndpoint = false);

		void moveTo(SvgObject* object, const Vec2& point, bool absolute = true);
		void lineTo(SvgObject* object, const Vec2& point, bool absolute = true);
		void hzLineTo(SvgObject* object, float xPoint, bool absolute = true);
		void vtLineTo(SvgObject* object, float yPoint, bool absolute = true);
		void bezier2To(SvgObject* object, const Vec2& control, const Vec2& dest, bool absolute = true);
		void bezier3To(SvgObject* object, const Vec2& control0, const Vec2& control1, const Vec2& dest, bool absolute = true);
		void smoothBezier2To(SvgObject* object, const Vec2& dest, bool absolute = true);
		void smoothBezier3To(SvgObject* object, const Vec2& control1, const Vec2& dest, bool absolute = true);
		void arcTo(SvgObject* object, const Vec2& radius, float xAxisRot, bool largeArc, bool sweep, const Vec2& dst, bool absolute = true);

		void copy(SvgObject* dest, const SvgObject* src);
		void renderInterpolation(NVGcontext* vg, const AnimObject* animObjectSrc, const SvgObject* interpolationSrc, const AnimObject* animObjectDst, const SvgObject* interpolationDst, float t);
	}
}

#endif 