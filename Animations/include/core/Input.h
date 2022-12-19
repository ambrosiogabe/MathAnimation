#ifndef MATH_ANIM_INPUT_H
#define MATH_ANIM_INPUT_H

#include "core.h"

namespace MathAnim
{
	// Equivalent to GLFW enum values
	enum class MouseButton : uint8
	{
		Left = 0,
		Right,
		Middle,
		B4, // B4 is for Button4, etc.
		B5,
		B6,
		B7,
		B8,
		Length
	};

	namespace Input
	{
		extern float mouseX;
		extern float mouseY;
		extern float deltaMouseX;
		extern float deltaMouseY;
		extern float scrollX;
		extern float scrollY;

		void mouseCallback(GLFWwindow* window, double xpos, double ypos);
		void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		void endFrame();
		
		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		bool keyPressed(int key);
		bool keyDown(int key);
		bool keyUp(int key);

		bool mouseClicked(MouseButton button);
		bool mouseDown(MouseButton button);
		bool mouseUp(MouseButton button);
	}
}

#endif