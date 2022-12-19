#ifndef MATH_ANIM_SVG_OBJECT_H
#define MATH_ANIM_SVG_OBJECT_H
#include "core.h"

namespace MathAnim
{
	struct AnimObject;
	struct OrthoCamera;
	struct PerspectiveCamera;
	struct Texture;
	struct Framebuffer;

	enum class CurveType : uint8
	{
		None = 0,
		Line,
		Bezier2,
		Bezier3
	};

	enum class FillType : uint8
	{
		NonZeroFillType = 0,
		EvenOddFillType,
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

		float calculateApproximatePerimeter() const;
		Curve split(float t0, float t1) const;
	};

	struct Path
	{
		Curve* curves;
		int numCurves;
		int maxCapacity;
		bool isHole;

		float calculateApproximatePerimeter() const;
	};

	struct SvgObject
	{
		Path* paths;
		int numPaths;
		float approximatePerimeter;
		BBox bbox;
		Vec2 _cursor;
		Vec4 fillColor;
		FillType fillType;

		void normalize();
		void calculateApproximatePerimeter();
		void calculateBBox();
		void render(const AnimObject* parent, const Texture& texture, const Vec2& textureOffset) const;
		void renderOutline(float t, const AnimObject* parent) const;
		void free();

		void serialize(RawMemory& memory) const;
		static SvgObject* deserialize(RawMemory& memory, uint32 version);
	};

	struct SvgGroup
	{
		char** uniqueObjectNames;
		SvgObject* uniqueObjects;
		int numUniqueObjects;

		SvgObject* objects;
		Vec2* objectOffsets;
		int numObjects;
		BBox bbox;

		void normalize();
		void calculateBBox();
		void free();
	};

	namespace Svg
	{
		SvgObject createDefault();
		SvgGroup createDefaultGroup();

		void init();

		void beginSvgGroup(SvgGroup* group);
		void pushSvgToGroup(SvgGroup* group, const SvgObject& obj, const std::string& id, const Vec2& offset = Vec2{ NAN, NAN });
		void endSvgGroup(SvgGroup* group);

		void beginPath(SvgObject* object, const Vec2& firstPoint, bool isAbsolute = true);
		void closePath(SvgObject* object, bool lineToEndpoint = false, bool isHole = false);

		// This implicitly closes the current path and begins a new path
		void moveTo(SvgObject* object, const Vec2& point, bool absolute = true);
		void lineTo(SvgObject* object, const Vec2& point, bool absolute = true);
		void hzLineTo(SvgObject* object, float xPoint, bool absolute = true);
		void vtLineTo(SvgObject* object, float yPoint, bool absolute = true);
		void bezier2To(SvgObject* object, const Vec2& control, const Vec2& dest, bool absolute = true);
		void bezier3To(SvgObject* object, const Vec2& control0, const Vec2& control1, const Vec2& dest, bool absolute = true);
		void smoothBezier2To(SvgObject* object, const Vec2& dest, bool absolute = true);
		void smoothBezier3To(SvgObject* object, const Vec2& control1, const Vec2& dest, bool absolute = true);
		void arcTo(SvgObject* object, const Vec2& radius, float xAxisRot, bool largeArc, bool sweep, const Vec2& dst, bool absolute = true);

		// Manually add a curve
		void addCurveManually(SvgObject* object, const Curve& curve);

		void copy(SvgObject* dest, const SvgObject* src);
		SvgObject* interpolate(const SvgObject* src, const SvgObject* dst, float t);
	}
}

#endif