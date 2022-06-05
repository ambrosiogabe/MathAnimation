#include "core.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Fonts.h"
#include "animation/Styles.h"
#include "animation/Animation.h"

#ifdef _RELEASE
#include "shaders/default.glsl.hpp"
#include "shaders/screen.glsl.hpp"
#include "shaders/vectorShader.glsl.hpp"
#endif

namespace MathAnim
{
	struct Vertex
	{
		Vec2 position;
		Vec4 color;
		uint32 textureId;
		Vec2 textureCoords;
	};

	struct VectorVertex
	{
		Vec2 position;
		Vec4 color;
		Vec2 uv;
		int32 isConcave;
	};

	namespace Renderer
	{
		// Internal variables
		static uint32 vao;
		static uint32 vbo;
		static uint32 numVertices;

		static uint32 vectorVao;
		static uint32 vectorVbo;
		static uint32 vectorNumVertices;

		static const int maxNumTrianglesPerBatch = 100;
		static const int maxNumVerticesPerBatch = maxNumTrianglesPerBatch * 3;
		static Vertex vertices[maxNumVerticesPerBatch];
		static VectorVertex vectorVertices[maxNumVerticesPerBatch];
		static OrthoCamera* camera;
		static Shader shader;
		static Shader vectorShader;
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
		static const int32 numTextureGraphicsIds = 8;
		static int32 numFontTextures = 0;
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

			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, color)));
			glEnableVertexAttribArray(1);

			glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)(offsetof(Vertex, textureId)));
			glEnableVertexAttribArray(2);

			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, textureCoords)));
			glEnableVertexAttribArray(3);
		}

		static void setupBatchedVectorVao()
		{
			// Create the batched vector vao
			glCreateVertexArrays(1, &vectorVao);
			glBindVertexArray(vectorVao);

			glGenBuffers(1, &vectorVbo);

			// Allocate space for the batched vector vao
			glBindBuffer(GL_ARRAY_BUFFER, vectorVbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(VectorVertex) * maxNumVerticesPerBatch, NULL, GL_DYNAMIC_DRAW);

			// Set up the batched vector vao attributes
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VectorVertex), (void*)(offsetof(VectorVertex, position)));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VectorVertex), (void*)(offsetof(VectorVertex, color)));
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VectorVertex), (void*)(offsetof(VectorVertex, uv)));
			glEnableVertexAttribArray(2);

			glVertexAttribIPointer(3, 1, GL_INT, sizeof(VectorVertex), (void*)(offsetof(VectorVertex, isConcave)));
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
#ifdef _DEBUG
			shader.compile("assets/shaders/default.glsl");
			screenShader.compile("assets/shaders/screen.glsl");
			vectorShader.compile("assets/shaders/vectorShader.glsl");
#elif defined(_RELEASE)
			shader.compileRaw(defaultShaderGlsl);
			screenShader.compileRaw(screenShaderGlsl);
			vectorShader.compileRaw(vectorShaderGlsl);
