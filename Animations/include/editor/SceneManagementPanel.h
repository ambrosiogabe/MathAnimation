#ifndef MATH_ANIM_SCENE_MANAGEMENT_PANEL_H
#define MATH_ANIM_SCENE_MANAGEMENT_PANEL_H
#include "core.h"

namespace MathAnim
{
	struct SceneData
	{
		std::vector<std::string> sceneNames;
		int currentScene;
	};

	namespace SceneManagementPanel
	{
		void init();

		void update(SceneData& sd);

		void free();

		RawMemory serialize(const SceneData& data);
		SceneData deserialize(RawMemory& memory);
	}
}

#endif 