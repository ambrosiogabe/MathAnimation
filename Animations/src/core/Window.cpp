#include "core.h"
#include "core/Window.h"
#include "core/Input.h"

namespace MathAnim
{
	static void resizeCallback(GLFWwindow* windowPtr, int newWidth, int newHeight)
	{
		Window* userWindow = (Window*)glfwGetWindowUserPointer(windowPtr);
		userWindow->width = newWidth;
		userWindow->height = newHeight;
		glViewport(0, 0, newWidth, newHeight);
	}

	Window::Window(int width, int height, const char* title)
		: width(width), height(height), title(title)
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_SAMPLES, 4);

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

	void Window::update(float dt)
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
