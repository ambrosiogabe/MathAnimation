#include "svg/Svg.h"
#include "svg/SvgParser.h"
#include "svg/SvgCache.h"
#include "animation/Animation.h"
#include "utils/CMath.h"
#include "renderer/Renderer.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Colors.h"
#include "renderer/PerspectiveCamera.h"
#include "core/Application.h"

#include <plutovg.h>

#ifndef isnan
#include "math.h"
#endif

namespace MathAnim
{
	namespace Svg
	{
		// ----------------- Private Variables -----------------
		constexpr int initialMaxCapacity = 5;

		// ----------------- Internal functions -----------------
		static void checkResize(Path& path);

		SvgObject createDefault()
		{
			SvgObject res = {};
			res.approximatePerimeter = 0.0f;
			// Dummy allocation to allow memory tracking when reallocing 
			// TODO: Fix the dang memory allocation library so I don't have to do this!
			res.paths = (Path*)g_memory_allocate(sizeof(Path));
			res.numPaths = 0;
			res.bbox.min = Vec2{ 0, 0 };
			res.bbox.max = Vec2{ 0, 0 };
			res._cursor = Vec2{ 0, 0 };
			res.fillColor = Vec4{ 1, 1, 1, 1 };
			res.fillType = FillType::NonZeroFillType;
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
			return res;
		}

		void init()
		{
		}

		void beginSvgGroup(SvgGroup* group)
		{
		}

