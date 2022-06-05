#ifndef MATH_ANIM_INPUT_H
#define MATH_ANIM_INPUT_H

#include "core.h"

namespace MathAnim
{
	namespace Input
	{
		extern float mouseX;
		extern float mouseY;
		extern float deltaMouseX;
		extern float deltaMouseY;
		extern bool keyPressed[GLFW_KEY_LAST];

		void mouseCallback(GLFWwindow* window, double xpos, double ypos);
		void endFrame();
		
		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		bool isKeyPressed(int key);
	}
}

#endif