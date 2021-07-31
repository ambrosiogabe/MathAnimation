#include "core.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "animation/Styles.h"

namespace MathAnim
{
	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;
	};

	namespace Renderer
	{
		// Internal variables
		static uint32 vao;
		static uint32 vbo;
		static uint32 numVertices;

		static const int maxNumTrianglesPerBatch = 100;
		static const int maxNumVerticesPerBatch = maxNumTrianglesPerBatch * 3;
		static Vertex vertices[maxNumVerticesPerBatch];
		static OrthoCamera* camera;
		static Shader shader;
		static Shader screenShader;

		// Default screen rectangle
		static float defaultScreenQuad[] = {
			-1.0f, -1.0f,   0.0f, 0.0f, // Bottom-left
			 1.0f,  1.0f,   1.0f, 1.0f, // Top-right
			-1.0f,  1.0f,   0.0f, 1.0f, // Top-left

			-1.0f, -1.0f,   0.0f, 0.0f, // Bottom-left
			 1.0f, -1.0f,   1.0f, 0.0f, // Bottom-right
			 1.0f,  1.0f,   1.0f, 1.0f  // Top-right
		};

		static uint32 screenVao;

		static void setupScreenVao()
		{
			// Create the screen vao
			glCreateVertexArrays(1, &screenVao);
			glBindVertexArray(screenVao);

			uint32 screenVbo;
			glGenBuffers(1, &screenVbo);

			// Allocate space for the screen vao
			glBindBuffer(GL_ARRAY_BUFFER, screenVbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(defaultScreenQuad), defaultScreenQuad, GL_STATIC_DRAW);

			// Set up the screen vao attributes
			// The position doubles as the texture coordinates so we can use the same floats for that
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
			glEnableVertexAttribArray(1);
		}

		static void setupBatchedVao()
		{
			// Create the batched vao
			glCreateVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vbo);

			// Allocate space for the batched vao
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * maxNumVerticesPerBatch, NULL, GL_DYNAMIC_DRAW);

			uint32 ebo;
			glGenBuffers(1, &ebo);

			std::array<uint32, maxNumTrianglesPerBatch * 3> elements;
			for (int i = 0; i < elements.size(); i += 3)
			{
				elements[i] = i + 0;
				elements[i + 1] = i + 1;
				elements[i + 2] = i + 2;
			}
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * maxNumTrianglesPerBatch * 3, elements.data(), GL_DYNAMIC_DRAW);

			// Set up the batched vao attributes
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, color)));
			glEnableVertexAttribArray(1);
		}

		static void GLAPIENTRY
			messageCallback(GLenum source,
				GLenum type,
				GLuint id,
				GLenum severity,
				GLsizei length,
				const GLchar* message,
				const void* userParam)
		{
			if (type == GL_DEBUG_TYPE_ERROR)
			{
				Logger::Error("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
					(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
					type, severity, message);

				GLenum err;
				while ((err = glGetError()) != GL_NO_ERROR)
				{
					Logger::Error("Error Code: 0x%8x", err);
				}
			}
		}

		void init(OrthoCamera& sceneCamera)
		{
			camera = &sceneCamera;
			numVertices = 0;

			// Load OpenGL functions using Glad
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				Logger::Error("Failed to initialize glad.");
				return;
			}
			Logger::Info("GLAD initialized.");
			Logger::Info("Hello OpenGL %d.%d", GLVersion.major, GLVersion.minor);

#ifdef _DEBUG
			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(messageCallback, 0);
#endif

			// Initialize default shader
			shader.compile("assets/shaders/default.glsl");
			screenShader.compile("assets/shaders/screen.glsl");

			setupBatchedVao();
			setupScreenVao();
		}

		void render()
		{
			flushBatch();
		}

		void renderFramebuffer(const Framebuffer& framebuffer)
		{
			screenShader.bind();

			const Texture& texture = framebuffer.getColorAttachment(0);
			glActiveTexture(GL_TEXTURE0);
			TextureUtil::bind(texture);
			screenShader.uploadInt("uTexture", 0);

			glBindVertexArray(screenVao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		void drawLine(const glm::vec2& start, const glm::vec2& end, const Style& style)
		{
			if (numVertices + 6 >= maxNumVerticesPerBatch)
			{
				flushBatch();
			}

			glm::vec2 direction = end - start;
			glm::vec2 normalDirection = glm::normalize(direction);
			glm::vec2 perpVector = glm::normalize(glm::vec2(normalDirection.y, -normalDirection.x));

			// "Bottom-left" corner of line
			vertices[numVertices].position = start + (perpVector * style.strokeWidth * 0.5f);
			vertices[numVertices].color = style.color;
			numVertices++;

			// Triangle 1
			// "Top-Left" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position + direction;
			vertices[numVertices].color = style.color;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position - (perpVector * style.strokeWidth);
			vertices[numVertices].color = style.color;
			numVertices++;

			// Triangle 2
			// "Bottom-left" corner of line
			vertices[numVertices].position = start + (perpVector * style.strokeWidth * 0.5f);
			vertices[numVertices].color = style.color;
			numVertices++;

			// "Bottom-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position - (perpVector * style.strokeWidth);
			vertices[numVertices].color = style.color;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position + direction;
			vertices[numVertices].color = style.color;
			numVertices++;
		}

		void flushBatch()
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

			shader.bind();
			shader.uploadMat4("uProjection", camera->calculateProjectionMatrix());
			shader.uploadMat4("uView", camera->calculateViewMatrix());

			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, maxNumTrianglesPerBatch * 3, GL_UNSIGNED_INT, NULL);

			// Clear the batch
			memset(&vertices, 0, sizeof(Vertex) * maxNumVerticesPerBatch);
			numVertices = 0;
		}

		void clearColor(const glm::vec4& color)
		{
			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	}
}
