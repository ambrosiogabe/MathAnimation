#include "animation/Shapes.h"
#include "animation/Animation.h"
#include "animation/Svg.h"

namespace MathAnim
{
	void Square::init(AnimObject* parent)
	{
		g_logger_assert(parent->svgObject == nullptr, "Square object initialized twice.");

		parent->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->svgObject = Svg::createDefault();

		float halfLength = sideLength / 2.0f;
		Svg::beginContour(parent->svgObject, { -halfLength, halfLength }, true);
		Svg::lineTo(parent->svgObject, { halfLength, halfLength });
		Svg::lineTo(parent->svgObject, { halfLength, -halfLength });
		Svg::lineTo(parent->svgObject, { -halfLength, -halfLength });
		Svg::lineTo(parent->svgObject, { -halfLength, halfLength });
		Svg::closeContour(parent->svgObject);

		parent->svgObject->calculateApproximatePerimeter();
	}
}