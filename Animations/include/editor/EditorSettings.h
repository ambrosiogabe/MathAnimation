#ifndef MATH_ANIM_EDITOR_SETTINGS
#define MATH_ANIM_EDITOR_SETTINGS
#include "core.h"

namespace MathAnim
{
	struct EditorSettingsData
	{
		float mouseSensitivity;
		float scrollSensitvity;
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