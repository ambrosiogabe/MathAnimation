#include "animation/Axis.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "svg/Svg.h"
#include "math/CMath.h"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	// ------------------ Internal Functions ------------------
	static Axis deserializeAxisV2(const nlohmann::json& j);

	void Axis::init(AnimObject*)
	{
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
		//	g_logger_assert(xRange.max > xRange.min, "Invalid x range [%d, %d]. Axis range must be in increasing order.", xRange.min, xRange.max);
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
		//	g_logger_assert(yRange.max > yRange.min, "Invalid y range [%d, %d]. Axis range must be in increasing order.", yRange.min, yRange.max);
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
		//	g_logger_assert(zRange.max > zRange.min, "Invalid z range [%d, %d]. Axis range must be in increasing order.", zRange.min, zRange.max);
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

	void Axis::serialize(nlohmann::json& memory) const
	{
		CMath::serialize(memory, "AxesLength", axesLength);
		CMath::serialize(memory, "XRange",xRange);
		CMath::serialize(memory, "YRange", yRange);
		CMath::serialize(memory, "ZRange", zRange);
		memory["XStep"] = xStep;
		memory["YStep"] = yStep;
		memory["ZStep"] = zStep;
		memory["TickWidth"] = tickWidth;
		memory["DrawNumbers"] = drawNumbers;
		memory["FontSizePx"] = fontSizePixels;
		memory["LabelPadding"] = labelPadding;
		memory["LabelStrokeWidth"] = labelStrokeWidth;
	}

	Axis Axis::deserialize(const nlohmann::json& j, uint32 version)
	{
		switch (version)
		{
		case 2:
			return deserializeAxisV2(j);
			break;
		default:
			g_logger_error("Tried to deserialize unknown version of axis %d. It looks like you have corrupted scene data.");
			break;
		}

		return {};
	}

	// ------------------ Internal Functions ------------------
	static Axis deserializeAxisV2(const nlohmann::json& j)
	{
		Axis res;
		res.axesLength = CMath::deserializeVec3(j);
		res.xRange = CMath::deserializeVec2i(j);
		res.yRange = CMath::deserializeVec2i(j);
		res.zRange = CMath::deserializeVec2i(j);
		res.xStep = j.contains("XStep") ? j["XStep"] : 0.0f;
		res.xStep = j.contains("YStep") ? j["YStep"] : 0.0f;
		res.xStep = j.contains("ZStep") ? j["ZStep"] : 0.0f;
		res.tickWidth = j.contains("TickWidth") ? j["TickWidth"] : 0.0f;
		res.drawNumbers = j.contains("DrawNumbers") ? j["DrawNumbers"] : false;
		res.fontSizePixels = j.contains("FontSizePixels") ? j["FontSizePixels"] : 0.0f;
		res.labelPadding = j.contains("LabelPadding") ? j["LabelPadding"] : 0.0f;
		res.labelStrokeWidth = j.contains("LabelStrokeWidth") ? j["LabelStrokeWidth"] : 0.0f;

		return res;
	}
}