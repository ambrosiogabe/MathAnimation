#ifndef MATH_ANIM_SHAPES_H
#define MATH_ANIM_SHAPES_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

struct RawMemory;

namespace MathAnim
{
	struct AnimObject;

	struct Square
	{
		float sideLength;

		void init(AnimObject* parent);
		void serialize(nlohmann::json& memory) const;

		static Square deserialize(RawMemory& memory, uint32 version);
	};

	struct Circle
	{
		float radius;

		void init(AnimObject* parent);
		void serialize(nlohmann::json& memory) const;

		static Circle deserialize(RawMemory& memory, uint32 version);
	};

	struct Arrow
	{
		float stemWidth;
		float stemLength;
		float tipWidth;
		float tipLength;

		void init(AnimObject* parent);
		void serialize(nlohmann::json& memory) const;

		static Arrow deserialize(RawMemory& memory, uint32 version);
	};

	struct Cube
	{
		float sideLength;

		void init(AnimObject* parent);
		void serialize(nlohmann::json& memory) const;

		static Cube deserialize(RawMemory& memory, uint32 version);
	};
}

#endif 