#endif

			setupBatchedVao();
			setupBatchedVectorVao();
			setupScreenVao();

			for (int i = 0; i < numTextureGraphicsIds; i++)
			{
				textureGraphicIds[i] = { 0 };
			}
		}

		void render()
		{
			flushBatch();
			flushVectorBatch();
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

		void drawSquare(const Vec2& start, const Vec2& size, const Style& style)
		{
			drawLine(start, start + Vec2{ size.x, 0 }, style);
			drawLine(start + Vec2{ 0, size.y }, start + size, style);
			drawLine(start, start + Vec2{ 0, size.y }, style);
			drawLine(start + Vec2{ size.x, 0 }, start + size, style);
		}

		void drawFilledSquare(const Vec2& start, const Vec2& size, const Style& style)
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
			vertices[numVertices].position = start + Vec2{ 0, size.y };
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
			vertices[numVertices].position = start + Vec2{ size.x, 0 };
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = start + size;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;
		}

		void drawLine(const Vec2& start, const Vec2& end, const Style& style)
		{
			if ((style.lineEnding == CapType::Flat && numVertices + 6 >= maxNumVerticesPerBatch) ||
				(style.lineEnding == CapType::Arrow && numVertices + 15 >= maxNumVerticesPerBatch))
			{
				flushBatch();
			}

			Vec2 direction = end - start;
			Vec2 normalDirection = CMath::normalize(direction);
			Vec2 perpVector = CMath::normalize(Vec2{ normalDirection.y, -normalDirection.x });

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

			if (style.lineEnding == CapType::Arrow)
			{
				// Add arrow tip
				Vec2 centerDot = end + (normalDirection * style.strokeWidth * 0.5f);
				Vec2 vectorToCenter = CMath::normalize(centerDot - (end - perpVector * style.strokeWidth * 0.5f));
				Vec2 oVectorToCenter = CMath::normalize(centerDot - (end + perpVector * style.strokeWidth * 0.5f));
				Vec2 bottomLeft = centerDot - vectorToCenter * style.strokeWidth * 4.0f;
				Vec2 bottomRight = centerDot - oVectorToCenter * style.strokeWidth * 4.0f;
				Vec2 top = centerDot + normalDirection * style.strokeWidth * 4.0f;

				// Left Triangle
				vertices[numVertices].position = centerDot;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = bottomLeft;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = top;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				// Right triangle
				vertices[numVertices].position = top;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = centerDot;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = bottomRight;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				// Center triangle
				vertices[numVertices].position = centerDot;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = end + perpVector * style.strokeWidth * 0.5f;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = end - perpVector * style.strokeWidth * 0.5f;
				vertices[numVertices].color = style.color;
				vertices[numVertices].textureId = 0;
				numVertices++;
			}
		}

		void drawFilledCircle(const Vec2& position, float radius, int numSegments, const Style& style)
		{
			float t = 0;
			float sectorSize = 360.0f / (float)numSegments;
			for (int i = 0; i < numSegments; i++)
			{
				float x = radius * glm::cos(glm::radians(t));
				float y = radius * glm::sin(glm::radians(t));
				float nextT = t + sectorSize;
				float nextX = radius * glm::cos(glm::radians(nextT));
				float nextY = radius * glm::sin(glm::radians(nextT));

				drawFilledTriangle(position, position + Vec2{ x, y }, position + Vec2{ nextX, nextY }, style);

				t += sectorSize;
			}
		}

		void drawFilledTriangle(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Style& style)
		{
			if (numVertices + 3 >= maxNumVerticesPerBatch)
			{
				flushBatch();
			}

			// One triangle per sector
			vertices[numVertices].position = p0;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			vertices[numVertices].position = p1;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			vertices[numVertices].position = p2;
			vertices[numVertices].color = style.color;
			vertices[numVertices].textureId = 0;
			numVertices++;
		}

		void drawBezier(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Style& style)
		{
			if (vectorNumVertices + 6 >= maxNumVerticesPerBatch)
			{
				flushVectorBatch();
			}

			glm::vec2 triangleCenter = (glm::vec2(p0.x, p0.y) + glm::vec2(p1.x, p1.y) + glm::vec2(p2.x, p2.y)) / 3.0f;

			float halfStrokeWidth = style.strokeWidth / 2.0f;

			glm::vec2 localP0 = glm::vec2(p0.x, p0.y) - triangleCenter;
			glm::vec2 localP0Dir = glm::normalize(localP0);
			glm::vec2 outerP0 = localP0 + (localP0Dir * halfStrokeWidth);
			glm::vec2 innerP0 = localP0 - (localP0Dir * halfStrokeWidth);
			outerP0 += triangleCenter;
			innerP0 += triangleCenter;

			glm::vec2 localP1 = glm::vec2(p1.x, p1.y) - triangleCenter;
			glm::vec2 localP1Dir = glm::normalize(localP1);
			glm::vec2 outerP1 = localP1 + (localP1Dir * halfStrokeWidth);
			glm::vec2 innerP1 = localP1 - (localP1Dir * halfStrokeWidth);
			outerP1 += triangleCenter;
			innerP1 += triangleCenter;

			glm::vec2 localP2 = glm::vec2(p2.x, p2.y) - triangleCenter;
			glm::vec2 localP2Dir = glm::normalize(localP2);
			glm::vec2 outerP2 = localP2 + (localP2Dir * halfStrokeWidth);
			glm::vec2 innerP2 = localP2 - (localP2Dir * halfStrokeWidth);
			outerP2 += triangleCenter;
			innerP2 += triangleCenter;

			// Do two triangles, one for the outer stroke and one for inner stroke
			vectorVertices[vectorNumVertices].position = { outerP0.x, outerP0.y };
			vectorVertices[vectorNumVertices].color = style.color;
			vectorVertices[vectorNumVertices].uv = { 0.0f, 0.0f };
			vectorVertices[vectorNumVertices].isConcave = false;
			vectorNumVertices++;

			vectorVertices[vectorNumVertices].position = { outerP1.x, outerP1.y };
			vectorVertices[vectorNumVertices].color = style.color;
			vectorVertices[vectorNumVertices].uv = { 0.5f, 0.0f };
			vectorVertices[vectorNumVertices].isConcave = false;
			vectorNumVertices++;

			vectorVertices[vectorNumVertices].position = { outerP2.x, outerP2.y };
			vectorVertices[vectorNumVertices].color = style.color;
			vectorVertices[vectorNumVertices].uv = { 1.0f, 1.0f };
			vectorVertices[vectorNumVertices].isConcave = false;
			vectorNumVertices++;

			// Inner triangle
			vectorVertices[vectorNumVertices].position = { innerP0.x, innerP0.y };
			vectorVertices[vectorNumVertices].color = style.color;
			vectorVertices[vectorNumVertices].uv = { 0.0f, 0.0f };
			vectorVertices[vectorNumVertices].isConcave = true;
			vectorNumVertices++;

			vectorVertices[vectorNumVertices].position = { innerP1.x, innerP1.y };
			vectorVertices[vectorNumVertices].color = style.color;
			vectorVertices[vectorNumVertices].uv = { 0.5f, 0.0f };
			vectorVertices[vectorNumVertices].isConcave = true;
			vectorNumVertices++;

			vectorVertices[vectorNumVertices].position = { innerP2.x, innerP2.y };
			vectorVertices[vectorNumVertices].color = style.color;
			vectorVertices[vectorNumVertices].uv = { 1.0f, 1.0f };
			vectorVertices[vectorNumVertices].isConcave = true;
			vectorNumVertices++;
		}

		void drawCubicBezier(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, const Style& style)
		{
			float maxX = glm::max(p0.x, glm::max(p1.x, glm::max(p2.x, p3.x)));
			float minX = glm::min(p0.x, glm::min(p1.x, glm::min(p2.x, p3.x)));
			float maxY = glm::max(p0.y, glm::max(p1.y, glm::max(p2.y, p3.y)));
			float minY = glm::min(p0.y, glm::min(p1.y, glm::min(p2.y, p3.y)));

			const glm::vec3 b0 = glm::vec3((p0.x - minX) / (maxX - minX), (p0.y - minY) / (maxY - minY), 1.0f);
			const glm::vec3 b1 = glm::vec3((p1.x - minX) / (maxX - minX), (p1.y - minY) / (maxY - minY), 1.0f);
			const glm::vec3 b2 = glm::vec3((p2.x - minX) / (maxX - minX), (p2.y - minY) / (maxY - minY), 1.0f);
			const glm::vec3 b3 = glm::vec3((p3.x - minX) / (maxX - minX), (p3.y - minY) / (maxY - minY), 1.0f);

			//const glm::mat4 matrixBasis3 = glm::transpose(glm::mat4(
			//	glm::vec4(1, 0, 0, 0),
			//	glm::vec4(-3, 3, 0, 0),
			//	glm::vec4(3, -6, 3, 0),
			//	glm::vec4(-1, 3, -3, 1)
			//));

			//const glm::mat4 matrixBasis3Inverse = glm::transpose(glm::mat4(
			//	glm::vec4(1, 0, 0, 0),
			//	glm::vec4(1, 1.0f / 3.0f, 0, 0),
			//	glm::vec4(1, 2.0f / 3.0f, 1.0f / 3.0f, 0),
			//	glm::vec4(1, 1, 1, 1)
			//));

			//// NOTE: For now all w's are implicitly 1
			//glm::vec3 powerBasis0 = glm::vec3(matrixBasis3 * glm::vec4(p0.x, p0.y, 1.0f, 1.0f));
			//glm::vec3 powerBasis1 = glm::vec3(matrixBasis3 * glm::vec4(p1.x, p1.y, 1.0f, 1.0f));
			//glm::vec3 powerBasis2 = glm::vec3(matrixBasis3 * glm::vec4(p2.x, p2.y, 1.0f, 1.0f));
			//glm::vec3 powerBasis3 = glm::vec3(matrixBasis3 * glm::vec4(p3.x, p3.y, 1.0f, 1.0f));

			//glm::mat3 determinantMatrix0 = glm::transpose(glm::mat3(
			//	powerBasis3,
			//	powerBasis2,
			//	powerBasis1
			//));

			//glm::mat3 determinantMatrix1 = glm::transpose(glm::mat3(
			//	powerBasis3,
			//	powerBasis2,
			//	powerBasis0
			//));

			//glm::mat3 determinantMatrix2 = glm::transpose(glm::mat3(
			//	powerBasis3,
			//	powerBasis1,
			//	powerBasis0
			//));

			//glm::mat3 determinantMatrix3 = glm::transpose(glm::mat3(
			//	powerBasis2,
			//	powerBasis1,
			//	powerBasis0
			//));

			//float d0 = glm::determinant(determinantMatrix0);
			//float d1 = -glm::determinant(determinantMatrix1);
			//float d2 = glm::determinant(determinantMatrix2);
			//float d3 = -glm::determinant(determinantMatrix3);

			float a1 = glm::dot(b0, glm::cross(b3, b2));
			float a2 = glm::dot(b1, glm::cross(b0, b3));
			float a3 = glm::dot(b2, glm::cross(b1, b1));

			float delta1 = a1 - (2.0f * a2) + (3.0f * a3);
			float delta2 = -a2 + (3.0f * a3);
			float delta3 = 3.0f * a3;

			// Determine the number of real roots this cubic bezier curve has
			//float delta1 = d0 * d2 - d1 * d1;
			//float delta2 = d1 * d2 - d0 * d3;
			//float delta3 = d1 * d3 - d2 * d2;
			//float discriminant = 4 * delta1 * delta3 - delta2 * delta2;
			float discriminant = (delta1 * delta1) * ((3.0f * delta2 * delta2) - (4.0f * delta1 * delta3));

			// First check for quadratic or line or point
			constexpr float epsilonComparison = 0.01f;
			if (glm::epsilonEqual(delta1, delta2, epsilonComparison) &&
				glm::epsilonEqual(delta2, 0.0f, epsilonComparison))
			{
				// d1 = d2 = 0.0

				if (glm::all(glm::epsilonEqual(b0, b1, epsilonComparison)) &&
					glm::all(glm::epsilonEqual(b1, b2, epsilonComparison)) &&
					glm::all(glm::epsilonEqual(b2, b3, epsilonComparison)))
				{
					// Case 6: Point
					// b0 = b1 = b2 = b3
					g_logger_info("We have a point.");
					return;
				}
				else if (glm::epsilonEqual(delta1, delta2, epsilonComparison) &&
					glm::epsilonEqual(delta2, delta3, epsilonComparison) &&
					glm::epsilonEqual(delta3, 0.0f, epsilonComparison))
				{
					// Case 5: Line
					// d1 = d2 = d3 = 0
					g_logger_info("We have a line.");
					return;
				}
				else
				{
					// Case 4: Quadratic
					// d1 = d2 = 0.0
					g_logger_info("We have a quadratic.");
					return;
				}
			}
			else if (discriminant > 0.0f)
			{
				// Case 1: The Serpentine
				g_logger_info("We have a serpentine.");
			}
			else if (discriminant < 0.0f)
			{
				// Case 2: The Loop
				g_logger_info("We have a loop.");
			}
			else if (glm::epsilonEqual(discriminant, 0.0f, epsilonComparison))
			{
				// Case 3: Cusp
				g_logger_info("We have a cusp.");
			}
		}

		void drawTexture(const RenderableTexture& renderable, const Vec4& color)
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
			vertices[numVertices].position = renderable.start + Vec2{ 0, renderable.size.y };
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart + Vec2{ 0, renderable.texCoordSize.y };
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
			vertices[numVertices].position = renderable.start + Vec2{ renderable.size.x, 0 };
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart + Vec2{ renderable.texCoordSize.x, 0 };
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = renderable.start + renderable.size;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = texId;
			vertices[numVertices].textureCoords = renderable.texCoordStart + renderable.texCoordSize;
			numVertices++;
		}

		void drawString(const std::string& string, const Font& font, const Vec2& position, float scale, const Vec4& color)
		{
			float x = position.x;
			float y = position.y;

			//for (int i = 0; i < string.length(); i++)
			//{
			//	char c = string[i];
			//	RenderableChar renderableChar = font.getCharInfo(c);
			//	float charWidth = renderableChar.texCoordSize.x * font.fontSize * scale;
			//	float charHeight = renderableChar.texCoordSize.y * font.fontSize * scale;
			//	float adjustedY = y - renderableChar.bearingY * font.fontSize * scale;

			//	drawTexture(RenderableTexture{
			//		&font.texture,
			//		{ x, adjustedY },
			//		{ charWidth, charHeight },
			//		renderableChar.texCoordStart,
			//		renderableChar.texCoordSize
			//		}, color);

			//	char nextC = i < string.length() - 1 ? string[i + 1] : '\0';
			//	//x += font.getKerning(c, nextC) * scale * font.fontSize;
			//	x += renderableChar.advance.x * scale * font.fontSize;
			//}
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

		void flushVectorBatch()
		{
			// Flush the vector batch
			glBindBuffer(GL_ARRAY_BUFFER, vectorVbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vectorVertices), vectorVertices, GL_DYNAMIC_DRAW);

			vectorShader.bind();
			vectorShader.uploadMat4("uProjection", camera->calculateProjectionMatrix());
			vectorShader.uploadMat4("uView", camera->calculateViewMatrix());

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_SUBTRACT);

			glBindVertexArray(vectorVao);
			glDrawArrays(GL_TRIANGLES, 0, maxNumTrianglesPerBatch * 3);

			// Clear the batch
			memset(&vectorVertices, 0, sizeof(VectorVertex) * maxNumVerticesPerBatch);
			vectorNumVertices = 0;

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
		}

		void clearColor(const Vec4& color)
		{
			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}
	}
}
