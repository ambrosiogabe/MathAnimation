#ifndef MATH_ANIM_PROJECT_SCREEN_H
#define MATH_ANIM_PROJECT_SCREEN_H
#include "core.h"
#include "renderer/Texture.h"

namespace MathAnim
{
	struct ProjectInfo
	{
		std::string projectFilepath;
		std::string previewImageFilepath;
		std::string projectName;
		Texture texture;
	};

	namespace ProjectScreen
	{
		void init();

		bool update();

		void free();

		const ProjectInfo& getSelectedProject();
	}
}

#endif