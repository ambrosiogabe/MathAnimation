#include "animation/Shapes.h"
#include "animation/Animation.h"
#include "animation/Svg.h"

namespace MathAnim
{
	// ------------------ Internal Functions ------------------
	static Square deserializeSquareV1(RawMemory& memory);
	static Circle deserializeCircleV1(RawMemory& memory);

	void Square::init(AnimObject* parent)
	{
		g_logger_assert(parent->_svgObjectStart == nullptr && parent->svgObject == nullptr, "Square object initialized twice.");

		parent->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->_svgObjectStart = Svg::createDefault();
		parent->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->svgObject = Svg::createDefault();

		float halfLength = sideLength / 2.0f;
		Svg::beginContour(parent->_svgObjectStart, { -halfLength, halfLength }, true);
		Svg::lineTo(parent->_svgObjectStart, { halfLength, halfLength });
		Svg::lineTo(parent->_svgObjectStart, { halfLength, -halfLength });
		Svg::lineTo(parent->_svgObjectStart, { -halfLength, -halfLength });
		Svg::lineTo(parent->_svgObjectStart, { -halfLength, halfLength });
		Svg::closeContour(parent->_svgObjectStart);

		parent->_svgObjectStart->calculateApproximatePerimeter();
	}

	void Square::serialize(RawMemory& memory) const
	{
		// sideLength       -> float
		memory.write<float>(&sideLength);
	}

	Square Square::deserialize(RawMemory& memory, uint32 version)
	{
		switch (version)
		{
		case 1:
			return deserializeSquareV1(memory);
			break;
		default:
			g_logger_error("Tried to deserialize unknown version of square %d. It looks like you have corrupted scene data.");
			break;
		}

		return {};
	}

	void Circle::init(AnimObject* parent)
	{
		g_logger_assert(parent->_svgObjectStart == nullptr && parent->svgObject == nullptr, "Circle object initialized twice.");

		parent->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->_svgObjectStart = Svg::createDefault();
		parent->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->svgObject = Svg::createDefault();

		// See here for how to construct circle with beziers 
		// https://stackoverflow.com/questions/1734745/how-to-create-circle-with-bézier-curves
		double equidistantControls = (4.0 / 3.0) * glm::tan(glm::pi<double>() / 8.0) * (double)radius;
		Svg::beginContour(parent->_svgObjectStart, { -radius, 0.0f }, true);
		Svg::bezier3To(parent->_svgObjectStart,
			{ -radius, (float)equidistantControls }, 
			{ -(float)equidistantControls, radius }, 
			{ 0.0f, radius });
		Svg::bezier3To(parent->_svgObjectStart,
			{ (float)equidistantControls, radius },
			{ radius, (float)equidistantControls },
			{ radius, 0.0f });
		Svg::bezier3To(parent->_svgObjectStart,
			{ radius, -(float)equidistantControls },
			{ (float)equidistantControls, -radius },
			{ 0.0f, -radius });
		Svg::bezier3To(parent->_svgObjectStart,
			{ -(float)equidistantControls, -radius },
			{ -radius, -(float)equidistantControls },
			{ -radius, 0.0f });
		Svg::closeContour(parent->_svgObjectStart);

		parent->_svgObjectStart->calculateApproximatePerimeter();
	}

	void Circle::serialize(RawMemory& memory) const
	{
		// radius   -> float
		memory.write<float>(&radius);
	}

	Circle Circle::deserialize(RawMemory& memory, uint32 version)
	{
		switch (version)
		{
		case 1:
			return deserializeCircleV1(memory);
			break;
		default:
			g_logger_error("Tried to deserialize unknown version of square %d. It looks like you have corrupted scene data.");
			break;
		}

		return {};
	}

	// ------------------ Internal Functions ------------------
	static Square deserializeSquareV1(RawMemory& memory)
	{
		// sideLength       -> float
		Square res;
		memory.read<float>(&res.sideLength);
		return res;
	}

	static Circle deserializeCircleV1(RawMemory& memory)
	{
		// radius   -> float
		Circle res;
		memory.read<float>(&res.radius);
		return res;
	}
}