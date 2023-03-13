#ifndef MATH_ANIM_SCENE_MANAGEMENT_PANEL_H
#define MATH_ANIM_SCENE_MANAGEMENT_PANEL_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

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

		void serialize(nlohmann::json& j, const SceneData& data);
		SceneData deserialize(const nlohmann::json& j);

		[[deprecated("This is here to support upgrading old legacy projects")]]
		SceneData legacy_deserialize(RawMemory& memory);
	}
}

#endif 