#ifndef MATH_ANIM_IMGUI_LAYER_H
#define MATH_ANIM_IMGUI_LAYER_H

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
	}
}

#endif 