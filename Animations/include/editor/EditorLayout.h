#ifndef MATH_ANIM_EDITOR_LAYOUT_H
#define MATH_ANIM_EDITOR_LAYOUT_H
#include "core.h"

namespace MathAnim
{
	namespace EditorLayout
	{
		void init(const std::filesystem::path& projectRoot);

		void update();

		const std::vector<std::filesystem::path>& getDefaultLayouts();
		const std::vector<std::filesystem::path>& getCustomLayouts();
	}
}

#endif 