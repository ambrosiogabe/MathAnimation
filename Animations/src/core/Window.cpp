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
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_SAMPLES, 16);

		windowPtr = (void*)glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (windowPtr == nullptr)
		{
			glfwTerminate();
			Logger::Error("Window creation failed.");
			return;
		}
		Logger::Info("GLFW window created");

		glfwSetWindowUserPointer((GLFWwindow*)windowPtr, (void*)this);
		makeContextCurrent();

		glfwSetCursorPosCallback((GLFWwindow*)windowPtr, Input::mouseCallback);
		glfwSetKeyCallback((GLFWwindow*)windowPtr, Input::keyCallback);
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

	void Window::update(float dt)
	{

	}

	void Window::cleanup()
	{
		// Clean up
		glfwTerminate();
	}
}
