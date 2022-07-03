#include "animation/Shapes.h"
#include "animation/Animation.h"
#include "animation/Svg.h"
#include "utils/CMath.h"

namespace MathAnim
{
	// ------------------ Internal Functions ------------------
	static Square deserializeSquareV1(RawMemory& memory);
	static Circle deserializeCircleV1(RawMemory& memory);
	static Cube deserializeCubeV1(RawMemory& memory);

	void Square::init(AnimObject* parent)
	{
		g_logger_assert(parent->_svgObjectStart == nullptr && parent->svgObject == nullptr, "Square object initialized twice.");

		parent->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->_svgObjectStart = Svg::createDefault();
		parent->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->svgObject = Svg::createDefault();

		float halfLength = sideLength / 2.0f;

		Svg::beginContour(parent->_svgObjectStart, { -halfLength, halfLength, 0.0f }, true);
		Svg::lineTo(parent->_svgObjectStart, { halfLength, halfLength, 0.0f });
		Svg::lineTo(parent->_svgObjectStart, { halfLength, -halfLength, 0.0f });
		Svg::lineTo(parent->_svgObjectStart, { -halfLength, -halfLength, 0.0f });
		Svg::lineTo(parent->_svgObjectStart, { -halfLength, halfLength, 0.0f });
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
		parent->_svgObjectStart->is3D = is3D;
		parent->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->svgObject = Svg::createDefault();
		parent->svgObject->is3D = is3D;

		// See here for how to construct circle with beziers 
		// https://stackoverflow.com/questions/1734745/how-to-create-circle-with-bézier-curves
		double equidistantControls = (4.0 / 3.0) * glm::tan(glm::pi<double>() / 8.0) * (double)radius;
		bool clockwiseFill = true;
		Svg::beginContour(parent->_svgObjectStart, { -radius, 0.0f, 0.0f }, clockwiseFill, is3D);

		Svg::bezier3To(parent->_svgObjectStart,
			Vec3{ -radius, (float)equidistantControls, 0.0f },
			Vec3{ -(float)equidistantControls, radius, 0.0f },
			Vec3{ 0.0f, radius, 0.0f });
		Svg::bezier3To(parent->_svgObjectStart,
			Vec3{ (float)equidistantControls, radius, 0.0f },
			Vec3{ radius, (float)equidistantControls, 0.0f },
			Vec3{ radius, 0.0f, 0.0f});
		Svg::bezier3To(parent->_svgObjectStart,
			Vec3{ radius, -(float)equidistantControls, 0.0f },
			Vec3{ (float)equidistantControls, -radius, 0.0f },
			Vec3{ 0.0f, -radius, 0.0f});
		Svg::bezier3To(parent->_svgObjectStart,
			Vec3{ -(float)equidistantControls, -radius, 0.0f },
			Vec3{ -radius, -(float)equidistantControls, 0.0f },
			Vec3{ -radius, 0.0f, 0.0f});

		Svg::closeContour(parent->_svgObjectStart);

		parent->_svgObjectStart->calculateApproximatePerimeter();
	}

	void Circle::serialize(RawMemory& memory) const
	{
		// radius   -> float
		// is3D     -> uint8
		uint8 is3DU8 = is3D ? 1 : 0;
		memory.write<float>(&radius);
		memory.write<uint8>(&is3DU8);
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

	void Cube::init(AnimObject* parent)
	{
		g_logger_assert(parent->_svgObjectStart == nullptr && parent->svgObject == nullptr, "Square object initialized twice.");

		parent->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->_svgObjectStart = Svg::createDefault();
		parent->_svgObjectStart->is3D = true;
		parent->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->svgObject = Svg::createDefault();
		parent->svgObject->is3D = true;

		float halfLength = sideLength / 2.0f;

		bool is3D = true;
		bool clockwiseFill = true;
		Svg::beginContour(parent->_svgObjectStart, Vec3{ -halfLength, -halfLength, -halfLength }, clockwiseFill, is3D);
		Svg::lineTo(parent->_svgObjectStart, Vec3{ -halfLength, halfLength, -halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ halfLength, halfLength, -halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ halfLength, -halfLength, -halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ -halfLength, -halfLength, -halfLength });
		Svg::closeContour(parent->_svgObjectStart);

		Svg::beginContour(parent->_svgObjectStart, Vec3{ halfLength, -halfLength, -halfLength }, clockwiseFill, is3D);
		Svg::lineTo(parent->_svgObjectStart, Vec3{ halfLength, halfLength, -halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ halfLength, halfLength, halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ halfLength, -halfLength, halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ halfLength, -halfLength, -halfLength });
		Svg::closeContour(parent->_svgObjectStart);

		Svg::beginContour(parent->_svgObjectStart, Vec3{ halfLength, -halfLength, halfLength }, clockwiseFill, is3D);
		Svg::lineTo(parent->_svgObjectStart, Vec3{ halfLength, halfLength, halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ -halfLength, halfLength, halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ -halfLength, -halfLength, halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ halfLength, -halfLength, halfLength });
		Svg::closeContour(parent->_svgObjectStart);

		Svg::beginContour(parent->_svgObjectStart, Vec3{ -halfLength, -halfLength, halfLength }, clockwiseFill, is3D);
		Svg::lineTo(parent->_svgObjectStart, Vec3{ -halfLength, halfLength, halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ -halfLength, halfLength, -halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ -halfLength, -halfLength, -halfLength });
		Svg::lineTo(parent->_svgObjectStart, Vec3{ -halfLength, -halfLength, halfLength });
		Svg::closeContour(parent->_svgObjectStart);

		parent->_svgObjectStart->calculateApproximatePerimeter();
	}

	void Cube::serialize(RawMemory& memory) const
	{
		// sideLength       -> float
		memory.write<float>(&sideLength);
	}

	Cube Cube::deserialize(RawMemory& memory, uint32 version)
	{
		switch (version)
		{
		case 1:
			return deserializeCubeV1(memory);
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
		// is3D     -> uint8
		Circle res;
		memory.read<float>(&res.radius);
		uint8 is3DU8;
		memory.read<uint8>(&is3DU8);
		res.is3D = is3DU8 == 1;
		return res;
	}

	static Cube deserializeCubeV1(RawMemory& memory)
	{
		// sideLength       -> float
		Cube res;
		memory.read<float>(&res.sideLength);
		return res;
	}
}