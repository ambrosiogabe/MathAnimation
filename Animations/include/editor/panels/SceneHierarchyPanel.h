#ifndef MATH_ANIM_SCENE_HIERARCHY_WINDOW_H
#define MATH_ANIM_SCENE_HIERARCHY_WINDOW_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct AnimObject;
	struct AnimationManagerData;

	namespace SceneHierarchyPanel
	{
		void init(AnimationManagerData* am);
		void free();

		void addNewAnimObject(const AnimObject& animObject);
		void update(AnimationManagerData* am);
		void deleteAnimObject(const AnimObject& animObject);

		void serialize(nlohmann::json& memory);
		void deserialize(RawMemory& memory);
	};
}

#endif
