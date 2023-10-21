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

	enum class KeyMods : uint8
	{
		None = 0,
		Shift = 1 << 0,
		Alt   = 1 << 1,
		Ctrl  = 1 << 2,
		Super = 1 << 3
	};
	MATH_ANIM_ENUM_FLAG_OPS(KeyMods);

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
		void characterCallback(GLFWwindow* window, uint32 codepoint);
		void endFrame();
		
		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		bool keyPressed(int key, KeyMods mods = KeyMods::None);
		bool keyDown(int key, KeyMods mods = KeyMods::None);
		bool keyUp(int key, KeyMods mods = KeyMods::None);
		bool keyRepeatedOrDown(int key, KeyMods mods = KeyMods::None);

		uint32 getLastCharacterTyped();

		bool mouseClicked(MouseButton button);
		bool mouseDown(MouseButton button);
		bool mouseUp(MouseButton button);
	}
}

#endif