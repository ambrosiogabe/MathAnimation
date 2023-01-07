#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "renderer/GLApi.h"

namespace MathAnim
{
	static void resizeCallback(GLFWwindow* windowPtr, int newWidth, int newHeight)
	{
		Window* userWindow = (Window*)glfwGetWindowUserPointer(windowPtr);
		userWindow->width = newWidth;
		userWindow->height = newHeight;
		GL::viewport(0, 0, newWidth, newHeight);
	}

	Window::Window(int width, int height, const char* title, WindowFlags flags)
		: width(width), height(height), title(title)
	{
		if (GLVersion.major >= 4 && GLVersion.minor >= 6)
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		}
		else if (GLVersion.major >= 3 && GLVersion.minor >= 1) 
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		}
		else
		{
			g_logger_error("You are running OpenGL version less than 3.1. This app does not support GL versions less than 3.1, you can try to update your graphics drivers to see if that helps.");
			g_logger_assert(false, "GL Version %d.%d not supported.", GLVersion.major, GLVersion.minor);
		}

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
