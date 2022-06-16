#include "core.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/PerspectiveCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Fonts.h"
#include "animation/Animation.h"
#include "core/Application.h"

#ifdef _RELEASE
#include "shaders/default.glsl.hpp"
#include "shaders/screen.glsl.hpp"
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

	struct Vertex3DLine
	{
		Vec3 currentPos;
		Vec3 previousPos;
		Vec3 nextPos;
		float thickness;
		uint32 color;
	};

	namespace Renderer
	{
		// Internal variables
		static uint32 vao;
		static uint32 vbo;
		static uint32 numVertices;

		static uint32 vao3DLines;
		static uint32 vbo3DLines;
		static uint32 numVertices3DLines;

		static constexpr int maxNumTrianglesPerBatch = 100;
		static constexpr int maxNumVerticesPerBatch = maxNumTrianglesPerBatch * 3;
		static Vertex vertices[maxNumVerticesPerBatch];
		static Vertex3DLine vertices3DLines[maxNumVerticesPerBatch];

		static OrthoCamera* orthoCamera;
		static PerspectiveCamera* perspCamera;

		static Shader shader;
		static Shader shader3DLine;
		static Shader screenShader;

		static constexpr int MAX_STACK_SIZE = 64;

		static glm::vec4 colorStack[MAX_STACK_SIZE];
		static float strokeWidthStack[MAX_STACK_SIZE];
		static CapType lineEndingStack[MAX_STACK_SIZE];

		static int colorStackPtr;
		static int strokeWidthStackPtr;
		static int lineEndingStackPtr;

		static constexpr glm::vec4 defaultColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		static constexpr float defaultStrokeWidth = 0.1f;
		static constexpr CapType defaultLineEnding = CapType::Flat;

		static constexpr int max3DPathSize = 100;
		static bool isDrawing3DPath;
		static Vertex3DLine current3DPath[max3DPathSize];
		static int numVertsIn3DPath;

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

		// ---------------------- Internal Functions ----------------------
		static uint32 getTexId(const Texture& texture);
		static void setupScreenVao();
		static void setupBatchedVao();
		static void setupVao3DLines();
		static void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

		void init(OrthoCamera& inOrthoCamera, PerspectiveCamera& inPerspCamera)
		{
			orthoCamera = &inOrthoCamera;
			perspCamera = &inPerspCamera;
			numVertices = 0;

			strokeWidthStackPtr = 0;
			colorStackPtr = 0;
			lineEndingStackPtr = 0;
			isDrawing3DPath = false;
			numVertsIn3DPath = 0;

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
			shader3DLine.compile("assets/shaders/shader3DLine.glsl");
#elif defined(_RELEASE)
			shader.compileRaw(defaultShaderGlsl);
			screenShader.compileRaw(screenShaderGlsl);
#endif

			setupBatchedVao();
			setupScreenVao();
			setupVao3DLines();

			for (int i = 0; i < numTextureGraphicsIds; i++)
			{
				textureGraphicIds[i] = { 0 };
			}
		}

		// ----------- Render calls ----------- 
		void render()
		{
			if (numVertices > 0)
			{
				flushBatch();
			}

			if (numVertices3DLines > 0)
			{
				flushBatch3D();
			}

			g_logger_assert(lineEndingStackPtr == 0, "Missing popLineEnding() call.");
			g_logger_assert(colorStackPtr == 0, "Missing popColor() call.");
			g_logger_assert(strokeWidthStackPtr == 0, "Missing popStrokeWidth() call.");
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

		// ----------- Styles ----------- 
		void pushStrokeWidth(float strokeWidth)
		{
			g_logger_assert(strokeWidthStackPtr < MAX_STACK_SIZE, "Ran out of room on the stroke width stack.");
			strokeWidthStack[strokeWidthStackPtr] = strokeWidth;
			strokeWidthStackPtr++;
		}

		void pushColor(const glm::vec4& color)
		{
			g_logger_assert(colorStackPtr < MAX_STACK_SIZE, "Ran out of room on the color stack.");
			colorStack[colorStackPtr] = color;
			colorStackPtr++;
		}

		void pushColor(const Vec4& color)
		{
			g_logger_assert(colorStackPtr < MAX_STACK_SIZE, "Ran out of room on the color stack.");
			colorStack[colorStackPtr] = glm::vec4(
				color.r, color.g, color.b, color.a
			);
			colorStackPtr++;
		}

		void pushLineEnding(CapType lineEnding)
		{
			g_logger_assert(lineEndingStackPtr < MAX_STACK_SIZE, "Ran out of room on the line ending stack.");
			lineEndingStack[lineEndingStackPtr] = lineEnding;
			lineEndingStackPtr++;
		}

		void popStrokeWidth(int numToPop)
		{
			strokeWidthStackPtr -= numToPop;
			g_logger_assert(strokeWidthStackPtr >= 0, "Popped to many values off of stroke width stack: %d", strokeWidthStackPtr);
		}

		void popColor(int numToPop)
		{
			colorStackPtr -= numToPop;
			g_logger_assert(colorStackPtr >= 0, "Popped to many values off of color stack: %d", colorStackPtr);
		}

		void popLineEnding(int numToPop)
		{
			lineEndingStackPtr -= numToPop;
			g_logger_assert(lineEndingStackPtr >= 0, "Popped to many values off of line ending stack: %d", lineEndingStackPtr);
		}

		// ----------- 2D stuff ----------- 
		void drawSquare(const Vec2& start, const Vec2& size)
		{
			drawLine(start, start + Vec2{ size.x, 0 });
			drawLine(start + Vec2{ 0, size.y }, start + size);
			drawLine(start, start + Vec2{ 0, size.y });
			drawLine(start + Vec2{ size.x, 0 }, start + size);
		}

		void drawFilledSquare(const Vec2& start, const Vec2& size)
		{
			if (numVertices + 6 >= maxNumVerticesPerBatch)
			{
				flushBatch();
			}

			glm::vec4 vColor = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;
			Vec4 color = Vec4{ vColor.r, vColor.g, vColor.g, vColor.a };

			// Triangle 1
			// "Bottom-left" corner of line
			vertices[numVertices].position = start;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Left" corner of line
			vertices[numVertices].position = start + Vec2{ 0, size.y };
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = start + size;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// Triangle 2
			// "Bottom-left" corner of line
			vertices[numVertices].position = start;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Bottom-Right" corner of line
			vertices[numVertices].position = start + Vec2{ size.x, 0 };
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = start + size;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;
		}

		void drawLine(const Vec2& start, const Vec2& end)
		{
			CapType lineEnding = lineEndingStackPtr > 0
				? lineEndingStack[lineEndingStackPtr - 1]
				: defaultLineEnding;
			if ((lineEnding == CapType::Flat && numVertices + 6 >= maxNumVerticesPerBatch) ||
				(lineEnding == CapType::Arrow && numVertices + 15 >= maxNumVerticesPerBatch))
			{
				flushBatch();
			}

			glm::vec4 vColor = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;
			Vec4 color = Vec4{ vColor.r, vColor.g, vColor.g, vColor.a };
			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;

			Vec2 direction = end - start;
			Vec2 normalDirection = CMath::normalize(direction);
			Vec2 perpVector = CMath::normalize(Vec2{ normalDirection.y, -normalDirection.x });

			// Triangle 1
			// "Bottom-left" corner of line
			vertices[numVertices].position = start + (perpVector * strokeWidth * 0.5f);
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Left" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position + direction;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position - (perpVector * strokeWidth);
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// Triangle 2
			// "Bottom-left" corner of line
			vertices[numVertices].position = start + (perpVector * strokeWidth * 0.5f);
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Bottom-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position - (perpVector * strokeWidth);
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			// "Top-Right" corner of line
			vertices[numVertices].position = vertices[numVertices - 1].position + direction;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			if (lineEnding == CapType::Arrow)
			{
				// Add arrow tip
				Vec2 centerDot = end + (normalDirection * strokeWidth * 0.5f);
				Vec2 vectorToCenter = CMath::normalize(centerDot - (end - perpVector * strokeWidth * 0.5f));
				Vec2 oVectorToCenter = CMath::normalize(centerDot - (end + perpVector * strokeWidth * 0.5f));
				Vec2 bottomLeft = centerDot - vectorToCenter * strokeWidth * 4.0f;
				Vec2 bottomRight = centerDot - oVectorToCenter * strokeWidth * 4.0f;
				Vec2 top = centerDot + normalDirection * strokeWidth * 4.0f;

				// Left Triangle
				vertices[numVertices].position = centerDot;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = bottomLeft;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = top;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				// Right triangle
				vertices[numVertices].position = top;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = centerDot;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = bottomRight;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				// Center triangle
				vertices[numVertices].position = centerDot;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = end + perpVector * strokeWidth * 0.5f;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;

				vertices[numVertices].position = end - perpVector * strokeWidth * 0.5f;
				vertices[numVertices].color = color;
				vertices[numVertices].textureId = 0;
				numVertices++;
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

			g_logger_warning("TODO: Not implemented.");

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

		void drawFilledCircle(const Vec2& position, float radius, int numSegments)
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

				drawFilledTriangle(position, position + Vec2{ x, y }, position + Vec2{ nextX, nextY });

				t += sectorSize;
			}
		}

		void drawFilledTriangle(const Vec2& p0, const Vec2& p1, const Vec2& p2)
		{
			if (numVertices + 3 >= maxNumVerticesPerBatch)
			{
				flushBatch();
			}

			glm::vec4 vColor = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;
			Vec4 color = Vec4{ vColor.r, vColor.g, vColor.g, vColor.a };

			vertices[numVertices].position = p0;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			vertices[numVertices].position = p1;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;

			vertices[numVertices].position = p2;
			vertices[numVertices].color = color;
			vertices[numVertices].textureId = 0;
			numVertices++;
		}

		// ----------- 3D stuff ----------- 
		// TODO: Consider just making these glm::vec3's. I'm not sure what kind
		// of impact, if any that will have
		void beginPath3D(const Vec3& start)
		{
			g_logger_assert(!isDrawing3DPath, "beginPath3D() cannot be called while a path is being drawn. Did you miss a call to endPath3D()?");
			g_logger_assert(numVertsIn3DPath == 0, "Invalid 3D path. Path began with non-zero number of vertices. Did you forget to call endPath3D()?");
			isDrawing3DPath = true;

			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;

			glm::vec4 color = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;
			uint32 packedColor =
				((uint32)(color.r * 255.0f) << 24) |
				((uint32)(color.g * 255.0f) << 16) |
				((uint32)(color.b * 255.0f) << 8) |
				((uint32)(color.a * 255.0f));

			current3DPath[numVertsIn3DPath].currentPos = start;
			current3DPath[numVertsIn3DPath].color = packedColor;
			current3DPath[numVertsIn3DPath].thickness = strokeWidth;
			numVertsIn3DPath++;
		}

		void lineTo3D(const Vec3& point)
		{
			g_logger_assert(isDrawing3DPath, "lineTo3D() cannot be called without calling beginPath3D(...) first.");
			g_logger_assert(numVertsIn3DPath < max3DPathSize, "Max path size exceeded. A 3D Path can only have up to %d points.", max3DPathSize);
			
			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;

			glm::vec4 color = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;
			uint32 packedColor =
				((uint32)(color.r * 255.0f) << 24) |
				((uint32)(color.g * 255.0f) << 16) |
				((uint32)(color.b * 255.0f) << 8) |
				((uint32)(color.a * 255.0f));

			current3DPath[numVertsIn3DPath].currentPos = point;
			current3DPath[numVertsIn3DPath].color = packedColor;
			current3DPath[numVertsIn3DPath].thickness = strokeWidth;
			numVertsIn3DPath++;
		}

		void endPath3D()
		{
			for (int vert = 0; vert < numVertsIn3DPath - 1; vert++)
			{
				Vec3 currentPos = current3DPath[vert].currentPos;
				Vec3 nextPos = current3DPath[vert + 1].currentPos;
				Vec3 nextNextPos = nextPos;
				Vec3 previousPos = currentPos;
				uint32 packedColor = current3DPath[vert].color;
				float thickness = current3DPath[vert].thickness;

				if (vert > 0)
				{
					previousPos = current3DPath[vert - 1].currentPos;
				}

				if (vert < numVertsIn3DPath - 2)
				{
					nextNextPos = current3DPath[vert + 2].currentPos;
				}

				if (numVertices3DLines + 6 >= maxNumVerticesPerBatch)
				{
					flushBatch3D();
				}

				// Triangle 1
				vertices3DLines[numVertices3DLines].previousPos = previousPos;
				vertices3DLines[numVertices3DLines].nextPos = nextPos;
				vertices3DLines[numVertices3DLines].currentPos = currentPos;
				vertices3DLines[numVertices3DLines].color = packedColor;
				vertices3DLines[numVertices3DLines].thickness = -thickness;
				numVertices3DLines++;

				vertices3DLines[numVertices3DLines].previousPos = previousPos;
				vertices3DLines[numVertices3DLines].nextPos = nextPos;
				vertices3DLines[numVertices3DLines].currentPos = currentPos;
				vertices3DLines[numVertices3DLines].color = packedColor;
				vertices3DLines[numVertices3DLines].thickness = thickness;
				numVertices3DLines++;

				// NOTE: This uses nextNextPos to get the second half of the line segment
				vertices3DLines[numVertices3DLines].previousPos = currentPos;
				vertices3DLines[numVertices3DLines].nextPos = nextNextPos;
				vertices3DLines[numVertices3DLines].currentPos = nextPos;
				vertices3DLines[numVertices3DLines].color = packedColor;
				vertices3DLines[numVertices3DLines].thickness = thickness;
				numVertices3DLines++;

				// Triangle 2
				vertices3DLines[numVertices3DLines].previousPos = previousPos;
				vertices3DLines[numVertices3DLines].nextPos = nextPos;
				vertices3DLines[numVertices3DLines].currentPos = currentPos;
				vertices3DLines[numVertices3DLines].color = packedColor;
				vertices3DLines[numVertices3DLines].thickness = -thickness;
				numVertices3DLines++;

				// NOTE: This uses nextNextPos to get the second half of the line segment
				vertices3DLines[numVertices3DLines].previousPos = currentPos;
				vertices3DLines[numVertices3DLines].nextPos = nextNextPos;
				vertices3DLines[numVertices3DLines].currentPos = nextPos;
				vertices3DLines[numVertices3DLines].color = packedColor;
				vertices3DLines[numVertices3DLines].thickness = thickness;
				numVertices3DLines++;

				// NOTE: This uses nextNextPos to get the second half of the line segment
				vertices3DLines[numVertices3DLines].previousPos = currentPos;
				vertices3DLines[numVertices3DLines].nextPos = nextNextPos;
				vertices3DLines[numVertices3DLines].currentPos = nextPos;
				vertices3DLines[numVertices3DLines].color = packedColor;
				vertices3DLines[numVertices3DLines].thickness = -thickness;
				numVertices3DLines++;
			}

			isDrawing3DPath = false;
			numVertsIn3DPath = 0;
		}

		// ----------- Miscellaneous ----------- 
		const OrthoCamera* getOrthoCamera()
		{
			return orthoCamera;
		}

		OrthoCamera* getMutableOrthoCamera()
		{
			return orthoCamera;
		}

		const PerspectiveCamera* get3DCamera()
		{
			return perspCamera;
		}

		PerspectiveCamera* getMutable3DCamera()
		{
			return perspCamera;
		}

		void flushBatch()
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

			shader.bind();
			shader.uploadMat4("uProjection", orthoCamera->calculateProjectionMatrix());
			shader.uploadMat4("uView", orthoCamera->calculateViewMatrix());

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
			glDrawElements(GL_TRIANGLES, maxNumVerticesPerBatch, GL_UNSIGNED_INT, NULL);

			// Clear the batch
			memset(&vertices, 0, sizeof(Vertex) * maxNumVerticesPerBatch);
			numVertices = 0;
			numFontTextures = 0;
		}

		void flushBatch3D()
		{
			glEnable(GL_DEPTH_TEST);

			glBindBuffer(GL_ARRAY_BUFFER, vbo3DLines);
			glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3DLine) * numVertices3DLines, vertices3DLines, GL_DYNAMIC_DRAW);

			shader3DLine.bind();
			shader3DLine.uploadMat4("uProjection", perspCamera->calculateProjectionMatrix());
			shader3DLine.uploadMat4("uView", perspCamera->calculateViewMatrix());
			shader3DLine.uploadFloat("uAspectRatio", Application::getOutputTargetAspectRatio());

			glBindVertexArray(vao3DLines);
			glDrawArrays(GL_TRIANGLES, 0, numVertices3DLines);

			// Clear the batch
			memset(&vertices3DLines, 0, sizeof(Vertex3DLine) * numVertices3DLines);
			numVertices3DLines = 0;

			glDisable(GL_DEPTH_TEST);
		}

		void clearColor(const Vec4& color)
		{
			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		// ---------------------- Internal Functions ----------------------
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

		static void setupVao3DLines()
		{
			// Create the batched vao
			glCreateVertexArrays(1, &vao3DLines);
			glBindVertexArray(vao3DLines);

			glGenBuffers(1, &vbo3DLines);

			// Allocate space for the batched vao
			glBindBuffer(GL_ARRAY_BUFFER, vbo3DLines);
			glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3DLine) * maxNumVerticesPerBatch, NULL, GL_DYNAMIC_DRAW);

			// Set up the batched vao attributes
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, currentPos)));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, previousPos)));
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, nextPos)));
			glEnableVertexAttribArray(2);

			glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, thickness)));
			glEnableVertexAttribArray(3);

			glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, color)));
			glEnableVertexAttribArray(4);
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
	}
}
