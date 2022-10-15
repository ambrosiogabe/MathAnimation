#include "animation/Svg.h"
#include "animation/Animation.h"
#include "animation/SvgParser.h"
#include "utils/CMath.h"
#include "renderer/Renderer.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Colors.h"
#include "renderer/PerspectiveCamera.h"
#include "core/Application.h"

#include <nanovg.h>

namespace MathAnim
{
	static float cacheLineHeight;
	static Vec2 cachePadding = { 10.0f, 10.0f };
	static Vec2 cacheCurrentPos;
	static Framebuffer svgCache;

	namespace Svg
	{
		// ----------------- Private Variables -----------------
		constexpr int initialMaxCapacity = 5;
		static OrthoCamera* orthoCamera;
		static PerspectiveCamera* perspCamera;
		static Vec2 cursor;
		static int renderPassCounter = 0;

		// ----------------- Internal functions -----------------
		static void checkResize(Path& path);
		static void generateSvgCache(uint32 width, uint32 height);

		SvgObject createDefault()
		{
			SvgObject res;
			res.approximatePerimeter = 0.0f;
			// Dummy allocation to allow memory tracking when reallocing 
			// TODO: Fix the dang memory allocation library so I don't have to do this!
			res.paths = (Path*)g_memory_allocate(sizeof(Path));
			res.numPaths = 0;
			return res;
		}

		SvgGroup createDefaultGroup()
		{
			SvgGroup res;
			// Dummy allocation to allow memory tracking when reallocing 
			// TODO: Fix the dang memory allocation library so I don't have to do this!
			res.objects = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			res.objectOffsets = (Vec2*)g_memory_allocate(sizeof(Vec2));
			res.numObjects = 0;
			res.uniqueObjects = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			res.uniqueObjectNames = (char**)g_memory_allocate(sizeof(char*));
			res.numUniqueObjects = 0;
			res.viewbox = Vec4{ 0, 0, 1, 1 };
			return res;
		}

		void init(OrthoCamera& sceneCamera2d, PerspectiveCamera& sceneCamera3d)
		{
			orthoCamera = &sceneCamera2d;
			perspCamera = &sceneCamera3d;

			constexpr int defaultWidth = 4096;
			generateSvgCache(defaultWidth, defaultWidth);

			cacheCurrentPos.x = 0;
			cacheCurrentPos.y = 0;
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
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "SVG_Cache_Reset");

			renderPassCounter = 0;
			cacheCurrentPos.x = 0;
			cacheCurrentPos.y = 0;

			svgCache.bind();
			glViewport(0, 0, svgCache.width, svgCache.height);
			//svgCache.clearColorAttachmentRgba(0, "#fc03ecFF"_hex);
			svgCache.clearColorAttachmentRgba(0, "#00000000"_hex);
			svgCache.clearDepthStencil();

			glPopDebugGroup();
		}

		const Texture& getSvgCache()
		{
			return svgCache.getColorAttachment(0);
		}

		Framebuffer const& getSvgCacheFb()
		{
			return svgCache;
		}

		const Vec2& getCacheCurrentPos()
		{
			return cacheCurrentPos;
		}

		const Vec2& getCachePadding()
		{
			return cachePadding;
		}

		void incrementCacheCurrentY()
		{
			cacheCurrentPos.y += cacheLineHeight + cachePadding.y;
			cacheLineHeight = 0;
			cacheCurrentPos.x = 0;
		}

		void incrementCacheCurrentX(float distance)
		{
			cacheCurrentPos.x += distance;
		}

		void checkLineHeight(float newLineHeight)
		{
			cacheLineHeight = glm::max(cacheLineHeight, newLineHeight);
		}

		void growCache()
		{
			// Double the size of the texture (up to 8192x8192 max)
			Svg::generateSvgCache(svgCache.width * 2, svgCache.height * 2);

			// TODO: If the cache exceeds the size of this framebuffer do:
			//   endFrame(vg);
			//   Svg::generateSvgCache(...);
			//   beginFrame(vg);
		}

		void beginFrame(NVGcontext* vg)
		{
			nvgBeginFrame(vg, (float)svgCache.width, (float)svgCache.height, 1.0f);
		}

