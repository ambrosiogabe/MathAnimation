#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "renderer/GLApi.h"
#include "platform/Platform.h"

namespace MathAnim
{
	static void resizeCallback(GLFWwindow* windowPtr, int newWidth, int newHeight)
	{
		Window* userWindow = (Window*)glfwGetWindowUserPointer(windowPtr);
		userWindow->width = newWidth;
		userWindow->height = newHeight;
		GL::viewport(0, 0, newWidth, newHeight);
	}

	Window::Window(int inWidth, int inHeight, const char* inTitle, WindowFlags flags)
		: width(inWidth), height(inHeight), title(inTitle)
	{
		// Minimum required version
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
		glfwWindowHint(GLFW_SAMPLES, 4);
		if (flags & WindowFlags::OpenMaximized)
		{
			glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
		}

		windowPtr = (void*)glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (windowPtr == nullptr)
		{
			glfwTerminate();
			g_logger_error("Window creation failed.");
			return;
		}
		g_logger_info("GLFW window created");

		glfwSetWindowUserPointer((GLFWwindow*)windowPtr, (void*)this);
		makeContextCurrent();

		glfwSetCursorPosCallback((GLFWwindow*)windowPtr, Input::mouseCallback);
		glfwSetKeyCallback((GLFWwindow*)windowPtr, Input::keyCallback);
		glfwSetMouseButtonCallback((GLFWwindow*)windowPtr, Input::mouseButtonCallback);
		glfwSetScrollCallback((GLFWwindow*)windowPtr, Input::scrollCallback);
		glfwSetFramebufferSizeCallback((GLFWwindow*)windowPtr, resizeCallback);

		if (flags & WindowFlags::OpenMaximized)
		{
			// Set width/height to actual window size
			glfwGetWindowSize((GLFWwindow*)windowPtr, &width, &height);
		}
	}

	void Window::setCursorMode(CursorMode cursorMode)
	{
		int glfwCursorMode =
			cursorMode == CursorMode::Locked ? GLFW_CURSOR_DISABLED :
			cursorMode == CursorMode::Normal ? GLFW_CURSOR_NORMAL :
			cursorMode == CursorMode::Hidden ? GLFW_CURSOR_HIDDEN :
			GLFW_CURSOR_HIDDEN;

		glfwSetInputMode((GLFWwindow*)windowPtr, GLFW_CURSOR, glfwCursorMode);
	}

	bool Window::setCursorPos(const Vec2i& screenCoordinates) const
	{
		if (screenCoordinates.x > this->width || screenCoordinates.x < 0) 
		{
			return false;
		}

		if (screenCoordinates.y > this->height || screenCoordinates.y < 0)
		{
			return false;
		}

		if (screenCoordinates.x == (int)Input::mouseX && screenCoordinates.y == (int)Input::mouseY)
		{
			// NOP;
			return true;
		}

		// Tell our mouse callback about this event to avoid lag in processing
		Input::mouseCallback((GLFWwindow*)this->windowPtr, (double)screenCoordinates.x, (double)screenCoordinates.y);
		return Platform::setCursorPos(*this, screenCoordinates);
	}

	void Window::makeContextCurrent()
	{
		glfwMakeContextCurrent((GLFWwindow*)windowPtr);
	}

	void Window::pollInput()
	{
		Input::endFrame();
		glfwPollEvents();
	}

	bool Window::shouldClose()
	{
		return glfwWindowShouldClose((GLFWwindow*)windowPtr);
	}

	void Window::swapBuffers()
	{
		glfwSwapBuffers((GLFWwindow*)windowPtr);
	}

	void Window::setVSync(bool on)
	{
		if (on)
		{
			glfwSwapInterval(1);
		}
		else
		{
			glfwSwapInterval(0);
		}
	}

	void Window::setTitle(const std::string& newTitle)
	{
		glfwSetWindowTitle((GLFWwindow*)windowPtr, newTitle.c_str());
	}

	float Window::getContentScale() const
	{
		float xScale, yScale;
		glfwGetWindowContentScale((GLFWwindow*)windowPtr, &xScale, &yScale);

		return xScale;
	}

	void Window::close()
	{
		glfwSetWindowShouldClose((GLFWwindow*)windowPtr, true);
	}

	void Window::update(float)
	{

	}

	glm::ivec2 Window::getMonitorWorkingSize()
	{
		glm::ivec2 ret;
		glm::ivec2 pos;
		glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &pos.x, &pos.y, &ret.x, &ret.y);
		return ret;
	}

	void Window::cleanup()
	{
		// Clean up
		glfwTerminate();
	}
}
