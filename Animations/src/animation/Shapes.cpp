#include "animation/Shapes.h"
#include "animation/Animation.h"
#include "svg/Svg.h"
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

		Svg::beginPath(parent->_svgObjectStart, { -sideLength / 2.0f, -sideLength / 2.0f });
		Svg::lineTo(parent->_svgObjectStart, { -sideLength / 2.0f, sideLength / 2.0f });
		Svg::lineTo(parent->_svgObjectStart, { sideLength / 2.0f, sideLength / 2.0f });
		Svg::lineTo(parent->_svgObjectStart, { sideLength / 2.0f, -sideLength / 2.0f });
		Svg::lineTo(parent->_svgObjectStart, { -sideLength / 2.0f, -sideLength / 2.0f });
		Svg::closePath(parent->_svgObjectStart);

		parent->_svgObjectStart->calculateApproximatePerimeter();
		parent->_svgObjectStart->calculateBBox();
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
		Vec2 translation = Vec2{ radius, radius };
		Svg::beginPath(parent->_svgObjectStart, Vec2{ -radius, 0.0f } + translation);

		Svg::bezier3To(parent->_svgObjectStart,
			Vec2{ -radius, (float)equidistantControls } + translation,
			Vec2{ -(float)equidistantControls, radius } + translation,
			Vec2{ 0.0f, radius } + translation);
		Svg::bezier3To(parent->_svgObjectStart,
			Vec2{ (float)equidistantControls, radius } + translation,
			Vec2{ radius, (float)equidistantControls } + translation,
			Vec2{ radius, 0.0f } + translation);
		Svg::bezier3To(parent->_svgObjectStart,
			Vec2{ radius, -(float)equidistantControls } + translation,
			Vec2{ (float)equidistantControls, -radius } + translation,
			Vec2{ 0.0f, -radius } + translation);
		Svg::bezier3To(parent->_svgObjectStart,
			Vec2{ -(float)equidistantControls, -radius } + translation,
			Vec2{ -radius, -(float)equidistantControls } + translation,
			Vec2{ -radius, 0.0f } + translation);

		Svg::closePath(parent->_svgObjectStart);

		parent->_svgObjectStart->calculateApproximatePerimeter();
		parent->_svgObjectStart->calculateBBox();
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

	void Arrow::init(AnimObject* parent)
	{
		g_logger_assert(parent->_svgObjectStart == nullptr && parent->svgObject == nullptr, "Arrow object initialized twice.");

		parent->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->_svgObjectStart = Svg::createDefault();
		parent->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->svgObject = Svg::createDefault();

		const float halfLength = stemLength / 2.0f;
		const float halfWidth = stemWidth / 2.0f;
		const float tipOffset = tipWidth - stemWidth;
		const float halfTipOffset = tipOffset / 2.0f;
		const float halfTipWidth = tipWidth / 2.0f;

		Svg::beginPath(parent->_svgObjectStart, Vec2{ -halfLength, -halfWidth });
		Svg::lineTo(parent->_svgObjectStart, Vec2{ 0.0f, stemWidth }, false);
		Svg::lineTo(parent->_svgObjectStart, Vec2{ stemLength, 0.0f }, false);
		Svg::lineTo(parent->_svgObjectStart, Vec2{ 0.0f, halfTipOffset }, false);
		Svg::lineTo(parent->_svgObjectStart, Vec2{ tipLength, -halfTipWidth }, false);
		Svg::lineTo(parent->_svgObjectStart, Vec2{ -tipLength, -halfTipWidth }, false);
		Svg::lineTo(parent->_svgObjectStart, Vec2{ 0.0f, halfTipOffset }, false);
		Svg::lineTo(parent->_svgObjectStart, Vec2{ -stemLength, 0.0f }, false);
		Svg::closePath(parent->_svgObjectStart);

		parent->_svgObjectStart->calculateApproximatePerimeter();
		parent->_svgObjectStart->calculateBBox();
	}

	void Arrow::serialize(RawMemory& memory) const
	{
		// stemWidth    -> f32
		// stemLength   -> f32
		// tipWidth	    -> f32
		// tipLength    -> f32
		memory.write<float>(&stemWidth);
		memory.write<float>(&stemLength);
		memory.write<float>(&tipWidth);
		memory.write<float>(&tipLength);
	}

	Arrow Arrow::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			// stemWidth    -> f32
			// stemLength   -> f32
			// tipWidth	    -> f32
			// tipLength    -> f32
			Arrow res = {};
			memory.read<float>(&res.stemWidth);
			memory.read<float>(&res.stemLength);
			memory.read<float>(&res.tipWidth);
			memory.read<float>(&res.tipLength);

			return res;
		}

		// Return dummy
		return Arrow{};
	}

	void Cube::init(AnimObject* parent)
	{
		g_logger_assert(parent->_svgObjectStart == nullptr && parent->svgObject == nullptr, "Square object initialized twice.");

		float halfLength = sideLength / 2.0f;

		Vec3 offsets[6] = {
			Vec3{0, 0, -halfLength}, // Back
			Vec3{0, 0, halfLength},  // Front
			Vec3{0, -halfLength, 0}, // Bottom
			Vec3{0, halfLength, 0},  // Top
			Vec3{-halfLength, 0, 0}, // Left
			Vec3{halfLength, 0, 0}   // Right
		};
		Vec3 rotations[6] = {
			Vec3{90, 0, 0},  // Back
			Vec3{90, 0, 0},  // Front
			Vec3{0, 0, 0},   // Bottom
			Vec3{0, 0, 0},   // Top
			Vec3{90, 90, 0}, // Left
			Vec3{90, 90, 0}  // Right
		};

		//for (int i = 0; i < 6; i++)
		//{
		//	AnimObject cubeFace = AnimObject::createDefaultFromParent(AnimObjectTypeV1::SvgObject, parent);
		//	cubeFace._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		//	*cubeFace._svgObjectStart = Svg::createDefault();
		//	cubeFace.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		//	*cubeFace.svgObject = Svg::createDefault();
		//	Svg::beginContour(cubeFace._svgObjectStart, Vec2{ -halfLength, -halfLength });
		//	Svg::lineTo(cubeFace._svgObjectStart, Vec2{ -halfLength, halfLength });
		//	Svg::lineTo(cubeFace._svgObjectStart, Vec2{ halfLength, halfLength });
		//	Svg::lineTo(cubeFace._svgObjectStart, Vec2{ halfLength, -halfLength });
		//	Svg::lineTo(cubeFace._svgObjectStart, Vec2{ -halfLength, -halfLength });
		//	Svg::closeContour(cubeFace._svgObjectStart);

		//	cubeFace._positionStart = offsets[i];
		//	cubeFace._rotationStart = rotations[i];
		//	cubeFace._scaleStart = Vec3{ 1.0f, 1.0f, 1.0f };
		//	cubeFace.is3D = true;
		//	parent->children.push_back(cubeFace);
		//}
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
		Circle res;
		memory.read<float>(&res.radius);
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