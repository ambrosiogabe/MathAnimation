#include "animation/Axis.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "animation/Svg.h"
#include "utils/CMath.h"

namespace MathAnim
{
	// ------------------ Internal Functions ------------------
	static Axis deserializeAxisV1(RawMemory& memory);

	void Axis::init(AnimObject* parent)
	{
		g_logger_assert(parent->_svgObjectStart == nullptr && parent->svgObject == nullptr, "Axis object initialized twice.");

		parent->_svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->_svgObjectStart = Svg::createDefault();
		parent->svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
		*parent->svgObject = Svg::createDefault();

		// Draw the x-axis
		float xRangeLength = ((float)xRange.max - (float)xRange.min) + xIncrement * 2.0f;
		float xStart = -xRangeLength / 2.0f;
		float xEnd = xStart + xRangeLength;
		Svg::beginContour(parent->_svgObjectStart, { xStart, 0.0f, 0.0f }, true, is3D);
		Svg::lineTo(parent->_svgObjectStart, { xEnd, 0.0f, 0.0f });

		// Draw the ticks
		{
			g_logger_assert(xRange.max > xRange.min, "Invalid x range [%d, %d]. Axis range must be in increasing order.", xRange.min, xRange.max);
			Vec3 cursor = Vec3{ xStart + xIncrement, -tickWidth / 2.0f, 0.0f };
			while (cursor.x <= xEnd - xIncrement)
			{
				Svg::moveTo(parent->_svgObjectStart, cursor);
				Svg::lineTo(parent->_svgObjectStart, cursor + Vec3{ 0.0f, tickWidth, 0.0f });
				cursor.x += xIncrement;
			}
		}
		Svg::closeContour(parent->_svgObjectStart);

		// Draw the y-axis
		float yRangeLength = ((float)yRange.max - (float)yRange.min) + yIncrement * 2.0f;
		float yStart = -yRangeLength / 2.0f;
		float yEnd = yStart + yRangeLength;
		Svg::beginContour(parent->_svgObjectStart, { 0.0f, yStart, 0.0f }, true, is3D);
		Svg::lineTo(parent->_svgObjectStart, { 0.0f, yEnd, 0.0f });

		// Draw the ticks
		{
			g_logger_assert(yRange.max > yRange.min, "Invalid y range [%d, %d]. Axis range must be in increasing order.", yRange.min, yRange.max);
			Vec3 cursor = Vec3{ -tickWidth / 2.0f, yStart + yIncrement, 0.0f };
			while (cursor.y <= yEnd - yIncrement)
			{
				Svg::moveTo(parent->_svgObjectStart, cursor);
				Svg::lineTo(parent->_svgObjectStart, cursor + Vec3{ tickWidth, 0.0f, 0.0f });
				cursor.y += yIncrement;
			}
		}
		Svg::closeContour(parent->_svgObjectStart);

		// Draw the z-axis	
		float zRangeLength = ((float)zRange.max - (float)zRange.min) + zIncrement * 2.0f;
		float zStart = -zRangeLength / 2.0f;
		float zEnd = zStart + zRangeLength;
		Svg::beginContour(parent->_svgObjectStart, { 0.0f, 0.0f, zStart }, true, is3D);
		Svg::lineTo(parent->_svgObjectStart, { 0.0f, 0.0f, zEnd });

		// Draw the ticks
		if (is3D)
		{
			g_logger_assert(zRange.max > zRange.min, "Invalid z range [%d, %d]. Axis range must be in increasing order.", zRange.min, zRange.max);
			Vec3 cursor = Vec3{ -tickWidth / 2.0f, 0.0f, zStart + zIncrement };
			while (cursor.z <= zRange.max - zIncrement)
			{
				Svg::moveTo(parent->_svgObjectStart, cursor);
				Svg::lineTo(parent->_svgObjectStart, cursor + Vec3{ tickWidth, 0.0f, 0.0f });
				cursor.z += zIncrement;
			}
		}
		Svg::closeContour(parent->_svgObjectStart);

		parent->_svgObjectStart->calculateApproximatePerimeter();
	}

	void Axis::serialize(RawMemory& memory) const
	{
		// xRange        -> Vec2i
		// yRange        -> Vec2i
		// zRange        -> Vec2i
		// xIncrement    -> float
		// yIncrement    -> float
		// zIncrement    -> float
		// tickWidth     -> float 
		// is3D          -> u8
		// drawNumbers   -> u8
		CMath::serialize(memory, xRange);
		CMath::serialize(memory, yRange);
		CMath::serialize(memory, zRange);
		memory.write<float>(&xIncrement);
		memory.write<float>(&yIncrement);
		memory.write<float>(&zIncrement);
		memory.write<float>(&tickWidth);
		const uint8 is3DU8 = is3D ? 1 : 0;
		memory.write<uint8>(&is3DU8);
		const uint8 drawNumbersU8 = drawNumbers ? 1 : 0;
		memory.write<uint8>(&drawNumbersU8);
	}

	Axis Axis::deserialize(RawMemory& memory, uint32 version)
	{
		switch (version)
		{
		case 1:
			return deserializeAxisV1(memory);
			break;
		default:
			g_logger_error("Tried to deserialize unknown version of axis %d. It looks like you have corrupted scene data.");
			break;
		}

		return {};
	}

	// ------------------ Internal Functions ------------------
	static Axis deserializeAxisV1(RawMemory& memory)
	{
		// xRange        -> Vec2i
		// yRange        -> Vec2i
		// zRange        -> Vec2i
		// xIncrement    -> float
		// yIncrement    -> float
		// zIncrement    -> float
		// tickWidth     -> float 
		// is3D          -> u8
		// drawNumbers   -> u8
		Axis res;
		res.xRange = CMath::deserializeVec2i(memory);
		res.yRange = CMath::deserializeVec2i(memory);
		res.zRange = CMath::deserializeVec2i(memory);
		memory.read<float>(&res.xIncrement);
		memory.read<float>(&res.yIncrement);
		memory.read<float>(&res.zIncrement);
		memory.read<float>(&res.tickWidth);
		uint8 is3DU8;
		memory.read<uint8>(&is3DU8);
		res.is3D = is3DU8 == 1;
		uint8 drawNumbersU8;
		memory.read<uint8>(&drawNumbersU8);
		res.drawNumbers = drawNumbersU8 == 1;

		return res;
	}
}