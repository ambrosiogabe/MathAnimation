#include "core/Input.h"

namespace MathAnim
{
	namespace Input
	{
		float mouseX = 0;
		float mouseY = 0;
		float deltaMouseX = 0;
		float deltaMouseY = 0;

		bool keyPressed[GLFW_KEY_LAST];
		
		static float mLastMouseX;
		static float mLastMouseY;
        static bool mFirstMouse = true;
		
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

		void endFrame()
		{
			deltaMouseX = 0;
			deltaMouseY = 0;
		}

		void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			keyPressed[key] = action == GLFW_REPEAT || action == GLFW_PRESS;
		}

		bool isKeyPressed(int key)
		{
			g_logger_assert(key >= 0 && key < GLFW_KEY_LAST, "Invalid key.");
			return keyPressed[key];
		}
	}
}