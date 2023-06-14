#ifndef MATH_ANIM_SHAPES_H
#define MATH_ANIM_SHAPES_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

struct RawMemory;

namespace MathAnim
{
	struct AnimObject;
	struct AnimationManagerData;

	struct Square
	{
		float sideLength;

		void init(AnimObject* self);
		void reInit(AnimObject* self);

		void serialize(nlohmann::json& j) const;
		static Square deserialize(const nlohmann::json& j, uint32 version);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static Square legacy_deserialize(RawMemory& memory, uint32 version);
	};

	struct Circle
	{
		float radius;

		void init(AnimObject* self);
		void reInit(AnimObject* self);

		void serialize(nlohmann::json& j) const;
		static Circle deserialize(const nlohmann::json& j, uint32 version);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static Circle legacy_deserialize(RawMemory& memory, uint32 version);
	};

	struct Arrow
	{
		float stemWidth;
		float stemLength;
		float tipWidth;
		float tipLength;

		void init(AnimObject* self);
		void reInit(AnimObject* self);

		void serialize(nlohmann::json& j) const;
		static Arrow deserialize(const nlohmann::json& j, uint32 version);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static Arrow legacy_deserialize(RawMemory& memory, uint32 version);
	};

	struct Cube
	{
		float sideLength;

		void init(AnimationManagerData* am, AnimObject* self);
		void reInit(AnimationManagerData* am, AnimObject* self);

		void serialize(nlohmann::json& j) const;
		static Cube deserialize(const nlohmann::json& j, uint32 version);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static Cube legacy_deserialize(RawMemory& memory, uint32 version);
	};
}

#endif 