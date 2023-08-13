#include "animation/Axis.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "core/Serialization.hpp"
#include "svg/Svg.h"
#include "math/CMath.h"
#include "editor/panels/SceneHierarchyPanel.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	void Axis::init(AnimObject*)
	{
		g_logger_warning("TODO: FIXME");
		//g_logger_assert(parent->_svgObjectStart == nullptr && parent->svgObject == nullptr, "Axis object initialized twice.");
		//g_logger_assert(parent->children.size() == 0, "Axis object initialized twice.");

		//float xStart = -axesLength.x / 2.0f;
		//float xEnd = axesLength.x / 2.0f;
		//float xMiddle = xStart + (xEnd - xStart) / 2.0f;

		//float yStart = -axesLength.y / 2.0f;
		//float yEnd = axesLength.y / 2.0f;
		//float yMiddle = yStart + (yEnd - yStart) / 2.0f;

		//// X-Axis
		//{
		//	// The first child will be the x-axis lines as an svg object
		//	AnimObject xAxis = AnimObject::createDefaultFromParent(AnimObjectTypeV1::SvgObject, parent);
		//	xAxis._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		//	*xAxis._svgObjectStart = Svg::createDefault();
		//	xAxis.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		//	*xAxis.svgObject = Svg::createDefault();
		//	xAxis._positionStart = Vec3{ 0, 0, 0 };

		//	// Draw the x-axis
		//	Svg::beginContour(xAxis._svgObjectStart, { xStart, 0.0f });
		//	Svg::lineTo(xAxis._svgObjectStart, { xEnd, 0.0f });

		//	// Draw the ticks
		//	g_logger_assert(xRange.max > xRange.min, "Invalid x range [{}, {}]. Axis range must be in increasing order.", xRange.min, xRange.max);
		//	float numTicks = ((float)xRange.max - (float)xRange.min) / xStep;
		//	float distanceBetweenTicks = axesLength.x / numTicks;
		//	int xNumber = xRange.min;
		//	Vec2 cursor = Vec2{ xStart, -tickWidth / 2.0f };
		//	for (int i = 0; i <= (int)glm::ceil(numTicks); i++)
		//	{
		//		if (!(cursor.x >= xMiddle && cursor.x <= xMiddle))
		//		{
		//			Svg::moveTo(xAxis._svgObjectStart, cursor);
		//			Svg::lineTo(xAxis._svgObjectStart, cursor + Vec2{ 0.0f, tickWidth });

		//			if (this->drawNumbers)
		//			{
		//				// Add an anim object child with the number this axis tick represents
		//				AnimObject labelChild = AnimObject::createDefaultFromParent(AnimObjectTypeV1::LaTexObject, parent);
		//				labelChild._positionStart = CMath::vector3From2(cursor + Vec2{ 0.0f, -labelPadding });
		//				labelChild.svgScale = fontSizePixels;
		//				labelChild.strokeWidth = labelStrokeWidth;
		//				// Convert xNumber to string
		//				std::string xTickLabel = std::to_string(xNumber);
		//				labelChild.as.laTexObject.setText(xTickLabel);
		//				labelChild.as.laTexObject.parseLaTex();
		//				parent->children.push_back(labelChild);
		//			}
		//		}

		//		cursor.x += distanceBetweenTicks;
		//		xNumber += this->xStep;
		//	}
		//	Svg::closeContour(xAxis._svgObjectStart);
		//	xAxis._svgObjectStart->calculateApproximatePerimeter();
		//	xAxis._svgObjectStart->calculateBBox();
		//	parent->children.push_back(xAxis);
		//}

		//// Y-Axis
		//{
		//	// The second child will be the y-axis lines as an svg object
		//	AnimObject yAxis = AnimObject::createDefaultFromParent(AnimObjectTypeV1::SvgObject, parent);
		//	yAxis._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		//	*yAxis._svgObjectStart = Svg::createDefault();
		//	yAxis.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		//	*yAxis.svgObject = Svg::createDefault();
		//	yAxis._positionStart = Vec3{ 0, 0, 0 };

		//	// Draw the y-axis
		//	Svg::beginContour(yAxis._svgObjectStart, { 0.0f, yStart });
		//	Svg::lineTo(yAxis._svgObjectStart, { 0.0f, yEnd });

		//	// Draw the ticks
		//	g_logger_assert(yRange.max > yRange.min, "Invalid y range [{}, {}]. Axis range must be in increasing order.", yRange.min, yRange.max);
		//	float numTicks = ((float)yRange.max - (float)yRange.min) / yStep;
		//	float distanceBetweenTicks = axesLength.y / numTicks;
		//	int yNumber = yRange.min;
		//	Vec2 cursor = Vec2{ -tickWidth / 2.0f, yStart };
		//	for (int i = 0; i <= (int)glm::ceil(numTicks); i++)
		//	{
		//		if (!(cursor.y >= yMiddle && cursor.y <= yMiddle))
		//		{
		//			Svg::moveTo(yAxis._svgObjectStart, cursor);
		//			Svg::lineTo(yAxis._svgObjectStart, cursor + Vec2{ tickWidth, 0.0f });

		//			if (this->drawNumbers)
		//			{
		//				// Add an anim object child with the number this axis tick represents
		//				AnimObject labelChild = AnimObject::createDefaultFromParent(AnimObjectTypeV1::LaTexObject, parent);
		//				labelChild._positionStart = CMath::vector3From2(cursor + Vec2{ tickWidth + labelPadding, 0.0f });
		//				labelChild.position = labelChild._positionStart;
		//				labelChild.svgScale = fontSizePixels;
		//				labelChild.strokeWidth = labelStrokeWidth;
		//				// Convert yNumber to string
		//				std::string yTickLabel = std::to_string(yNumber);
		//				labelChild.as.laTexObject.setText(yTickLabel);
		//				labelChild.as.laTexObject.parseLaTex();
		//				parent->children.push_back(labelChild);
		//			}
		//		}

		//		cursor.y += distanceBetweenTicks;
		//		yNumber += this->yStep;
		//	}

		//	Svg::closeContour(yAxis._svgObjectStart);
		//	yAxis._svgObjectStart->calculateApproximatePerimeter();
		//	yAxis._svgObjectStart->calculateBBox();
		//	parent->children.push_back(yAxis);
		//}

		// Z-Axis
		//if (is3D)
		//{
		//	// Draw the z-axis	
		//	float zStart = -axesLength.z / 2.0f;
		//	float zEnd = axesLength.z / 2.0f;
		//	Svg::beginContour(parent->_svgObjectStart, { 0.0f, 0.0f, zStart }, is3D);
		//	Svg::lineTo(parent->_svgObjectStart, { 0.0f, 0.0f, zEnd });

		//	// Draw the ticks
		//	g_logger_assert(zRange.max > zRange.min, "Invalid z range [{}, {}]. Axis range must be in increasing order.", zRange.min, zRange.max);
		//	float numTicks = (float)zRange.max - (float)zRange.min;
		//	float distanceBetweenTicks = axesLength.z / numTicks;
		//	Vec3 cursor = Vec3{ 0.0f, -tickWidth / 2.0f, zStart };
		//	while (cursor.z <= zEnd)
		//	{
		//		Svg::moveTo(parent->_svgObjectStart, cursor);
		//		Svg::lineTo(parent->_svgObjectStart, cursor + Vec3{ 0.0f, tickWidth, 0.0f });
		//		cursor.z += distanceBetweenTicks;
		//	}
		//	Svg::closeContour(parent->_svgObjectStart);
		//}
	}

	void Axis::reInit(AnimationManagerData* am, AnimObject* obj)
	{
		// First remove all generated children, which were generated as a result
		// of this object (presumably)
		// NOTE: This is direct descendants, no recursive children here
		for (int i = 0; i < obj->generatedChildrenIds.size(); i++)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, obj->generatedChildrenIds[i]);
			if (child)
			{
				SceneHierarchyPanel::deleteAnimObject(*child);
				AnimationManager::removeAnimObject(am, obj->generatedChildrenIds[i]);
			}
		}
		obj->generatedChildrenIds.clear();

		// Next init again which should regenerate the children
		init(obj);
	}

	void Axis::serialize(nlohmann::json& memory) const
	{
		SERIALIZE_VEC(memory, this, axesLength);
		SERIALIZE_VEC(memory, this, xRange);
		SERIALIZE_VEC(memory, this, yRange);
		SERIALIZE_VEC(memory, this, zRange);
		SERIALIZE_NON_NULL_PROP(memory, this, xStep);
		SERIALIZE_NON_NULL_PROP(memory, this, yStep);
		SERIALIZE_NON_NULL_PROP(memory, this, zStep);
		SERIALIZE_NON_NULL_PROP(memory, this, tickWidth);
		SERIALIZE_NON_NULL_PROP(memory, this, drawNumbers);
		SERIALIZE_NON_NULL_PROP(memory, this, fontSizePixels);
		SERIALIZE_NON_NULL_PROP(memory, this, labelPadding);
		SERIALIZE_NON_NULL_PROP(memory, this, labelStrokeWidth);
	}

	Axis Axis::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
		case 3:
		{
			Axis res = {};

			DESERIALIZE_VEC3(&res, axesLength, j, (Vec3{10, 10, 10}));
			DESERIALIZE_VEC2i(&res, xRange, j, (Vec2i{1, 1}));
			DESERIALIZE_VEC2i(&res, yRange, j, (Vec2i{1, 1}));
			DESERIALIZE_VEC2i(&res, zRange, j, (Vec2i{1, 1}));
			DESERIALIZE_PROP(&res, xStep, j, 1.0f);
			DESERIALIZE_PROP(&res, yStep, j, 1.0f);
			DESERIALIZE_PROP(&res, zStep, j, 1.0f);
			DESERIALIZE_PROP(&res, tickWidth, j, 60.0f);
			DESERIALIZE_PROP(&res, drawNumbers, j, false);
			DESERIALIZE_PROP(&res, fontSizePixels, j, 20.0f);
			DESERIALIZE_PROP(&res, labelPadding, j, 10.0f);
			DESERIALIZE_PROP(&res, labelStrokeWidth, j, 0.03f);

			return res;
		}
		break;
		default:
			g_logger_error("Tried to deserialize unknown version of axis '{}'. It looks like you have corrupted scene data.", version);
			break;
		}

		return {};
	}

	Axis Axis::legacy_deserialize(RawMemory& memory, uint32 version)
	{
		switch (version)
		{
		case 1:
		{
			// axesLength    -> Vec3
			// xRange        -> Vec2i
			// yRange        -> Vec2i
			// zRange        -> Vec2i
			// xStep         -> float
			// yStep         -> float
			// zStep         -> float
			// tickWidth     -> float 
			// drawNumbers   -> u8
			// fontSizePx    -> float
			// labelPadding  -> float
			// labelStrokeWidth -> float
			Axis res;
			res.axesLength = CMath::legacy_deserializeVec3(memory);
			res.xRange = CMath::legacy_deserializeVec2i(memory);
			res.yRange = CMath::legacy_deserializeVec2i(memory);
			res.zRange = CMath::legacy_deserializeVec2i(memory);
			memory.read<float>(&res.xStep);
			memory.read<float>(&res.yStep);
			memory.read<float>(&res.zStep);
			memory.read<float>(&res.tickWidth);
			uint8 drawNumbersU8;
			memory.read<uint8>(&drawNumbersU8);
			res.drawNumbers = drawNumbersU8 == 1;
			memory.read<float>(&res.fontSizePixels);
			memory.read<float>(&res.labelPadding);
			memory.read<float>(&res.labelStrokeWidth);

			return res;
		}
		break;
		default:
			g_logger_error("Tried to deserialize unknown version of axis '{}'. It looks like you have corrupted scene data.", version);
			break;
		}
		return {};
	}
}