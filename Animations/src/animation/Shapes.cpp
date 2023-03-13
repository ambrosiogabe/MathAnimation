#include "animation/Shapes.h"
#include "animation/Animation.h"
#include "core/Serialization.hpp"
#include "svg/Svg.h"
#include "math/CMath.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	// ------------------ Internal Functions ------------------
	static Square deserializeSquareV2(const nlohmann::json& j);
	static Circle deserializeCircleV2(const nlohmann::json& j);
	static Cube deserializeCubeV2(const nlohmann::json& j);

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

		parent->_svgObjectStart->finalize();
		parent->retargetSvgScale();
	}

	void Square::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_NON_NULL_PROP(memory, this, sideLength);
	}

	Square Square::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
			return deserializeSquareV2(j);
			break;
		default:
			g_logger_error("Square serialized with unknown version '%d'", version);
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

		parent->_svgObjectStart->finalize();
		parent->retargetSvgScale();
	}

	void Circle::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_NON_NULL_PROP(memory, this, radius);
	}

	Circle Circle::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
			return deserializeCircleV2(j);
			break;
		default:
			break;
		}

		g_logger_warning("Circle serialized with unknown version '%d'.", version);
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

		parent->_svgObjectStart->finalize();
		parent->retargetSvgScale();
	}

	void Arrow::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_NON_NULL_PROP(memory, this, stemWidth);
		SERIALIZE_NON_NULL_PROP(memory, this, stemLength);
		SERIALIZE_NON_NULL_PROP(memory, this, tipWidth);
		SERIALIZE_NON_NULL_PROP(memory, this, tipLength);
	}

	Arrow Arrow::deserialize(const nlohmann::json& j, uint32 version)
	{
		if (version == 2)
		{
			Arrow res = {};
			DESERIALIZE_PROP(&res, stemWidth, j, 0.0f);
			DESERIALIZE_PROP(&res, stemLength, j, 0.0f);
			DESERIALIZE_PROP(&res, tipLength, j, 0.0f);
			DESERIALIZE_PROP(&res, tipWidth, j, 0.0f);

			return res;
		}

		g_logger_warning("Arrow serialized with unknown version '%d'.", version);
		return {};
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

	void Cube::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_NON_NULL_PROP(memory, this, sideLength);
	}

	Cube Cube::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
			return deserializeCubeV2(j);
			break;
		default:
			break;
		}

		g_logger_warning("Cube serialized with unknown version '%d'.", version);
		return {};
	}

	// ------------------ Internal Functions ------------------
	static Square deserializeSquareV2(const nlohmann::json& j)
	{
		Square res = {};
		DESERIALIZE_PROP(&res, sideLength, j, 0.0f);
		return res;
	}

	static Circle deserializeCircleV2(const nlohmann::json& j)
	{
		Circle res = {};
		DESERIALIZE_PROP(&res, radius, j, 0.0f);
		return res;
	}

	static Cube deserializeCubeV2(const nlohmann::json& j)
	{
		Cube res = {};
		DESERIALIZE_PROP(&res, sideLength, j, 0.0f);
		return res;
	}
}