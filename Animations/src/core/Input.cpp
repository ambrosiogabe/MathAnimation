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

		bool keyPressed[GLFW_KEY_LAST + 1];

		// ----------- Internal Variables -----------
		static float mLastMouseX;
		static float mLastMouseY;
		static bool mFirstMouse = true;
		static bool mouseButtonDownLastFrame[(uint8)MouseButton::Length];
		static bool mouseButtonDown[(uint8)MouseButton::Length];
		static bool mouseButtonUp[(uint8)MouseButton::Length];

		void mouseCallback(GLFWwindow* window, double xpos, double ypos)
		{
			mouseX = (float)xpos;
			mouseY = (float)ypos;
			if (mFirstMouse)
			{
				mLastMouseX = (float)xpos;
				mLastMouseY = (float)ypos;
				mFirstMouse = false;
			}

			deltaMouseX = (float)xpos - mLastMouseX;
			deltaMouseY = mLastMouseY - (float)ypos;
			mLastMouseX = (float)xpos;
			mLastMouseY = (float)ypos;
		}

		void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
		{
			mouseButtonDown[button] = action == GLFW_PRESS;
			mouseButtonUp[button] = action == GLFW_RELEASE;
		}

		void endFrame()
		{
			// Copy the mouse button down states to mouse button down last frame
			g_memory_copyMem(mouseButtonDownLastFrame, mouseButtonDown, sizeof(bool) * (uint8)MouseButton::Length);
			deltaMouseX = 0;
			deltaMouseY = 0;
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