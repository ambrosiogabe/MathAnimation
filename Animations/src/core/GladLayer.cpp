#include "core.h"

#include "renderer/GLApi.h"

namespace MathAnim
{
	namespace GladLayer
	{
		static void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

		void init()
		{
			// Initialize glfw first
			glfwInit();
			g_logger_info("GLFW initialized.");

			// Create dummy window to figure out what GL version we have
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

			GLFWwindow* windowPtr = glfwCreateWindow(1, 1, "Dummy", nullptr, nullptr);
			if (windowPtr == nullptr)
			{
				glfwTerminate();
				g_logger_error("Dummy window creation failed, cannot determine OpenGL version.");
				return;
			}
			glfwMakeContextCurrent((GLFWwindow*)windowPtr);
			glfwSetWindowShouldClose((GLFWwindow*)windowPtr, true);

			// Load OpenGL functions using Glad
            int version = gladLoadGL(static_cast<GLADloadfunc>(glfwGetProcAddress));

			if (version == 0)
			{
				g_logger_error("Failed to initialize glad.");
				return;
			}
			g_logger_info("GLAD initialized.");
			g_logger_info("Running OpenGL %d.%d", GLVersion.major, GLVersion.minor);
			GL::init(GLVersion.major, GLVersion.minor);

			// Destroy the dummy window now that we've patched the function pointers
			glfwDestroyWindow(windowPtr);

#ifdef _DEBUG
			GL::enable(GL_DEBUG_OUTPUT);
			GL::debugMessageCallback(messageCallback, 0);
#endif

			// Enable blending
			GL::enable(GL_BLEND);
			GL::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Enable multisampling
			GL::enable(GL_MULTISAMPLE);
		}

		void deinit()
		{
			glfwTerminate();
			g_logger_info("Glad/GLFW de-initialized.");
		}

		static void GLAPIENTRY
			messageCallback(GLenum,
				GLenum type,
				GLuint,
				GLenum severity,
				GLsizei,
				const GLchar* message,
				const void*)
		{
			if (type == GL_DEBUG_TYPE_ERROR)
			{
				g_logger_error("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
					(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
					type, severity, message);

				GLenum err;
				while ((err = GL::getError()) != GL_NO_ERROR)
				{
					g_logger_error("Error Code: 0x%8x", err);
				}
			}
		}
	}
}