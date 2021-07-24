#include "core.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/Shader.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace Renderer
	{
		struct Vertex
		{
			glm::vec2 position;
			glm::vec3 color;
		};

		static uint32 vao;
		static uint32 vbo;
		static uint32 numVertices;

		static const int maxNumTrianglesPerBatch = 100;
		static const int maxNumVerticesPerBatch = maxNumTrianglesPerBatch * 3;
		static Vertex vertices[maxNumVerticesPerBatch];
		static OrthoCamera* camera;
		static Shader shader;

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

			// Initialize default shader
			shader.compile("C:/dev/C++/MathAnimations/assets/shaders/default.glsl");

			// 1. Buffer the data
			vao;
			glCreateVertexArrays(1, &vao);
			glBindVertexArray(vao);

			vbo;
			glGenBuffers(1, &vbo);

			// 1a. copy our vertices array in a buffer for OpenGL to use
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

			// 1b. then set our vertex attributes pointers
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, color)));
			glEnableVertexAttribArray(1);
		}

		void render()
		{
			flushBatch();
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
