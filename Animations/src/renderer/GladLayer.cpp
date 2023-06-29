#include "renderer/GladLayer.h"
#include "core.h"
#include "renderer/GLApi.h"

#ifdef _WIN32
#define APIENTRY __stdcall 
#else 
#define APIENTRY
#endif

namespace MathAnim
{
	namespace GladLayer
	{

		static void APIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

		void initGlfw()
		{
			// Initialize glfw first
			glfwInit();
			g_logger_info("GLFW initialized.");
		}

		GlVersion init()
		{
			// Load OpenGL functions using Glad
            int version = gladLoadGL(static_cast<GLADloadfunc>(glfwGetProcAddress));

			if (version == 0)
			{
				g_logger_error("Failed to initialize glad.");
				return {0, 0};
			}
			g_logger_info("GLAD initialized.");
			GlVersion res;
			res.major = GLAD_VERSION_MAJOR(version);
			res.minor = GLAD_VERSION_MINOR(version);
			GL::init(res.major, res.minor);

#ifdef _DEBUG
			GL::enable(GL_DEBUG_OUTPUT);
			GL::debugMessageCallback(messageCallback, 0);
#endif

			// Enable blending
			GL::enable(GL_BLEND);
			GL::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Enable multisampling
			GL::enable(GL_MULTISAMPLE);

			return res;
		}

		void deinit()
		{
			glfwTerminate();
			g_logger_info("Glad/GLFW de-initialized.");
		}

#pragma warning( push )
#pragma warning( disable : 4505 )
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
				g_logger_error("GL CALLBACK: {} type = {:#010x}, severity = {:#010x}, message = {}",
					(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
					type, severity, message);

				GLenum err;
				while ((err = GL::getError()) != GL_NO_ERROR)
				{
					g_logger_error("Error Code: {:#010x}", err);
				}
			}
		}
#pragma warning( pop )
	}
}

#undef APIENTRY
