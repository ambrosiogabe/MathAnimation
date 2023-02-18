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


		// ----------- Internal Variables -----------
		static bool keyDownLastFrame[GLFW_KEY_LAST + 1] = {};
		static bool keyDownData[GLFW_KEY_LAST + 1] = {};
		static bool keyUpData[GLFW_KEY_LAST + 1] = {};

		// Shift+Key combo data
		static bool shiftKeyDownLastFrame[GLFW_KEY_LAST + 1] = {};
		static bool shiftKeyDownData[GLFW_KEY_LAST + 1] = {};
		static bool shiftKeyUpData[GLFW_KEY_LAST + 1] = {};

		// Ctrl+Key combo data
		static bool ctrlKeyDownLastFrame[GLFW_KEY_LAST + 1] = {};
		static bool ctrlKeyDownData[GLFW_KEY_LAST + 1] = {};
		static bool ctrlKeyUpData[GLFW_KEY_LAST + 1] = {};

		// Alt+Key combo data
		static bool altKeyDownLastFrame[GLFW_KEY_LAST + 1] = {};
		static bool altKeyDownData[GLFW_KEY_LAST + 1] = {};
		static bool altKeyUpData[GLFW_KEY_LAST + 1] = {};

		// Super+Key combo data
		static bool superKeyDownLastFrame[GLFW_KEY_LAST + 1] = {};
		static bool superKeyDownData[GLFW_KEY_LAST + 1] = {};
		static bool superKeyUpData[GLFW_KEY_LAST + 1] = {};

		static float lastMouseX;
		static float lastMouseY;
		static bool firstMouse = true;
		static bool mouseButtonDownLastFrame[(uint8)MouseButton::Length] = {};
		static bool mouseButtonDown[(uint8)MouseButton::Length] = {};
		static bool mouseButtonUp[(uint8)MouseButton::Length] = {};

		void mouseCallback(GLFWwindow*, double xpos, double ypos)
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

		void mouseButtonCallback(GLFWwindow*, int button, int action, int)
		{
			mouseButtonDown[button] = action == GLFW_PRESS;
			mouseButtonUp[button] = action == GLFW_RELEASE;
		}

		void scrollCallback(GLFWwindow*, double xoffset, double yoffset)
		{
			scrollX = (float)xoffset;
			scrollY = (float)yoffset;
		}

		void endFrame()
		{
			// Copy the mouse button down states to mouse button down last frame
			g_memory_copyMem(mouseButtonDownLastFrame, mouseButtonDown, sizeof(bool) * (uint8)MouseButton::Length);

			// Copy key down states
			g_memory_copyMem(keyDownLastFrame, keyDownData, sizeof(bool) * (GLFW_KEY_LAST + 1));
			g_memory_copyMem(shiftKeyDownLastFrame, shiftKeyDownData, sizeof(bool) * (GLFW_KEY_LAST + 1));
			g_memory_copyMem(superKeyDownLastFrame, superKeyDownData, sizeof(bool) * (GLFW_KEY_LAST + 1));
			g_memory_copyMem(ctrlKeyDownLastFrame, ctrlKeyDownData, sizeof(bool) * (GLFW_KEY_LAST + 1));
			g_memory_copyMem(altKeyDownLastFrame, altKeyDownData, sizeof(bool) * (GLFW_KEY_LAST + 1));

			// Key up states need to be reset
			g_memory_zeroMem(keyUpData, sizeof(bool) * (GLFW_KEY_LAST + 1));
			g_memory_zeroMem(shiftKeyUpData, sizeof(bool) * (GLFW_KEY_LAST + 1));
			g_memory_zeroMem(ctrlKeyUpData, sizeof(bool) * (GLFW_KEY_LAST + 1));
			g_memory_zeroMem(altKeyUpData, sizeof(bool) * (GLFW_KEY_LAST + 1));
			g_memory_zeroMem(superKeyUpData, sizeof(bool) * (GLFW_KEY_LAST + 1));

			// Reset mouse states
			deltaMouseX = 0;
			deltaMouseY = 0;
			scrollX = 0;
			scrollY = 0;
		}

		void keyCallback(GLFWwindow*, int key, int, int action, int mods)
		{
			if (mods & GLFW_MOD_SHIFT)
			{
				shiftKeyDownData[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
				shiftKeyUpData[key] = action == GLFW_RELEASE;
			}

			if (mods & GLFW_MOD_ALT)
			{
				altKeyDownData[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
				altKeyUpData[key] = action == GLFW_RELEASE;
			}

			if (mods & GLFW_MOD_CONTROL)
			{
				ctrlKeyDownData[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
				ctrlKeyUpData[key] = action == GLFW_RELEASE;
			}

			if (mods & GLFW_MOD_SUPER)
			{
				superKeyDownData[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
				superKeyUpData[key] = action == GLFW_RELEASE;
			}

			keyDownData[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
			keyUpData[key] = action == GLFW_RELEASE;
		}

		bool keyPressed(int key, KeyMods mods)
		{
			if (key >= 0 && key <= GLFW_KEY_LAST)
			{
				bool res = keyDownLastFrame[key] && !keyDownData[key];
				bool shiftMod = shiftKeyDownLastFrame[key] || shiftKeyDownData[key];
				bool ctrlMod = ctrlKeyDownLastFrame[key] || ctrlKeyDownData[key];
				bool altMod = altKeyDownLastFrame[key] || altKeyDownData[key];
				bool superMod = superKeyDownLastFrame[key] || superKeyDownData[key];

				if (mods == KeyMods::None)
					return res && !shiftMod && !ctrlMod && !altMod && !superMod;
				
				res = (uint8)(mods & KeyMods::Shift) ? res && shiftMod : res && !shiftMod;
				res = (uint8)(mods & KeyMods::Alt) ? res && altMod : res && !altMod;
				res = (uint8)(mods & KeyMods::Ctrl) ? res && ctrlMod : res && !ctrlMod;
				res = (uint8)(mods & KeyMods::Super) ? res && superMod : res && !superMod;

				return res;
			}
			else
			{
				g_logger_error("Cannot check if key '%d' is pressed. Key is out of range of known key codes.", key);
			}

			return false;
		}

		bool keyDown(int key, KeyMods mods)
		{
			if (key >= 0 && key <= GLFW_KEY_LAST)
			{
				bool res = keyDownData[key];
				bool shiftMod = shiftKeyDownData[key];
				bool ctrlMod = ctrlKeyDownData[key];
				bool altMod = altKeyDownData[key];
				bool superMod = superKeyDownData[key];

				if (mods == KeyMods::None)
					return res && !shiftMod && !ctrlMod && !altMod && !superMod;

				res = (uint8)(mods & KeyMods::Shift) ? res && shiftMod : res && !shiftMod;
				res = (uint8)(mods & KeyMods::Alt) ? res && altMod : res && !altMod;
				res = (uint8)(mods & KeyMods::Ctrl) ? res && ctrlMod : res && !ctrlMod;
				res = (uint8)(mods & KeyMods::Super) ? res && superMod : res && !superMod;

				return res;
			}
			else
			{
				g_logger_error("Cannot check if key '%d' is down. Key is out of range of known key codes.", key);
			}

			return false;
		}

		bool keyUp(int key, KeyMods mods)
		{
			if (key >= 0 && key <= GLFW_KEY_LAST)
			{
				bool res = keyUpData[key];
				bool shiftMod = shiftKeyUpData[key];
				bool ctrlMod = ctrlKeyUpData[key];
				bool altMod = altKeyUpData[key];
				bool superMod = superKeyUpData[key];

				if (mods == KeyMods::None)
					return res && !shiftMod && !ctrlMod && !altMod && !superMod;

				res = (uint8)(mods & KeyMods::Shift) ? res && shiftMod : res && !shiftMod;
				res = (uint8)(mods & KeyMods::Alt) ? res && altMod : res && !altMod;
				res = (uint8)(mods & KeyMods::Ctrl) ? res && ctrlMod : res && !ctrlMod;
				res = (uint8)(mods & KeyMods::Super) ? res && superMod : res && !superMod;

				return res;
			}
			else
			{
				g_logger_error("Cannot check if key '%d' is up. Key is out of range of known key codes.", key);
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