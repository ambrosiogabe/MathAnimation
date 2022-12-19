#ifndef MATH_ANIM_EDITOR_SETTINGS
#define MATH_ANIM_EDITOR_SETTINGS
#include "core.h"

namespace MathAnim
{
	enum class ViewMode : int
	{
		WireMesh,
		Normal,
		Length
	};

	struct EditorSettingsData
	{
		float mouseSensitivity;
		float scrollSensitvity;
		ViewMode viewMode;
	};

	namespace EditorSettings
	{
		void init();
		void imgui();
		void free();

		const EditorSettingsData& getSettings();
	}
}

#endif 