		void endFrame(NVGcontext* vg)
		{
			const char message[] = "NanoVG_Render_Pass[";
			constexpr size_t messageLength = sizeof(message) / sizeof(char);
			constexpr size_t bufferLength = messageLength + 7;
			char buffer[bufferLength] = {};
			// Buffer overflow protection
			if (renderPassCounter >= 9999)
			{
				renderPassCounter = 0;
			}
			snprintf(buffer, bufferLength, "%s%d]\0", message, renderPassCounter);
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, buffer);
			nvgEndFrame(vg);
			glPopDebugGroup();
			renderPassCounter++;
		}

		PerspectiveCamera const& getPerspCamera()
		{
			return *perspCamera;
		}

		OrthoCamera const& getOrthoCamera()
		{
			return *orthoCamera;
		}

		void beginSvgGroup(SvgGroup* group, const Vec4& viewbox)
		{
			group->viewbox = viewbox;
		}

		void pushSvgToGroup(SvgGroup* group, const SvgObject& obj, const std::string& id, const Vec2& offset)
		{
			group->numObjects++;
			group->objects = (SvgObject*)g_memory_realloc(group->objects, sizeof(SvgObject) * group->numObjects);
			g_logger_assert(group->objects != nullptr, "Ran out of RAM.");
			group->objectOffsets = (Vec2*)g_memory_realloc(group->objectOffsets, sizeof(Vec2) * group->numObjects);
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

		void beginPath(SvgObject* object, const Vec2& firstPoint, bool isAbsolute)
		{
			if (!isAbsolute)
			{
				g_logger_assert(object->numPaths > 0, "Cannot have non-absolute beginPath without prior paths.");
			}

			object->numPaths++;
			object->paths = (Path*)g_memory_realloc(object->paths, sizeof(Path) * object->numPaths);
			g_logger_assert(object->paths != nullptr, "Ran out of RAM.");

			object->paths[object->numPaths - 1].maxCapacity = initialMaxCapacity;
			object->paths[object->numPaths - 1].curves = (Curve*)g_memory_allocate(sizeof(Curve) * initialMaxCapacity);
			object->paths[object->numPaths - 1].numCurves = 0;
			object->paths[object->numPaths - 1].isHole = false;

			if (isAbsolute)
			{
				cursor = firstPoint;
			}
			else
			{
				cursor = firstPoint + cursor;
			}
			object->paths[object->numPaths - 1].curves[0].p0 = cursor;
		}

		void closePath(SvgObject* object, bool lineToEndpoint, bool isHole)
		{
			g_logger_assert(object->numPaths > 0, "object->numContours == 0. Cannot close contour when no contour exists.");
			g_logger_assert(object->paths[object->numPaths - 1].numCurves > 0, "contour->numCurves == 0. Cannot close contour with 0 vertices. There must be at least one vertex to close a contour.");

			object->paths[object->numPaths - 1].isHole = isHole;
			if (lineToEndpoint)
			{
				if (object->paths[object->numPaths - 1].numCurves > 0)
				{
					Vec2 firstPoint = object->paths[object->numPaths - 1].curves[0].p0;
					lineTo(object, firstPoint, true);
				}
			}

			cursor = Vec2{ 0, 0 };
		}

		void moveTo(SvgObject* object, const Vec2& point, bool absolute)
		{
			// If no object has started, begin the object here
			if (object->numPaths == 0)
			{
				beginPath(object, point);
				absolute = true;
			}
			else
			{
				// Otherwise begin a new path and close the old path
				// TODO: Should this close the old path with a lineto?
				bool isHole = object->numPaths > 1;
				closePath(object, true, isHole);
				cursor = absolute ? point : cursor + point;
				beginPath(object, cursor);
			}
		}

		void lineTo(SvgObject* object, const Vec2& point, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a lineTo when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = cursor;
			path.curves[path.numCurves - 1].as.line.p1 = absolute ? point : point + cursor;
			path.curves[path.numCurves - 1].type = CurveType::Line;

			cursor = path.curves[path.numCurves - 1].as.line.p1;
		}

		void hzLineTo(SvgObject* object, float xPoint, bool absolute)
		{
			Vec2 position = absolute
				? Vec2{ xPoint, cursor.y }
			: Vec2{ xPoint, 0.0f } + cursor;
			lineTo(object, position, true);
		}

		void vtLineTo(SvgObject* object, float yPoint, bool absolute)
		{
			Vec2 position = absolute
				? Vec2{ cursor.x, yPoint }
			: Vec2{ 0.0f, yPoint } + cursor;
			lineTo(object, position, true);
		}

		void bezier2To(SvgObject* object, const Vec2& control, const Vec2& dest, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a bezier2To when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = cursor;

			path.curves[path.numCurves - 1].as.bezier2.p1 = absolute ? control : control + cursor;
			cursor = path.curves[path.numCurves - 1].as.bezier2.p1;

			path.curves[path.numCurves - 1].as.bezier2.p2 = absolute ? dest : dest + cursor;
			cursor = path.curves[path.numCurves - 1].as.bezier2.p2;

			path.curves[path.numCurves - 1].type = CurveType::Bezier2;
		}

		void bezier3To(SvgObject* object, const Vec2& control0, const Vec2& control1, const Vec2& dest, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a bezier3To when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = cursor;

			path.curves[path.numCurves - 1].as.bezier3.p1 = absolute ? control0 : control0 + cursor;
			cursor = path.curves[path.numCurves - 1].as.bezier3.p1;

			path.curves[path.numCurves - 1].as.bezier3.p2 = absolute ? control1 : control1 + cursor;
			cursor = path.curves[path.numCurves - 1].as.bezier3.p2;

			path.curves[path.numCurves - 1].as.bezier3.p3 = absolute ? dest : dest + cursor;
			cursor = path.curves[path.numCurves - 1].as.bezier3.p3;

			path.curves[path.numCurves - 1].type = CurveType::Bezier3;
		}

		void smoothBezier2To(SvgObject* object, const Vec2& dest, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a bezier3To when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = cursor;

			Vec2 control0 = cursor;
			if (path.numCurves > 1)
			{
				if (path.curves[path.numCurves - 2].type == CurveType::Bezier2)
				{
					Vec2 prevControl1 = path.curves[path.numCurves - 2].as.bezier2.p1;
					// Reflect the previous c2 about the current cursor
					control0 = (-1.0f * (prevControl1 - cursor)) + cursor;
				}
			}
			path.curves[path.numCurves - 1].as.bezier2.p1 = control0;
			cursor = path.curves[path.numCurves - 1].as.bezier2.p1;

			path.curves[path.numCurves - 1].as.bezier2.p2 = absolute ? dest : dest + cursor;
			cursor = path.curves[path.numCurves - 1].as.bezier2.p2;

			path.curves[path.numCurves - 1].type = CurveType::Bezier2;
		}

		void smoothBezier3To(SvgObject* object, const Vec2& control1, const Vec2& dest, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a bezier3To when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = cursor;

			Vec2 control0 = cursor;
			if (path.numCurves > 1)
			{
				if (path.curves[path.numCurves - 2].type == CurveType::Bezier3)
				{
					Vec2 prevControl1 = path.curves[path.numCurves - 2].as.bezier3.p2;
					// Reflect the previous c2 about the current cursor
					control0 = (-1.0f * (prevControl1 - cursor)) + cursor;
				}
			}
			path.curves[path.numCurves - 1].as.bezier3.p1 = control0;
			cursor = path.curves[path.numCurves - 1].as.bezier3.p1;

			path.curves[path.numCurves - 1].as.bezier3.p2 = absolute ? control1 : control1 + cursor;
			cursor = path.curves[path.numCurves - 1].as.bezier3.p2;

			path.curves[path.numCurves - 1].as.bezier3.p3 = absolute ? dest : dest + cursor;
			cursor = path.curves[path.numCurves - 1].as.bezier3.p3;

			path.curves[path.numCurves - 1].type = CurveType::Bezier3;
		}

		void arcTo(SvgObject* object, const Vec2& radius, float xAxisRot, bool largeArc, bool sweep, const Vec2& dst, bool absolute)
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
			if (dest->numPaths != src->numPaths)
			{
				// Free any extra paths the destination has
				// If the destination has less, this loop doesn't run
				for (int contouri = src->numPaths; contouri < dest->numPaths; contouri++)
				{
					g_memory_free(dest->paths[contouri].curves);
					dest->paths[contouri].curves = nullptr;
					dest->paths[contouri].numCurves = 0;
					dest->paths[contouri].maxCapacity = 0;
				}

				// Then reallocate memory. If dest had less, this will acquire enough new memory
				// If dest had more, this will get rid of the extra memory
				dest->paths = (Path*)g_memory_realloc(dest->paths, sizeof(Path) * src->numPaths);

				// Go through and initialize the curves for any new curves that were added
				for (int contouri = dest->numPaths; contouri < src->numPaths; contouri++)
				{
					dest->paths[contouri].curves = (Curve*)g_memory_allocate(sizeof(Curve) * initialMaxCapacity);
					dest->paths[contouri].maxCapacity = initialMaxCapacity;
					dest->paths[contouri].numCurves = 0;
				}

				// Then set the number of paths equal
				dest->numPaths = src->numPaths;
			}

			g_logger_assert(dest->numPaths == src->numPaths, "How did this happen?");

			// Finally we can just go through and set all the data equal to each other
			for (int pathi = 0; pathi < src->numPaths; pathi++)
			{
				Path& dstPath = dest->paths[pathi];
				dstPath.numCurves = 0;
				dstPath.isHole = src->paths[pathi].isHole;

				for (int curvei = 0; curvei < src->paths[pathi].numCurves; curvei++)
				{
					const Curve& srcCurve = src->paths[pathi].curves[curvei];

					dstPath.numCurves++;
					checkResize(dstPath);

					// Copy the data
					Curve& dstCurve = dstPath.curves[curvei];
					dstCurve.p0 = srcCurve.p0;
					dstCurve.as = srcCurve.as;
					dstCurve.type = srcCurve.type;
				}

				g_logger_assert(dstPath.numCurves == src->paths[pathi].numCurves, "How did this happen?");
			}

			dest->calculateApproximatePerimeter();
			dest->calculateBBox();
		}

		SvgObject* interpolate(const SvgObject* src, const SvgObject* dst, float t)
		{
			SvgObject* res = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			*res = Svg::createDefault();

			// If one object has more paths than the other object,
			// then we'll just skip every other contour for the object
			// with less contours and hopefully it looks cool
			const SvgObject* lessPaths = src->numPaths <= dst->numPaths
				? src
				: dst;
			const SvgObject* morePaths = src->numPaths > dst->numPaths
				? src
				: dst;
			int numPathsToSkip = glm::max(morePaths->numPaths / lessPaths->numPaths, 1);
			int lessPathi = 0;
			int morePathi = 0;
			while (lessPathi < lessPaths->numPaths)
			{
				const Path* path0 = &lessPaths->paths[lessPathi];
				const Path* path1 = &morePaths->paths[morePathi];

				bool isHole = false;
				if (lessPaths == src)
				{
					isHole = t < 0.5f
						? path0->isHole
						: path1->isHole;
				}
				else
				{
					isHole = t > 0.5f
						? path1->isHole
						: path0->isHole;
				}

				// It's undefined to interpolate between two paths if one of the paths has no curves
				bool shouldLoop = path0->numCurves > 0 && path1->numCurves > 0;
				if (!shouldLoop)
				{
					continue;
				}

				const Path* pathToFree = nullptr;
				if (path0->numCurves != path1->numCurves)
				{
					// We need to make them equal to interpolate properly
					// we'll add extra points to the path with less curves
					// to make sure the interpolation is 1-1
					const Path& pathToIncrease = path0->numCurves < path1->numCurves
						? *path0
						: *path1;
					int desiredNumCurves = glm::max(path0->numCurves, path1->numCurves);

					Path* outputPath = (Path*)g_memory_allocate(sizeof(Path));
					outputPath->curves = (Curve*)g_memory_allocate(sizeof(Curve) * desiredNumCurves);
					outputPath->numCurves = desiredNumCurves;
					outputPath->maxCapacity = desiredNumCurves;

					// Sometimes math is beautiful...
					// This algorithm works by splitting each curve a similar number of times
					// So if you need the curve to have 10 points but it has 4 points the algorithm
					// will split like the following:
					//    Curve 0: 3 split curves
					//    Curve 1: 2 split curves
					//    Curve 2: 3 split curves
					//    Curve 3: 2 split curves
					//
					//    Total Split Curves: 10 curves (which is the desired number of curves)
					// The math here just kind of works out by using natural rounding to determine
					// how many splits to apply to each curve in the sequence
					int inputCurvei = 0;
					int outputCurvei = 0;
					int numCurvesAdded = 0;
					int numCurvesLeft = pathToIncrease.numCurves;
					while (outputCurvei < desiredNumCurves)
					{
						float numSplits = (float)(desiredNumCurves - numCurvesAdded) / (float)numCurvesLeft;
						// Round to the nearest half. 
						//   3.5 rounds to 4
						//   3.49 rounds to 3
						float fractionalSplits = numSplits - glm::floor(numSplits);
						int roundedNumSplits = (int)glm::floor(numSplits);
						if (fractionalSplits >= 0.5f)
						{
							roundedNumSplits = (int)glm::ceil(numSplits);
						}
						g_logger_assert(outputCurvei + roundedNumSplits <= outputPath->numCurves, "How did this happen? Somehow we are adding too many splits to an interpolated path.");
						g_logger_assert(inputCurvei < pathToIncrease.numCurves, "How did this happen? Somehow we tried to increment past too many input curves in an interpolated path.");

						// Increment the totals to what they will be for the next iteration
						numCurvesLeft--;
						numCurvesAdded += roundedNumSplits;

						// Split this curve roundedNumSplits number of times evenly
						Curve nextCurve = pathToIncrease.curves[inputCurvei];
						while (roundedNumSplits > 1)
						{
							float tSplit = 1.0f / (float)roundedNumSplits;

							const Vec2& p0 = nextCurve.p0;
							outputPath->curves[outputCurvei].p0 = p0;

							// Interpolate the curve by tSplit
							float percentOfCurveToDraw = tSplit;

							switch (nextCurve.type)
							{
							case CurveType::Bezier3:
							{
								const Vec2& p1 = nextCurve.as.bezier3.p1;
								const Vec2& p2 = nextCurve.as.bezier3.p2;
								const Vec2& p3 = nextCurve.as.bezier3.p3;

								// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
								{
									// First split
									float t0 = 0.0f;
									float t1 = percentOfCurveToDraw;
									float u0 = 1.0f - t0;
									float u1 = (1.0f - t1);

									Vec2 q0 = ((u0 * u0 * u0) * p0) +
										((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
										((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
										((t0 * t0 * t0) * p3);
									Vec2 q1 = ((u0 * u0 * u1) * p0) +
										((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
										((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
										((t0 * t0 * t1) * p3);
									Vec2 q2 = ((u0 * u1 * u1) * p0) +
										((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
										((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
										((t0 * t1 * t1) * p3);
									Vec2 q3 = ((u1 * u1 * u1) * p0) +
										((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
										((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
										((t1 * t1 * t1) * p3);

									// Copy the split position into output curve
									outputPath->curves[outputCurvei].type = CurveType::Bezier3;
									outputPath->curves[outputCurvei].as.bezier3.p1 = q1;
									outputPath->curves[outputCurvei].as.bezier3.p2 = q2;
									outputPath->curves[outputCurvei].as.bezier3.p3 = q3;
								}

								{
									// Second split
									float t0 = 1.0f - percentOfCurveToDraw;
									float t1 = 1.0f;
									float u0 = 1.0f - t0;
									float u1 = (1.0f - t1);

									Vec2 q0 = ((u0 * u0 * u0) * p0) +
										((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
										((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
										((t0 * t0 * t0) * p3);
									Vec2 q1 = ((u0 * u0 * u1) * p0) +
										((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
										((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
										((t0 * t0 * t1) * p3);
									Vec2 q2 = ((u0 * u1 * u1) * p0) +
										((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
										((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
										((t0 * t1 * t1) * p3);
									Vec2 q3 = ((u1 * u1 * u1) * p0) +
										((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
										((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
										((t1 * t1 * t1) * p3);

									// Copy the second split position into next curve
									nextCurve.type = CurveType::Bezier3;
									nextCurve.p0 = q0;
									nextCurve.as.bezier3.p1 = q1;
									nextCurve.as.bezier3.p2 = q2;
									nextCurve.as.bezier3.p3 = q3;
								}
							}
							break;
							case CurveType::Bezier2:
							{
								Vec2 p1 = nextCurve.as.bezier2.p1;
								Vec2 p2 = nextCurve.as.bezier2.p1;
								Vec2 p3 = nextCurve.as.bezier2.p2;

								// Degree elevated quadratic bezier curve
								Vec2 pr0 = p0;
								Vec2 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
								Vec2 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
								Vec2 pr3 = p3;

								p1 = pr1;
								p2 = pr2;
								p3 = pr3;

								// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
								{
									// Split 1
									float t0 = 0.0f;
									float t1 = percentOfCurveToDraw;
									float u0 = 1.0f - t0;
									float u1 = (1.0f - t1);

									Vec2 q0 = ((u0 * u0 * u0) * p0) +
										((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
										((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
										((t0 * t0 * t0) * p3);
									Vec2 q1 = ((u0 * u0 * u1) * p0) +
										((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
										((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
										((t0 * t0 * t1) * p3);
									Vec2 q2 = ((u0 * u1 * u1) * p0) +
										((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
										((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
										((t0 * t1 * t1) * p3);
									Vec2 q3 = ((u1 * u1 * u1) * p0) +
										((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
										((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
										((t1 * t1 * t1) * p3);

									// Copy the split position into output curve
									outputPath->curves[outputCurvei].type = CurveType::Bezier3;
									outputPath->curves[outputCurvei].as.bezier3.p1 = q1;
									outputPath->curves[outputCurvei].as.bezier3.p2 = q2;
									outputPath->curves[outputCurvei].as.bezier3.p3 = q3;
								}

								{
									// Split 2
									float t0 = (1.0f - percentOfCurveToDraw);
									float t1 = 1.0f;
									float u0 = 1.0f - t0;
									float u1 = (1.0f - t1);

									Vec2 q0 = ((u0 * u0 * u0) * p0) +
										((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
										((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
										((t0 * t0 * t0) * p3);
									Vec2 q1 = ((u0 * u0 * u1) * p0) +
										((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
										((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
										((t0 * t0 * t1) * p3);
									Vec2 q2 = ((u0 * u1 * u1) * p0) +
										((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
										((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
										((t0 * t1 * t1) * p3);
									Vec2 q3 = ((u1 * u1 * u1) * p0) +
										((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
										((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
										((t1 * t1 * t1) * p3);

									// Copy the second split position into next curve
									nextCurve.type = CurveType::Bezier3;
									nextCurve.p0 = q0;
									nextCurve.as.bezier3.p1 = q1;
									nextCurve.as.bezier3.p2 = q2;
									nextCurve.as.bezier3.p3 = q3;
								}
							}
							break;
							case CurveType::Line:
							{
								Vec2 p1 = nextCurve.as.line.p1;
								// First split
								outputPath->curves[outputCurvei].type = CurveType::Line;
								outputPath->curves[outputCurvei].p0 = p0;
								outputPath->curves[outputCurvei].as.line.p1 = ((p1 - p0) * percentOfCurveToDraw) + p0;

								// Set up next split
								nextCurve.type = CurveType::Line;
								nextCurve.p0 = outputPath->curves[outputCurvei].as.line.p1;
								nextCurve.as.line.p1 = p1;
							}
							break;
							default:
								g_logger_warning("Unknown curve type in render %d", (int)nextCurve.type);
								break;
							}

							outputCurvei++;
							roundedNumSplits--;
						}

						outputPath->curves[outputCurvei] = nextCurve;
						outputCurvei++;
						inputCurvei++;
					}

					pathToFree = outputPath;
					// Re-assign whichever path we had to increase
					if (path0->numCurves < path1->numCurves)
					{
						path0 = pathToFree;
					}
					else
					{
						path1 = pathToFree;
					}
				}

				// TODO: This may break stuff...
				if (lessPaths != src)
				{
					const Path* tmp = path0;
					path0 = path1;
					path1 = tmp;
				}

				// Move to the start, which is the interpolation between both of the
				// first vertices
				const Vec2& p0a = path0->curves[0].p0;
				const Vec2& p0b = path1->curves[0].p0;
				Vec2 interpP0 = {
					(p0b.x - p0a.x) * t + p0a.x,
					(p0b.y - p0a.y) * t + p0a.y
				};

				Svg::beginPath(res, interpP0, true);

				for (int curvei = 0; curvei < path0->numCurves; curvei++)
				{
					// Interpolate between the two curves, treat both curves
					// as bezier3 curves no matter what to make it easier
					glm::vec2 p1a, p2a, p3a;
					glm::vec2 p1b, p2b, p3b;

					g_logger_assert(curvei < path1->numCurves, "Path1 has an inequal number of curves for some reason.");
					const Curve& lessCurve = path0->curves[curvei];
					const Curve& moreCurve = path1->curves[curvei];

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
					Svg::bezier3To(res,
						Vec2{ interpP1.x, interpP1.y },
						Vec2{ interpP2.x, interpP2.y },
						Vec2{ interpP3.x, interpP3.y }
					);
				}

				Svg::closePath(res, false, isHole);

				if (pathToFree)
				{
					if (pathToFree->curves)
					{
						g_memory_free(pathToFree->curves);
					}
					g_memory_free((void*)pathToFree);
				}

				lessPathi++;
				morePathi += numPathsToSkip;
				if (morePathi >= morePaths->numPaths)
				{
					morePathi = morePaths->numPaths - 1;
				}
			}

			res->calculateApproximatePerimeter();
			res->calculateBBox();

			return res;
		}

		// ----------------- Internal functions -----------------
		static void checkResize(Path& path)
		{
			if (path.numCurves > path.maxCapacity)
			{
				path.maxCapacity *= 2;
				path.curves = (Curve*)g_memory_realloc(path.curves, sizeof(Curve) * path.maxCapacity);
				g_logger_assert(path.curves != nullptr, "Ran out of RAM.");
			}
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
	static void renderCreateAnimation2D(NVGcontext* vg, float t, const AnimObject* parent, const Vec2& textureOffset, bool reverse, const SvgObject* obj, bool isSvgGroup);

	void SvgObject::normalize(const Vec2& inMin, const Vec2& inMax)
	{
		// First find the min max of the entire curve
		Vec2 min = inMin;
		Vec2 max = inMax;
		if (min.x == FLT_MAX && min.y == FLT_MAX && max.x == FLT_MIN && max.y == FLT_MIN)
		{
			for (int pathi = 0; pathi < this->numPaths; pathi++)
			{
				for (int curvei = 0; curvei < this->paths[pathi].numCurves; curvei++)
				{
					const Curve& curve = paths[pathi].curves[curvei];
					const Vec2& p0 = curve.p0;

					min = CMath::min(p0, min);
					max = CMath::max(p0, max);

					switch (curve.type)
					{
					case CurveType::Bezier3:
					{
						const Vec2& p1 = curve.as.bezier3.p1;
						const Vec2& p2 = curve.as.bezier3.p2;
						const Vec2& p3 = curve.as.bezier3.p3;

						min = CMath::min(p1, min);
						max = CMath::max(p1, max);

						min = CMath::min(p2, min);
						max = CMath::max(p2, max);

						min = CMath::min(p3, min);
						max = CMath::max(p3, max);
					}
					break;
					case CurveType::Bezier2:
					{
						const Vec2& p1 = curve.as.bezier2.p1;
						const Vec2& p2 = curve.as.bezier2.p2;

						min = CMath::min(p1, min);
						max = CMath::max(p1, max);

						min = CMath::min(p2, min);
						max = CMath::max(p2, max);
					}
					break;
					case CurveType::Line:
					{
						const Vec2& p1 = curve.as.line.p1;

						min = CMath::min(p1, min);
						max = CMath::max(p1, max);
					}
					break;
					}
				}
			}
		}

		// Then map everything to a [0.0-1.0] range from there
		Vec2 hzOutputRange = Vec2{ 0.0f, 1.0f };
		Vec2 vtOutputRange = Vec2{ 0.0f, 1.0f };
		for (int pathi = 0; pathi < this->numPaths; pathi++)
		{
			for (int curvei = 0; curvei < this->paths[pathi].numCurves; curvei++)
			{
				Curve& curve = paths[pathi].curves[curvei];
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

	float Curve::calculateApproximatePerimeter() const
	{
		switch (type)
		{
		case CurveType::Bezier3:
		{
			const Vec2& p1 = this->as.bezier3.p1;
			const Vec2& p2 = this->as.bezier3.p2;
			const Vec2& p3 = this->as.bezier3.p3;
			float chordLength = CMath::length(p3 - p0);
			float controlNetLength = CMath::length(p1 - p0) + CMath::length(p2 - p1) + CMath::length(p3 - p2);
			float approxLength = (chordLength + controlNetLength) / 2.0f;
			return approxLength;
		}
		break;
		case CurveType::Bezier2:
		{
			const Vec2& p1 = this->as.bezier2.p1;
			const Vec2& p2 = this->as.bezier2.p2;
			float chordLength = CMath::length(p2 - p0);
			float controlNetLength = CMath::length(p1 - p0) + CMath::length(p2 - p1);
			float approxLength = (chordLength + controlNetLength) / 2.0f;
			return approxLength;
		}
		break;
		case CurveType::Line:
		{
			const Vec2& p1 = this->as.line.p1;
			return CMath::length(p1 - p0);
		}
		break;
		}

		return 0.0f;
	}

	float Path::calculateApproximatePerimeter() const
	{
		float approxPerimeter = 0.0f;
		for (int curvei = 0; curvei < this->numCurves; curvei++)
		{
			const Curve& curve = this->curves[curvei];
			approxPerimeter += curve.calculateApproximatePerimeter();
		}

		return approxPerimeter;
	}

	void SvgObject::calculateApproximatePerimeter()
	{
		approximatePerimeter = 0.0f;

		for (int pathi = 0; pathi < this->numPaths; pathi++)
		{
			approximatePerimeter += this->paths[pathi].calculateApproximatePerimeter();
		}
	}

	void SvgObject::calculateBBox()
	{
		bbox.min.x = FLT_MAX;
		bbox.min.y = FLT_MAX;
		bbox.max.x = FLT_MIN;
		bbox.max.y = FLT_MIN;

		for (int pathi = 0; pathi < numPaths; pathi++)
		{
			if (paths[pathi].numCurves > 0)
			{
				for (int curvei = 0; curvei < paths[pathi].numCurves; curvei++)
				{
					const Curve& curve = paths[pathi].curves[curvei];
					Vec2 p0 = Vec2{ curve.p0.x, curve.p0.y };

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

	void SvgObject::render(NVGcontext* vg, const AnimObject* parent, const Vec2& offset) const
	{
		renderCreateAnimation(vg, 1.01f, parent, offset, false, false);
	}

	void SvgObject::renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent, const Vec2& offset, bool reverse, bool isSvgGroup) const
	{
		Vec2 svgTextureOffset = Vec2{
			(float)cacheCurrentPos.x + parent->strokeWidth * 0.5f,
			(float)cacheCurrentPos.y + parent->strokeWidth * 0.5f
		};

		// Check if the SVG cache needs to regenerate
		float svgTotalWidth = ((bbox.max.x - bbox.min.x) * parent->svgScale) + parent->strokeWidth;
		float svgTotalHeight = ((bbox.max.y - bbox.min.y) * parent->svgScale) + parent->strokeWidth;
		{
			float newRightX = svgTextureOffset.x + svgTotalWidth;
			if (newRightX >= svgCache.width)
			{
				// Move to the newline
				Svg::incrementCacheCurrentY();
			}

			float newBottomY = svgTextureOffset.y + svgTotalHeight;
			if (newBottomY >= svgCache.height)
			{
				Svg::growCache();
			}

			svgTextureOffset = Vec2{
				(float)cacheCurrentPos.x + parent->strokeWidth * 0.5f,
				(float)cacheCurrentPos.y + parent->strokeWidth * 0.5f
			};
		}

		// First render to the cache
		svgCache.bind();
		glViewport(0, 0, svgCache.width, svgCache.height);

		// Reset the draw buffers to draw to FB_attachment_0
		GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, compositeDrawBuffers);

		if (isSvgGroup)
		{
			svgTextureOffset.x += offset.x * parent->svgScale;
			svgTextureOffset.y += offset.y * parent->svgScale;
		}

		renderCreateAnimation2D(vg, t, parent, svgTextureOffset, reverse, this, isSvgGroup);

		// Don't blit svg groups to a bunch of quads, they get rendered as one quad together
		if (isSvgGroup)
		{
			return;
		}

		// Subtract half stroke width to make sure it's getting the correct coords
		svgTextureOffset -= Vec2{ parent->strokeWidth * 0.5f, parent->strokeWidth * 0.5f };
		Vec2 cacheUvMin = Vec2{
			svgTextureOffset.x / svgCache.width,
			1.0f - (svgTextureOffset.y / svgCache.width) - (svgTotalHeight / svgCache.height)
		};
		Vec2 cacheUvMax = cacheUvMin +
			Vec2{
				svgTotalWidth / svgCache.width,
				svgTotalHeight / svgCache.height
		};

		Svg::incrementCacheCurrentX(svgTotalWidth + cachePadding.x);
		Svg::checkLineHeight(svgTotalHeight);

		if (parent->is3D)
		{
			Renderer::drawTexturedQuad3D(
				svgCache.getColorAttachment(0),
				Vec2{ svgTotalWidth / parent->svgScale, svgTotalHeight / parent->svgScale },
				cacheUvMin,
				cacheUvMax,
				parent->globalTransform,
				parent->isTransparent
			);
		}
		else
		{
			Renderer::drawTexturedQuad(
				svgCache.getColorAttachment(0),
				Vec2{ svgTotalWidth / parent->svgScale, svgTotalHeight / parent->svgScale },
				cacheUvMin,
				cacheUvMax,
				parent->id,
				parent->globalTransform
			);
		}
	}

	void SvgObject::free()
	{
		for (int pathi = 0; pathi < numPaths; pathi++)
		{
			if (paths[pathi].curves)
			{
				g_memory_free(paths[pathi].curves);
			}
			paths[pathi].numCurves = 0;
			paths[pathi].maxCapacity = 0;
		}

		if (paths)
		{
			g_memory_free(paths);
		}

		numPaths = 0;
		approximatePerimeter = 0.0f;
	}

	static void growBufferIfNeeded(uint8** buffer, size_t* capacity, size_t numElements, size_t numElementsToAdd)
	{
		if (numElements + numElementsToAdd >= *capacity)
		{
			*capacity = (numElements + numElementsToAdd) * 2;
			*buffer = (uint8*)g_memory_realloc(*buffer, *capacity);
			g_logger_assert(*buffer != nullptr, "Ran out of RAM.");
		}
	}

	static void writeBuffer(uint8** buffer, size_t* capacity, size_t* numElements, char* string, size_t stringLength = 0)
	{
		if (stringLength == 0)
		{
			stringLength = std::strlen(string);
		}

		growBufferIfNeeded(buffer, capacity, *numElements, stringLength);
		g_memory_copyMem(*buffer + *numElements, (void*)string, stringLength * sizeof(uint8));
		*numElements += stringLength;
	}

	void SvgObject::serialize(RawMemory& memory) const
	{
		size_t numElements = 0;
		size_t capacity = 256;
		uint8* buffer = (uint8*)g_memory_allocate(sizeof(uint8) * capacity);
		constexpr size_t tmpBufferSize = 256;
		char tmpBuffer[tmpBufferSize];
		for (int pathi = 0; pathi < numPaths; pathi++)
		{
			// Move to the first path point
			Path& path = paths[pathi];
			if (path.numCurves > 0)
			{
				size_t numBytesWritten = snprintf(tmpBuffer, tmpBufferSize, "M%0.6f %0.6f ", path.curves[0].p0.x, path.curves[0].p0.y);
				g_logger_assert(numBytesWritten >= 0 && numBytesWritten < tmpBufferSize, "Tmp buffer is too small to write out.");
				writeBuffer(&buffer, &capacity, &numElements, tmpBuffer);
			}

			for (int curvei = 0; curvei < path.numCurves; curvei++)
			{
				Curve& curve = path.curves[curvei];
				switch (curve.type)
				{
				case CurveType::Line:
				{
					writeBuffer(&buffer, &capacity, &numElements, "L", 1);
					size_t numBytesWritten = snprintf(tmpBuffer, tmpBufferSize, "%0.6f %0.6f ", curve.as.line.p1.x, curve.as.line.p1.y);
					g_logger_assert(numBytesWritten >= 0 && numBytesWritten < tmpBufferSize, "Tmp buffer is too small to write out.");
					writeBuffer(&buffer, &capacity, &numElements, tmpBuffer);
				}
				break;
				case CurveType::Bezier2:
				{
					writeBuffer(&buffer, &capacity, &numElements, "Q", 1);
					size_t numBytesWritten = snprintf(
						tmpBuffer,
						tmpBufferSize,
						"%0.6f %0.6f %0.6f %0.6f ",
						curve.as.bezier2.p1.x,
						curve.as.bezier2.p1.y,
						curve.as.bezier2.p2.x,
						curve.as.bezier2.p2.y
					);
					g_logger_assert(numBytesWritten >= 0 && numBytesWritten < tmpBufferSize, "Tmp buffer is too small to write out.");
					writeBuffer(&buffer, &capacity, &numElements, tmpBuffer);
				}
				break;
				case CurveType::Bezier3:
				{
					writeBuffer(&buffer, &capacity, &numElements, "C", 1);
					size_t numBytesWritten = snprintf(
						tmpBuffer,
						tmpBufferSize,
						"%0.6f %0.6f %0.6f %0.6f %0.6f %0.6f ",
						curve.as.bezier3.p1.x,
						curve.as.bezier3.p1.y,
						curve.as.bezier3.p2.x,
						curve.as.bezier3.p2.y,
						curve.as.bezier3.p3.x,
						curve.as.bezier3.p3.y
					);
					g_logger_assert(numBytesWritten >= 0 && numBytesWritten < tmpBufferSize, "Tmp buffer is too small to write out.");
					writeBuffer(&buffer, &capacity, &numElements, tmpBuffer);
				}
				break;
				default:
					g_logger_error("Unknown curve type: %d", curve.type);
					break;
				}
			}
		}

		uint64 stringLength = (uint64)numElements;
		memory.write<uint64>(&stringLength);
		memory.writeDangerous(buffer, numElements);

		g_memory_free(buffer);
	}

	SvgObject* SvgObject::deserialize(RawMemory& memory, uint32 version)
	{
		SvgObject* res = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		uint64 stringLength;
		memory.read<uint64>(&stringLength);
		uint8* string = (uint8*)g_memory_allocate(sizeof(uint8) * (stringLength + 1));
		memory.readDangerous(string, stringLength);
		string[stringLength] = '\0';

		*res = SvgParser::parseSvgPath((const char*)string, stringLength);
		g_memory_free(string);

		return res;
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
		Vec2 translation = Vec2{ viewbox.values[0], viewbox.values[1] };
		bbox.min = Vec2{ FLT_MAX, FLT_MAX };
		bbox.max = Vec2{ FLT_MIN, FLT_MIN };

		for (int i = 0; i < numObjects; i++)
		{
			SvgObject& obj = objects[i];
			const Vec2& offset = objectOffsets[i];

			Vec2 absOffset = offset - translation;
			obj.calculateBBox();
			bbox.min = CMath::min(obj.bbox.min + absOffset, bbox.min);
			bbox.max = CMath::max(obj.bbox.max + absOffset, bbox.max);
		}
	}

	void SvgGroup::render(NVGcontext* vg, AnimObject* parent) const
	{
		renderCreateAnimation(vg, 1.01f, parent, false);
	}

	void SvgGroup::renderCreateAnimation(NVGcontext* vg, float t, AnimObject* parent, bool reverse) const
	{
		Vec2 translation = Vec2{ viewbox.values[0], viewbox.values[1] };
		Vec2 bboxOffset = Vec2{ bbox.min.x, bbox.min.y };

		// TODO: Offload all this stuff into some sort of TexturePacker data structure
		{
			Vec2 svgTextureOffset = Vec2{
				(float)cacheCurrentPos.x + parent->strokeWidth * 0.5f,
				(float)cacheCurrentPos.y + parent->strokeWidth * 0.5f
			};

			// Check if the SVG cache needs to regenerate
			float svgTotalWidth = ((bbox.max.x - bbox.min.x) * parent->svgScale) + parent->strokeWidth;
			float svgTotalHeight = ((bbox.max.y - bbox.min.y) * parent->svgScale) + parent->strokeWidth;
			{
				float newRightX = svgTextureOffset.x + svgTotalWidth;
				if (newRightX >= svgCache.width)
				{
					// Move to the newline
					Svg::incrementCacheCurrentY();
				}

				float newBottomY = svgTextureOffset.y + svgTotalHeight;
				if (newBottomY >= svgCache.height)
				{
					Svg::growCache();
				}
			}
		}

		float numberObjectsToDraw = t * (float)numObjects;
		constexpr float numObjectsToLag = 2.0f;
		float numObjectsDrawn = 0.0f;
		for (int i = 0; i < numObjects; i++)
		{
			const SvgObject& obj = objects[i];
			const Vec2& offset = objectOffsets[i];

			float denominator = i == numObjects - 1 ? 1.0f : numObjectsToLag;
			float percentOfLetterToDraw = (numberObjectsToDraw - numObjectsDrawn) / denominator;
			Vec2 absOffset = offset - translation - bboxOffset;
			obj.renderCreateAnimation(vg, percentOfLetterToDraw, parent, absOffset, reverse, true);
			numObjectsDrawn += 1.0f;

			if (numObjectsDrawn >= numberObjectsToDraw)
			{
				break;
			}
		}

		Vec2 svgTextureOffset = Vec2{
			(float)cacheCurrentPos.x + parent->strokeWidth * 0.5f,
			(float)cacheCurrentPos.y + parent->strokeWidth * 0.5f
		};
		float svgTotalWidth = ((bbox.max.x - bbox.min.x) * parent->svgScale) + parent->strokeWidth;
		float svgTotalHeight = ((bbox.max.y - bbox.min.y) * parent->svgScale) + parent->strokeWidth;

		Vec2 cacheUvMin = Vec2{
			svgTextureOffset.x / svgCache.width,
			1.0f - (svgTextureOffset.y / svgCache.height) - (svgTotalHeight / svgCache.height)
		};
		Vec2 cacheUvMax = cacheUvMin +
			Vec2{
				svgTotalWidth / svgCache.width,
				svgTotalHeight / svgCache.height
		};

		if (parent->drawDebugBoxes)
		{
			// First render to the cache
			svgCache.bind();
			glViewport(0, 0, svgCache.width, svgCache.height);

			// Reset the draw buffers to draw to FB_attachment_0
			GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(4, compositeDrawBuffers);

			float strokeWidthCorrectionPos = cachePadding.x * 0.5f;
			float strokeWidthCorrectionNeg = -cachePadding.x;
			nvgBeginPath(vg);
			nvgStrokeColor(vg, nvgRGBA(0, 255, 255, 255));
			nvgStrokeWidth(vg, cachePadding.x);
			nvgMoveTo(vg,
				cacheUvMin.x * svgCache.width + strokeWidthCorrectionPos,
				(1.0f - cacheUvMax.y) * svgCache.height + strokeWidthCorrectionPos
			);
			nvgRect(vg,
				cacheUvMin.x * svgCache.width + strokeWidthCorrectionPos,
				(1.0f - cacheUvMax.y) * svgCache.height + strokeWidthCorrectionPos,
				(cacheUvMax.x - cacheUvMin.x) * svgCache.width + strokeWidthCorrectionNeg,
				(cacheUvMax.y - cacheUvMin.y) * svgCache.height + strokeWidthCorrectionNeg
			);
			nvgClosePath(vg);
			nvgStroke(vg);
		}

		// Then blit the SVG group to the screen
		if (parent->is3D)
		{
			Renderer::drawTexturedQuad3D(
				svgCache.getColorAttachment(0),
				Vec2{ svgTotalWidth / parent->svgScale, svgTotalHeight / parent->svgScale },
				cacheUvMin,
				cacheUvMax,
				parent->globalTransform,
				parent->isTransparent
			);
		}
		else
		{
			Renderer::drawTexturedQuad(
				svgCache.getColorAttachment(0),
				Vec2{ svgTotalWidth / parent->svgScale, svgTotalHeight / parent->svgScale },
				cacheUvMin,
				cacheUvMax,
				parent->id,
				parent->globalTransform
			);
		}

		Svg::incrementCacheCurrentX(((bbox.max.x - bbox.min.x) * parent->svgScale) + parent->strokeWidth + cachePadding.x);
		Svg::checkLineHeight(((bbox.max.y - bbox.min.y) * parent->svgScale) + parent->strokeWidth);
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
	static void renderCreateAnimation2D(NVGcontext* vg, float t, const AnimObject* parent, const Vec2& textureOffset, bool reverse, const SvgObject* obj, bool isSvgGroup)
	{
		constexpr float defaultStrokeWidth = 5.0f;

		if (reverse)
		{
			t = 1.0f - t;
		}

		// Start the fade in after 80% of the svg object is drawn
		constexpr float fadeInStart = 0.8f;
		float lengthToDraw = t * (float)obj->approximatePerimeter;
		float amountToFadeIn = ((t - fadeInStart) / (1.0f - fadeInStart));
		float percentToFadeIn = glm::max(glm::min(amountToFadeIn, 1.0f), 0.0f);

		// Instead of translating, we'll map every coordinate from the SVG min-max range to
		// the preferred coordinate range
		//nvgTranslate(vg, textureOffset.x, textureOffset.y);
		Vec2 scaledBboxMin = obj->bbox.min;
		scaledBboxMin.x *= parent->svgScale;
		scaledBboxMin.y *= parent->svgScale;
		// TODO: This may cause issues with SVGs that have negative values
		// Comment out this if-statement if it does
		if (!isSvgGroup)
		{
			scaledBboxMin = CMath::max(scaledBboxMin, Vec2{ 0, 0 });
		}
		Vec2 minCoord = textureOffset + scaledBboxMin;
		Vec2 bboxSize = (obj->bbox.max - obj->bbox.min);
		bboxSize.x *= parent->svgScale;
		bboxSize.y *= parent->svgScale;
		Vec2 maxCoord = minCoord + bboxSize;

		Vec2 inXRange = Vec2{ obj->bbox.min.x * parent->svgScale, obj->bbox.max.x * parent->svgScale };
		Vec2 inYRange = Vec2{ obj->bbox.min.y * parent->svgScale, obj->bbox.max.y * parent->svgScale };
		Vec2 outXRange = Vec2{ minCoord.x, maxCoord.x };
		Vec2 outYRange = Vec2{ minCoord.y, maxCoord.y };

		if (lengthToDraw > 0 && obj->numPaths > 0)
		{
			float lengthDrawn = 0.0f;
			nvgBeginPath(vg);

			for (int pathi = 0; pathi < obj->numPaths; pathi++)
			{
				if (obj->paths[pathi].numCurves > 0)
				{
					// Fade the stroke out as the svg fades in
					const glm::u8vec4& strokeColor = parent->strokeColor;
					if (glm::epsilonEqual(parent->strokeWidth, 0.0f, 0.01f))
					{
						nvgStrokeColor(vg, nvgRGBA(strokeColor.r, strokeColor.g, strokeColor.b, (unsigned char)((float)strokeColor.a * (1.0f - percentToFadeIn))));
						nvgStrokeWidth(vg, defaultStrokeWidth);
					}
					else
					{
						nvgStrokeColor(vg, nvgRGBA(strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a));
						nvgStrokeWidth(vg, parent->strokeWidth);
					}

					{
						Vec2 p0 = obj->paths[pathi].curves[0].p0;
						p0.x *= parent->svgScale;
						p0.y *= parent->svgScale;
						p0.x = CMath::mapRange(inXRange, outXRange, p0.x);
						p0.y = CMath::mapRange(inYRange, outYRange, p0.y);

						nvgMoveTo(vg,
							p0.x,
							p0.y
						);
					}

					for (int curvei = 0; curvei < obj->paths[pathi].numCurves; curvei++)
					{
						float lengthLeft = lengthToDraw - lengthDrawn;
						if (lengthLeft < 0.0f)
						{
							break;
						}

						const Curve& curve = obj->paths[pathi].curves[curvei];
						Vec2 p0 = curve.p0;

						switch (curve.type)
						{
						case CurveType::Bezier3:
						{
							Vec2 p1 = curve.as.bezier3.p1;
							Vec2 p2 = curve.as.bezier3.p2;
							Vec2 p3 = curve.as.bezier3.p3;

							float chordLength = CMath::length(p3 - p0);
							float controlNetLength = CMath::length(p1 - p0) + CMath::length(p2 - p1) + CMath::length(p3 - p2);
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

								Vec2 q0 = ((u0 * u0 * u0) * p0) +
									((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
									((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
									((t0 * t0 * t0) * p3);
								Vec2 q1 = ((u0 * u0 * u1) * p0) +
									((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
									((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
									((t0 * t0 * t1) * p3);
								Vec2 q2 = ((u0 * u1 * u1) * p0) +
									((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
									((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
									((t0 * t1 * t1) * p3);
								Vec2 q3 = ((u1 * u1 * u1) * p0) +
									((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
									((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
									((t1 * t1 * t1) * p3);

								p1 = q1;
								p2 = q2;
								p3 = q3;
							}

							p1.x *= parent->svgScale;
							p1.y *= parent->svgScale;
							p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
							p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

							p2.x *= parent->svgScale;
							p2.y *= parent->svgScale;
							p2.x = CMath::mapRange(inXRange, outXRange, p2.x);
							p2.y = CMath::mapRange(inYRange, outYRange, p2.y);

							p3.x *= parent->svgScale;
							p3.y *= parent->svgScale;
							p3.x = CMath::mapRange(inXRange, outXRange, p3.x);
							p3.y = CMath::mapRange(inYRange, outYRange, p3.y);

							nvgBezierTo(
								vg,
								p1.x, p1.y,
								p2.x, p2.y,
								p3.x, p3.y
							);
						}
						break;
						case CurveType::Bezier2:
						{
							Vec2 p1 = curve.as.bezier2.p1;
							Vec2 p2 = curve.as.bezier2.p1;
							Vec2 p3 = curve.as.bezier2.p2;

							// Degree elevated quadratic bezier curve
							Vec2 pr0 = p0;
							Vec2 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
							Vec2 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
							Vec2 pr3 = p3;

							float chordLength = CMath::length(pr3 - pr0);
							float controlNetLength = CMath::length(pr1 - pr0) + CMath::length(pr2 - pr1) + CMath::length(pr3 - pr2);
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

								Vec2 q0 = ((u0 * u0 * u0) * p0) +
									((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
									((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
									((t0 * t0 * t0) * p3);
								Vec2 q1 = ((u0 * u0 * u1) * p0) +
									((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
									((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
									((t0 * t0 * t1) * p3);
								Vec2 q2 = ((u0 * u1 * u1) * p0) +
									((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
									((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
									((t0 * t1 * t1) * p3);
								Vec2 q3 = ((u1 * u1 * u1) * p0) +
									((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
									((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
									((t1 * t1 * t1) * p3);

								pr1 = q1;
								pr2 = q2;
								pr3 = q3;
							}

							pr1.x *= parent->svgScale;
							pr1.y *= parent->svgScale;
							pr1.x = CMath::mapRange(inXRange, outXRange, pr1.x);
							pr1.y = CMath::mapRange(inYRange, outYRange, pr1.y);

							pr2.x *= parent->svgScale;
							pr2.y *= parent->svgScale;
							pr2.x = CMath::mapRange(inXRange, outXRange, pr2.x);
							pr2.y = CMath::mapRange(inYRange, outYRange, pr2.y);

							pr3.x *= parent->svgScale;
							pr3.y *= parent->svgScale;
							pr3.x = CMath::mapRange(inXRange, outXRange, pr3.x);
							pr3.y = CMath::mapRange(inYRange, outYRange, pr3.y);

							nvgBezierTo(
								vg,
								pr1.x, pr1.y,
								pr2.x, pr2.y,
								pr3.x, pr3.y
							);
						}
						break;
						case CurveType::Line:
						{
							Vec2 p1 = curve.as.line.p1;
							float curveLength = CMath::length(p1 - p0);
							lengthDrawn += curveLength;

							if (lengthLeft < curveLength)
							{
								float percentOfCurveToDraw = lengthLeft / curveLength;
								p1 = (p1 - p0) * percentOfCurveToDraw + p0;
							}

							p1.x *= parent->svgScale;
							p1.y *= parent->svgScale;
							p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
							p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

							nvgLineTo(vg, p1.x, p1.y);
						}
						break;
						default:
							g_logger_warning("Unknown curve type in render %d", (int)curve.type);
							break;
						}
					}
				}

				if (obj->paths[pathi].isHole)
				{
					nvgPathWinding(vg, NVG_HOLE);
				}
				else
				{
					nvgPathWinding(vg, NVG_SOLID);
				}

				if (lengthDrawn > lengthToDraw)
				{
					break;
				}
			}

			nvgStroke(vg);

			// Fill the path as well if it's fading in
			if (amountToFadeIn > 0)
			{
				const glm::u8vec4& fillColor = parent->fillColor;
				nvgFillColor(vg, nvgRGBA(fillColor.r, fillColor.g, fillColor.b, (unsigned char)(fillColor.a * percentToFadeIn)));
				nvgFill(vg);
			}
		}

		if (parent->drawDebugBoxes)
		{
			float debugStrokeWidth = cachePadding.x;
			float strokeWidthCorrectionPos = debugStrokeWidth * 0.5f;
			float strokeWidthCorrectionNeg = -debugStrokeWidth;

			if (parent->drawCurveDebugBoxes)
			{
				for (int pathi = 0; pathi < obj->numPaths; pathi++)
				{
					if (obj->paths[pathi].numCurves > 0)
					{
						for (int curvei = 0; curvei < obj->paths[pathi].numCurves; curvei++)
						{
							const Curve& curve = obj->paths[pathi].curves[curvei];
							Vec2 p0 = curve.p0;
							p0.x *= parent->svgScale;
							p0.y *= parent->svgScale;
							p0.x = CMath::mapRange(inXRange, outXRange, p0.x);
							p0.y = CMath::mapRange(inYRange, outYRange, p0.y);

							switch (curve.type)
							{
							case CurveType::Bezier3:
							{
								Vec2 p1 = curve.as.bezier3.p1;
								Vec2 p2 = curve.as.bezier3.p2;
								Vec2 p3 = curve.as.bezier3.p3;

								p1.x *= parent->svgScale;
								p1.y *= parent->svgScale;
								p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
								p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

								p2.x *= parent->svgScale;
								p2.y *= parent->svgScale;
								p2.x = CMath::mapRange(inXRange, outXRange, p2.x);
								p2.y = CMath::mapRange(inYRange, outYRange, p2.y);

								p3.x *= parent->svgScale;
								p3.y *= parent->svgScale;
								p3.x = CMath::mapRange(inXRange, outXRange, p3.x);
								p3.y = CMath::mapRange(inYRange, outYRange, p3.y);

								BBox bbox = CMath::bezier3BBox(p0, p1, p2, p3);

								nvgBeginPath(vg);
								nvgStrokeWidth(vg, debugStrokeWidth);
								nvgStrokeColor(vg, nvgRGB(255, 0, 0));
								nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
								nvgMoveTo(vg,
									bbox.min.x + strokeWidthCorrectionPos,
									bbox.min.y + strokeWidthCorrectionPos
								);
								nvgRect(vg,
									bbox.min.x + strokeWidthCorrectionPos,
									bbox.min.y + strokeWidthCorrectionPos,
									bbox.max.x - bbox.min.x + strokeWidthCorrectionNeg,
									bbox.max.y - bbox.min.y + strokeWidthCorrectionNeg
								);
								nvgClosePath(vg);
								nvgStroke(vg);
							}
							break;
							case CurveType::Bezier2:
							{
								Vec2 p1 = curve.as.bezier2.p1;
								Vec2 p2 = curve.as.bezier2.p1;
								Vec2 p3 = curve.as.bezier2.p2;

								p1.x *= parent->svgScale;
								p1.y *= parent->svgScale;
								p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
								p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

								p2.x *= parent->svgScale;
								p2.y *= parent->svgScale;
								p2.x = CMath::mapRange(inXRange, outXRange, p2.x);
								p2.y = CMath::mapRange(inYRange, outYRange, p2.y);

								p3.x *= parent->svgScale;
								p3.y *= parent->svgScale;
								p3.x = CMath::mapRange(inXRange, outXRange, p3.x);
								p3.y = CMath::mapRange(inYRange, outYRange, p3.y);

								// Degree elevated quadratic bezier curve
								Vec2 pr0 = p0;
								Vec2 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
								Vec2 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
								Vec2 pr3 = p3;

								BBox bbox = CMath::bezier3BBox(pr0, pr1, pr2, pr3);

								nvgBeginPath(vg);
								nvgStrokeWidth(vg, debugStrokeWidth);
								nvgStrokeColor(vg, nvgRGB(255, 0, 0));
								nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
								nvgMoveTo(vg,
									bbox.min.x + strokeWidthCorrectionPos,
									bbox.min.y + strokeWidthCorrectionPos
								);
								nvgRect(vg,
									bbox.min.x + strokeWidthCorrectionPos,
									bbox.min.y + strokeWidthCorrectionPos,
									bbox.max.x - bbox.min.x + strokeWidthCorrectionNeg,
									bbox.max.y - bbox.min.y + strokeWidthCorrectionNeg
								);
								nvgClosePath(vg);
								nvgStroke(vg);
							}
							break;
							case CurveType::Line:
							{
								Vec2 p1 = curve.as.line.p1;

								p1.x *= parent->svgScale;
								p1.y *= parent->svgScale;
								p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
								p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

								BBox bbox = CMath::bezier1BBox(p0, p1);

								nvgBeginPath(vg);
								nvgStrokeWidth(vg, debugStrokeWidth);
								nvgStrokeColor(vg, nvgRGB(255, 0, 0));
								nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
								nvgMoveTo(vg,
									bbox.min.x + strokeWidthCorrectionPos,
									bbox.min.y + strokeWidthCorrectionPos
								);
								nvgRect(vg,
									bbox.min.x + strokeWidthCorrectionPos,
									bbox.min.y + strokeWidthCorrectionPos,
									bbox.max.x - bbox.min.x + strokeWidthCorrectionNeg,
									bbox.max.y - bbox.min.y + strokeWidthCorrectionNeg
								);
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
			}

			nvgBeginPath(vg);
			nvgStrokeWidth(vg, debugStrokeWidth);
			nvgStrokeColor(vg, nvgRGB(0, 255, 0));
			nvgFillColor(vg, nvgRGBA(0, 0, 0, 0));
			nvgMoveTo(vg,
				scaledBboxMin.x + textureOffset.x - (parent->strokeWidth * 0.5f) + strokeWidthCorrectionPos,
				scaledBboxMin.y + textureOffset.y - (parent->strokeWidth * 0.5f) + strokeWidthCorrectionPos
			);
			nvgRect(vg,
				scaledBboxMin.x + textureOffset.x - (parent->strokeWidth * 0.5f) + strokeWidthCorrectionPos,
				scaledBboxMin.y + textureOffset.y - (parent->strokeWidth * 0.5f) + strokeWidthCorrectionPos,
				bboxSize.x + parent->strokeWidth + strokeWidthCorrectionNeg,
				bboxSize.y + parent->strokeWidth + strokeWidthCorrectionNeg
			);
			nvgStroke(vg);
			nvgClosePath(vg);
		}

		nvgResetTransform(vg);
	}
}