		void pushSvgToGroup(SvgGroup* group, const SvgObject& obj, const std::string& id, const Vec2& offset)
		{
			group->numObjects++;
			group->objects = (SvgObject*)g_memory_realloc(group->objects, sizeof(SvgObject) * group->numObjects);
			g_logger_assert(group->objects != nullptr, "Ran out of RAM.");
			group->objectOffsets = (Vec2*)g_memory_realloc(group->objectOffsets, sizeof(Vec2) * group->numObjects);
			g_logger_assert(group->objectOffsets != nullptr, "Ran out of RAM.");

			group->objectOffsets[group->numObjects - 1] = offset;
			group->objects[group->numObjects - 1] = Svg::createDefault();
			Svg::copy(group->objects + (group->numObjects - 1), &obj);

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
				object->_cursor = firstPoint;
			}
			else
			{
				object->_cursor = firstPoint + object->_cursor;
			}
			object->paths[object->numPaths - 1].curves[0].p0 = object->_cursor;
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
				// Apparently having to move to commands in a row is valid, something like:
				//    M0, 0 M10, 10
				// is perfectly valid (for whatever reason), the first MoveTo command
				// just does nothing, so I won't begin a separate sub-path if a MoveTo
				// is followed by another MoveTo
				if (object->paths[object->numPaths - 1].numCurves > 0)
				{
					bool isHole = object->numPaths > 1;
					closePath(object, true, isHole);
				}
				object->_cursor = absolute ? point : object->_cursor + point;
				if (object->paths[object->numPaths - 1].numCurves > 0)
				{
					beginPath(object, object->_cursor);
				}
			}
		}

		void lineTo(SvgObject* object, const Vec2& point, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a lineTo when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = object->_cursor;
			path.curves[path.numCurves - 1].as.line.p1 = absolute ? point : point + object->_cursor;
			path.curves[path.numCurves - 1].type = CurveType::Line;

			object->_cursor = path.curves[path.numCurves - 1].as.line.p1;
		}

		void hzLineTo(SvgObject* object, float xPoint, bool absolute)
		{
			Vec2 position = absolute
				? Vec2{ xPoint, object->_cursor.y }
			: Vec2{ xPoint, 0.0f } + object->_cursor;
			lineTo(object, position, true);
		}

		void vtLineTo(SvgObject* object, float yPoint, bool absolute)
		{
			Vec2 position = absolute
				? Vec2{ object->_cursor.x, yPoint }
			: Vec2{ 0.0f, yPoint } + object->_cursor;
			lineTo(object, position, true);
		}

		void bezier2To(SvgObject* object, const Vec2& control, const Vec2& dest, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a bezier2To when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = object->_cursor;

			path.curves[path.numCurves - 1].as.bezier2.p1 = absolute ? control : control + object->_cursor;
			path.curves[path.numCurves - 1].as.bezier2.p2 = absolute ? dest : dest + object->_cursor;

			// Only update the cursor to the final point of the curve
			object->_cursor = path.curves[path.numCurves - 1].as.bezier2.p2;

			path.curves[path.numCurves - 1].type = CurveType::Bezier2;
		}

		void bezier3To(SvgObject* object, const Vec2& control0, const Vec2& control1, const Vec2& dest, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a bezier3To when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = object->_cursor;

			path.curves[path.numCurves - 1].as.bezier3.p1 = absolute ? control0 : control0 + object->_cursor;
			path.curves[path.numCurves - 1].as.bezier3.p2 = absolute ? control1 : control1 + object->_cursor;
			path.curves[path.numCurves - 1].as.bezier3.p3 = absolute ? dest : dest + object->_cursor;

			// Only update the cursor to the final point of the curve
			object->_cursor = path.curves[path.numCurves - 1].as.bezier3.p3;

			path.curves[path.numCurves - 1].type = CurveType::Bezier3;
		}

		void smoothBezier2To(SvgObject* object, const Vec2& dest, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a bezier3To when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = object->_cursor;

			Vec2 control0 = object->_cursor;
			if (path.numCurves > 1)
			{
				if (path.curves[path.numCurves - 2].type == CurveType::Bezier2)
				{
					Vec2 prevControl1 = path.curves[path.numCurves - 2].as.bezier2.p1;
					// Reflect the previous c2 about the current cursor
					control0 = (-1.0f * (prevControl1 - object->_cursor)) + object->_cursor;
				}
			}
			path.curves[path.numCurves - 1].as.bezier2.p1 = control0;
			path.curves[path.numCurves - 1].as.bezier2.p2 = absolute ? dest : dest + object->_cursor;

			// Only update the cursor to the final point on the curve
			object->_cursor = path.curves[path.numCurves - 1].as.bezier2.p2;

			path.curves[path.numCurves - 1].type = CurveType::Bezier2;
		}

		void smoothBezier3To(SvgObject* object, const Vec2& control1, const Vec2& dest, bool absolute)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a bezier3To when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1].p0 = object->_cursor;

			Vec2 control0 = object->_cursor;
			if (path.numCurves > 1)
			{
				if (path.curves[path.numCurves - 2].type == CurveType::Bezier3)
				{
					Vec2 prevControl1 = path.curves[path.numCurves - 2].as.bezier3.p2;
					// Reflect the previous c2 about the current cursor
					control0 = (-1.0f * (prevControl1 - object->_cursor)) + object->_cursor;
				}
			}
			path.curves[path.numCurves - 1].as.bezier3.p1 = control0;
			path.curves[path.numCurves - 1].as.bezier3.p2 = absolute ? control1 : control1 + object->_cursor;
			path.curves[path.numCurves - 1].as.bezier3.p3 = absolute ? dest : dest + object->_cursor;

			// Only update the cursor to the final point on the curve
			object->_cursor = path.curves[path.numCurves - 1].as.bezier3.p3;

			path.curves[path.numCurves - 1].type = CurveType::Bezier3;
		}

		// Implementation taken from https://github.com/BigBadaboom/androidsvg/blob/5db71ef0007b41644258c1f139f941017aef7de3/androidsvg/src/main/java/com/caverock/androidsvg/utils/SVGAndroidRenderer.java#L2889
		void arcTo(SvgObject* object, const Vec2& radius, float xAxisRot, bool largeArc, bool sweep, const Vec2& inDst, bool absolute)
		{
			Vec2 dst = inDst;
			if (!absolute)
			{
				dst += object->_cursor;
			}

			if (CMath::compare(object->_cursor, dst, 0.001f))
			{
				// If the endpoints (x, y) and (x0, y0) are identical, then this
				// is equivalent to omitting the elliptical arc segment entirely.
				// (behaviour specified by the spec)
				return;
			}

			// Handle degenerate case (behaviour specified by the spec)
			if (CMath::compare(radius.x, 0.0f, 0.001f) || CMath::compare(radius.y, 0.001f))
			{
				lineTo(object, dst);
				return;
			}

			// Sign of the radii is ignored (behaviour specified by the spec)
			float rx = CMath::abs(radius.x);
			float ry = CMath::abs(radius.y);

			// Convert angle from degrees to radians
			typedef float Precision;
			Precision angleRad = CMath::toRadians(fmodf(xAxisRot, 360.0f));
			Precision cosAngle = glm::cos(angleRad);
			Precision sinAngle = glm::sin(angleRad);

			// We simplify the calculations by transforming the arc so that the origin is at the
			// midpoint calculated above followed by a rotation to line up the coordinate axes
			// with the axes of the ellipse.

			// Compute the midpoint of the line between the current and the end point
			Precision dx2 = (object->_cursor.x - dst.x) / 2.0;
			Precision dy2 = (object->_cursor.y - dst.y) / 2.0;

			// Step 1 : Compute (x1', y1')
			// x1,y1 is the midpoint vector rotated to take the arc's angle out of consideration
			Precision x1 = (cosAngle * dx2 + sinAngle * dy2);
			Precision y1 = (-sinAngle * dx2 + cosAngle * dy2);

			Precision rx_sq = rx * rx;
			Precision ry_sq = ry * ry;
			Precision x1_sq = x1 * x1;
			Precision y1_sq = y1 * y1;

			// Check that radii are large enough.
			// If they are not, the spec says to scale them up so they are.
			// This is to compensate for potential rounding errors/differences between SVG implementations.
			Precision radiiCheck = x1_sq / rx_sq + y1_sq / ry_sq;
			if (radiiCheck > 0.99999)
			{
				Precision radiiScale = glm::sqrt(radiiCheck) * 1.00001;
				rx = (float)(radiiScale * rx);
				ry = (float)(radiiScale * ry);
				rx_sq = rx * rx;
				ry_sq = ry * ry;
			}

			// Step 2 : Compute (cx1, cy1) - the transformed centre point
			Precision sign = (largeArc == sweep) ? -1 : 1;
			Precision sq = ((rx_sq * ry_sq) - (rx_sq * y1_sq) - (ry_sq * x1_sq)) / ((rx_sq * y1_sq) + (ry_sq * x1_sq));
			sq = (sq < 0) ? 0 : sq;
			Precision coef = (sign * glm::sqrt(sq));
			Precision cx1 = coef * ((rx * y1) / ry);
			Precision cy1 = coef * -((ry * x1) / rx);

			// Step 3 : Compute (cx, cy) from (cx1, cy1)
			Precision sx2 = (object->_cursor.x + dst.x) / 2.0;
			Precision sy2 = (object->_cursor.y + dst.y) / 2.0;
			Precision cx = sx2 + (cosAngle * cx1 - sinAngle * cy1);
			Precision cy = sy2 + (sinAngle * cx1 + cosAngle * cy1);

			// Step 4 : Compute the angleStart (angle1) and the angleExtent (dangle)
			Precision ux = (x1 - cx1) / rx;
			Precision uy = (y1 - cy1) / ry;
			Precision vx = (-x1 - cx1) / rx;
			Precision vy = (-y1 - cy1) / ry;
			Precision p, n;

			// Angle betwen two vectors is +/- acos( u.v / len(u) * len(v))
			// Where '.' is the dot product. And +/- is calculated from the sign of the cross product (u x v)

			// Compute the start angle
			// The angle between (ux,uy) and the 0deg angle (1,0)
			n = glm::sqrt((ux * ux) + (uy * uy));  // len(u) * len(1,0) == len(u)
			p = ux;                                // u.v == (ux,uy).(1,0) == (1 * ux) + (0 * uy) == ux
			sign = (uy < 0) ? -1.0 : 1.0;          // u x v == (1 * uy - ux * 0) == uy
			Precision angleStart = sign * glm::acos(p / n);  // No need for checkedArcCos() here. (p >= n) should always be true.

			// Compute the angle extent
			n = glm::sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
			p = ux * vx + uy * vy;
			sign = (ux * vy - uy * vx < 0) ? -1.0f : 1.0f;
			Precision angleExtent = sign * (
				(p / n < -1.0) ? glm::pi<Precision>() : (p / n > 1.0) ? 0 : glm::acos(p / n)
				);

			// Catch angleExtents of 0, which will cause problems later in arcToBeziers
			if (CMath::compare(angleExtent, 0.001f))
			{
				lineTo(object, dst);
				return;
			}

			if (!sweep && angleExtent > 0)
			{
				angleExtent -= glm::two_pi<Precision>();
			}
			else if (sweep && angleExtent < 0)
			{
				angleExtent += glm::two_pi<Precision>();
			}
			angleExtent = fmodf(angleExtent, glm::two_pi<Precision>());
			angleStart = fmodf(angleStart, glm::two_pi<Precision>());

			// Many elliptical arc implementations including the Java2D and Android ones, only
			// support arcs that are axis aligned.  Therefore we need to substitute the arc
			// with bezier curves.  The following method call will generate the beziers for
			// a unit circle that covers the arc angles we want.
			int numSegments = (int)glm::ceil(glm::abs(angleExtent) * 2.0 / glm::pi<Precision>());  // (angleExtent / 90deg)

			Precision angleIncrement = angleExtent / numSegments;

			// The length of each control point vector is given by the following formula.
			Precision controlLength = 4.0 / 3.0 * glm::sin(angleIncrement / 2.0) / (1.0 + glm::cos(angleIncrement / 2.0));

			const int bezierPointsLength = numSegments * 6;
			float* bezierPoints = (float*)g_memory_allocate(sizeof(float) * bezierPointsLength);
			int pos = 0;

			for (int i = 0; i < numSegments; i++)
			{
				Precision angle = angleStart + i * angleIncrement;
				// Calculate the control vector at this angle
				Precision dx = glm::cos(angle);
				Precision dy = glm::sin(angle);
				// First control point
				bezierPoints[pos++] = (float)(dx - controlLength * dy);
				bezierPoints[pos++] = (float)(dy + controlLength * dx);
				// Second control point
				angle += angleIncrement;
				dx = glm::cos(angle);
				dy = glm::sin(angle);
				bezierPoints[pos++] = (float)(dx + controlLength * dy);
				bezierPoints[pos++] = (float)(dy - controlLength * dx);
				// Endpoint of bezier
				bezierPoints[pos++] = (float)dx;
				bezierPoints[pos++] = (float)dy;
			}

			// Calculate a transformation matrix that will move and scale these bezier points to the correct location.
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(rx, ry, 0.0f));
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), xAxisRot, glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(cx, cy, 0.0f));
			glm::mat4 transformation = translation * rotation * scale;

			// The last point in the bezier set should match exactly the last coord pair in the arc (ie: x,y). But
			// considering all the mathematical manipulation we have been doing, it is bound to be off by a tiny
			// fraction. Experiments show that it can be up to around 0.00002.  So why don't we just set it to
			// exactly what it ought to be.
			bezierPoints[bezierPointsLength - 2] = dst.x;
			bezierPoints[bezierPointsLength - 1] = dst.y;

			// Final step is to add the bezier curves to the path
			for (int i = 0; i < bezierPointsLength; i += 6)
			{
				glm::vec4 c1 = glm::vec4(bezierPoints[i], bezierPoints[i + 1], 0.0f, 1.0f);
				glm::vec4 c2 = glm::vec4(bezierPoints[i + 2], bezierPoints[i + 3], 0.0f, 1.0f);
				glm::vec4 p2 = glm::vec4(bezierPoints[i + 4], bezierPoints[i + 5], 0.0f, 1.0f);

				c1 = transformation * c1;
				c2 = transformation * c2;

				if (i != bezierPointsLength - 6)
				{
					p2 = transformation * p2;
				}

				bezier3To(
					object,
					Vec2{ c1.x, c1.y },
					Vec2{ c2.x, c2.y },
					Vec2{ p2.x, p2.y }
				);
			}

			g_memory_free(bezierPoints);
		}

		void addCurveManually(SvgObject* object, const Curve& curve)
		{
			g_logger_assert(object->numPaths > 0, "object->numPaths == 0. Cannot create a lineTo when no path exists.");
			Path& path = object->paths[object->numPaths - 1];
			path.numCurves++;
			checkResize(path);

			path.curves[path.numCurves - 1] = curve;

			switch (curve.type)
			{
			case CurveType::Line:
				object->_cursor = curve.as.line.p1;
				break;
			case CurveType::Bezier2:
				object->_cursor = curve.as.bezier2.p2;
				break;
			case CurveType::Bezier3:
				object->_cursor = curve.as.bezier3.p3;
				break;
			case CurveType::None:
				break;
			}
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

			dest->fillType = src->fillType;
			dest->fillColor = src->fillColor;

			dest->calculateApproximatePerimeter();
			dest->calculateBBox();
		}

		SvgObject* interpolate(const SvgObject* src, const SvgObject* dst, float t)
		{
			// Count the total number of curves in all paths
			int srcNumCurves = 0;
			int dstNumCurves = 0;
			for (int pathi = 0; pathi < src->numPaths; pathi++)
			{
				srcNumCurves += src->paths[pathi].numCurves;
			}
			for (int pathi = 0; pathi < dst->numPaths; pathi++)
			{
				dstNumCurves += dst->paths[pathi].numCurves;
			}

			SvgObject* modifiedSrcObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			SvgObject* modifiedDstObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			*modifiedSrcObject = Svg::createDefault();
			*modifiedDstObject = Svg::createDefault();

			const SvgObject* splitObj = srcNumCurves < dstNumCurves
				? src
				: dst;
			const SvgObject* nonSplitObj = srcNumCurves < dstNumCurves
				? dst
				: src;

			// After we finish splitting everything, we may need to swap
			// the objects so that we interpolate in the proper direction
			bool shouldSwapSplitObjs = splitObj != src;

			// We need to make them equal to interpolate properly.
			// So this algorithm will make sure we end up with two
			// SVGObjects that have an equal number of subpaths and
			// total curves while the SVGObjects are identical to the
			// originals
			int desiredNumCurves = glm::max(srcNumCurves, dstNumCurves);
			int splitPathi = 0;
			int nonSplitPathi = 0;

			const Path* splitPath = splitObj->paths + splitPathi;
			const Path* nonSplitPath = nonSplitObj->paths + nonSplitPathi;

			g_logger_assert(splitPath->numCurves > 0, "It's undefined to interpolate between a path with 0 curves.");
			g_logger_assert(nonSplitPath->numCurves > 0, "It's undefined to interpolate between a path with 0 curves.");

			// Begin the paths
			Svg::beginPath(modifiedSrcObject, splitPath->curves[0].p0);
			Svg::beginPath(modifiedDstObject, nonSplitPath->curves[0].p0);

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
			int numCurvesAdded = 0;
			int splitCurvei = 0;
			int nonSplitCurvei = 0;
			int numNonSplitCurvesLeft = desiredNumCurves;
			int numSplitCurvesLeft = glm::min(srcNumCurves, dstNumCurves);
			while (numCurvesAdded < desiredNumCurves)
			{
				float numSplits = (float)numNonSplitCurvesLeft / (float)numSplitCurvesLeft;
				// Round to the nearest half. 
				//   3.5 rounds to 4
				//   3.49 rounds to 3
				float fractionalSplits = numSplits - glm::floor(numSplits);
				int roundedNumSplits = glm::max((int)glm::floor(numSplits), 1);
				if (fractionalSplits >= 0.5f)
				{
					roundedNumSplits = (int)glm::ceil(numSplits);
				}
				g_logger_assert(nonSplitCurvei + roundedNumSplits <= desiredNumCurves, "How did this happen? Somehow we are adding too many splits to an interpolated path.");

				{
					// Check if we're starting a new sub-path for split obj
					bool beginNewSubpath = false;
					bool splitIsHole, nonSplitIsHole;
					if (splitCurvei >= splitPath->numCurves)
					{
						splitIsHole = splitObj->paths[splitPathi].isHole;
						nonSplitIsHole = nonSplitObj->paths[nonSplitPathi].isHole;
						beginNewSubpath = true;

						splitPathi++;
						g_logger_assert(splitPathi < splitObj->numPaths, "How did this happen? Somehow we tried to increment past too many paths while interpolating SVG objects.");
						splitPath = splitObj->paths + splitPathi;
						g_logger_assert(splitPath->numCurves > 0, "It's undefined to interpolate between a path with 0 curves.");
						splitCurvei = 0;
					}

					// Check if we're starting a new sub-path for nonsplit obj
					if (nonSplitCurvei >= nonSplitPath->numCurves)
					{
						nonSplitIsHole = nonSplitObj->paths[nonSplitPathi].isHole;
						splitIsHole = splitObj->paths[splitPathi].isHole;
						beginNewSubpath = true;

						nonSplitPathi++;
						g_logger_assert(nonSplitPathi < nonSplitObj->numPaths, "How did this happen? Somehow we tried to increment past too many paths while interpolating SVG objects.");
						nonSplitPath = nonSplitObj->paths + nonSplitPathi;
						g_logger_assert(nonSplitPath->numCurves > 0, "It's undefined to interpolate between a path with 0 curves.");
						nonSplitCurvei = 0;
					}

					// Begin new sub-path if determined necessary
					if (beginNewSubpath)
					{
						// Close both paths so we have equal sub-path splits
						Svg::closePath(modifiedSrcObject, true, splitIsHole);
						Svg::closePath(modifiedDstObject, true, nonSplitIsHole);

						//Vec2 splitFirstPathP0 = splitPath->curves[0].p0;
						//Vec2 nonSplitFirstPathP0 = nonSplitPath->curves[0].p0;
						//Svg::beginPath(modifiedSrcObject, splitFirstPathP0);
						//Svg::beginPath(modifiedDstObject, nonSplitFirstPathP0);
						//
						//Vec2 splitFirstP0 = splitPath->curves[splitCurvei].p0;
						//Vec2 nonSplitFirstP0 = nonSplitPath->curves[nonSplitCurvei].p0;
						//Svg::lineTo(modifiedSrcObject, splitFirstP0);
						//Svg::lineTo(modifiedDstObject, nonSplitFirstP0);

						Vec2 splitFirstP0 = splitPath->curves[splitCurvei].p0;
						Vec2 nonSplitFirstP0 = nonSplitPath->curves[nonSplitCurvei].p0;
						Svg::beginPath(modifiedSrcObject, splitFirstP0);
						Svg::beginPath(modifiedDstObject, nonSplitFirstP0);
					}
				}

				// Check if we have enough curves in the nonSplit object to add 
				// to the split object. If not, reduce the number of times
				// we'll split to avoid buffer overflow
				int beginNewNonSplitPathAt = -1;
				int beginNewSplitPathAt = -1;
				if (nonSplitCurvei + roundedNumSplits > nonSplitPath->numCurves)
				{
					// The split is between a pathStart/pathEnd so, we'll have to just end the path early
					// and recalculate stuff...
					// How many can we add without buffer overrun?
					beginNewNonSplitPathAt = nonSplitPath->numCurves - nonSplitCurvei;
					beginNewSplitPathAt = roundedNumSplits - beginNewNonSplitPathAt;
				}

				// Increment the totals to what they will be for the next iteration
				numNonSplitCurvesLeft -= roundedNumSplits; // Subtract the number of splits from numerator
				numSplitCurvesLeft -= 1; // Subtract 1 from denominator
				numCurvesAdded += roundedNumSplits; // Keep track of the total number added

				// First copy the non-split curves to the new object
				for (int i = 0; i < roundedNumSplits; i++)
				{
					if (i == beginNewNonSplitPathAt)
					{
						bool isHole = nonSplitObj->paths[nonSplitPathi].isHole;
						nonSplitPathi++;
						g_logger_assert(nonSplitPathi < nonSplitObj->numPaths, "How did this happen? Somehow we tried to increment past too many paths while interpolating SVG objects.");
						nonSplitPath = nonSplitObj->paths + nonSplitPathi;
						g_logger_assert(nonSplitPath->numCurves > 0, "It's undefined to interpolate between a path with 0 curves.");
						nonSplitCurvei = 0;

						// Begin new path so that things don't get weird
						Svg::closePath(modifiedDstObject, true, isHole);

						Vec2 nonSplitFirstP0 = nonSplitPath->curves[nonSplitCurvei].p0;
						Svg::beginPath(modifiedDstObject, nonSplitFirstP0);
					}

					Svg::addCurveManually(modifiedDstObject, nonSplitPath->curves[nonSplitCurvei]);
					nonSplitCurvei++;
				}

				// Then add the split curves to the other new object
				// Split this curve roundedNumSplits number of times evenly
				// Copy the curve because it gets modified
				Curve nextCurve = splitPath->curves[splitCurvei];
				while (roundedNumSplits > 0)
				{
					float tSplit = 1.0f / (float)roundedNumSplits;

					const Vec2& p0 = nextCurve.p0;

					if (roundedNumSplits == beginNewSplitPathAt)
					{
						// Begin new path so things don't get weird...
						bool isHole = splitObj->paths[splitPathi].isHole;

						// Begin new path so that things don't get weird
						Svg::closePath(modifiedSrcObject, true, isHole);
						Svg::beginPath(modifiedSrcObject, p0);
					}

					// Interpolate the curve by tSplit
					float percentOfCurveToDraw = tSplit;

					if (roundedNumSplits > 2)
					{
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
								float u1 = 1.0f - t1;

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
								Svg::bezier3To(modifiedSrcObject, q1, q2, q3);
								nextCurve.p0 = q3;
							}

							{
								// Second split
								float t0 = percentOfCurveToDraw;
								float t1 = 1.0f;
								float u0 = 1.0f - t0;
								float u1 = 1.0f - t1;

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
								nextCurve.as.bezier3.p1 = q1;
								nextCurve.as.bezier3.p2 = q2;
								nextCurve.as.bezier3.p3 = q3;
							}
						}
						break;
						case CurveType::Bezier2:
						{
							Vec2 p1 = nextCurve.as.bezier2.p1;
							Vec2 p2 = nextCurve.as.bezier2.p2;

							// Degree elevated quadratic bezier curve
							Vec2 pr1 = ((1.0f / 3.0f) * p0) + ((2.0f / 3.0f) * p1);
							Vec2 pr2 = ((2.0f / 3.0f) * p1) + ((1.0f / 3.0f) * p2);

							p1 = pr1;
							p2 = pr2;
							Vec2 p3 = p2;

							// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
							{
								// Split 1
								float t0 = 0.0f;
								float t1 = percentOfCurveToDraw;
								float u0 = 1.0f - t0;
								float u1 = 1.0f - t1;

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
								Svg::bezier3To(modifiedSrcObject, q1, q2, q3);
								nextCurve.p0 = q3;
							}

							{
								// Split 2
								float t0 = percentOfCurveToDraw;
								float t1 = 1.0f;
								float u0 = 1.0f - t0;
								float u1 = 1.0f - t1;

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
							Vec2 interpP1 = ((p1 - p0) * percentOfCurveToDraw) + p0;
							Svg::lineTo(modifiedSrcObject, interpP1);

							// Set up next split
							nextCurve.type = CurveType::Line;
							nextCurve.p0 = interpP1;
							nextCurve.as.line.p1 = p1;
						}
						break;
						case CurveType::None:
							break;
						}
					}
					else
					{
						// Add the last curve manually
						Svg::addCurveManually(modifiedSrcObject, nextCurve);
					}

					roundedNumSplits--;
				}

				// Increment split curve
				splitCurvei++;
			}

			// Close the paths
			//Svg::lineTo(modifiedSrcObject, splitPath->curves[0].p0);
			//Svg::lineTo(modifiedDstObject, nonSplitPath->curves[0].p0);
			Svg::closePath(modifiedSrcObject, false, splitPath->isHole);
			Svg::closePath(modifiedDstObject, false, nonSplitPath->isHole);

			if (shouldSwapSplitObjs)
			{
				SvgObject* tmp = modifiedDstObject;
				modifiedDstObject = modifiedSrcObject;
				modifiedSrcObject = tmp;
			}

			g_logger_assert(modifiedDstObject->numPaths == modifiedSrcObject->numPaths, "Somehow the interpolated objects didn't end up with the same number of sub-paths.");

			SvgObject* res = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			*res = Svg::createDefault();
			for (int pathi = 0; pathi < modifiedSrcObject->numPaths; pathi++)
			{
				const Path* path0 = modifiedSrcObject->paths + pathi;
				const Path* path1 = modifiedDstObject->paths + pathi;

				g_logger_assert(path0->numCurves == path1->numCurves, "Somehow the interpolated objects didn't end up with the same number of curves in a sub-path.");

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
					Vec2 p1a, p2a, p3a;
					Vec2 p1b, p2b, p3b;

					const Curve& srcCurve = path0->curves[curvei];
					const Curve& dstCurve = path1->curves[curvei];

					// First get the control points depending on the type of the curve
					switch (srcCurve.type)
					{
					case CurveType::Bezier3:
						p1a = srcCurve.as.bezier3.p1;
						p2a = srcCurve.as.bezier3.p2;
						p3a = srcCurve.as.bezier3.p3;
						break;
					case CurveType::Bezier2:
					{
						Vec2 p0 = srcCurve.p0;
						Vec2 p1 = srcCurve.as.bezier2.p1;
						Vec2 p2 = srcCurve.as.bezier2.p2;

						// Degree elevated quadratic bezier curve
						p1a = ((1.0f / 3.0f) * p0) + ((2.0f / 3.0f) * p1);
						p2a = ((2.0f / 3.0f) * p1) + ((1.0f / 3.0f) * p2);
						p3a = p2;
					}
					break;
					case CurveType::Line:
					{
						Vec2 tmpP1a = (srcCurve.as.line.p1 - srcCurve.p0) * (1.0f / 3.0f) + srcCurve.p0;
						Vec2 tmpP2a = (srcCurve.as.line.p1 - srcCurve.p0) * (2.0f / 3.0f) + srcCurve.p0;
						p1a = tmpP1a;
						p2a = tmpP2a;
						p3a = srcCurve.as.line.p1;
					}
					break;
					case CurveType::None:
						break;
					}

					switch (dstCurve.type)
					{
					case CurveType::Bezier3:
						p1b = dstCurve.as.bezier3.p1;
						p2b = dstCurve.as.bezier3.p2;
						p3b = dstCurve.as.bezier3.p3;
						break;
					case CurveType::Bezier2:
					{
						Vec2 p0 = dstCurve.p0;
						Vec2 p1 = dstCurve.as.bezier2.p1;
						Vec2 p2 = dstCurve.as.bezier2.p2;

						// Degree elevated quadratic bezier curve
						p1b = ((1.0f / 3.0f) * p0) + ((2.0f / 3.0f) * p1);
						p2b = ((2.0f / 3.0f) * p1) + ((1.0f / 3.0f) * p2);
						p3b = p2;
					}
					break;
					case CurveType::Line:
					{
						Vec2 tmpP1b = (dstCurve.as.line.p1 - dstCurve.p0) * (1.0f / 3.0f) + dstCurve.p0;
						Vec2 tmpP2b = (dstCurve.as.line.p1 - dstCurve.p0) * (2.0f / 3.0f) + dstCurve.p0;
						p1b = tmpP1b;
						p2b = tmpP2b;
						p3b = dstCurve.as.line.p1;
					}
					break;
					case CurveType::None:
						break;
					}

					// Then interpolate between the control points
					Vec2 interpP1 = (p1b - p1a) * t + p1a;
					Vec2 interpP2 = (p2b - p2a) * t + p2a;
					Vec2 interpP3 = (p3b - p3a) * t + p3a;

					// Then draw
					Svg::bezier3To(res, interpP1, interpP2, interpP3);
				}

				bool isHole = t < 0.5
					? path0->isHole
					: path1->isHole;
				Svg::closePath(res, true, isHole);
			}

			res->calculateApproximatePerimeter();
			res->calculateBBox();

			// Free temporary memory
			modifiedSrcObject->free();
			modifiedDstObject->free();
			g_memory_free(modifiedSrcObject);
			g_memory_free(modifiedDstObject);
			modifiedSrcObject = nullptr;
			modifiedDstObject = nullptr;

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
	}

	// ----------------- SvgObject functions -----------------
	// SvgObject internal functions
	static void fillWithPluto(plutovg_t* pluto, const AnimObject* parent, const SvgObject* obj);
	static void renderOutline2D(float t, const AnimObject* parent, const SvgObject* obj);

	void SvgObject::normalize()
	{
		// First find the min max of the entire curve
		Vec2 min = { FLT_MAX, FLT_MAX };
		Vec2 max = { FLT_MIN, FLT_MIN };
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
				case CurveType::None:
					break;
				}
			}
		}

		// Then map everything to a [0.0-1.0] range from there
		Vec2 hzOutputRange = Vec2{ 0.0f, 1.0f };
		// Maintain aspect ratio while normalizing
		Vec2 vtOutputRange = Vec2{ 0.0f, (max.y - min.y) / (max.x - min.x) };
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
				case CurveType::None:
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
		case CurveType::None:
			break;
		}

		return 0.0f;
	}

	Curve Curve::split(float _t0, float _t1) const
	{
		Curve res{};
		res.type = type;

		switch (type)
		{
		case CurveType::Bezier3:
		{
			const Vec2& p1 = as.bezier3.p1;
			const Vec2& p2 = as.bezier3.p2;
			const Vec2& p3 = as.bezier3.p3;

			// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
			float t0 = _t0;
			float t1 = _t1;
			float u0 = (1.0f - t0);
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

			res.p0 = q0;
			res.as.bezier3.p1 = q1;
			res.as.bezier3.p2 = q2;
			res.as.bezier3.p3 = q3;
		}
		break;
		case CurveType::Bezier2:
		{
			// Elevate the bezier2 to a bezier3
			res.type = CurveType::Bezier3;

			const Vec2& p1 = as.bezier2.p1;
			const Vec2& p2 = as.bezier2.p1;
			const Vec2& p3 = as.bezier2.p2;

			// Degree elevated quadratic bezier curve
			Vec2 pr0 = p0;
			Vec2 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
			Vec2 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
			Vec2 pr3 = p3;

			// Interpolate the curve
			// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-bézier-curve-by-reusing-a-basic-bézier-curve-function
			float t0 = _t0;
			float t1 = _t1;
			float u0 = (1.0f - t0);
			float u1 = (1.0f - t1);

			Vec2 q0 = ((u0 * u0 * u0) * pr0) +
				((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * pr1) +
				((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * pr2) +
				((t0 * t0 * t0) * pr3);
			Vec2 q1 = ((u0 * u0 * u1) * pr0) +
				((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * pr1) +
				((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * pr2) +
				((t0 * t0 * t1) * pr3);
			Vec2 q2 = ((u0 * u1 * u1) * pr0) +
				((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * pr1) +
				((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * pr2) +
				((t0 * t1 * t1) * pr3);
			Vec2 q3 = ((u1 * u1 * u1) * pr0) +
				((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * pr1) +
				((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * pr2) +
				((t1 * t1 * t1) * pr3);

			res.p0 = q0;
			res.as.bezier3.p1 = q1;
			res.as.bezier3.p2 = q2;
			res.as.bezier3.p3 = q3;
		}
		break;
		case CurveType::Line:
		{
			Vec2 dir = this->as.line.p1 - this->p0;
			res.p0 = (_t0 * dir) + this->p0;
			res.as.line.p1 = (_t1 * dir) + this->p0;
		}
		break;
		case CurveType::None:
			break;
		}

		return res;
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
		bbox.max.x = -FLT_MAX;
		bbox.max.y = -FLT_MAX;

		for (int pathi = 0; pathi < numPaths; pathi++)
		{
			if (paths[pathi].numCurves > 0)
			{
				for (int curvei = 0; curvei < paths[pathi].numCurves; curvei++)
				{
					const Curve& curve = paths[pathi].curves[curvei];

					switch (curve.type)
					{
					case CurveType::Bezier3:
					{
						BBox subBbox = CMath::bezier3BBox(curve.p0, curve.as.bezier3.p1, curve.as.bezier3.p2, curve.as.bezier3.p3);
						bbox.min = CMath::min(bbox.min, subBbox.min);
						bbox.max = CMath::max(bbox.max, subBbox.max);
					}
					break;
					case CurveType::Bezier2:
					{
						BBox subBbox = CMath::bezier2BBox(curve.p0, curve.as.bezier2.p1, curve.as.bezier2.p2);
						bbox.min = CMath::min(bbox.min, subBbox.min);
						bbox.max = CMath::max(bbox.max, subBbox.max);
					}
					break;
					case CurveType::Line:
					{
						BBox subBbox = CMath::bezier1BBox(curve.p0, curve.as.line.p1);
						bbox.min = CMath::min(bbox.min, subBbox.min);
						bbox.max = CMath::max(bbox.max, subBbox.max);
					}
					break;
					case CurveType::None:
						break;
					}
				}
			}
		}
	}

	void SvgObject::render(const AnimObject* parent, const Texture& texture, const Vec2& textureOffset) const
	{
		Vec2 bboxSize = (bbox.max - bbox.min) * parent->svgScale;

		// Setup pluto context to render SVG to
		plutovg_surface_t* surface = plutovg_surface_create((int)bboxSize.x, (int)bboxSize.y);
		plutovg_t* pluto = plutovg_create(surface);

		fillWithPluto(pluto, parent, this);

		unsigned char* pixels = plutovg_surface_get_data(surface);
		int surfaceWidth = plutovg_surface_get_width(surface);
		int surfaceHeight = plutovg_surface_get_height(surface);
		texture.uploadSubImage(
			(int)textureOffset.x,
			(int)(texture.height - textureOffset.y - surfaceHeight),
			surfaceWidth,
			surfaceHeight,
			pixels,
			surfaceWidth * surfaceHeight * sizeof(uint8) * 4,
			true
		);

		plutovg_surface_destroy(surface);
		plutovg_destroy(pluto);
	}

	void SvgObject::renderOutline(float t, const AnimObject* parent) const
	{
		renderOutline2D(t, parent, this);
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

	static void writeBuffer(uint8** buffer, size_t* capacity, size_t* numElements, const char* string, size_t stringLength = 0)
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
		tmpBuffer[0] = '\0';

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
				case CurveType::None:
					break;
				}
			}
		}

		// fillType         -> u8
		// fillColor        -> Vec4
		// pathLength       -> u64
		// path             -> u8[pathLength]
		memory.write<FillType>(&fillType);
		CMath::serialize(memory, fillColor);
		uint64 stringLength = (uint64)numElements;
		memory.write<uint64>(&stringLength);
		memory.writeDangerous(buffer, numElements);

		g_memory_free(buffer);
	}

	SvgObject* SvgObject::deserialize(RawMemory& memory, uint32 version)
	{
		SvgObject* res = (SvgObject*)g_memory_allocate(sizeof(SvgObject));

		// fillType         -> u8
		// fillColor        -> Vec4
		// pathLength       -> u64
		// path             -> u8[pathLength]
		memory.read<FillType>(&res->fillType);
		res->fillColor = CMath::deserializeVec4(memory);
		uint64 stringLength;
		memory.read<uint64>(&stringLength);
		uint8* string = (uint8*)g_memory_allocate(sizeof(uint8) * (stringLength + 1));
		memory.readDangerous(string, stringLength);
		string[stringLength] = '\0';

		if (stringLength > 0)
		{
			if (!SvgParser::parseSvgPath((const char*)string, stringLength, res))
			{
				g_logger_error("Error deserializing SVG. Bad path data: '%s'", string);
			}
		}
		else
		{
			*res = Svg::createDefault();
		}
		g_memory_free(string);

		return res;
	}

	void SvgGroup::normalize()
	{
		calculateBBox();

		Vec2 svgGroupSize = bbox.max - bbox.min;
		for (int i = 0; i < numObjects; i++)
		{
			SvgObject& obj = objects[i];
			Vec2& offset = objectOffsets[i];

			// Normalize the SVG object with respsect to the size of the group

			// Map everything to [0.0, 1.0] range except it needs to be scaled
			// relative to the size in the overall svg group
			Vec2 hzInputRange = Vec2{ obj.bbox.min.x, obj.bbox.max.x };
			Vec2 vtInputRange = Vec2{ obj.bbox.min.y, obj.bbox.max.y };
			float outputWidth = (obj.bbox.max.x - obj.bbox.min.x) / svgGroupSize.x;
			float outputHeight = (obj.bbox.max.y - obj.bbox.min.y) / svgGroupSize.y * (svgGroupSize.y / svgGroupSize.x);
			Vec2 hzOutputRange = Vec2{ 0.0f, outputWidth };
			Vec2 vtOutputRange = Vec2{ 0.0f, outputHeight };
			for (int pathi = 0; pathi < obj.numPaths; pathi++)
			{
				for (int curvei = 0; curvei < obj.paths[pathi].numCurves; curvei++)
				{
					Curve& curve = obj.paths[pathi].curves[curvei];
					curve.p0.x = CMath::mapRange(hzInputRange, hzOutputRange, curve.p0.x);
					curve.p0.y = CMath::mapRange(vtInputRange, vtOutputRange, curve.p0.y);

					switch (curve.type)
					{
					case CurveType::Bezier3:
					{
						curve.as.bezier3.p1.x = CMath::mapRange(hzInputRange, hzOutputRange, curve.as.bezier3.p1.x);
						curve.as.bezier3.p1.y = CMath::mapRange(vtInputRange, vtOutputRange, curve.as.bezier3.p1.y);

						curve.as.bezier3.p2.x = CMath::mapRange(hzInputRange, hzOutputRange, curve.as.bezier3.p2.x);
						curve.as.bezier3.p2.y = CMath::mapRange(vtInputRange, vtOutputRange, curve.as.bezier3.p2.y);

						curve.as.bezier3.p3.x = CMath::mapRange(hzInputRange, hzOutputRange, curve.as.bezier3.p3.x);
						curve.as.bezier3.p3.y = CMath::mapRange(vtInputRange, vtOutputRange, curve.as.bezier3.p3.y);
					}
					break;
					case CurveType::Bezier2:
					{
						curve.as.bezier2.p1.x = CMath::mapRange(hzInputRange, hzOutputRange, curve.as.bezier2.p1.x);
						curve.as.bezier2.p1.y = CMath::mapRange(vtInputRange, vtOutputRange, curve.as.bezier2.p1.y);

						curve.as.bezier2.p2.x = CMath::mapRange(hzInputRange, hzOutputRange, curve.as.bezier2.p2.x);
						curve.as.bezier2.p2.y = CMath::mapRange(vtInputRange, vtOutputRange, curve.as.bezier2.p2.y);
					}
					break;
					case CurveType::Line:
					{
						curve.as.line.p1.x = CMath::mapRange(hzInputRange, hzOutputRange, curve.as.line.p1.x);
						curve.as.line.p1.y = CMath::mapRange(vtInputRange, vtOutputRange, curve.as.line.p1.y);
					}
					break;
					case CurveType::None:
						break;
					}
				}
			}

			Vec2 originalBboxMin = obj.bbox.min;
			// Calculate the boundaries using the new ranges
			obj.calculateBBox();
			obj.calculateApproximatePerimeter();

			float outputGroupWidth = 1.0f;
			float outputGroupHeight = (svgGroupSize.y / svgGroupSize.x);

			// If no offset was provided, then just use the minimum
			// coordinates as the natural offset
			if (isnan(offset.x))
			{
				offset.x = originalBboxMin.x;
			}

			if (isnan(offset.y))
			{
				offset.y = originalBboxMin.y;
			}

			offset.x = CMath::mapRange(Vec2{ this->bbox.min.x, this->bbox.max.x }, Vec2{ 0.0f, outputGroupWidth }, offset.x);
			offset.y = CMath::mapRange(Vec2{ this->bbox.min.y, this->bbox.max.y }, Vec2{ 0.0f, outputGroupHeight }, offset.y);
			offset.y = outputGroupHeight - offset.y;
			Vec2 centerOffset = Vec2{
				(obj.bbox.max.x - obj.bbox.min.x) / 2.0f,
				-(obj.bbox.max.y - obj.bbox.min.y) / 2.0f
			};
			offset += centerOffset;
			// Center the whole object
			offset -= Vec2{ outputGroupWidth / 2.0f, outputGroupHeight / 2.0f };
		}
		calculateBBox();
	}

	void SvgGroup::calculateBBox()
	{
		bbox.min = Vec2{ NAN, NAN };
		bbox.max = Vec2{ NAN, NAN };

		for (int i = 0; i < numObjects; i++)
		{
			SvgObject& obj = objects[i];
			const Vec2& offset = objectOffsets[i];

			Vec2 absOffset = offset;
			if (isnan(absOffset.x))
			{
				absOffset.x = 0.0f;
			}
			if (isnan(absOffset.y))
			{
				absOffset.y = 0.0f;
			}

			obj.calculateBBox();
			bbox.min = CMath::min(obj.bbox.min + absOffset, bbox.min);
			bbox.max = CMath::max(obj.bbox.max + absOffset, bbox.max);
		}
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
			for (int i = 0; i < numObjects; i++)
			{
				objects[i].free();
			}
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
	}

	// ------------------- Svg Object Internal functions -------------------
	static void fillWithPluto(plutovg_t* pluto, const AnimObject* parent, const SvgObject* obj)
	{
		// Can't render SVG's with 0 paths
		if (obj->numPaths <= 0)
		{
			return;
		}

		// Instead of translating, we'll map every coordinate from the SVG min-max range to
		// the preferred coordinate range
		Vec2 scaledBboxMin = obj->bbox.min * parent->svgScale;
		Vec2 scaledBboxMax = obj->bbox.max * parent->svgScale;
		Vec2 inXRange = Vec2{ scaledBboxMin.x, scaledBboxMax.x };
		Vec2 inYRange = Vec2{ scaledBboxMin.y, scaledBboxMax.y };

		Vec2 bboxSize = scaledBboxMax - scaledBboxMin;
		Vec2 minCoord = Vec2{ 0, 0 };
		Vec2 maxCoord = minCoord + bboxSize;
		Vec2 outXRange = Vec2{ minCoord.x, maxCoord.x };
		Vec2 outYRange = Vec2{ minCoord.y, maxCoord.y };

		plutovg_new_path(pluto);

		for (int pathi = 0; pathi < obj->numPaths; pathi++)
		{
			if (obj->paths[pathi].numCurves > 0)
			{
				{
					Vec2 p0 = obj->paths[pathi].curves[0].p0;
					p0.x *= parent->svgScale;
					p0.y *= parent->svgScale;
					p0.x = CMath::mapRange(inXRange, outXRange, p0.x);
					p0.y = CMath::mapRange(inYRange, outYRange, p0.y);

					plutovg_move_to(pluto,
						p0.x,
						p0.y
					);
				}

				for (int curvei = 0; curvei < obj->paths[pathi].numCurves; curvei++)
				{
					const Curve& curve = obj->paths[pathi].curves[curvei];
					Vec2 p0 = curve.p0;

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

						plutovg_cubic_to(
							pluto,
							p1.x, p1.y,
							p2.x, p2.y,
							p3.x, p3.y
						);
					}
					break;
					case CurveType::Bezier2:
					{
						Vec2 p1 = curve.as.bezier2.p1;
						Vec2 p2 = curve.as.bezier2.p2;

						p1.x *= parent->svgScale;
						p1.y *= parent->svgScale;
						p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
						p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

						p2.x *= parent->svgScale;
						p2.y *= parent->svgScale;
						p2.x = CMath::mapRange(inXRange, outXRange, p2.x);
						p2.y = CMath::mapRange(inYRange, outYRange, p2.y);

						plutovg_quad_to(
							pluto,
							p1.x, p1.y,
							p2.x, p2.y
						);
					}
					break;
					case CurveType::Line:
					{
						Vec2 p1 = curve.as.line.p1;

						p1.x *= parent->svgScale;
						p1.y *= parent->svgScale;
						p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
						p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

						plutovg_line_to(
							pluto,
							p1.x, p1.y
						);
					}
					break;
					case CurveType::None:
						break;
					}
				}
			}
		}

		// Draw the SVG with full alpha since we apply alpha changes at the compositing level
		const glm::u8vec4& fillColor = parent->fillColor;
		// Render the SVG in white then color it when blitting the
		// texture to a quad
		plutovg_set_rgba(pluto, 1.0, 1.0, 1.0, 1.0);
		plutovg_close_path(pluto);
		if (obj->fillType == FillType::EvenOddFillType)
		{
			plutovg_set_fill_rule(pluto, plutovg_fill_rule_even_odd);
		}
		else if (obj->fillType == FillType::NonZeroFillType)
		{
			plutovg_set_fill_rule(pluto, plutovg_fill_rule_non_zero);
		}
		plutovg_fill_preserve(pluto);
	}

	static void renderOutline2D(float t, const AnimObject* parent, const SvgObject* obj)
	{
		constexpr float defaultStrokeWidth = 0.02f;

		// Start the fade in after 80% of the svg object is drawn
		float lengthToDraw = t * (float)obj->approximatePerimeter;
		Vec2 svgSize = obj->bbox.max - obj->bbox.min;

		Vec2 inXRange = Vec2{ obj->bbox.min.x, obj->bbox.max.x };
		Vec2 inYRange = Vec2{ obj->bbox.min.y, obj->bbox.max.y };
		Vec2 outXRange = Vec2{ -svgSize.x / 2.0f, svgSize.x / 2.0f };
		Vec2 outYRange = Vec2{ svgSize.y / 2.0f, -svgSize.y / 2.0f };

		if (lengthToDraw > 0 && obj->numPaths > 0)
		{
			float lengthDrawn = 0.0f;

			for (int pathi = 0; pathi < obj->numPaths; pathi++)
			{
				Path2DContext* context = nullptr;
				if (obj->paths[pathi].numCurves > 0)
				{
					Renderer::pushColor(parent->strokeColor);
					if (glm::epsilonEqual(parent->strokeWidth, 0.0f, 0.01f))
					{
						Renderer::pushStrokeWidth(defaultStrokeWidth);
					}
					else
					{
						Renderer::pushStrokeWidth(parent->strokeWidth);
					}

					{
						Vec2 p0 = obj->paths[pathi].curves[0].p0;

						p0.x = CMath::mapRange(inXRange, outXRange, p0.x);
						p0.y = CMath::mapRange(inYRange, outYRange, p0.y);

						context = Renderer::beginPath(Vec2{ p0.x, p0.y }, parent->globalTransform);
					}
					g_logger_assert(context != nullptr, "We have bigger problems.");

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

							p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
							p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

							p2.x = CMath::mapRange(inXRange, outXRange, p2.x);
							p2.y = CMath::mapRange(inYRange, outYRange, p2.y);

							p3.x = CMath::mapRange(inXRange, outXRange, p3.x);
							p3.y = CMath::mapRange(inYRange, outYRange, p3.y);

							Renderer::cubicTo(
								context,
								p1,
								p2,
								p3
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

							pr1.x = CMath::mapRange(inXRange, outXRange, pr1.x);
							pr1.y = CMath::mapRange(inYRange, outYRange, pr1.y);

							pr2.x = CMath::mapRange(inXRange, outXRange, pr2.x);
							pr2.y = CMath::mapRange(inYRange, outYRange, pr2.y);

							pr3.x = CMath::mapRange(inXRange, outXRange, pr3.x);
							pr3.y = CMath::mapRange(inYRange, outYRange, pr3.y);

							Renderer::cubicTo(
								context,
								pr1,
								pr2,
								pr3
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
								p1 = ((p1 - p0) * percentOfCurveToDraw) + p0;
							}

							p1.x = CMath::mapRange(inXRange, outXRange, p1.x);
							p1.y = CMath::mapRange(inYRange, outYRange, p1.y);

							Renderer::lineTo(
								context,
								p1
							);
						}
						break;
						case CurveType::None:
							break;
						}
					}

					Renderer::popStrokeWidth();
					Renderer::popColor();
				}

				if (lengthDrawn >= lengthToDraw && t < 1.0f)
				{
					Renderer::endPath(context, false);
					Renderer::free(context);
					break;
				}
				else
				{
					Renderer::endPath(context);
					Renderer::free(context);
				}
			}
		}
	}
}