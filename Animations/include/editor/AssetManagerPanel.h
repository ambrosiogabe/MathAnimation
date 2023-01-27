#ifndef MATH_ANIM_ASSET_MANAGER_PANEL_H
#define MATH_ANIM_ASSET_MANAGER_PANEL_H
#include "core.h"

namespace MathAnim
{
	namespace AssetManagerPanel
	{
		void init(const std::filesystem::path& assetsRoot);

		void update();

		void free();
	}
}

#endif 