#include "animation/Svg.h"
#include "animation/Animation.h"
#include "utils/CMath.h"
#include "renderer/Renderer.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Colors.h"
#include "core/Application.h"

#include <nanovg.h>

namespace MathAnim
{
	static float cacheCurrentX, cacheCurrentY, cacheLineHeight;
	static Framebuffer svgCache;

	namespace Svg
	{
		// ----------------- Private Variables -----------------
		constexpr int initialMaxCapacity = 5;
		static OrthoCamera* camera;
		static Vec3 cursor;
		static bool moveToP0 = false;

		// ----------------- Internal functions -----------------
		static void checkResize(Contour& contour);
		static void render2DInterpolation(NVGcontext* vg, const AnimObject* animObjectSrc, const SvgObject* interpolationSrc, const AnimObject* animObjectDst, const SvgObject* interpolationDst, float t);
		static void render3DInterpolation(NVGcontext* vg, const AnimObject* animObjectSrc, const SvgObject* interpolationSrc, const AnimObject* animObjectDst, const SvgObject* interpolationDst, float t);
		static void transformPoint(Vec3* point, const Vec3& offset, const Vec3& viewboxPos, const Vec3& viewboxSize);
		static void generateSvgCache(uint32 width, uint32 height);

		SvgObject createDefault()
		{
			SvgObject res;
			res.approximatePerimeter = 0.0f;
			// Dummy allocation to allow memory tracking when reallocing 
			// TODO: Fix the dang memory allocation library so I don't have to do this!
			res.contours = (Contour*)g_memory_allocate(sizeof(Contour));
			res.numContours = 0;
			res.is3D = false;
			return res;
		}

		SvgGroup createDefaultGroup()
		{
			SvgGroup res;
			// Dummy allocation to allow memory tracking when reallocing 
			// TODO: Fix the dang memory allocation library so I don't have to do this!
			res.objects = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			res.objectOffsets = (Vec3*)g_memory_allocate(sizeof(Vec3));
			res.numObjects = 0;
			res.uniqueObjects = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			res.uniqueObjectNames = (char**)g_memory_allocate(sizeof(char*));
			res.numUniqueObjects = 0;
			res.viewbox = Vec4{ 0, 0, 1, 1 };
			return res;
		}

		void init(OrthoCamera& sceneCamera)
		{
			camera = &sceneCamera;
			constexpr int defaultWidth = 4096;
			generateSvgCache(defaultWidth, defaultWidth);

			cacheCurrentX = 0;
			cacheCurrentY = 0;
		}

		void free()
		{
			if (svgCache.fbo != UINT32_MAX)
			{
				svgCache.destroy();
			}
		}

		void endFrame()
		{
			cacheCurrentX = 0;
			cacheCurrentY = 0;

			svgCache.bind();
			glViewport(0, 0, svgCache.width, svgCache.height);
			//svgCache.clearColorAttachmentRgba(0, "#fc03ecFF"_hex);
			svgCache.clearColorAttachmentRgba(0, "#00000000"_hex);
			svgCache.clearDepthStencil();
		}

		void beginSvgGroup(SvgGroup* group, const Vec4& viewbox)
		{
			group->viewbox = viewbox;
		}

		void pushSvgToGroup(SvgGroup* group, const SvgObject& obj, const std::string& id, const Vec3& offset)
		{
			group->numObjects++;
			group->objects = (SvgObject*)g_memory_realloc(group->objects, sizeof(SvgObject) * group->numObjects);
			g_logger_assert(group->objects != nullptr, "Ran out of RAM.");
			group->objectOffsets = (Vec3*)g_memory_realloc(group->objectOffsets, sizeof(Vec3) * group->numObjects);
			g_logger_assert(group->objectOffsets != nullptr, "Ran out of RAM.");

			group->objectOffsets[group->numObjects - 1] = offset;
			group->objects[group->numObjects - 1] = obj;

			// Horribly inefficient... do something better eventually
			bool isUnique = true;
			for (int i = 0; i < group->numUniqueObjects; i++)
			{
				if (std::strcmp(group->uniqueObjectNames[i], id.c_str()) == 0)
				{
					isUnique = false;
				}
			}
			if (isUnique)
			{
				group->numUniqueObjects++;
				group->uniqueObjectNames = (char**)g_memory_realloc(group->uniqueObjectNames, sizeof(char**) * group->numUniqueObjects);
				g_logger_assert(group->uniqueObjectNames != nullptr, "Ran out of RAM.");
				group->uniqueObjects = (SvgObject*)g_memory_realloc(group->uniqueObjects, sizeof(SvgObject) * group->numUniqueObjects);
				g_logger_assert(group->uniqueObjects != nullptr, "Ran out of RAM.");

				group->uniqueObjects[group->numUniqueObjects - 1] = obj;
				group->uniqueObjectNames[group->numUniqueObjects - 1] = (char*)g_memory_allocate(sizeof(char) * (id.length() + 1));
				g_memory_copyMem(group->uniqueObjectNames[group->numUniqueObjects - 1], (void*)id.c_str(), id.length() * sizeof(char));
				group->uniqueObjectNames[group->numUniqueObjects - 1][id.length()] = '\0';
			}
		}

		void endSvgGroup(SvgGroup* group)
		{
			Vec3 viewboxPos = Vec3{ group->viewbox.values[0], group->viewbox.values[1], 0.0f };
			Vec3 viewboxSize = Vec3{ group->viewbox.values[2], group->viewbox.values[3], 1.0f };

			// Normalize all the SVGs within the viewbox
			//for (int svgi = 0; svgi < group->numObjects; svgi++)
			//{
			//	// First get the original SVG element size recorded
			//	group->objects[svgi].calculateSvgSize();
			//	// Then normalize it and make sure the perimeter is calculated
			//	//group->objects[svgi].normalize();
			//	group->objects[svgi].calculateApproximatePerimeter();
			//	group->objects[svgi].calculateBBox();
			//}
			group->normalize();
		}

		void beginContour(SvgObject* object, const Vec3& firstPoint, bool is3D)
		{
			object->is3D = is3D;
			object->numContours++;
			object->contours = (Contour*)g_memory_realloc(object->contours, sizeof(Contour) * object->numContours);
			g_logger_assert(object->contours != nullptr, "Ran out of RAM.");

			object->contours[object->numContours - 1].maxCapacity = initialMaxCapacity;
			object->contours[object->numContours - 1].curves = (Curve*)g_memory_allocate(sizeof(Curve) * initialMaxCapacity);
			object->contours[object->numContours - 1].numCurves = 0;

			object->contours[object->numContours - 1].curves[0].p0 = firstPoint;
			cursor = firstPoint;
			moveToP0 = false;
		}

		void closeContour(SvgObject* object, bool lineToEndpoint)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot close contour when no contour exists.");
			g_logger_assert(object->contours[object->numContours - 1].numCurves > 0, "contour->numCurves == 0. Cannot close contour with 0 vertices. There must be at least one vertex to close a contour.");

			if (lineToEndpoint)
			{
				if (object->contours[object->numContours - 1].numCurves > 0)
				{
					Vec3 firstPoint = object->contours[object->numContours - 1].curves[0].p0;
					lineTo(object, firstPoint, true);
				}
			}

			cursor = Vec3{ 0, 0, 0 };
		}

		void moveTo(SvgObject* object, const Vec3& point, bool absolute)
		{
			// If no object has started, begin the object here
			if (object->numContours == 0)
			{
				beginContour(object, point);
				absolute = true;
				moveToP0 = false;
			}
			else
			{
				cursor = absolute ? point : cursor + point;
				moveToP0 = true;
			}
		}

		void lineTo(SvgObject* object, const Vec3& point, bool absolute)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot create a lineTo when no contour exists.");
			Contour& contour = object->contours[object->numContours - 1];
			contour.numCurves++;
			checkResize(contour);

			contour.curves[contour.numCurves - 1].p0 = cursor;
			contour.curves[contour.numCurves - 1].as.line.p1 = absolute ? point : point + cursor;
			contour.curves[contour.numCurves - 1].type = CurveType::Line;
			contour.curves[contour.numCurves - 1].moveToP0 = moveToP0;

