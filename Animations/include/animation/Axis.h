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

		void serialize(nlohmann::json& j) const;
		static Axis deserialize(const nlohmann::json& j, uint32 version);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static Axis legacy_deserialize(RawMemory& memory, uint32 version);
	};
}

#endif 