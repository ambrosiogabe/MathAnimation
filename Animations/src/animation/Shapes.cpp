#include "animation/Shapes.h"
#include "animation/Animation.h"
#include "core/Serialization.hpp"
#include "svg/Svg.h"
#include "math/CMath.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	void Square::init(AnimObject* self)
	{
		g_logger_assert(self->_svgObjectStart == nullptr && self->svgObject == nullptr, "Square object initialized twice.");

		self->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*self->_svgObjectStart = Svg::createDefault();
		self->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*self->svgObject = Svg::createDefault();

		Svg::beginPath(self->_svgObjectStart, { -sideLength / 2.0f, -sideLength / 2.0f });
		Svg::lineTo(self->_svgObjectStart, { -sideLength / 2.0f, sideLength / 2.0f });
		Svg::lineTo(self->_svgObjectStart, { sideLength / 2.0f, sideLength / 2.0f });
		Svg::lineTo(self->_svgObjectStart, { sideLength / 2.0f, -sideLength / 2.0f });
		Svg::lineTo(self->_svgObjectStart, { -sideLength / 2.0f, -sideLength / 2.0f });
		Svg::closePath(self->_svgObjectStart);

		self->_svgObjectStart->finalize();
		self->retargetSvgScale();
	}

	void Square::reInit(AnimObject* self)
	{
		// TODO: Do something better than this
		self->svgObject->free();
		g_memory_free(self->svgObject);
		self->svgObject = nullptr;

		self->_svgObjectStart->free();
		g_memory_free(self->_svgObjectStart);
		self->_svgObjectStart = nullptr;

		self->as.square.init(self);
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
		{
			Square res = {};
			DESERIALIZE_PROP(&res, sideLength, j, 0.0f);
			return res;
		}
		break;
		default:
			g_logger_error("Square serialized with unknown version '%d'", version);
			break;
		}

		return {};
	}

	Square Square::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		switch (version)
		{
		case 1:
		{
			// sideLength       -> float
			Square res;
			memory.read<float>(&res.sideLength);
			return res;
		}
		break;
		default:
			g_logger_error("Tried to deserialize unknown version of square %d. It looks like you have corrupted scene data.");
			break;
		}

		return {};
	}

	void Circle::init(AnimObject* self)
	{
		g_logger_assert(self->_svgObjectStart == nullptr && self->svgObject == nullptr, "Circle object initialized twice.");

		self->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*self->_svgObjectStart = Svg::createDefault();
		self->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*self->svgObject = Svg::createDefault();

		// See here for how to construct circle with beziers 
		// https://stackoverflow.com/questions/1734745/how-to-create-circle-with-bézier-curves
		double equidistantControls = (4.0 / 3.0) * glm::tan(glm::pi<double>() / 8.0) * (double)radius;
		Vec2 translation = Vec2{ radius, radius };
		Svg::beginPath(self->_svgObjectStart, Vec2{ -radius, 0.0f } + translation);

		Svg::bezier3To(self->_svgObjectStart,
			Vec2{ -radius, (float)equidistantControls } + translation,
			Vec2{ -(float)equidistantControls, radius } + translation,
			Vec2{ 0.0f, radius } + translation);
		Svg::bezier3To(self->_svgObjectStart,
			Vec2{ (float)equidistantControls, radius } + translation,
			Vec2{ radius, (float)equidistantControls } + translation,
			Vec2{ radius, 0.0f } + translation);
		Svg::bezier3To(self->_svgObjectStart,
			Vec2{ radius, -(float)equidistantControls } + translation,
			Vec2{ (float)equidistantControls, -radius } + translation,
			Vec2{ 0.0f, -radius } + translation);
		Svg::bezier3To(self->_svgObjectStart,
			Vec2{ -(float)equidistantControls, -radius } + translation,
			Vec2{ -radius, -(float)equidistantControls } + translation,
			Vec2{ -radius, 0.0f } + translation);

		Svg::closePath(self->_svgObjectStart);

		self->_svgObjectStart->finalize();
		self->retargetSvgScale();
	}

	void Circle::reInit(AnimObject* self)
	{
		self->svgObject->free();
		g_memory_free(self->svgObject);
		self->svgObject = nullptr;

		self->_svgObjectStart->free();
		g_memory_free(self->_svgObjectStart);
		self->_svgObjectStart = nullptr;

		self->as.circle.init(self);
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
		{
			Circle res = {};
			DESERIALIZE_PROP(&res, radius, j, 0.0f);
			return res;
		}
		break;
		default:
			break;
		}

		g_logger_warning("Circle serialized with unknown version '%d'.", version);
		return {};
	}

	Circle Circle::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		switch (version)
		{
		case 1:
		{
			// radius   -> float
			Circle res;
			memory.read<float>(&res.radius);
			return res;
		}
		break;
		default:
			g_logger_error("Tried to deserialize unknown version of square %d. It looks like you have corrupted scene data.");
			break;
		}

		return {};
	}

	void Arrow::init(AnimObject* self)
	{
		g_logger_assert(self->_svgObjectStart == nullptr && self->svgObject == nullptr, "Arrow object initialized twice.");

		self->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*self->_svgObjectStart = Svg::createDefault();
		self->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*self->svgObject = Svg::createDefault();

		const float halfLength = stemLength / 2.0f;
		const float halfWidth = stemWidth / 2.0f;
		const float tipOffset = tipWidth - stemWidth;
		const float halfTipOffset = tipOffset / 2.0f;
		const float halfTipWidth = tipWidth / 2.0f;

		Svg::beginPath(self->_svgObjectStart, Vec2{ -halfLength, -halfWidth });
		Svg::lineTo(self->_svgObjectStart, Vec2{ 0.0f, stemWidth }, false);
		Svg::lineTo(self->_svgObjectStart, Vec2{ stemLength, 0.0f }, false);
		Svg::lineTo(self->_svgObjectStart, Vec2{ 0.0f, halfTipOffset }, false);
		Svg::lineTo(self->_svgObjectStart, Vec2{ tipLength, -halfTipWidth }, false);
		Svg::lineTo(self->_svgObjectStart, Vec2{ -tipLength, -halfTipWidth }, false);
		Svg::lineTo(self->_svgObjectStart, Vec2{ 0.0f, halfTipOffset }, false);
		Svg::lineTo(self->_svgObjectStart, Vec2{ -stemLength, 0.0f }, false);
		Svg::closePath(self->_svgObjectStart);

		self->_svgObjectStart->finalize();
		self->retargetSvgScale();
	}

	void Arrow::reInit(AnimObject* self)
	{
		self->svgObject->free();
		g_memory_free(self->svgObject);
		self->svgObject = nullptr;

		self->_svgObjectStart->free();
		g_memory_free(self->_svgObjectStart);
		self->_svgObjectStart = nullptr;

		self->as.arrow.init(self);
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

	Arrow Arrow::legacy_deserialize(RawMemory& memory, uint32 version)
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

	void Cube::init(AnimObject* self)
	{
		g_logger_assert(self->_svgObjectStart == nullptr && self->svgObject == nullptr, "Square object initialized twice.");

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
		//	AnimObject cubeFace = AnimObject::createDefaultFromself(AnimObjectTypeV1::SvgObject, self);
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
		//	self->children.push_back(cubeFace);
		//}
	}

	void Cube::reInit(AnimObject* self)
	{
		self->svgObject->free();
		g_memory_free(self->svgObject);
		self->svgObject = nullptr;

		self->_svgObjectStart->free();
		g_memory_free(self->_svgObjectStart);
		self->_svgObjectStart = nullptr;

		self->as.cube.init(self);
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
		{
			Cube res = {};
			DESERIALIZE_PROP(&res, sideLength, j, 0.0f);
			return res;
		}
		break;
		default:
			break;
		}

		g_logger_warning("Cube serialized with unknown version '%d'.", version);
		return {};
	}

	Cube Cube::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		switch (version)
		{
		case 1:
		{
			// sideLength       -> float
			Cube res;
			memory.read<float>(&res.sideLength);
			return res;
		}
		break;
		default:
			g_logger_error("Tried to deserialize unknown version of square %d. It looks like you have corrupted scene data.");
			break;
		}

		return {};
	}
}