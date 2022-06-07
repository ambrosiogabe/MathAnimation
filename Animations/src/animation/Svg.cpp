#include "animation/Svg.h"
#include "animation/Animation.h"

#include <nanovg.h>

namespace MathAnim
{
	namespace Svg
	{
		// ----------------- Private Variables -----------------
		constexpr int initialMaxCapacity = 5;

		// ----------------- Internal functions -----------------
		static void checkResize(Contour& contour);

		SvgObject createDefault()
		{
			SvgObject res;
			res.approximatePerimeter = 0.0f;
			// Dummy allocation to allow memory tracking when reallocing 
			// TODO: Fix the dang memory allocation library so I don't have to do this!
			res.contours = (Contour*)g_memory_allocate(sizeof(Contour));
			res.numContours = 0;
			return res;
		}

		void beginContour(SvgObject* object, const Vec2& firstPoint, bool clockwiseFill)
		{
			object->numContours++;
			object->contours = (Contour*)g_memory_realloc(object->contours, sizeof(Contour) * object->numContours);
			g_logger_assert(object->contours != nullptr, "Ran out of RAM.");

			object->contours[object->numContours - 1].clockwiseFill = clockwiseFill;
			object->contours[object->numContours - 1].maxCapacity = initialMaxCapacity;
			object->contours[object->numContours - 1].curves = (Curve*)g_memory_allocate(sizeof(Curve) * initialMaxCapacity);
			object->contours[object->numContours - 1].numCurves = 0;

			object->contours[object->numContours - 1].curves[0].p0 = firstPoint;
		}

		void closeContour(SvgObject* object)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot close contour when no contour exists.");
			g_logger_assert(object->contours[object->numContours - 1].numCurves > 0, "contour->numCurves == 0. Cannot close contour with 0 vertices. There must be at least one vertex to close a contour.");

			// NOP
			// We'll let nanovg handle closing of contours
			// Fix all the p0's in the curves
			Vec2 lastP0 = object->contours[object->numContours - 1].curves[0].p0;
			for (int curvei = 0; curvei < object->contours[object->numContours - 1].numCurves; curvei++)
			{
				Curve& curve = object->contours[object->numContours - 1].curves[curvei];
				curve.p0 = lastP0;
				switch (curve.type)
				{
				case CurveType::Bezier3:
					lastP0 = curve.as.bezier3.p3;
					break;
				case CurveType::Bezier2:
					lastP0 = curve.as.bezier2.p2;
					break;
				case CurveType::Line:
					lastP0 = curve.as.line.p1;
					break;
				default:
					g_logger_error("Unknown curve type %d", (int)curve.type);
					break;
				}
			}
		}

		void lineTo(SvgObject* object, const Vec2& point)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot create a lineTo when no contour exists.");
			Contour& contour = object->contours[object->numContours - 1];
			contour.numCurves++;
			checkResize(contour);

