#include "core.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Fonts.h"
#include "animation/Styles.h"

namespace MathAnim
{
	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;
		uint32 textureId;
		glm::vec2 textureCoords;
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
		static const uint32 numTextureGraphicsIds = 8;
		static uint32 numFontTextures = 0;
		static Texture textureGraphicIds[numTextureGraphicsIds];
		static int32 uTextures[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

		static uint32 getTexId(const Texture& texture)
		{
			for (int i = 0; i < numTextureGraphicsIds; i++)
			{
				if (texture.graphicsId == textureGraphicIds[i].graphicsId || i >= numFontTextures)
				{
					textureGraphicIds[i].graphicsId = texture.graphicsId;
					numFontTextures++;
					return i + 1;
				}
			}

			g_logger_warning("Could not find texture id in Renderer::drawTexture.");
			return 0;
		}

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
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, position)));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, color)));
			glEnableVertexAttribArray(1);

			glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)(offsetof(Vertex, textureId)));
			glEnableVertexAttribArray(2);

			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, textureCoords)));
			glEnableVertexAttribArray(3);
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

		void init(OrthoCamera& sceneCamera)
		{
			camera = &sceneCamera;
			numVertices = 0;

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

			// Initialize default shader
			shader.compile("assets/shaders/default.glsl");
			screenShader.compile("assets/shaders/screen.glsl");

			setupBatchedVao();
			setupScreenVao();

			for (int i = 0; i < numTextureGraphicsIds; i++)
			{
				textureGraphicIds[i] = {0};
			}
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
			texture.bind();
			screenShader.uploadInt("uTexture", 0);

			glBindVertexArray(screenVao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		void drawSquare(const glm::vec2& start, const glm::vec2& size, const Style& style)
		{
			drawLine(start, start + glm::vec2(size.x, 0), style);
			drawLine(start + glm::vec2(0, size.y), start + size, style);
			drawLine(start, start + glm::vec2(0, size.y), style);
			drawLine(start + glm::vec2(size.x, 0), start + size, style);
		}

		void drawFilledSquare(const glm::vec2& start, const glm::vec2& size, const Style& style)
		{
			if (numVertices + 6 >= maxNumVerticesPerBatch)
			{
				flushBatch();
			}

			// Triangle 1
			// "Bottom-left" corner of line
			vertices[numVertices].position = start;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Left" corner of line
			vertices[numVertices].position = start + glm::vec2(0, size.y);
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = start + size;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// Triangle 2
			// "Bottom-left" corner of line
			vertices[numVertices].position = start;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Bottom-Right" corner of line
			vertices[numVertices].position = start + glm::vec2(size.x, 0);
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = start + size;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;
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

			// Triangle 1
			// "Bottom-left" corner of line
			vertices[numVertices].position = start + (perpVector * style.strokeWidth * 0.5f);
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Left" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position + direction;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position - (perpVector * style.strokeWidth);
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// Triangle 2
			// "Bottom-left" corner of line
			vertices[numVertices].position = start + (perpVector * style.strokeWidth * 0.5f);
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Bottom-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position - (perpVector * style.strokeWidth);
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position + direction;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;
		}

		void drawTexture(const RenderableTexture& renderable, const glm::vec3& color)
		{
			if (numVertices + 6 >= maxNumVerticesPerBatch)
			{
				flushBatch();
			}

			uint32 texId = getTexId(*renderable.texture);

			// Triangle 1
			// "Bottom-left" corner
			vertices[numVertices].position = renderable.start;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart;
			numVertices++;

			// "Top-Left" corner
			vertices[numVertices].position = renderable.start + glm::vec2(0, renderable.size.y);
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart + glm::vec2(0, renderable.texCoordSize.y);
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = renderable.start + renderable.size;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart + renderable.texCoordSize;
			numVertices++;

			// Triangle 2
			// "Bottom-left" corner of line
			vertices[numVertices].position = renderable.start;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart;
			numVertices++;

			// "Bottom-Right" corner of line
			vertices[numVertices].position = renderable.start + glm::vec2(renderable.size.x, 0);
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart + glm::vec2(renderable.texCoordSize.x, 0);
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = renderable.start + renderable.size;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart + renderable.texCoordSize;
			numVertices++;
		}

		void drawString(const std::string& string, const Font& font, const glm::vec2& position, float scale, const glm::vec3& color)
		{
			float x = position.x;
			float y = position.y;
			
			for (int i = 0; i < string.length(); i++)
			{
				char c = string[i];
				RenderableChar renderableChar = font.getCharInfo(c);
				float charWidth = renderableChar.texCoordSize.x * font.fontSize * scale;
				float charHeight = renderableChar.texCoordSize.y * font.fontSize * scale;
				float adjustedY = y - renderableChar.bearingY * font.fontSize * scale;

				drawTexture(RenderableTexture{
					&font.texture,
					{ x, adjustedY },
					{ charWidth, charHeight },
					renderableChar.texCoordStart,
					renderableChar.texCoordSize
				}, color);

				char nextC = i < string.length() - 1 ? string[i + 1] : '\0';
				//x += font.getKerning(c, nextC) * scale * font.fontSize;
				x += renderableChar.advance.x * scale * font.fontSize;
			}
		}

		void flushBatch()
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

			shader.bind();
			shader.uploadMat4("uProjection", camera->calculateProjectionMatrix());
			shader.uploadMat4("uView", camera->calculateViewMatrix());

			for (int i = 0; i < numTextureGraphicsIds; i++)
			{
				if (textureGraphicIds[i].graphicsId != 0)
				{
					glActiveTexture(GL_TEXTURE0 + i);
					textureGraphicIds[i].bind();
				}
			}
			shader.uploadIntArray("uFontTextures[0]", 8, uTextures);

			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, maxNumTrianglesPerBatch * 3, GL_UNSIGNED_INT, NULL);

			// Clear the batch
			memset(&vertices, 0, sizeof(Vertex) * maxNumVerticesPerBatch);
			numVertices = 0;
			numFontTextures = 0;
		}

		void clearColor(const glm::vec4& color)
		{
			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	}
}
