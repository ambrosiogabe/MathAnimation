#ifndef MATH_ANIM_IMGUI_LAYER_H
#define MATH_ANIM_IMGUI_LAYER_H

struct ImFont;

namespace MathAnim
{
	struct Window;

	namespace ImGuiLayer
	{
		void init(const Window& window);

		void beginFrame();
		void endFrame();

		void keyEvent();
		void mouseEvent();

		void free();

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