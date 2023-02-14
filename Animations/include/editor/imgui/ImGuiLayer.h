#ifndef MATH_ANIM_IMGUI_LAYER_H
#define MATH_ANIM_IMGUI_LAYER_H
#include "core.h"

struct ImFont;

namespace MathAnim
{
	struct Window;
	union Vec2;

	namespace ImGuiLayer
	{
		void init(int glVersionMajor, int glVersionMinor, const Window& window, const char* jsonLayoutFile = nullptr);

		void beginFrame();
		void endFrame();

		void keyEvent();
		void mouseEvent();

		void free();
		void saveEditorLayout();
		void loadEditorLayout(const std::filesystem::path& layoutPath, const Vec2& targetResolution);

		ImFont* getDefaultFont();
		ImFont* getMediumFont();
		ImFont* getMonoFont();

		ImFont* getLargeSolidIconFont();
		ImFont* getMediumSolidIconFont();
		ImFont* getLargeRegularIconFont();
		ImFont* getMediumRegularIconFont();
	}
}

#endif 