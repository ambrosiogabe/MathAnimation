#include "core.h"

namespace MathAnim
{
	namespace GladLayer
	{
		static void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

		void init()
		{
			// Load OpenGL functions using Glad
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				g_logger_error("Failed to initialize glad.");
				return;
			}
			g_logger_info("GLAD initialized.");
			g_logger_info("Hello OpenGL %d.%d", GLVersion.major, GLVersion.minor);

#ifdef _DEBUG
			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(messageCallback, 0);
#endif

			// Enable blending
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Enable multisampling
			glEnable(GL_MULTISAMPLE);
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
				while ((err = glGetError()) != GL_NO_ERROR)
				{
					g_logger_error("Error Code: 0x%8x", err);
				}
			}
		}
	}
}