			cursor = contour.curves[contour.numCurves - 1].as.line.p1;
			moveToP0 = false;
		}

		void hzLineTo(SvgObject* object, float xPoint, bool absolute)
		{
			Vec3 position = absolute
				? Vec3{ xPoint, cursor.y, 0.0f }
			: Vec3{ xPoint, 0.0f, 0.0f } + cursor;
			lineTo(object, position, true);
		}

		void vtLineTo(SvgObject* object, float yPoint, bool absolute)
		{
			Vec3 position = absolute
				? Vec3{ cursor.x, yPoint, 0.0f }
			: Vec3{ 0.0f, yPoint, 0.0f } + cursor;
			lineTo(object, position, true);
		}

		void bezier2To(SvgObject* object, const Vec3& control, const Vec3& dest, bool absolute)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot create a bezier2To when no contour exists.");
			Contour& contour = object->contours[object->numContours - 1];
			contour.numCurves++;
			checkResize(contour);

			contour.curves[contour.numCurves - 1].p0 = cursor;

			contour.curves[contour.numCurves - 1].as.bezier2.p1 = absolute ? control : control + cursor;
			cursor = contour.curves[contour.numCurves - 1].as.bezier2.p1;

			contour.curves[contour.numCurves - 1].as.bezier2.p2 = absolute ? dest : dest + cursor;
			cursor = contour.curves[contour.numCurves - 1].as.bezier2.p2;

			contour.curves[contour.numCurves - 1].type = CurveType::Bezier2;
			contour.curves[contour.numCurves - 1].moveToP0 = moveToP0;

			moveToP0 = false;
		}

		void bezier3To(SvgObject* object, const Vec3& control0, const Vec3& control1, const Vec3& dest, bool absolute)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot create a bezier3To when no contour exists.");
			Contour& contour = object->contours[object->numContours - 1];
			contour.numCurves++;
			checkResize(contour);

			contour.curves[contour.numCurves - 1].p0 = cursor;

			contour.curves[contour.numCurves - 1].as.bezier3.p1 = absolute ? control0 : control0 + cursor;
			cursor = contour.curves[contour.numCurves - 1].as.bezier3.p1;

			contour.curves[contour.numCurves - 1].as.bezier3.p2 = absolute ? control1 : control1 + cursor;
			cursor = contour.curves[contour.numCurves - 1].as.bezier3.p2;

			contour.curves[contour.numCurves - 1].as.bezier3.p3 = absolute ? dest : dest + cursor;
			cursor = contour.curves[contour.numCurves - 1].as.bezier3.p3;

			contour.curves[contour.numCurves - 1].type = CurveType::Bezier3;
			contour.curves[contour.numCurves - 1].moveToP0 = moveToP0;

			moveToP0 = false;
		}

		void smoothBezier2To(SvgObject* object, const Vec3& dest, bool absolute)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot create a bezier3To when no contour exists.");
			Contour& contour = object->contours[object->numContours - 1];
			contour.numCurves++;
			checkResize(contour);

			contour.curves[contour.numCurves - 1].p0 = cursor;

			Vec3 control0 = cursor;
			if (contour.numCurves > 1)
			{
				if (contour.curves[contour.numCurves - 2].type == CurveType::Bezier2)
				{
					Vec3 prevControl1 = contour.curves[contour.numCurves - 2].as.bezier2.p1;
					// Reflect the previous c2 about the current cursor
					control0 = (-1.0f * (prevControl1 - cursor)) + cursor;
				}
			}
			contour.curves[contour.numCurves - 1].as.bezier2.p1 = control0;
			cursor = contour.curves[contour.numCurves - 1].as.bezier2.p1;

			contour.curves[contour.numCurves - 1].as.bezier2.p2 = absolute ? dest : dest + cursor;
			cursor = contour.curves[contour.numCurves - 1].as.bezier2.p2;

			contour.curves[contour.numCurves - 1].type = CurveType::Bezier2;
			contour.curves[contour.numCurves - 1].moveToP0 = moveToP0;

			moveToP0 = false;
		}

		void smoothBezier3To(SvgObject* object, const Vec3& control1, const Vec3& dest, bool absolute)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot create a bezier3To when no contour exists.");
			Contour& contour = object->contours[object->numContours - 1];
			contour.numCurves++;
			checkResize(contour);

			contour.curves[contour.numCurves - 1].p0 = cursor;

			Vec3 control0 = cursor;
			if (contour.numCurves > 1)
			{
				if (contour.curves[contour.numCurves - 2].type == CurveType::Bezier3)
				{
					Vec3 prevControl1 = contour.curves[contour.numCurves - 2].as.bezier3.p2;
					// Reflect the previous c2 about the current cursor
					control0 = (-1.0f * (prevControl1 - cursor)) + cursor;
				}
			}
			contour.curves[contour.numCurves - 1].as.bezier3.p1 = control0;
			cursor = contour.curves[contour.numCurves - 1].as.bezier3.p1;

			contour.curves[contour.numCurves - 1].as.bezier3.p2 = absolute ? control1 : control1 + cursor;
			cursor = contour.curves[contour.numCurves - 1].as.bezier3.p2;

			contour.curves[contour.numCurves - 1].as.bezier3.p3 = absolute ? dest : dest + cursor;
			cursor = contour.curves[contour.numCurves - 1].as.bezier3.p3;

			contour.curves[contour.numCurves - 1].type = CurveType::Bezier3;
			contour.curves[contour.numCurves - 1].moveToP0 = moveToP0;

			moveToP0 = false;
		}

		void arcTo(SvgObject* object, const Vec2& radius, float xAxisRot, bool largeArc, bool sweep, const Vec3& dst, bool absolute)
		{
			// TODO: All this should probably be implementation details...
			if (CMath::compare(cursor, dst))
			{
				// If the cursor == dst then no arc is emitted
				return;
			}

			if (CMath::compare(radius.x, 0.0f) || CMath::compare(radius.y, 0.0f))
			{
				// Treat the arc as a line to if the radius x or y is 0
				lineTo(object, dst, absolute);
				return;
			}

			// TMP: Implement actual arcTo functionality
			lineTo(object, dst, absolute);

			// float rx = glm::abs(radius.x);
			// float ry = glm::abs(radius.y);
			// xAxisRot = fmodf(xAxisRot, 360.0f);
		}

		void copy(SvgObject* dest, const SvgObject* src)
		{
			if (dest->numContours != src->numContours)
			{
				// Free any extra contours the destination has
				// If the destination has less, this loop doesn't run
				for (int contouri = src->numContours; contouri < dest->numContours; contouri++)
				{
					g_memory_free(dest->contours[contouri].curves);
					dest->contours[contouri].curves = nullptr;
					dest->contours[contouri].numCurves = 0;
					dest->contours[contouri].maxCapacity = 0;
				}

				// Then reallocate memory. If dest had less, this will acquire enough new memory
				// If dest had more, this will get rid of the extra memory
				dest->contours = (Contour*)g_memory_realloc(dest->contours, sizeof(Contour) * src->numContours);

				// Go through and initialize the curves for any new curves that were added
				for (int contouri = dest->numContours; contouri < src->numContours; contouri++)
				{
					dest->contours[contouri].curves = (Curve*)g_memory_allocate(sizeof(Curve) * initialMaxCapacity);
					dest->contours[contouri].maxCapacity = initialMaxCapacity;
					dest->contours[contouri].numCurves = 0;
				}

				// Then set the number of contours equal
				dest->numContours = src->numContours;
			}

			g_logger_assert(dest->is3D == src->is3D, "Cannot copy 2D svg object to 3D object or vice versa.");
			g_logger_assert(dest->numContours == src->numContours, "How did this happen?");

			// Finally we can just go through and set all the data equal to each other
			for (int contouri = 0; contouri < src->numContours; contouri++)
			{
				Contour& dstContour = dest->contours[contouri];
				dstContour.numCurves = 0;

				for (int curvei = 0; curvei < src->contours[contouri].numCurves; curvei++)
				{
					const Curve& srcCurve = src->contours[contouri].curves[curvei];

					dstContour.numCurves++;
					checkResize(dstContour);

					// Copy the data
					Curve& dstCurve = dstContour.curves[curvei];
					dstCurve.p0 = srcCurve.p0;
					dstCurve.as = srcCurve.as;
					dstCurve.type = srcCurve.type;
					dstCurve.moveToP0 = srcCurve.moveToP0;
				}

				g_logger_assert(dstContour.numCurves == src->contours[contouri].numCurves, "How did this happen?");
			}

			dest->calculateApproximatePerimeter();
			dest->calculateBBox();
		}

		void renderInterpolation(NVGcontext* vg, const AnimObject* animObjectSrc, const SvgObject* interpolationSrc, const AnimObject* animObjectDst, const SvgObject* interpolationDst, float t)
		{
			g_logger_assert(interpolationSrc->is3D == interpolationDst->is3D, "Cannot interpolate between 2D and 3D svg object.");
			if (interpolationSrc->is3D)
			{
				render3DInterpolation(vg, animObjectSrc, interpolationSrc, animObjectDst, interpolationDst, t);
			}
			else
			{
				render2DInterpolation(vg, animObjectSrc, interpolationSrc, animObjectDst, interpolationDst, t);
			}
		}

		// ----------------- Internal functions -----------------
		static void checkResize(Contour& contour)
		{
			if (contour.numCurves > contour.maxCapacity)
			{
				contour.maxCapacity *= 2;
				contour.curves = (Curve*)g_memory_realloc(contour.curves, sizeof(Curve) * contour.maxCapacity);
				g_logger_assert(contour.curves != nullptr, "Ran out of RAM.");
			}
		}

		static void render3DInterpolation(NVGcontext* vg, const AnimObject* animObjectSrc, const SvgObject* interpolationSrc, const AnimObject* animObjectDst, const SvgObject* interpolationDst, float t)
		{
			g_logger_warning("TODO: Implement me.");
			//// Interpolate fill color
			//glm::vec4 fillColorSrc = glm::vec4(
			//	(float)animObjectSrc->fillColor.r / 255.0f,
			//	(float)animObjectSrc->fillColor.g / 255.0f,
			//	(float)animObjectSrc->fillColor.b / 255.0f,
			//	(float)animObjectSrc->fillColor.a / 255.0f
			//);
			//glm::vec4 fillColorDst = glm::vec4(
			//	(float)animObjectDst->fillColor.r / 255.0f,
			//	(float)animObjectDst->fillColor.g / 255.0f,
			//	(float)animObjectDst->fillColor.b / 255.0f,
			//	(float)animObjectDst->fillColor.a / 255.0f
			//);
			//glm::vec4 fillColorInterp = (fillColorDst - fillColorSrc) * t + fillColorSrc;
			//NVGcolor fillColor = nvgRGBA(
			//	(uint8)(fillColorInterp.r * 255.0f),
			//	(uint8)(fillColorInterp.g * 255.0f),
			//	(uint8)(fillColorInterp.b * 255.0f),
			//	(uint8)(fillColorInterp.a * 255.0f)
			//);

			//// Interpolate stroke color
			//glm::vec4 strokeColorSrc = glm::vec4(
			//	(float)animObjectSrc->strokeColor.r / 255.0f,
			//	(float)animObjectSrc->strokeColor.g / 255.0f,
			//	(float)animObjectSrc->strokeColor.b / 255.0f,
			//	(float)animObjectSrc->strokeColor.a / 255.0f
			//);
			//glm::vec4 strokeColorDst = glm::vec4(
			//	(float)animObjectDst->strokeColor.r / 255.0f,
			//	(float)animObjectDst->strokeColor.g / 255.0f,
			//	(float)animObjectDst->strokeColor.b / 255.0f,
			//	(float)animObjectDst->strokeColor.a / 255.0f
			//);
			//glm::vec4 strokeColorInterp = (strokeColorDst - strokeColorSrc) * t + strokeColorSrc;
			//NVGcolor strokeColor = nvgRGBA(
			//	(uint8)(strokeColorInterp.r * 255.0f),
			//	(uint8)(strokeColorInterp.g * 255.0f),
			//	(uint8)(strokeColorInterp.b * 255.0f),
			//	(uint8)(strokeColorInterp.a * 255.0f)
			//);

			//// Interpolate position
			//const Vec3& dstPos = animObjectDst->position;
			//const Vec3& srcPos = animObjectSrc->position;
			//glm::vec3 interpolatedPos = glm::vec3(
			//	(dstPos.x - srcPos.x) * t + srcPos.x,
			//	(dstPos.y - srcPos.y) * t + srcPos.y,
			//	(dstPos.z - srcPos.z) * t + srcPos.z
			//);

			//// Interpolate rotation
			//const Vec3& dstRotation = animObjectDst->rotation;
			//const Vec3& srcRotation = animObjectSrc->rotation;
			//glm::vec3 interpolatedRotation = glm::vec3(
			//	(dstRotation.x - srcRotation.x) * t + srcRotation.x,
			//	(dstRotation.x - srcRotation.y) * t + srcRotation.y,
			//	(dstRotation.x - srcRotation.z) * t + srcRotation.z
			//);

			//// Apply transformations
			//nvgTranslate(vg, interpolatedPos.x - Svg::camera->position.x, interpolatedPos.y - Svg::camera->position.y);
			//if (interpolatedRotation.z != 0.0f)
			//{
			//	nvgRotate(vg, glm::radians(interpolatedRotation.z));
			//}

			//// Interpolate stroke width
			//float dstStrokeWidth = animObjectDst->strokeWidth;
			//if (glm::epsilonEqual(dstStrokeWidth, 0.0f, 0.01f))
			//{
			//	dstStrokeWidth = 5.0f;
			//}
			//float srcStrokeWidth = animObjectSrc->strokeWidth;
			//if (glm::epsilonEqual(srcStrokeWidth, 0.0f, 0.01f))
			//{
			//	srcStrokeWidth = 5.0f;
			//}
			//float strokeWidth = (dstStrokeWidth - srcStrokeWidth) * t + srcStrokeWidth;

			//// If one object has more contours than the other object,
			//// then we'll just skip every other contour for the object
			//// with less contours and hopefully it looks cool
			//const SvgObject* lessContours = interpolationSrc->numContours <= interpolationDst->numContours
			//	? interpolationSrc
			//	: interpolationDst;
			//const SvgObject* moreContours = interpolationSrc->numContours > interpolationDst->numContours
			//	? interpolationSrc
			//	: interpolationDst;
			//int numContoursToSkip = glm::max(moreContours->numContours / lessContours->numContours, 1);
			//int lessContouri = 0;
			//int moreContouri = 0;
			//while (lessContouri < lessContours->numContours)
			//{
			//	nvgBeginPath(vg);

			//	nvgFillColor(vg, fillColor);
			//	nvgStrokeColor(vg, strokeColor);
			//	nvgStrokeWidth(vg, strokeWidth);

			//	const Contour& lessCurves = lessContours->contours[lessContouri];
			//	const Contour& moreCurves = moreContours->contours[moreContouri];

			//	// It's undefined to interpolate between two contours if one of the contours has no curves
			//	bool shouldLoop = moreCurves.numCurves > 0 && lessCurves.numCurves > 0;
			//	if (shouldLoop)
			//	{
			//		// Move to the start, which is the interpolation between both of the
			//		// first vertices
			//		const Vec3& p0a = lessCurves.curves[0].p0;
			//		const Vec3& p0b = moreCurves.curves[0].p0;
			//		Vec3 interpP0 = {
			//			(p0b.x - p0a.x) * t + p0a.x,
			//			(p0b.y - p0a.y) * t + p0a.y
			//		};

			//		nvgMoveTo(vg, interpP0.x, interpP0.y);
			//	}

			//	int maxNumCurves = glm::max(lessCurves.numCurves, moreCurves.numCurves);
			//	for (int curvei = 0; curvei < maxNumCurves; curvei++)
			//	{
			//		// Interpolate between the two curves, treat both curves
			//		// as bezier3 curves no matter what to make it easier
			//		glm::vec2 p1a, p2a, p3a;
			//		glm::vec2 p1b, p2b, p3b;

			//		if (curvei > lessCurves.numCurves || curvei > moreCurves.numCurves)
			//		{
			//			g_logger_error("Cannot interpolate between two contours with different number of curves yet.");
			//			break;
			//		}
			//		const Curve& lessCurve = lessCurves.curves[curvei];
			//		const Curve& moreCurve = moreCurves.curves[curvei];

			//		// First get the control points depending on the type of the curve
			//		switch (lessCurve.type)
			//		{
			//		case CurveType::Bezier3:
			//			p1a = glm::vec2(lessCurve.as.bezier3.p1.x, lessCurve.as.bezier3.p1.y);
			//			p2a = glm::vec2(lessCurve.as.bezier3.p2.x, lessCurve.as.bezier3.p2.y);
			//			p3a = glm::vec2(lessCurve.as.bezier3.p3.x, lessCurve.as.bezier3.p3.y);
			//			break;
			//		case CurveType::Bezier2:
			//		{
			//			glm::vec2 p0 = glm::vec2(lessCurve.p0.x, lessCurve.p0.y);
			//			glm::vec2 p1 = glm::vec2(lessCurve.as.bezier2.p1.x, lessCurve.as.bezier2.p1.y);
			//			glm::vec2 p2 = glm::vec2(lessCurve.as.bezier2.p2.x, lessCurve.as.bezier2.p2.y);

			//			// Degree elevated quadratic bezier curve
			//			p1a = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
			//			p2a = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
			//			p3a = p2;
			//		}
			//		break;
			//		case CurveType::Line:
			//			p1a = glm::vec2(lessCurve.p0.x, lessCurve.p0.y);
			//			p2a = glm::vec2(lessCurve.as.line.p1.x, lessCurve.as.line.p1.y);
			//			p3a = p2a;
			//			break;
			//		default:
			//			g_logger_warning("Unknown curve type %d", lessCurve.type);
			//			break;
			//		}

			//		switch (moreCurve.type)
			//		{
			//		case CurveType::Bezier3:
			//			p1b = glm::vec2(moreCurve.as.bezier3.p1.x, moreCurve.as.bezier3.p1.y);
			//			p2b = glm::vec2(moreCurve.as.bezier3.p2.x, moreCurve.as.bezier3.p2.y);
			//			p3b = glm::vec2(moreCurve.as.bezier3.p3.x, moreCurve.as.bezier3.p3.y);
			//			break;
			//		case CurveType::Bezier2:
			//		{
			//			glm::vec2 p0 = glm::vec2(moreCurve.p0.x, moreCurve.p0.y);
			//			glm::vec2 p1 = glm::vec2(moreCurve.as.bezier2.p1.x, moreCurve.as.bezier2.p1.y);
			//			glm::vec2 p2 = glm::vec2(moreCurve.as.bezier2.p2.x, moreCurve.as.bezier2.p2.y);

			//			// Degree elevated quadratic bezier curve
			//			p1b = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
			//			p2b = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
			//			p3b = p2;
			//		}
			//		break;
			//		case CurveType::Line:
			//			p1b = glm::vec2(moreCurve.p0.x, moreCurve.p0.y);
			//			p2b = glm::vec2(moreCurve.as.line.p1.x, moreCurve.as.line.p1.y);
			//			p3b = p2b;
			//			break;
			//		default:
			//			g_logger_warning("Unknown curve type %d", moreCurve.type);
			//			break;
			//		}

			//		// Then interpolate between the control points
			//		glm::vec2 interpP1 = (p1b - p1a) * t + p1a;
			//		glm::vec2 interpP2 = (p2b - p2a) * t + p2a;
			//		glm::vec2 interpP3 = (p3b - p3a) * t + p3a;

			//		// Then draw
			//		nvgBezierTo(vg,
			//			interpP1.x, interpP1.y,
			//			interpP2.x, interpP2.y,
			//			interpP3.x, interpP3.y);
			//	}

			//	nvgStroke(vg);
			//	nvgFill(vg);
			//	nvgClosePath(vg);

			//	lessContouri++;
			//	moreContouri += numContoursToSkip;
			//	if (moreContouri >= moreContours->numContours)
			//	{
			//		moreContouri = moreContours->numContours - 1;
			//	}
			//}

			//nvgResetTransform(vg);
		}

		static void render2DInterpolation(NVGcontext* vg, const AnimObject* animObjectSrc, const SvgObject* interpolationSrc, const AnimObject* animObjectDst, const SvgObject* interpolationDst, float t)
		{
			// Interpolate fill color
			glm::vec4 fillColorSrc = glm::vec4(
				(float)animObjectSrc->fillColor.r / 255.0f,
				(float)animObjectSrc->fillColor.g / 255.0f,
				(float)animObjectSrc->fillColor.b / 255.0f,
				(float)animObjectSrc->fillColor.a / 255.0f
			);
			glm::vec4 fillColorDst = glm::vec4(
				(float)animObjectDst->fillColor.r / 255.0f,
				(float)animObjectDst->fillColor.g / 255.0f,
				(float)animObjectDst->fillColor.b / 255.0f,
				(float)animObjectDst->fillColor.a / 255.0f
			);
			glm::vec4 fillColorInterp = (fillColorDst - fillColorSrc) * t + fillColorSrc;
			NVGcolor fillColor = nvgRGBA(
				(uint8)(fillColorInterp.r * 255.0f),
				(uint8)(fillColorInterp.g * 255.0f),
				(uint8)(fillColorInterp.b * 255.0f),
				(uint8)(fillColorInterp.a * 255.0f)
			);

			// Interpolate stroke color
			glm::vec4 strokeColorSrc = glm::vec4(
				(float)animObjectSrc->strokeColor.r / 255.0f,
				(float)animObjectSrc->strokeColor.g / 255.0f,
				(float)animObjectSrc->strokeColor.b / 255.0f,
				(float)animObjectSrc->strokeColor.a / 255.0f
			);
			glm::vec4 strokeColorDst = glm::vec4(
				(float)animObjectDst->strokeColor.r / 255.0f,
				(float)animObjectDst->strokeColor.g / 255.0f,
				(float)animObjectDst->strokeColor.b / 255.0f,
				(float)animObjectDst->strokeColor.a / 255.0f
			);
			glm::vec4 strokeColorInterp = (strokeColorDst - strokeColorSrc) * t + strokeColorSrc;
			NVGcolor strokeColor = nvgRGBA(
				(uint8)(strokeColorInterp.r * 255.0f),
				(uint8)(strokeColorInterp.g * 255.0f),
				(uint8)(strokeColorInterp.b * 255.0f),
				(uint8)(strokeColorInterp.a * 255.0f)
			);

			// Interpolate position
			const Vec3& dstPos = animObjectDst->position;
			const Vec3& srcPos = animObjectSrc->position;
			glm::vec3 interpolatedPos = glm::vec3(
				(dstPos.x - srcPos.x) * t + srcPos.x,
				(dstPos.y - srcPos.y) * t + srcPos.y,
				(dstPos.z - srcPos.z) * t + srcPos.z
			);

			// Interpolate rotation
			const Vec3& dstRotation = animObjectDst->rotation;
			const Vec3& srcRotation = animObjectSrc->rotation;
			glm::vec3 interpolatedRotation = glm::vec3(
				(dstRotation.x - srcRotation.x) * t + srcRotation.x,
				(dstRotation.x - srcRotation.y) * t + srcRotation.y,
				(dstRotation.x - srcRotation.z) * t + srcRotation.z
			);

			// Apply transformations
			Vec2 cameraCenteredPos = Svg::camera->projectionSize / 2.0f - Svg::camera->position;
			nvgTranslate(vg, interpolatedPos.x - cameraCenteredPos.x, interpolatedPos.y - cameraCenteredPos.y);
			if (interpolatedRotation.z != 0.0f)
			{
				nvgRotate(vg, glm::radians(interpolatedRotation.z));
			}

			// Interpolate stroke width
			float dstStrokeWidth = animObjectDst->strokeWidth;
			if (glm::epsilonEqual(dstStrokeWidth, 0.0f, 0.01f))
			{
				dstStrokeWidth = 5.0f;
			}
			float srcStrokeWidth = animObjectSrc->strokeWidth;
			if (glm::epsilonEqual(srcStrokeWidth, 0.0f, 0.01f))
			{
				srcStrokeWidth = 5.0f;
			}
			float strokeWidth = (dstStrokeWidth - srcStrokeWidth) * t + srcStrokeWidth;

			// If one object has more contours than the other object,
			// then we'll just skip every other contour for the object
			// with less contours and hopefully it looks cool
			const SvgObject* lessContours = interpolationSrc->numContours <= interpolationDst->numContours
				? interpolationSrc
				: interpolationDst;
			const SvgObject* moreContours = interpolationSrc->numContours > interpolationDst->numContours
				? interpolationSrc
				: interpolationDst;
			int numContoursToSkip = glm::max(moreContours->numContours / lessContours->numContours, 1);
			int lessContouri = 0;
			int moreContouri = 0;
			while (lessContouri < lessContours->numContours)
			{
				nvgBeginPath(vg);

				nvgFillColor(vg, fillColor);
				nvgStrokeColor(vg, strokeColor);
				nvgStrokeWidth(vg, strokeWidth);

				const Contour& lessCurves = lessContours->contours[lessContouri];
				const Contour& moreCurves = moreContours->contours[moreContouri];

				// It's undefined to interpolate between two contours if one of the contours has no curves
				bool shouldLoop = moreCurves.numCurves > 0 && lessCurves.numCurves > 0;
				if (shouldLoop)
				{
					// Move to the start, which is the interpolation between both of the
					// first vertices
					const Vec3& p0a = lessCurves.curves[0].p0;
					const Vec3& p0b = moreCurves.curves[0].p0;
					Vec3 interpP0 = {
						(p0b.x - p0a.x) * t + p0a.x,
						(p0b.y - p0a.y) * t + p0a.y
					};

					nvgMoveTo(vg, interpP0.x, interpP0.y);
				}

				int maxNumCurves = glm::max(lessCurves.numCurves, moreCurves.numCurves);
				for (int curvei = 0; curvei < maxNumCurves; curvei++)
				{
					// Interpolate between the two curves, treat both curves
					// as bezier3 curves no matter what to make it easier
					glm::vec2 p1a, p2a, p3a;
					glm::vec2 p1b, p2b, p3b;

					if (curvei > lessCurves.numCurves || curvei > moreCurves.numCurves)
					{
						g_logger_error("Cannot interpolate between two contours with different number of curves yet.");
						break;
					}
					const Curve& lessCurve = lessCurves.curves[curvei];
					const Curve& moreCurve = moreCurves.curves[curvei];

					// First get the control points depending on the type of the curve
					switch (lessCurve.type)
					{
					case CurveType::Bezier3:
						p1a = glm::vec2(lessCurve.as.bezier3.p1.x, lessCurve.as.bezier3.p1.y);
						p2a = glm::vec2(lessCurve.as.bezier3.p2.x, lessCurve.as.bezier3.p2.y);
						p3a = glm::vec2(lessCurve.as.bezier3.p3.x, lessCurve.as.bezier3.p3.y);
						break;
					case CurveType::Bezier2:
					{
						glm::vec2 p0 = glm::vec2(lessCurve.p0.x, lessCurve.p0.y);
						glm::vec2 p1 = glm::vec2(lessCurve.as.bezier2.p1.x, lessCurve.as.bezier2.p1.y);
						glm::vec2 p2 = glm::vec2(lessCurve.as.bezier2.p2.x, lessCurve.as.bezier2.p2.y);

						// Degree elevated quadratic bezier curve
						p1a = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
						p2a = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
						p3a = p2;
					}
					break;
					case CurveType::Line:
						p1a = glm::vec2(lessCurve.p0.x, lessCurve.p0.y);
						p2a = glm::vec2(lessCurve.as.line.p1.x, lessCurve.as.line.p1.y);
						p3a = p2a;
						break;
					default:
						g_logger_warning("Unknown curve type %d", lessCurve.type);
						break;
					}

					switch (moreCurve.type)
					{
					case CurveType::Bezier3:
						p1b = glm::vec2(moreCurve.as.bezier3.p1.x, moreCurve.as.bezier3.p1.y);
						p2b = glm::vec2(moreCurve.as.bezier3.p2.x, moreCurve.as.bezier3.p2.y);
						p3b = glm::vec2(moreCurve.as.bezier3.p3.x, moreCurve.as.bezier3.p3.y);
						break;
					case CurveType::Bezier2:
					{
						glm::vec2 p0 = glm::vec2(moreCurve.p0.x, moreCurve.p0.y);
						glm::vec2 p1 = glm::vec2(moreCurve.as.bezier2.p1.x, moreCurve.as.bezier2.p1.y);
						glm::vec2 p2 = glm::vec2(moreCurve.as.bezier2.p2.x, moreCurve.as.bezier2.p2.y);

						// Degree elevated quadratic bezier curve
						p1b = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
						p2b = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
						p3b = p2;
					}
					break;
					case CurveType::Line:
						p1b = glm::vec2(moreCurve.p0.x, moreCurve.p0.y);
						p2b = glm::vec2(moreCurve.as.line.p1.x, moreCurve.as.line.p1.y);
						p3b = p2b;
						break;
					default:
						g_logger_warning("Unknown curve type %d", moreCurve.type);
						break;
					}

					// Then interpolate between the control points
					glm::vec2 interpP1 = (p1b - p1a) * t + p1a;
					glm::vec2 interpP2 = (p2b - p2a) * t + p2a;
					glm::vec2 interpP3 = (p3b - p3a) * t + p3a;

					// Then draw
					nvgBezierTo(vg,
						interpP1.x, interpP1.y,
						interpP2.x, interpP2.y,
						interpP3.x, interpP3.y);
				}

				nvgStroke(vg);
				nvgFill(vg);
				nvgClosePath(vg);

				lessContouri++;
				moreContouri += numContoursToSkip;
				if (moreContouri >= moreContours->numContours)
				{
					moreContouri = moreContours->numContours - 1;
				}
			}

			nvgResetTransform(vg);
		}

		static void transformPoint(Vec3* point, const Vec3& offset, const Vec3& viewboxPos, const Vec3& viewboxSize)
		{
			*point = ((*point + offset) - viewboxPos);
			point->x /= viewboxSize.x;
			point->y /= viewboxSize.y;
		}

		static void generateSvgCache(uint32 width, uint32 height)
		{
			if (svgCache.fbo != UINT32_MAX)
			{
				svgCache.destroy();
			}

			if (width > 4096 || height > 4096)
			{
				g_logger_error("SVG cache cannot be bigger than 4096x4096 pixels. The SVG will be truncated.");
				width = 4096;
				height = 4096;
			}

			// Default the svg framebuffer cache to 1024x1024 and resize if necessary
			Texture cacheTexture = TextureBuilder()
				.setFormat(ByteFormat::RGBA8_UI)
				.setMinFilter(FilterMode::Linear)
				.setMagFilter(FilterMode::Linear)
				.setWidth(width)
				.setHeight(height)
				.build();
			svgCache = FramebufferBuilder(width, height)
				.addColorAttachment(cacheTexture)
				.includeDepthStencil()
				.generate();
		}
	}

	// ----------------- SvgObject functions -----------------
	// SvgObject internal functions
	static void renderCreateAnimation2D(NVGcontext* vg, float t, const AnimObject* parent, const Vec3& textureOffset, bool reverse, const SvgObject* obj, bool renderBBoxes);
	static void renderCreateAnimation3D(float t, const AnimObject* parent, bool reverse, const SvgObject* obj);

	void SvgObject::normalize(const Vec2& inMin, const Vec2& inMax)
	{
		// First find the min max of the entire curve
		Vec2 min = inMin;
		Vec2 max = inMax;
		if (min.x == FLT_MAX && min.y == FLT_MAX && max.x == FLT_MIN && max.y == FLT_MIN)
		{
			for (int contouri = 0; contouri < this->numContours; contouri++)
			{
				for (int curvei = 0; curvei < this->contours[contouri].numCurves; curvei++)
				{
					const Curve& curve = contours[contouri].curves[curvei];
					glm::vec3 p0 = { curve.p0.x, curve.p0.y, curve.p0.z };

					min.x = glm::min(p0.x, min.x);
					max.x = glm::max(p0.x, max.x);
					min.y = glm::min(p0.y, min.y);
					max.y = glm::max(p0.y, max.y);

					switch (curve.type)
					{
					case CurveType::Bezier3:
					{
						glm::vec3 p1 = { curve.as.bezier3.p1.x, curve.as.bezier3.p1.y, curve.as.bezier3.p1.z };
						glm::vec3 p2 = { curve.as.bezier3.p2.x, curve.as.bezier3.p2.y, curve.as.bezier3.p2.z };
						glm::vec3 p3 = { curve.as.bezier3.p3.x, curve.as.bezier3.p3.y, curve.as.bezier3.p3.z };

						min.x = glm::min(p1.x, min.x);
						max.x = glm::max(p1.x, max.x);
						min.y = glm::min(p1.y, min.y);
						max.y = glm::max(p1.y, max.y);

						min.x = glm::min(p2.x, min.x);
						max.x = glm::max(p2.x, max.x);
						min.y = glm::min(p2.y, min.y);
						max.y = glm::max(p2.y, max.y);

						min.x = glm::min(p3.x, min.x);
						max.x = glm::max(p3.x, max.x);
						min.y = glm::min(p3.y, min.y);
						max.y = glm::max(p3.y, max.y);
					}
					break;
					case CurveType::Bezier2:
					{
						glm::vec3 p1 = { curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, curve.as.bezier2.p1.z };
						glm::vec3 p2 = { curve.as.bezier2.p2.x, curve.as.bezier2.p2.y, curve.as.bezier2.p2.z };

						min.x = glm::min(p1.x, min.x);
						max.x = glm::max(p1.x, max.x);
						min.y = glm::min(p1.y, min.y);
						max.y = glm::max(p1.y, max.y);

						min.x = glm::min(p2.x, min.x);
						max.x = glm::max(p2.x, max.x);
						min.y = glm::min(p2.y, min.y);
						max.y = glm::max(p2.y, max.y);
					}
					break;
					case CurveType::Line:
					{
						glm::vec3 p1 = { curve.as.line.p1.x, curve.as.line.p1.y, curve.as.line.p1.z };

						min.x = glm::min(p1.x, min.x);
						max.x = glm::max(p1.x, max.x);
						min.y = glm::min(p1.y, min.y);
						max.y = glm::max(p1.y, max.y);
					}
					break;
					}
				}
			}
		}

		// Then map everything to a [0.0-1.0] range from there
		Vec2 hzOutputRange = Vec2{ 0.0f, 1.0f };
		Vec2 vtOutputRange = Vec2{ 0.0f, 1.0f };
		for (int contouri = 0; contouri < this->numContours; contouri++)
		{
			for (int curvei = 0; curvei < this->contours[contouri].numCurves; curvei++)
			{
				Curve& curve = contours[contouri].curves[curvei];
				curve.p0.x = CMath::mapRange(Vec2{ min.x, max.x }, hzOutputRange, curve.p0.x);
				curve.p0.y = CMath::mapRange(Vec2{ min.y, max.y }, vtOutputRange, curve.p0.y);

				switch (curve.type)
				{
				case CurveType::Bezier3:
				{
					curve.as.bezier3.p1.x = CMath::mapRange(Vec2{ min.x, max.x }, hzOutputRange, curve.as.bezier3.p1.x);
					curve.as.bezier3.p1.y = CMath::mapRange(Vec2{ min.y, max.y }, vtOutputRange, curve.as.bezier3.p1.y);

					curve.as.bezier3.p2.x = CMath::mapRange(Vec2{ min.x, max.x }, hzOutputRange, curve.as.bezier3.p2.x);
					curve.as.bezier3.p2.y = CMath::mapRange(Vec2{ min.y, max.y }, vtOutputRange, curve.as.bezier3.p2.y);

					curve.as.bezier3.p3.x = CMath::mapRange(Vec2{ min.x, max.x }, hzOutputRange, curve.as.bezier3.p3.x);
					curve.as.bezier3.p3.y = CMath::mapRange(Vec2{ min.y, max.y }, vtOutputRange, curve.as.bezier3.p3.y);

				}
				break;
				case CurveType::Bezier2:
				{
					curve.as.bezier2.p1.x = CMath::mapRange(Vec2{ min.x, max.x }, hzOutputRange, curve.as.bezier2.p1.x);
					curve.as.bezier2.p1.y = CMath::mapRange(Vec2{ min.y, max.y }, vtOutputRange, curve.as.bezier2.p1.y);

					curve.as.bezier2.p2.x = CMath::mapRange(Vec2{ min.x, max.x }, hzOutputRange, curve.as.bezier2.p2.x);
					curve.as.bezier2.p2.y = CMath::mapRange(Vec2{ min.y, max.y }, vtOutputRange, curve.as.bezier2.p2.y);
				}
				break;
				case CurveType::Line:
				{
					curve.as.line.p1.x = CMath::mapRange(Vec2{ min.x, max.x }, hzOutputRange, curve.as.line.p1.x);
					curve.as.line.p1.y = CMath::mapRange(Vec2{ min.y, max.y }, vtOutputRange, curve.as.line.p1.y);
				}
				break;
				}
			}
		}
	}

	void SvgObject::calculateApproximatePerimeter()
	{
		approximatePerimeter = 0.0f;

		for (int contouri = 0; contouri < this->numContours; contouri++)
		{
			for (int curvei = 0; curvei < this->contours[contouri].numCurves; curvei++)
			{
				const Curve& curve = contours[contouri].curves[curvei];
				glm::vec3 p0 = { curve.p0.x, curve.p0.y, curve.p0.z };

				switch (curve.type)
				{
				case CurveType::Bezier3:
				{
					glm::vec3 p1 = { curve.as.bezier3.p1.x, curve.as.bezier3.p1.y, curve.as.bezier3.p1.z };
					glm::vec3 p2 = { curve.as.bezier3.p2.x, curve.as.bezier3.p2.y, curve.as.bezier3.p2.z };
					glm::vec3 p3 = { curve.as.bezier3.p3.x, curve.as.bezier3.p3.y, curve.as.bezier3.p3.z };
					float chordLength = glm::length(p3 - p0);
					float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p3 - p2);
					float approxLength = (chordLength + controlNetLength) / 2.0f;
					approximatePerimeter += approxLength;
				}
				break;
				case CurveType::Bezier2:
				{
					glm::vec3 p1 = { curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, curve.as.bezier2.p1.z };
					glm::vec3 p2 = { curve.as.bezier2.p2.x, curve.as.bezier2.p2.y, curve.as.bezier2.p2.z };
					float chordLength = glm::length(p2 - p0);
					float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1);
					float approxLength = (chordLength + controlNetLength) / 2.0f;
					approximatePerimeter += approxLength;
				}
				break;
				case CurveType::Line:
				{
					glm::vec3 p1 = { curve.as.line.p1.x, curve.as.line.p1.y, curve.as.line.p1.z };
					approximatePerimeter += glm::length(p1 - p0);
				}
				break;
				}
			}
		}
	}

	void SvgObject::calculateBBox()
	{
		bbox.min.x = FLT_MAX;
		bbox.min.y = FLT_MAX;
		bbox.max.x = FLT_MIN;
		bbox.max.y = FLT_MIN;

		for (int contouri = 0; contouri < numContours; contouri++)
		{
			if (contours[contouri].numCurves > 0)
			{
				for (int curvei = 0; curvei < contours[contouri].numCurves; curvei++)
				{
					const Curve& curve = contours[contouri].curves[curvei];
					Vec3 p0 = Vec3{ curve.p0.x, curve.p0.y, 0.0f };

					switch (curve.type)
					{
					case CurveType::Bezier3:
					{
						BBox subBbox = CMath::bezier3BBox(p0, curve.as.bezier3.p1, curve.as.bezier3.p2, curve.as.bezier3.p3);
						bbox.min = CMath::min(bbox.min, subBbox.min);
						bbox.max = CMath::max(bbox.max, subBbox.max);
					}
					break;
					case CurveType::Bezier2:
					{
						BBox subBbox = CMath::bezier2BBox(p0, curve.as.bezier2.p1, curve.as.bezier2.p2);
						bbox.min = CMath::min(bbox.min, subBbox.min);
						bbox.max = CMath::max(bbox.max, subBbox.max);
					}
					break;
					case CurveType::Line:
					{
						BBox subBbox = CMath::bezier1BBox(p0, curve.as.line.p1);
						bbox.min = CMath::min(bbox.min, subBbox.min);
						bbox.max = CMath::max(bbox.max, subBbox.max);
					}
					break;
					default:
						g_logger_warning("Unknown curve type in render %d", (int)curve.type);
						break;
					}
				}
			}
		}
	}

	void SvgObject::render(NVGcontext* vg, const AnimObject* parent, const Vec3& offset, bool renderBBoxes) const
	{
		renderCreateAnimation(vg, 1.01f, parent, offset, false, false, renderBBoxes);
	}

	void SvgObject::renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent, const Vec3& offset, bool reverse, bool isSvgGroup, bool renderBBoxes) const
	{
		if (this->is3D)
		{
			renderCreateAnimation3D(t, parent, reverse, this);
		}
		else
		{
			Vec3 svgTextureOffset = Vec3{
				(float)cacheCurrentX + parent->strokeWidth * 0.5f,
				(float)cacheCurrentY + parent->strokeWidth * 0.5f,
				0.0f
			};

			// Check if the SVG cache needs to regenerate
			float svgTotalWidth = ((bbox.max.x - bbox.min.x) * parent->scale.x) + parent->strokeWidth;
			float svgTotalHeight = ((bbox.max.y - bbox.min.y) * parent->scale.y) + parent->strokeWidth;
			{
				float newRightX = svgTextureOffset.x + svgTotalWidth;
				if (newRightX >= svgCache.width)
				{
					// Move to the newline
					cacheCurrentY += cacheLineHeight;
					cacheLineHeight = 0;
					cacheCurrentX = 0;
				}

				float newBottomY = svgTextureOffset.y + svgTotalHeight;
				if (newBottomY >= svgCache.height)
				{
					// Double the size of the texture (up to 8192x8192 max)
					Svg::generateSvgCache(svgCache.width * 2, svgCache.height * 2);
				}

				svgTextureOffset = Vec3{
					(float)cacheCurrentX + parent->strokeWidth * 0.5f,
					(float)cacheCurrentY + parent->strokeWidth * 0.5f,
					0.0f
				};
			}

			// Render to the framebuffer then blit the framebuffer to the screen
			// with the appropriate transformations
			int32 lastFboId;
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastFboId);

			// First render to the cache
			svgCache.bind();
			glViewport(0, 0, svgCache.width, svgCache.height);

			// Reset the draw buffers to draw to FB_attachment_0
			GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE };
			glDrawBuffers(3, compositeDrawBuffers);

			if (isSvgGroup)
			{
				svgTextureOffset.x += offset.x * parent->scale.x;
				svgTextureOffset.y += offset.y * parent->scale.y;
			}

			nvgBeginFrame(vg, svgCache.width, svgCache.height, 1.0f);
			renderCreateAnimation2D(vg, t, parent, svgTextureOffset, reverse, this, renderBBoxes);
			nvgEndFrame(vg);

			// Subtract half stroke width to make sure it's getting the correct coords
			svgTextureOffset -= Vec3{ parent->strokeWidth * 0.5f, parent->strokeWidth * 0.5f, 0.0f };
			Vec2 cacheUvMin = Vec2{
				(svgTextureOffset.x + 0.5f) / (float)svgCache.width,
				1.0f - ((svgTextureOffset.y + 0.5f) / (float)svgCache.height) - (svgTotalHeight / (float)svgCache.height)
			};
			Vec2 cacheUvMax = cacheUvMin +
				Vec2{
					((svgTotalWidth - 0.5f) / (float)svgCache.width),
					((svgTotalHeight - 0.5f) / (float)svgCache.height)
			};

			// Then bind the previous fbo and blit it to the screen with
			// the appropriate transformations if it's not an svg group
			// SVG Groups get drawn in one draw call
			glBindFramebuffer(GL_FRAMEBUFFER, lastFboId);
			// Reset the draw buffers to draw to FB_attachment_0
			glDrawBuffers(3, compositeDrawBuffers);

			if (!isSvgGroup)
			{
				cacheCurrentX += svgTotalWidth;
				cacheLineHeight = glm::max(cacheLineHeight, svgTotalHeight);

				glm::mat4 transform = glm::identity<glm::mat4>();
				Vec2 cameraCenteredPos = Svg::camera->projectionSize / 2.0f - Svg::camera->position;
				transform = glm::translate(
					transform,
					glm::vec3(
						parent->position.x - cameraCenteredPos.x + (offset.x * parent->scale.x),
						parent->position.y - cameraCenteredPos.y + (offset.y * parent->scale.y),
						0.0f
					)
				);
				if (!CMath::compare(parent->rotation.z, 0.0f))
				{
					transform = glm::rotate(transform, parent->rotation.z, glm::vec3(0, 0, 1));
				}

				// Correct for aspect ratio
				float targetRatio = Application::getOutputTargetAspectRatio();
				svgTotalHeight /= targetRatio;

				glEnable(GL_BLEND);
				Renderer::drawTexture(
					svgCache.getColorAttachment(0),
					Vec2{ svgTotalWidth, svgTotalHeight },
					cacheUvMin,
					cacheUvMax,
					transform
				);
			}
		}
	}

	void SvgObject::free()
	{
		for (int contouri = 0; contouri < numContours; contouri++)
		{
			if (contours[contouri].curves)
			{
				g_memory_free(contours[contouri].curves);
			}
			contours[contouri].numCurves = 0;
			contours[contouri].maxCapacity = 0;
		}

		if (contours)
		{
			g_memory_free(contours);
		}

		numContours = 0;
		approximatePerimeter = 0.0f;
	}

	void SvgGroup::normalize()
	{
		Vec3 translation = Vec3{ viewbox.values[0], viewbox.values[1], 0.0f };

		calculateBBox();
		//for (int i = 0; i < numObjects; i++)
		//{
		//	SvgObject& obj = objects[i];
		//	Vec3& offset = objectOffsets[i];
		//	Vec2 absOffset = CMath::vector2From3(offset - translation);
		//	obj.normalize(bbox.min, bbox.max);
		//	offset.x = CMath::mapRange(Vec2{ bbox.min.x, bbox.max.x }, Vec2{ 0.0f, 1.0f }, absOffset.x);
		//	offset.y = CMath::mapRange(Vec2{ bbox.min.y, bbox.max.y }, Vec2{ 0.0f, 1.0f }, absOffset.y);
		//	obj.calculateSvgSize();
		//}
		//viewbox.values[0] = CMath::mapRange(Vec2{ bbox.min.x, bbox.max.x }, Vec2{ 0.0f, 1.0f }, viewbox.values[0]);
		//viewbox.values[1] = CMath::mapRange(Vec2{ bbox.min.x, bbox.max.x }, Vec2{ 0.0f, 1.0f }, viewbox.values[1]);
		//viewbox.values[2] = CMath::mapRange(Vec2{ bbox.min.x, bbox.max.x }, Vec2{ 0.0f, 1.0f }, viewbox.values[2]);
		//viewbox.values[3] = CMath::mapRange(Vec2{ bbox.min.x, bbox.max.x }, Vec2{ 0.0f, 1.0f }, viewbox.values[3]);
		//calculateBBox();
	}

	void SvgGroup::calculateBBox()
	{
		Vec3 translation = Vec3{ viewbox.values[0], viewbox.values[1], 0.0f };
		bbox.min = Vec2{ FLT_MAX, FLT_MAX };
		bbox.max = Vec2{ FLT_MIN, FLT_MIN };

		for (int i = 0; i < numObjects; i++)
		{
			SvgObject& obj = objects[i];
			const Vec3& offset = objectOffsets[i];

			Vec2 absOffset = CMath::vector2From3(offset - translation);
			obj.calculateBBox();
			bbox.min = CMath::min(obj.bbox.min + absOffset, bbox.min);
			bbox.max = CMath::max(obj.bbox.max + absOffset, bbox.max);
		}
	}

	void SvgGroup::render(NVGcontext* vg, AnimObject* parent, bool renderBBoxes) const
	{
		renderCreateAnimation(vg, 1.01f, parent, false, renderBBoxes);
	}

	void SvgGroup::renderCreateAnimation(NVGcontext* vg, float t, AnimObject* parent, bool reverse, bool renderBBoxes) const
	{
		Vec3 translation = Vec3{ viewbox.values[0], viewbox.values[1], 0.0f };
		Vec3 bboxOffset = Vec3{ bbox.min.x, bbox.min.y, 0.0f };

		float numberObjectsToDraw = t * (float)numObjects;
		constexpr float numObjectsToLag = 2.0f;
		float numObjectsDrawn = 0.0f;
		for (int i = 0; i < numObjects; i++)
		{
			const SvgObject& obj = objects[i];
			const Vec3& offset = objectOffsets[i];

			float denominator = i == numObjects - 1 ? 1.0f : numObjectsToLag;
			float percentOfLetterToDraw = (numberObjectsToDraw - numObjectsDrawn) / denominator;
			Vec3 absOffset = offset - translation - bboxOffset + Vec3{ parent->strokeWidth * 0.5f, parent->strokeWidth * 0.5f, 0.0f };
			obj.renderCreateAnimation(vg, percentOfLetterToDraw, parent, absOffset, reverse, true, renderBBoxes);
			numObjectsDrawn += 1.0f;

			if (numObjectsDrawn >= numberObjectsToDraw)
			{
				break;
			}
		}

		// Then blit the SVG group to the screen
		glm::mat4 transform = glm::identity<glm::mat4>();
		Vec2 cameraCenteredPos = Svg::camera->projectionSize / 2.0f - Svg::camera->position;
		transform = glm::translate(
			transform,
			glm::vec3(
				parent->position.x - cameraCenteredPos.x,
				parent->position.y - cameraCenteredPos.y,
				0.0f
			)
		);
		if (!CMath::compare(parent->rotation.z, 0.0f))
		{
			transform = glm::rotate(transform, parent->rotation.z, glm::vec3(0, 0, 1));
		}

		Vec3 svgTextureOffset = Vec3{
				(float)cacheCurrentX + parent->strokeWidth * 0.5f,
				(float)cacheCurrentY + parent->strokeWidth * 0.5f,
				0.0f
		};
		float svgTotalWidth = ((bbox.max.x - bbox.min.x) * parent->scale.x) + parent->strokeWidth;
		float svgTotalHeight = ((bbox.max.y - bbox.min.y) * parent->scale.y) + parent->strokeWidth;
		Vec2 cacheUvMin = Vec2{
			(svgTextureOffset.x + 0.5f) / (float)svgCache.width,
			1.0f - ((svgTextureOffset.y + 0.5f) / (float)svgCache.height) - (svgTotalHeight / (float)svgCache.height)
		};
		Vec2 cacheUvMax = cacheUvMin +
			Vec2{
				((svgTotalWidth - 0.5f) / (float)svgCache.width),
				((svgTotalHeight - 0.5f) / (float)svgCache.height)
		};

		// Correct for aspect ratio
		float targetRatio = Application::getOutputTargetAspectRatio();
		svgTotalHeight /= targetRatio;

		glEnable(GL_BLEND);
		Renderer::drawTexture(
			svgCache.getColorAttachment(0),
			Vec2{ svgTotalWidth, svgTotalHeight },
			cacheUvMin,
			cacheUvMax,
			transform
		);

		if (renderBBoxes)
		{
			// Render to the framebuffer then blit the framebuffer to the screen
			// with the appropriate transformations
			int32 lastFboId;
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastFboId);

			// First render to the cache
			svgCache.bind();
			glViewport(0, 0, svgCache.width, svgCache.height);

			// Reset the draw buffers to draw to FB_attachment_0
			GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE };
			glDrawBuffers(3, compositeDrawBuffers);

			nvgBeginFrame(vg, svgCache.width, svgCache.height, 1.0f);
			Vec3 svgTextureOffset = Vec3{ (float)cacheCurrentX, (float)cacheCurrentY, 0.0f };

			nvgBeginPath(vg);
			nvgStrokeColor(vg, nvgRGBA(0, 255, 255, 255));
			nvgStrokeWidth(vg, 5.0f);
			nvgMoveTo(vg, cacheCurrentX, cacheCurrentY);
			nvgRect(vg,
				cacheCurrentX,
				cacheCurrentY,
				((bbox.max.x - bbox.min.x) * parent->scale.x) + parent->strokeWidth,
				((bbox.max.y - bbox.min.y) * parent->scale.y) + parent->strokeWidth
			);
			nvgClosePath(vg);
			nvgStroke(vg);
			nvgEndFrame(vg);

			// Then bind the previous fbo and blit it to the screen with
			// the appropriate transformations
			glBindFramebuffer(GL_FRAMEBUFFER, lastFboId);

			// Reset the draw buffers to draw to FB_attachment_0
			glDrawBuffers(3, compositeDrawBuffers);
		}

		cacheCurrentX += ((bbox.max.x - bbox.min.x) * parent->scale.x) + (parent->strokeWidth * 0.5f);
		cacheLineHeight = glm::max(cacheLineHeight, ((bbox.max.y - bbox.min.y) * parent->scale.y) + (parent->strokeWidth * 0.5f));
	}

	void SvgGroup::free()
	{
		for (int i = 0; i < numUniqueObjects; i++)
		{
			if (uniqueObjectNames && uniqueObjectNames[i])
			{
				g_memory_free(uniqueObjectNames[i]);
				uniqueObjectNames[i] = nullptr;
			}

			if (uniqueObjects)
			{
				uniqueObjects[i].free();
			}
		}

		if (uniqueObjectNames)
		{
			g_memory_free(uniqueObjectNames);
		}

		if (uniqueObjects)
		{
			g_memory_free(uniqueObjects);
		}

		if (objects)
		{
			g_memory_free(objects);
		}

		if (objectOffsets)
		{
			g_memory_free(objectOffsets);
		}

		numUniqueObjects = 0;
		numObjects = 0;
		objectOffsets = nullptr;
		objects = nullptr;
		uniqueObjects = nullptr;
		uniqueObjectNames = nullptr;
		viewbox = Vec4{ 0, 0, 0, 0 };
	}

	// ------------------- Svg Object Internal functions -------------------
	static void renderCreateAnimation2D(NVGcontext* vg, float t, const AnimObject* parent, const Vec3& textureOffset, bool reverse, const SvgObject* obj, bool renderBBoxes)
	{
		if (reverse)
		{
			t = 1.0f - t;
		}

		// Start the fade in after 80% of the svg object is drawn
		constexpr float fadeInStart = 0.8f;
		const glm::vec2 position = { parent->position.x, parent->position.y };
		float lengthToDraw = t * (float)obj->approximatePerimeter;
		float amountToFadeIn = ((t - fadeInStart) / (1.0f - fadeInStart));
		float percentToFadeIn = glm::max(glm::min(amountToFadeIn, 1.0f), 0.0f);

		// NOTE(voxel): Quick and Dirty
		//Vec2 cameraCenteredPos = Svg::camera->projectionSize / 2.0f - Svg::camera->position;
		//nvgTranslate(vg, position.x - cameraCenteredPos.x, position.y - cameraCenteredPos.y);
		nvgTranslate(vg, textureOffset.x, textureOffset.y);
		if (parent->rotation.z != 0.0f)
		{
			//nvgRotate(vg, glm::radians(parent->rotation.z));
		}

		if (lengthToDraw > 0)
		{
			float lengthDrawn = 0.0f;
			for (int contouri = 0; contouri < obj->numContours; contouri++)
			{
				if (obj->contours[contouri].numCurves > 0)
				{
					nvgBeginPath(vg);

					// Fade the stroke out as the svg fades in
					const glm::u8vec4& strokeColor = parent->strokeColor;
					if (glm::epsilonEqual(parent->strokeWidth, 0.0f, 0.01f))
					{
						nvgStrokeColor(vg, nvgRGBA(strokeColor.r, strokeColor.g, strokeColor.b, (unsigned char)((float)strokeColor.a * (1.0f - percentToFadeIn))));
						nvgStrokeWidth(vg, 5.0f);
					}
					else
					{
						nvgStrokeColor(vg, nvgRGBA(strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a));
						nvgStrokeWidth(vg, parent->strokeWidth);
					}

					nvgMoveTo(vg,
						obj->contours[contouri].curves[0].p0.x * parent->scale.x,
						obj->contours[contouri].curves[0].p0.y * parent->scale.y
					);

					for (int curvei = 0; curvei < obj->contours[contouri].numCurves; curvei++)
					{
						float lengthLeft = lengthToDraw - lengthDrawn;
						if (lengthLeft < 0.0f)
						{
							break;
						}

						const Curve& curve = obj->contours[contouri].curves[curvei];
						glm::vec4 p0 = glm::vec4(
							curve.p0.x,
							curve.p0.y,
							0.0f,
							1.0f
						);
						if (curve.moveToP0)
						{
							nvgMoveTo(vg, p0.x * parent->scale.x, p0.y * parent->scale.y);
						}

						switch (curve.type)
						{
						case CurveType::Bezier3:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier3.p1.x, curve.as.bezier3.p1.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier3.p2.x, curve.as.bezier3.p2.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier3.p3.x, curve.as.bezier3.p3.y, 0.0f, 1.0f };

							float chordLength = glm::length(p3 - p0);
							float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p3 - p2);
							float approxLength = (chordLength + controlNetLength) / 2.0f;
							lengthDrawn += approxLength;

							if (lengthLeft < approxLength)
							{
								// Interpolate the curve
								float percentOfCurveToDraw = lengthLeft / approxLength;

								// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
								float t0 = 0.0f;
								float t1 = percentOfCurveToDraw;
								float u0 = 1.0f;
								float u1 = (1.0f - t1);

								glm::vec4 q0 = ((u0 * u0 * u0) * p0) +
									((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
									((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
									((t0 * t0 * t0) * p3);
								glm::vec4 q1 = ((u0 * u0 * u1) * p0) +
									((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
									((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
									((t0 * t0 * t1) * p3);
								glm::vec4 q2 = ((u0 * u1 * u1) * p0) +
									((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
									((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
									((t0 * t1 * t1) * p3);
								glm::vec4 q3 = ((u1 * u1 * u1) * p0) +
									((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
									((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
									((t1 * t1 * t1) * p3);

								p1 = q1;
								p2 = q2;
								p3 = q3;
							}

							nvgBezierTo(
								vg,
								p1.x * parent->scale.x, p1.y * parent->scale.y,
								p2.x * parent->scale.x, p2.y * parent->scale.y,
								p3.x * parent->scale.x, p3.y * parent->scale.y
							);
						}
						break;
						case CurveType::Bezier2:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier2.p2.x, curve.as.bezier2.p2.y, 0.0f, 1.0f };

							// Degree elevated quadratic bezier curve
							glm::vec4 pr0 = p0;
							glm::vec4 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
							glm::vec4 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
							glm::vec4 pr3 = p3;

							float chordLength = glm::length(pr3 - pr0);
							float controlNetLength = glm::length(pr1 - pr0) + glm::length(pr2 - pr1) + glm::length(pr3 - pr2);
							float approxLength = (chordLength + controlNetLength) / 2.0f;
							lengthDrawn += approxLength;

							if (lengthLeft < approxLength)
							{
								// Interpolate the curve
								float percentOfCurveToDraw = lengthLeft / approxLength;

								p1 = (pr1 - pr0) * percentOfCurveToDraw + pr0;
								p2 = (pr2 - pr1) * percentOfCurveToDraw + pr1;
								p3 = (pr3 - pr2) * percentOfCurveToDraw + pr2;

								// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
								float t0 = 0.0f;
								float t1 = percentOfCurveToDraw;
								float u0 = 1.0f;
								float u1 = (1.0f - t1);

								glm::vec4 q0 = ((u0 * u0 * u0) * p0) +
									((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
									((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
									((t0 * t0 * t0) * p3);
								glm::vec4 q1 = ((u0 * u0 * u1) * p0) +
									((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
									((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
									((t0 * t0 * t1) * p3);
								glm::vec4 q2 = ((u0 * u1 * u1) * p0) +
									((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
									((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
									((t0 * t1 * t1) * p3);
								glm::vec4 q3 = ((u1 * u1 * u1) * p0) +
									((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
									((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
									((t1 * t1 * t1) * p3);

								pr1 = q1;
								pr2 = q2;
								pr3 = q3;
							}

							nvgBezierTo(
								vg,
								pr1.x * parent->scale.x, pr1.y * parent->scale.y,
								pr2.x * parent->scale.x, pr2.y * parent->scale.y,
								pr3.x * parent->scale.x, pr3.y * parent->scale.y
							);
						}
						break;
						case CurveType::Line:
						{
							glm::vec4 p1 = glm::vec4(
								curve.as.line.p1.x,
								curve.as.line.p1.y,
								0.0f,
								1.0f
							);
							float curveLength = glm::length(p1 - p0);
							lengthDrawn += curveLength;

							if (lengthLeft < curveLength)
							{
								float percentOfCurveToDraw = lengthLeft / curveLength;
								p1 = (p1 - p0) * percentOfCurveToDraw + p0;
							}

							nvgLineTo(vg, p1.x * parent->scale.x, p1.y * parent->scale.y);
						}
						break;
						default:
							g_logger_warning("Unknown curve type in render %d", (int)curve.type);
							break;
						}
					}
				}

				nvgStroke(vg);

				if (lengthDrawn > lengthToDraw)
				{
					break;
				}
			}
		}

		if (amountToFadeIn > 0)
		{
			for (int contouri = 0; contouri < obj->numContours; contouri++)
			{
				if (obj->contours[contouri].numCurves > 0)
				{
					// TODO: De-deuplicate this by just calling render
					const glm::u8vec4& fillColor = parent->fillColor;
					nvgFillColor(vg, nvgRGBA(fillColor.r, fillColor.g, fillColor.b, (unsigned char)(fillColor.a * percentToFadeIn)));
					nvgBeginPath(vg);
					nvgPathWinding(vg, NVG_CW);

					nvgMoveTo(vg,
						obj->contours[contouri].curves[0].p0.x * parent->scale.x,
						obj->contours[contouri].curves[0].p0.y * parent->scale.y
					);

					for (int curvei = 0; curvei < obj->contours[contouri].numCurves; curvei++)
					{
						const Curve& curve = obj->contours[contouri].curves[curvei];
						glm::vec4 p0 = glm::vec4(
							curve.p0.x,
							curve.p0.y,
							0.0f,
							1.0f
						);

						if (curvei != 0 && curve.moveToP0)
						{
							nvgMoveTo(vg, p0.x * parent->scale.x, p0.y * parent->scale.y);
							// TODO: Does this work consistently???
							nvgPathWinding(vg, NVG_HOLE);
						}

						switch (curve.type)
						{
						case CurveType::Bezier3:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier3.p1.x, curve.as.bezier3.p1.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier3.p2.x, curve.as.bezier3.p2.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier3.p3.x, curve.as.bezier3.p3.y, 0.0f, 1.0f };

							nvgBezierTo(
								vg,
								p1.x * parent->scale.x, p1.y * parent->scale.y,
								p2.x * parent->scale.x, p2.y * parent->scale.y,
								p3.x * parent->scale.x, p3.y * parent->scale.y
							);
						}
						break;
						case CurveType::Bezier2:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier2.p2.x, curve.as.bezier2.p2.y, 0.0f, 1.0f };

							// Degree elevated quadratic bezier curve
							glm::vec4 pr0 = p0;
							glm::vec4 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
							glm::vec4 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
							glm::vec4 pr3 = p3;

							nvgBezierTo(
								vg,
								pr1.x * parent->scale.x, pr1.y * parent->scale.y,
								pr2.x * parent->scale.x, pr2.y * parent->scale.y,
								pr3.x * parent->scale.x, pr3.y * parent->scale.y
							);
						}
						break;
						case CurveType::Line:
						{
							glm::vec4 p1 = glm::vec4(
								curve.as.line.p1.x * parent->scale.x,
								curve.as.line.p1.y * parent->scale.y,
								0.0f,
								1.0f
							);

							nvgLineTo(vg, p1.x, p1.y);
						}
						break;
						default:
							g_logger_warning("Unknown curve type in render %d", (int)curve.type);
							break;
						}
					}

					nvgClosePath(vg);
				}
			}

			nvgFill(vg);
		}

		if (renderBBoxes)
		{
			for (int contouri = 0; contouri < obj->numContours; contouri++)
			{
				if (obj->contours[contouri].numCurves > 0)
				{
					for (int curvei = 0; curvei < obj->contours[contouri].numCurves; curvei++)
					{
						const Curve& curve = obj->contours[contouri].curves[curvei];
						glm::vec4 p0 = glm::vec4(
							curve.p0.x,
							curve.p0.y,
							0.0f,
							1.0f
						);

						switch (curve.type)
						{
						case CurveType::Bezier3:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier3.p1.x, curve.as.bezier3.p1.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier3.p2.x, curve.as.bezier3.p2.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier3.p3.x, curve.as.bezier3.p3.y, 0.0f, 1.0f };

							Vec2 tp0 = Vec2{ p0.x * parent->scale.x, p0.y * parent->scale.y };
							Vec2 tp1 = Vec2{ p1.x * parent->scale.x, p1.y * parent->scale.y };
							Vec2 tp2 = Vec2{ p2.x * parent->scale.x, p2.y * parent->scale.y };
							Vec2 tp3 = Vec2{ p3.x * parent->scale.x, p3.y * parent->scale.y };
							BBox bbox = CMath::bezier3BBox(tp0, tp1, tp2, tp3);

							nvgBeginPath(vg);
							nvgStrokeWidth(vg, 5.0f);
							nvgStrokeColor(vg, nvgRGB(255, 0, 0));
							nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
							nvgMoveTo(vg, bbox.min.x, bbox.min.y);
							nvgRect(vg, bbox.min.x, bbox.min.y, bbox.max.x - bbox.min.x, bbox.max.y - bbox.min.y);
							nvgClosePath(vg);
							nvgStroke(vg);
						}
						break;
						case CurveType::Bezier2:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier2.p2.x, curve.as.bezier2.p2.y, 0.0f, 1.0f };

							// Degree elevated quadratic bezier curve
							glm::vec4 pr0 = p0;
							glm::vec4 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
							glm::vec4 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
							glm::vec4 pr3 = p3;

							Vec2 tp0 = Vec2{ p0.x * parent->scale.x, p0.y * parent->scale.y };
							Vec2 tp1 = Vec2{ p1.x * parent->scale.x, p1.y * parent->scale.y };
							Vec2 tp2 = Vec2{ p3.x * parent->scale.x, p3.y * parent->scale.y };
							BBox bbox = CMath::bezier2BBox(tp0, tp1, tp2);

							nvgBeginPath(vg);
							nvgStrokeWidth(vg, 5.0f);
							nvgStrokeColor(vg, nvgRGB(255, 0, 0));
							nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
							nvgMoveTo(vg, bbox.min.x, bbox.min.y);
							nvgRect(vg, bbox.min.x, bbox.min.y, bbox.max.x - bbox.min.x, bbox.max.y - bbox.min.y);
							nvgClosePath(vg);
							nvgStroke(vg);
						}
						break;
						case CurveType::Line:
						{
							glm::vec4 p1 = glm::vec4(
								curve.as.line.p1.x * parent->scale.x,
								curve.as.line.p1.y * parent->scale.y,
								0.0f,
								1.0f
							);

							Vec2 tp0 = Vec2{ p0.x * parent->scale.x, p0.y * parent->scale.y };
							Vec2 tp1 = Vec2{ p1.x, p1.y };
							BBox bbox = CMath::bezier1BBox(tp0, tp1);

							nvgBeginPath(vg);
							nvgStrokeWidth(vg, 5.0f);
							nvgStrokeColor(vg, nvgRGB(255, 0, 0));
							nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
							nvgMoveTo(vg, bbox.min.x, bbox.min.y);
							nvgRect(vg, bbox.min.x, bbox.min.y, bbox.max.x - bbox.min.x, bbox.max.y - bbox.min.y);
							nvgClosePath(vg);
							nvgStroke(vg);
						}
						break;
						default:
							g_logger_warning("Unknown curve type in render %d", (int)curve.type);
							break;
						}
					}
				}
			}

			nvgBeginPath(vg);
			nvgStrokeWidth(vg, 5.0f);
			nvgStrokeColor(vg, nvgRGB(0, 255, 0));
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
			nvgMoveTo(vg,
				(obj->bbox.min.x * parent->scale.x) - (parent->strokeWidth * 0.5f),
				(obj->bbox.min.y * parent->scale.y) - (parent->strokeWidth * 0.5f)
			);
			nvgRect(vg,
				(obj->bbox.min.x * parent->scale.x) - (parent->strokeWidth * 0.5f),
				(obj->bbox.min.y * parent->scale.y) - (parent->strokeWidth * 0.5f),
				((obj->bbox.max.x - obj->bbox.min.x) * parent->scale.x) + parent->strokeWidth,
				((obj->bbox.max.y - obj->bbox.min.y) * parent->scale.y) + parent->strokeWidth
			);
			nvgStroke(vg);
			nvgClosePath(vg);
		}

		nvgResetTransform(vg);
	}

	static void render3D(NVGcontext* vg, const AnimObject* parent, const SvgObject* obj)
	{

	}

	static void renderCreateAnimation3D(float t, const AnimObject* parent, bool reverse, const SvgObject* obj)
	{
		if (reverse)
		{
			t = 1.0f - t;
		}


		// TODO: Add rotation to 3D lines somehow...
		Renderer::translate3D(parent->position);
		Renderer::rotate3D(parent->rotation);
		//if (parent->rotation.z != 0.0f)
		//{
		//	nvgRotate(vg, glm::radians(parent->rotation.z));
		//}

		float lengthToDraw = t * (float)obj->approximatePerimeter;
		if (lengthToDraw > 0)
		{
			float lengthDrawn = 0.0f;
			for (int contouri = 0; contouri < obj->numContours; contouri++)
			{
				if (obj->contours[contouri].numCurves > 0)
				{
					// Fade the stroke out as the svg fades in
					const glm::u8vec4& strokeColor = parent->strokeColor;
					glm::vec4 unpackedStrokeColor = glm::vec4(strokeColor) / 255.0f;
					if (glm::epsilonEqual(parent->strokeWidth, 0.0f, 0.01f))
					{
						Renderer::pushColor(unpackedStrokeColor);
						Renderer::pushStrokeWidth(0.2f);
					}
					else
					{
						Renderer::pushColor(unpackedStrokeColor);
						Renderer::pushStrokeWidth(parent->strokeWidth);
					}

					Renderer::beginPath3D(obj->contours[contouri].curves[0].p0);

					bool completedPath = true;
					for (int curvei = 0; curvei < obj->contours[contouri].numCurves; curvei++)
					{
						float lengthLeft = lengthToDraw - lengthDrawn;
						if (lengthLeft < 0.0f)
						{
							completedPath = false;
							break;
						}

						const Curve& curve = obj->contours[contouri].curves[curvei];
						glm::vec4 p0 = glm::vec4(
							curve.p0.x,
							curve.p0.y,
							curve.p0.z,
							1.0f
						);
						if (curve.moveToP0)
						{
							// TODO: Add built-in moveTo support for the 3D paths
							Renderer::endPath3D(false);
							Renderer::beginPath3D(curve.p0);
						}

						switch (curve.type)
						{
						case CurveType::Bezier3:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier3.p1.x, curve.as.bezier3.p1.y, curve.as.bezier3.p1.z, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier3.p2.x, curve.as.bezier3.p2.y, curve.as.bezier3.p2.z, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier3.p3.x, curve.as.bezier3.p3.y, curve.as.bezier3.p3.z, 1.0f };

							float chordLength = glm::length(p3 - p0);
							float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p3 - p2);
							float approxLength = (chordLength + controlNetLength) / 2.0f;
							lengthDrawn += approxLength;

							if (lengthLeft < approxLength)
							{
								completedPath = false;

								// Interpolate the curve
								float percentOfCurveToDraw = lengthLeft / approxLength;

								// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
								float t0 = 0.0f;
								float t1 = percentOfCurveToDraw;
								float u0 = 1.0f;
								float u1 = (1.0f - t1);

								glm::vec4 q0 = ((u0 * u0 * u0) * p0) +
									((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
									((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
									((t0 * t0 * t0) * p3);
								glm::vec4 q1 = ((u0 * u0 * u1) * p0) +
									((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
									((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
									((t0 * t0 * t1) * p3);
								glm::vec4 q2 = ((u0 * u1 * u1) * p0) +
									((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
									((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
									((t0 * t1 * t1) * p3);
								glm::vec4 q3 = ((u1 * u1 * u1) * p0) +
									((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
									((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
									((t1 * t1 * t1) * p3);

								p1 = q1;
								p2 = q2;
								p3 = q3;
							}

							Renderer::bezier3To3D(
								Vec3{ p1.x, p1.y, p1.z },
								Vec3{ p2.x, p2.y, p2.z },
								Vec3{ p3.x, p3.y, p3.z }
							);
						}
						break;
						case CurveType::Bezier2:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, curve.as.bezier2.p1.z, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier2.p1.x, curve.as.bezier2.p1.y, curve.as.bezier2.p1.z, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier2.p2.x, curve.as.bezier2.p2.y, curve.as.bezier2.p2.z, 1.0f };

							// Degree elevated quadratic bezier curve
							glm::vec4 pr0 = p0;
							glm::vec4 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
							glm::vec4 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
							glm::vec4 pr3 = p2;

							float chordLength = glm::length(pr3 - pr0);
							float controlNetLength = glm::length(pr1 - pr0) + glm::length(pr2 - pr1) + glm::length(pr3 - pr2);
							float approxLength = (chordLength + controlNetLength) / 2.0f;
							lengthDrawn += approxLength;

							if (lengthLeft < approxLength)
							{
								completedPath = false;

								// Interpolate the curve
								float percentOfCurveToDraw = lengthLeft / approxLength;

								p1 = (pr1 - pr0) * percentOfCurveToDraw + pr0;
								p2 = (pr2 - pr1) * percentOfCurveToDraw + pr1;
								p3 = (pr3 - pr2) * percentOfCurveToDraw + pr2;

								// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
								float t0 = 0.0f;
								float t1 = percentOfCurveToDraw;
								float u0 = 1.0f;
								float u1 = (1.0f - t1);

								glm::vec4 q0 = ((u0 * u0 * u0) * p0) +
									((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
									((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
									((t0 * t0 * t0) * p3);
								glm::vec4 q1 = ((u0 * u0 * u1) * p0) +
									((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
									((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
									((t0 * t0 * t1) * p3);
								glm::vec4 q2 = ((u0 * u1 * u1) * p0) +
									((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
									((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
									((t0 * t1 * t1) * p3);
								glm::vec4 q3 = ((u1 * u1 * u1) * p0) +
									((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
									((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
									((t1 * t1 * t1) * p3);

								pr1 = q1;
								pr2 = q2;
								pr3 = q3;
							}

							// TODO: Replace this with bezier2
							Renderer::bezier3To3D(
								Vec3{ p1.x, p1.y, p1.z },
								Vec3{ p2.x, p2.y, p2.y },
								Vec3{ p3.x, p3.y, p3.z }
							);
						}
						break;
						case CurveType::Line:
						{
							glm::vec4 p1 = glm::vec4(
								curve.as.line.p1.x,
								curve.as.line.p1.y,
								curve.as.line.p1.z,
								1.0f
							);
							float curveLength = glm::length(p1 - p0);
							lengthDrawn += curveLength;

							if (lengthLeft < curveLength)
							{
								completedPath = false;
								float percentOfCurveToDraw = lengthLeft / curveLength;
								p1 = (p1 - p0) * percentOfCurveToDraw + p0;
							}

							Renderer::lineTo3D(
								Vec3{ p1.x, p1.y, p1.z }
							);
						}
						break;
						default:
							g_logger_warning("Unknown curve type in render %d", (int)curve.type);
							break;
						}
					}
					bool shouldClosePath = completedPath;
					Renderer::endPath3D(shouldClosePath);

					Renderer::popColor();
					Renderer::popStrokeWidth();
				}

				if (lengthDrawn > lengthToDraw)
				{
					break;
				}
			}
		}

		Renderer::resetTransform3D();
	}
}