#include "core/Input.h"

namespace MathAnim
{
	namespace Input
	{
		// ----------- Global Variables -----------
		float mouseX = 0;
		float mouseY = 0;
		float deltaMouseX = 0;
		float deltaMouseY = 0;
		float scrollX = 0;
		float scrollY = 0;

		bool keyPressed[GLFW_KEY_LAST + 1];

		// ----------- Internal Variables -----------
		static float lastMouseX;
		static float lastMouseY;
		static bool firstMouse = true;
		static bool mouseButtonDownLastFrame[(uint8)MouseButton::Length];
		static bool mouseButtonDown[(uint8)MouseButton::Length];
		static bool mouseButtonUp[(uint8)MouseButton::Length];

		void mouseCallback(GLFWwindow* window, double xpos, double ypos)
		{
			mouseX = (float)xpos;
			mouseY = (float)ypos;
			if (firstMouse)
			{
				lastMouseX = (float)xpos;
				lastMouseY = (float)ypos;
				firstMouse = false;
			}

			deltaMouseX = (float)xpos - lastMouseX;
			deltaMouseY = lastMouseY - (float)ypos;
			lastMouseX = (float)xpos;
			lastMouseY = (float)ypos;
		}

		void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
		{
			mouseButtonDown[button] = action == GLFW_PRESS;
			mouseButtonUp[button] = action == GLFW_RELEASE;
		}

		void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
		{
			scrollX = (float)xoffset;
			scrollY = (float)yoffset;
		}

		void endFrame()
		{
			// Copy the mouse button down states to mouse button down last frame
			g_memory_copyMem(mouseButtonDownLastFrame, mouseButtonDown, sizeof(bool) * (uint8)MouseButton::Length);
			deltaMouseX = 0;
			deltaMouseY = 0;
			scrollX = 0;
			scrollY = 0;
		}

		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			keyPressed[key] = action == GLFW_REPEAT || action == GLFW_PRESS;
		}

		bool isKeyPressed(int key)
		{
			if (key >= 0 && key < GLFW_KEY_LAST)
			{
				return keyPressed[key];
			}
			else
			{
				g_logger_error("Cannot check if key '%d' is pressed. Key is out of range of known key codes.", key);
			}
			
			return false;
		}

		bool mouseClicked(MouseButton button)
		{
			// If the mouse was down last frame then up this frame, that counts as a "click"
			return mouseButtonDownLastFrame[(uint8)button] && !mouseButtonDown[(uint8)button];
		}

		bool mouseDown(MouseButton button)
		{
			return mouseButtonDown[(uint8)button];
		}

		bool mouseUp(MouseButton button)
		{
			return mouseButtonUp[(uint8)button];
		}
	}
}