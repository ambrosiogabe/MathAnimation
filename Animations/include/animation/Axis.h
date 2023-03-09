#ifndef MATH_ANIM_AXIS_H
#define MATH_ANIM_AXIS_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct AnimObject;

	struct Axis
	{
		Vec3 axesLength;
		Vec2i xRange;
		Vec2i yRange;
		Vec2i zRange;
		float xStep;
		float yStep;
		float zStep;
		float tickWidth;
		bool drawNumbers;
		float fontSizePixels;
		float labelPadding;
		float labelStrokeWidth;

		void init(AnimObject* parent);
		void serialize(nlohmann::json& memory) const;

		static Axis deserialize(const nlohmann::json& j, uint32 version);
	};
}

#endif 