			contour.curves[contour.numCurves - 1].as.line.p1 = point;
			contour.curves[contour.numCurves - 1].type = CurveType::Line;
		}

		void bezier2To(SvgObject* object, const Vec2& control, const Vec2& dest)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot create a bezier2To when no contour exists.");
			Contour& contour = object->contours[object->numContours - 1];
			contour.numCurves++;
			checkResize(contour);

			contour.curves[contour.numCurves - 1].as.bezier2.p1 = control;
			contour.curves[contour.numCurves - 1].as.bezier2.p2 = dest;
			contour.curves[contour.numCurves - 1].type = CurveType::Bezier2;
		}

		void bezier3To(SvgObject* object, const Vec2& control0, const Vec2& control1, const Vec2& dest)
		{
			g_logger_assert(object->numContours > 0, "object->numContours == 0. Cannot create a bezier3To when no contour exists.");
			Contour& contour = object->contours[object->numContours - 1];
			contour.numCurves++;
			checkResize(contour);

			contour.curves[contour.numCurves - 1].as.bezier3.p1 = control0;
			contour.curves[contour.numCurves - 1].as.bezier3.p2 = control1;
			contour.curves[contour.numCurves - 1].as.bezier3.p3 = dest;
			contour.curves[contour.numCurves - 1].type = CurveType::Bezier3;
		}

		// ----------------- Internal functions -----------------
		static void checkResize(Contour& contour)
		{
			if (contour.numCurves > contour.maxCapacity)
			{
				contour.numCurves *= 2;
				contour.curves = (Curve*)g_memory_realloc(contour.curves, sizeof(Curve) * contour.numCurves);
				g_logger_assert(contour.curves != nullptr, "Ran out of RAM.");
			}
		}
	}

	// ----------------- SvgObject functions -----------------
	void SvgObject::calculateApproximatePerimeter()
	{
		approximatePerimeter = 0.0f;

		for (int contouri = 0; contouri < this->numContours; contouri++)
		{
			for (int curvei = 0; curvei < this->contours[contouri].numCurves; curvei++)
			{
				const Curve& curve = contours[contouri].curves[curvei];
				glm::vec2 p0 = { curve.p0.x, curve.p0.y };

				switch (curve.type)
				{
				case CurveType::Bezier3:
				{
					glm::vec2 p1 = { curve.as.bezier3.p1.x, curve.as.bezier3.p1.y };
					glm::vec2 p2 = { curve.as.bezier3.p2.x, curve.as.bezier3.p2.y };
					glm::vec2 p3 = { curve.as.bezier3.p3.x, curve.as.bezier3.p3.y };
					float chordLength = glm::length(p3 - p0);
					float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p3 - p2);
					float approxLength = (chordLength + controlNetLength) / 2.0f;
					approximatePerimeter += approxLength;
				}
				break;
				case CurveType::Bezier2:
				{
					glm::vec2 p1 = { curve.as.bezier2.p1.x, curve.as.bezier2.p1.y };
					glm::vec2 p2 = { curve.as.bezier2.p2.x, curve.as.bezier2.p2.y };
					float chordLength = glm::length(p2 - p0);
					float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1);
					float approxLength = (chordLength + controlNetLength) / 2.0f;
					approximatePerimeter += approxLength;
				}
				break;
				case CurveType::Line:
				{
					glm::vec2 p1 = { curve.as.line.p1.x, curve.as.line.p1.y };
					approximatePerimeter += glm::length(p1 - p0);
				}
				break;
				}
			}
		}
	}

	void SvgObject::render(NVGcontext* vg, const AnimObject* parent) const
	{
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		for (int contouri = 0; contouri < numContours; contouri++)
		{
			if (contours[contouri].numCurves > 0)
			{
				nvgBeginPath(vg);
				nvgStrokeWidth(vg, 5.0f);
				nvgPathWinding(vg, contours[contouri].clockwiseFill ? NVG_CW : NVG_CCW);

				nvgMoveTo(vg, 
					contours[contouri].curves[0].p0.x + parent->position.x, 
					contours[contouri].curves[0].p0.y + parent->position.y
				);

				for (int curvei = 0; curvei < contours[contouri].numCurves; curvei++)
				{
					const Curve& curve = contours[contouri].curves[curvei];
					glm::vec4 p0 = glm::vec4(
						curve.p0.x + parent->position.x,
						curve.p0.y + parent->position.y,
						0.0f,
						1.0f
					);

					switch (curve.type)
					{
					case CurveType::Bezier3:
					{
						glm::vec4& p1 = glm::vec4{ curve.as.bezier3.p1.x + parent->position.x, curve.as.bezier3.p1.y + parent->position.y, 0.0f, 1.0f };
						glm::vec4& p2 = glm::vec4{ curve.as.bezier3.p2.x + parent->position.x, curve.as.bezier3.p2.y + parent->position.y, 0.0f, 1.0f };
						glm::vec4& p3 = glm::vec4{ curve.as.bezier3.p3.x + parent->position.x, curve.as.bezier3.p3.y + parent->position.y, 0.0f, 1.0f };

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
						glm::vec4& p1 = glm::vec4{ curve.as.bezier2.p1.x + parent->position.x, curve.as.bezier2.p1.y + parent->position.y, 0.0f, 1.0f };
						glm::vec4& p2 = glm::vec4{ curve.as.bezier2.p1.x + parent->position.x, curve.as.bezier2.p1.y + parent->position.y, 0.0f, 1.0f };
						glm::vec4& p3 = glm::vec4{ curve.as.bezier2.p2.x + parent->position.x, curve.as.bezier2.p2.y + parent->position.y, 0.0f, 1.0f };

						// Degree elevated quadratic bezier curve
						glm::vec4 pr0 = p0;
						glm::vec4 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
						glm::vec4 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
						glm::vec4 pr3 = p2;

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
						glm::vec4 p1 = glm::vec4(
							curve.as.line.p1.x + parent->position.x,
							curve.as.line.p1.y + parent->position.y,
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
			}

			nvgClosePath(vg);
		}

		nvgFill(vg);
	}

	void SvgObject::renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent) const
	{
		// Start the fade in after 80% of the svg object is drawn
		constexpr float fadeInStart = 0.8f;
		const glm::vec2 position = { parent->position.x, parent->position.y };
		float lengthToDraw = t * (float)approximatePerimeter;
		float amountToFadeIn = ((t - fadeInStart) / (1.0f - fadeInStart));
		float percentToFadeIn = glm::max(glm::min(amountToFadeIn, 1.0f), 0.0f);

		if (lengthToDraw > 0)
		{
			float lengthDrawn = 0.0f;
			for (int contouri = 0; contouri < numContours; contouri++)
			{
				if (contours[contouri].numCurves > 0)
				{
					nvgBeginPath(vg);
					// Fade the stroke out as the font fades in
					nvgStrokeColor(vg, nvgRGBA(255, 255, 255, (unsigned char)(255.0f * (1.0f - percentToFadeIn))));
					nvgStrokeWidth(vg, 5.0f);

					nvgMoveTo(vg,
						contours[contouri].curves[0].p0.x + parent->position.x,
						contours[contouri].curves[0].p0.y + parent->position.y
					);

					for (int curvei = 0; curvei < contours[contouri].numCurves; curvei++)
					{
						float lengthLeft = lengthToDraw - lengthDrawn;
						if (lengthLeft < 0.0f)
						{
							break;
						}

						const Curve& curve = contours[contouri].curves[curvei];
						glm::vec4 p0 = glm::vec4(
							curve.p0.x + position.x,
							curve.p0.y + position.y,
							0.0f,
							1.0f
						);

						switch (curve.type)
						{
						case CurveType::Bezier3:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier3.p1.x + position.x, curve.as.bezier3.p1.y + position.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier3.p2.x + position.x, curve.as.bezier3.p2.y + position.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier3.p3.x + position.x, curve.as.bezier3.p3.y + position.y, 0.0f, 1.0f };

							float chordLength = glm::length(p3 - p0);
							float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p3 - p2);
							float approxLength = (chordLength + controlNetLength) / 2.0f;
							lengthDrawn += approxLength;

							if (lengthLeft < approxLength)
							{
								// Interpolate the curve
								float percentOfCurveToDraw = lengthLeft / approxLength;

								p1 = (p1 - p0) * percentOfCurveToDraw + p0;
								p2 = (p2 - p1) * percentOfCurveToDraw + p1;
								p3 = (p3 - p2) * percentOfCurveToDraw + p2;
							}

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
							glm::vec4& p1 = glm::vec4{ curve.as.bezier2.p1.x + position.x, curve.as.bezier2.p1.y + position.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier2.p1.x + position.x, curve.as.bezier2.p1.y + position.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier2.p2.x + position.x, curve.as.bezier2.p2.y + position.y, 0.0f, 1.0f };

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
								// Interpolate the curve
								float percentOfCurveToDraw = lengthLeft / approxLength;

								pr1 = (pr1 - pr0) * percentOfCurveToDraw + pr0;
								pr2 = (pr2 - pr1) * percentOfCurveToDraw + pr1;
								pr3 = (pr3 - pr2) * percentOfCurveToDraw + pr2;
							}

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
							glm::vec4 p1 = glm::vec4(
								curve.as.line.p1.x + position.x,
								curve.as.line.p1.y + position.y,
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

							nvgLineTo(vg, p1.x, p1.y);
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
			for (int contouri = 0; contouri < numContours; contouri++)
			{
				if (contours[contouri].numCurves > 0)
				{
					// TODO: De-deuplicate this by just calling render
					nvgFillColor(vg, nvgRGBA(255, 255, 255, (unsigned char)(255.0f * percentToFadeIn)));
					nvgBeginPath(vg);
					nvgStrokeWidth(vg, 5.0f);
					nvgPathWinding(vg, contours[contouri].clockwiseFill ? NVG_CW : NVG_CCW);

					nvgMoveTo(vg,
						contours[contouri].curves[0].p0.x + parent->position.x,
						contours[contouri].curves[0].p0.y + parent->position.y
					);

					for (int curvei = 0; curvei < contours[contouri].numCurves; curvei++)
					{
						const Curve& curve = contours[contouri].curves[curvei];
						glm::vec4 p0 = glm::vec4(
							curve.p0.x + position.x,
							curve.p0.y + position.y,
							0.0f,
							1.0f
						);

						switch (curve.type)
						{
						case CurveType::Bezier3:
						{
							glm::vec4& p1 = glm::vec4{ curve.as.bezier3.p1.x + position.x, curve.as.bezier3.p1.y + position.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier3.p2.x + position.x, curve.as.bezier3.p2.y + position.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier3.p3.x + position.x, curve.as.bezier3.p3.y + position.y, 0.0f, 1.0f };

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
							glm::vec4& p1 = glm::vec4{ curve.as.bezier2.p1.x + position.x, curve.as.bezier2.p1.y + position.y, 0.0f, 1.0f };
							glm::vec4& p2 = glm::vec4{ curve.as.bezier2.p1.x + position.x, curve.as.bezier2.p1.y + position.y, 0.0f, 1.0f };
							glm::vec4& p3 = glm::vec4{ curve.as.bezier2.p2.x + position.x, curve.as.bezier2.p2.y + position.y, 0.0f, 1.0f };

							// Degree elevated quadratic bezier curve
							glm::vec4 pr0 = p0;
							glm::vec4 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
							glm::vec4 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
							glm::vec4 pr3 = p2;

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
							glm::vec4 p1 = glm::vec4(
								curve.as.line.p1.x + position.x,
								curve.as.line.p1.y + position.y,
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
			contours[contouri].clockwiseFill = false;
		}

		if (contours)
		{
			g_memory_free(contours);
		}

		numContours = 0;
		approximatePerimeter = 0.0f;
	}
}