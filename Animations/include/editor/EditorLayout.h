#ifndef MATH_ANIM_EDITOR_LAYOUT_H
#define MATH_ANIM_EDITOR_LAYOUT_H
#include "core.h"

namespace MathAnim
{
	namespace EditorLayout
	{
		void init(const std::filesystem::path& projectRoot);

		void update();

		void addCustomLayout(const std::filesystem::path& layoutFilepath);
		const std::vector<std::filesystem::path>& getDefaultLayouts();
		const std::vector<std::filesystem::path>& getCustomLayouts();

		const std::filesystem::path& getLayoutsRoot();
		bool isReserved(const char* name);
	}
}

#endif 