#ifndef MATH_ANIM_PROJECT_APP_H
#define MATH_ANIM_PROJECT_APP_H
#include "core.h"

namespace MathAnim
{
	struct Window;

	namespace ProjectApp
	{
		void init();

		std::string run();

		void free();

		Window* getWindow();

		const std::filesystem::path& getAppRoot();
	}
}

#endif 