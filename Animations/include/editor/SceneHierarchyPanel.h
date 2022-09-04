#ifndef MATH_ANIM_SCENE_HIERARCHY_WINDOW_H
#define MATH_ANIM_SCENE_HIERARCHY_WINDOW_H
#include "core.h"

namespace MathAnim
{
	struct AnimObject;

	namespace SceneHierarchyPanel
	{
		void init();
		void free();

		void addNewAnimObject(const AnimObject& animObject);
		void update();
		void deleteAnimObject(const AnimObject& animObject);

		void serialize(RawMemory& memory);
		void deserialize(RawMemory& memory);
	};
}

#endif
