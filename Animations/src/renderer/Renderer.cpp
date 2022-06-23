#include "core.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/PerspectiveCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Fonts.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "core/Application.h"

#include <nanovg.h>

#ifdef _RELEASE
#include "shaders/default.glsl.hpp"
#include "shaders/screen.glsl.hpp"
#endif

namespace MathAnim
{
	template<typename T>
	struct SimpleVector
	{
		T* data;
		int32 numElements;
		int32 maxCapacity;

		void init(int32 initialCapacity = 8)
		{
			numElements = 0;
			maxCapacity = initialCapacity;
			data = (T*)g_memory_allocate(sizeof(T) * initialCapacity);
		}

		void push(const T& t)
		{
			checkGrow();
			data[numElements] = t;
			numElements++;
		}

		T pop()
		{
			T res = data[numElements - 1];
			numElements--;
			return res;
		}

		inline int32 size() const
		{
			return numElements;
		}

		void softClear()
		{
			numElements = 0;
		}

		void hardClear()
		{
			numElements = 0;
			maxCapacity = 0;
			if (data)
			{
				g_memory_free(data);
			}
			data = nullptr;
		}

		void checkGrow(int numElementsToAdd = 1)
		{
			if (numElements + numElementsToAdd < maxCapacity)
			{
				return;
			}

			maxCapacity = (numElements + numElementsToAdd) * 2;
			if (!data)
			{
				data = (T*)g_memory_allocate(sizeof(T) * maxCapacity);
			}
			else
			{
				data = (T*)g_memory_realloc(data, sizeof(T) * maxCapacity);
			}
			g_logger_assert(data != nullptr, "Ran out of RAM.");
		}
	};

	struct DrawCmd
	{
		uint32 textureId;
		uint32 vertexOffset;
		uint32 indexOffset;
		uint32 elementCount;
	};

	struct Vertex2D
	{
		Vec2 position;
		Vec4 color;
		Vec2 textureCoords;
	};

	struct DrawList2D
	{
		SimpleVector<Vertex2D> vertices;
		SimpleVector<uint16> indices;
		SimpleVector<DrawCmd> drawCommands;
		SimpleVector<uint32> textureIdStack;

		uint32 vao;
		uint32 vbo;
		uint32 ebo;

		void init();

		// TODO: Add a bunch of methods like this...
		// void addRect();

		void setupGraphicsBuffers();
		void render(const Shader& shader) const;
		void reset();
		void free();
	};

	struct Vertex3DLine
	{
		Vec3 currentPos;
		Vec3 previousPos;
		Vec3 nextPos;
		float thickness;
		uint32 color;
	};

	struct DrawList3DLine
	{
		SimpleVector<Vertex3DLine> vertices;

		uint32 vao;
		uint32 vbo;

		void init();

		void addLine(const Vec3& previousPos, const Vec3& currentPos, const Vec3& nextPos, const Vec3& nextNextPos, uint32 packedColor, float thickness);

		void setupGraphicsBuffers();
		void render(const Shader& shader) const;
		void reset();
		void free();
	};

	struct Vertex3D
	{
		Vec3 position;
		Vec4 color;
		Vec2 textureCoords;
	};

	struct DrawList3D
	{
		SimpleVector<Vertex3D> vertices;
		SimpleVector<uint16> indices;
		SimpleVector<DrawCmd> drawCommands;
		SimpleVector<uint32> textureIdStack;

		uint32 vao;
		uint32 ebo;
		uint32 vbo;

		void init();
		void setupGraphicsBuffers();
		void free();
	};

	namespace Renderer
	{
		// Internal variables
		static DrawList2D drawList2D;
		static DrawList3DLine drawList3DLine;
		static DrawList3D drawList3D;

		static OrthoCamera* orthoCamera;
		static PerspectiveCamera* perspCamera;

		static Shader shader2D;
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

		// ---------------------- Internal Functions ----------------------
		static void setupScreenVao();
		static void GLAPIENTRY messageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
		static uint32 getColorCompressed();
		static const glm::vec4& getColor();
		static float getStrokeWidth();

		void init(OrthoCamera& inOrthoCamera, PerspectiveCamera& inPerspCamera)
		{
			orthoCamera = &inOrthoCamera;
			perspCamera = &inPerspCamera;

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

			// Enable multisampling
			glEnable(GL_MULTISAMPLE);

			// Initialize default shader
#ifdef _DEBUG
			shader2D.compile("assets/shaders/default.glsl");
			screenShader.compile("assets/shaders/screen.glsl");
			shader3DLine.compile("assets/shaders/shader3DLine.glsl");
#elif defined(_RELEASE)
			shader.compileRaw(defaultShaderGlsl);
			screenShader.compileRaw(screenShaderGlsl);
#endif

			drawList2D.init();
			drawList3DLine.init();
			drawList3D.init();
			setupScreenVao();
		}

		void free()
		{
			drawList2D.free();
			drawList3DLine.free();
			drawList3D.free();
		}

		// ----------- Render calls ----------- 
		void renderToFramebuffer(NVGcontext* vg, int frame, Framebuffer& framebuffer)
		{
			framebuffer.bind();

			AnimationManager::render(vg, frame, framebuffer);
			nvgEndFrame(vg);

			drawList2D.render(shader2D);
			drawList2D.reset();

			drawList3DLine.render(shader3DLine);
			drawList3DLine.reset();

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
			// TODO: Move this into the drawList2D
			//if (numVertices + 6 >= maxNumVerticesPerBatch)
			//{
			//	flushBatch();
			//}

			//glm::vec4 vColor = colorStackPtr > 0
			//	? colorStack[colorStackPtr - 1]
			//	: defaultColor;
			//Vec4 color = Vec4{ vColor.r, vColor.g, vColor.g, vColor.a };

			//// Triangle 1
			//// "Bottom-left" corner of line
			//vertices[numVertices].position = start;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// "Top-Left" corner of line
			//vertices[numVertices].position = start + Vec2{ 0, size.y };
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// "Top-Right" corner of line
			//vertices[numVertices].position = start + size;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// Triangle 2
			//// "Bottom-left" corner of line
			//vertices[numVertices].position = start;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// "Bottom-Right" corner of line
			//vertices[numVertices].position = start + Vec2{ size.x, 0 };
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// "Top-Right" corner of line
			//vertices[numVertices].position = start + size;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;
		}

		void drawLine(const Vec2& start, const Vec2& end)
		{
			// TODO: Move this into DrawList2D
			//CapType lineEnding = lineEndingStackPtr > 0
			//	? lineEndingStack[lineEndingStackPtr - 1]
			//	: defaultLineEnding;
			//if ((lineEnding == CapType::Flat && numVertices + 6 >= maxNumVerticesPerBatch) ||
			//	(lineEnding == CapType::Arrow && numVertices + 15 >= maxNumVerticesPerBatch))
			//{
			//	flushBatch();
			//}

			//glm::vec4 vColor = colorStackPtr > 0
			//	? colorStack[colorStackPtr - 1]
			//	: defaultColor;
			//Vec4 color = Vec4{ vColor.r, vColor.g, vColor.g, vColor.a };
			//float strokeWidth = strokeWidthStackPtr > 0
			//	? strokeWidthStack[strokeWidthStackPtr - 1]
			//	: defaultStrokeWidth;

			//Vec2 direction = end - start;
			//Vec2 normalDirection = CMath::normalize(direction);
			//Vec2 perpVector = CMath::normalize(Vec2{ normalDirection.y, -normalDirection.x });

			//// Triangle 1
			//// "Bottom-left" corner of line
			//vertices[numVertices].position = start + (perpVector * strokeWidth * 0.5f);
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// "Top-Left" corner of line
			//vertices[numVertices].position = vertices[numVertices - 1].position + direction;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// "Top-Right" corner of line
			//vertices[numVertices].position = vertices[numVertices - 1].position - (perpVector * strokeWidth);
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// Triangle 2
			//// "Bottom-left" corner of line
			//vertices[numVertices].position = start + (perpVector * strokeWidth * 0.5f);
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// "Bottom-Right" corner of line
			//vertices[numVertices].position = vertices[numVertices - 1].position - (perpVector * strokeWidth);
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//// "Top-Right" corner of line
			//vertices[numVertices].position = vertices[numVertices - 1].position + direction;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//if (lineEnding == CapType::Arrow)
			//{
			//	// Add arrow tip
			//	Vec2 centerDot = end + (normalDirection * strokeWidth * 0.5f);
			//	Vec2 vectorToCenter = CMath::normalize(centerDot - (end - perpVector * strokeWidth * 0.5f));
			//	Vec2 oVectorToCenter = CMath::normalize(centerDot - (end + perpVector * strokeWidth * 0.5f));
			//	Vec2 bottomLeft = centerDot - vectorToCenter * strokeWidth * 4.0f;
			//	Vec2 bottomRight = centerDot - oVectorToCenter * strokeWidth * 4.0f;
			//	Vec2 top = centerDot + normalDirection * strokeWidth * 4.0f;

			//	// Left Triangle
			//	vertices[numVertices].position = centerDot;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;

			//	vertices[numVertices].position = bottomLeft;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;

			//	vertices[numVertices].position = top;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;

			//	// Right triangle
			//	vertices[numVertices].position = top;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;

			//	vertices[numVertices].position = centerDot;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;

			//	vertices[numVertices].position = bottomRight;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;

			//	// Center triangle
			//	vertices[numVertices].position = centerDot;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;

			//	vertices[numVertices].position = end + perpVector * strokeWidth * 0.5f;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;

			//	vertices[numVertices].position = end - perpVector * strokeWidth * 0.5f;
			//	vertices[numVertices].color = color;
			//	vertices[numVertices].textureId = 0;
			//	numVertices++;
			//}
		}

		void drawTexture(const RenderableTexture& renderable, const Vec4& color)
		{
			// TODO: Move this into DrawList2D
			// And make it something like pushTexture(texture);
			// Then drawRect(pos, size, uvPos, uvSize);

			//if (numVertices + 6 >= maxNumVerticesPerBatch)
			//{
			//	flushBatch();
			//}

			//uint32 texId = getTexId(*renderable.texture);

			//// Triangle 1
			//// "Bottom-left" corner
			//vertices[numVertices].position = renderable.start;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = texId;
			//vertices[numVertices].textureCoords = renderable.texCoordStart;
			//numVertices++;

			//// "Top-Left" corner
			//vertices[numVertices].position = renderable.start + Vec2{ 0, renderable.size.y };
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = texId;
			//vertices[numVertices].textureCoords = renderable.texCoordStart + Vec2{ 0, renderable.texCoordSize.y };
			//numVertices++;

			//// "Top-Right" corner of line
			//vertices[numVertices].position = renderable.start + renderable.size;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = texId;
			//vertices[numVertices].textureCoords = renderable.texCoordStart + renderable.texCoordSize;
			//numVertices++;

			//// Triangle 2
			//// "Bottom-left" corner of line
			//vertices[numVertices].position = renderable.start;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = texId;
			//vertices[numVertices].textureCoords = renderable.texCoordStart;
			//numVertices++;

			//// "Bottom-Right" corner of line
			//vertices[numVertices].position = renderable.start + Vec2{ renderable.size.x, 0 };
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = texId;
			//vertices[numVertices].textureCoords = renderable.texCoordStart + Vec2{ renderable.texCoordSize.x, 0 };
			//numVertices++;

			//// "Top-Right" corner of line
			//vertices[numVertices].position = renderable.start + renderable.size;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = texId;
			//vertices[numVertices].textureCoords = renderable.texCoordStart + renderable.texCoordSize;
			//numVertices++;
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
			// TODO: Move this into DrawList2D

			//if (numVertices + 3 >= maxNumVerticesPerBatch)
			//{
			//	flushBatch();
			//}

			//glm::vec4 vColor = colorStackPtr > 0
			//	? colorStack[colorStackPtr - 1]
			//	: defaultColor;
			//Vec4 color = Vec4{ vColor.r, vColor.g, vColor.g, vColor.a };

			//vertices[numVertices].position = p0;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//vertices[numVertices].position = p1;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;

			//vertices[numVertices].position = p2;
			//vertices[numVertices].color = color;
			//vertices[numVertices].textureId = 0;
			//numVertices++;
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

		void endPath3D(bool closePath)
		{
			int endPoint = closePath
				? numVertsIn3DPath
				: numVertsIn3DPath - 1;
			for (int vert = 0; vert < endPoint; vert++)
			{
				Vec3 currentPos = current3DPath[vert].currentPos;
				Vec3 nextPos = closePath
					? current3DPath[(vert + 1) % numVertsIn3DPath].currentPos
					: current3DPath[vert + 1].currentPos;
				Vec3 nextNextPos = closePath
					? current3DPath[(vert + 2) % numVertsIn3DPath].currentPos
					: nextPos;
				Vec3 previousPos = currentPos;
				uint32 packedColor = current3DPath[vert].color;
				float thickness = current3DPath[vert].thickness;

				if (vert > 0)
				{
					previousPos = current3DPath[vert - 1].currentPos;
				}
				else if (vert == 0 && closePath)
				{
					previousPos = current3DPath[numVertsIn3DPath - 1].currentPos;
				}

				if (vert < numVertsIn3DPath - 2)
				{
					nextNextPos = current3DPath[vert + 2].currentPos;
				}

				// Triangle 1
				drawList3DLine.addLine(previousPos, currentPos, nextPos, nextNextPos, packedColor, thickness);
			}

			isDrawing3DPath = false;
			numVertsIn3DPath = 0;
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

		void bezier2To3D(const Vec3& p1, const Vec3& p2)
		{

		}

		void bezier3To3D(const Vec3& p1, const Vec3& p2, const Vec3& p3)
		{

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

		void clearColor(const Vec4& color)
		{
			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		// ---------------------- Begin Internal Functions ----------------------
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

		static uint32 getColorCompressed()
		{
			const glm::vec4& color = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;
			uint32 packedColor =
				((uint32)(color.r * 255.0f) << 24) |
				((uint32)(color.g * 255.0f) << 16) |
				((uint32)(color.b * 255.0f) << 8) |
				((uint32)(color.a * 255.0f));
			return packedColor;
		}

		static const glm::vec4& getColor()
		{
			const glm::vec4& color = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;
			return color;
		}

		static float getStrokeWidth()
		{
			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;
			return strokeWidth;
		}
		// ---------------------- End Internal Functions ----------------------
	}

	// ---------------------- Begin DrawList2D Functions ----------------------
	void DrawList2D::init()
	{
		vao = UINT32_MAX;
		ebo = UINT32_MAX;
		vbo = UINT32_MAX;

		vertices.init();
		indices.init();
		drawCommands.init();
		textureIdStack.init();
		setupGraphicsBuffers();
	}

	// TODO: Add a bunch of methods like this...
	// void addRect();
	void DrawList2D::setupGraphicsBuffers()
	{
		// Create the batched vao
		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);

		// Allocate space for the batched vao
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * vertices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		uint32 ebo;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * indices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		// Set up the batched vao attributes
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, position)));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, color)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, textureCoords)));
		glEnableVertexAttribArray(2);
	}

	void DrawList2D::render(const Shader& shader) const
	{
		if (vertices.size() == 0)
		{
			return;
		}

		// TODO: Upload a list of draw commands and do one render call
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * vertices.size(), vertices.data, GL_DYNAMIC_DRAW);

		shader.bind();
		shader.uploadMat4("uProjection", Renderer::orthoCamera->calculateProjectionMatrix());
		shader.uploadMat4("uView", Renderer::orthoCamera->calculateViewMatrix());

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, vertices.size(), GL_UNSIGNED_INT, NULL);
	}

	void DrawList2D::reset()
	{
		vertices.softClear();
		indices.softClear();
		drawCommands.softClear();
		g_logger_assert(textureIdStack.size() == 0, "Mismatched texture ID stack. Are you missing a drawList2D.popTexture()?");
	}

	void DrawList2D::free()
	{
		if (vbo != UINT32_MAX)
		{
			glDeleteBuffers(1, &vbo);
		}

		if (ebo != UINT32_MAX)
		{
			glDeleteBuffers(1, &ebo);
		}

		if (vao != UINT32_MAX)
		{
			glDeleteVertexArrays(1, &vao);
		}

		vbo = UINT32_MAX;
		ebo = UINT32_MAX;
		vao = UINT32_MAX;

		vertices.hardClear();
		indices.hardClear();
		drawCommands.hardClear();
		textureIdStack.hardClear();
	}
	// ---------------------- End DrawList2D Functions ----------------------

	// ---------------------- Begin DrawList3DLine Functions ----------------------
	void DrawList3DLine::init()
	{
		vao = UINT32_MAX;
		vbo = UINT32_MAX;

		vertices.init();
		setupGraphicsBuffers();
	}

	void DrawList3DLine::addLine(const Vec3& previousPos, const Vec3& currentPos, const Vec3& nextPos, const Vec3& nextNextPos, uint32 packedColor, float thickness)
	{
		vertices.push(Vertex3DLine{
			currentPos,
			previousPos,
			nextPos,
			-thickness,
			packedColor
			});

		vertices.push(Vertex3DLine{
			currentPos,
			previousPos,
			nextPos,
			thickness,
			packedColor
			});

		// NOTE: This uses nextNextPos to get the second half of the line segment
		vertices.push(Vertex3DLine{
			nextPos,
			currentPos,
			nextNextPos,
			thickness,
			packedColor
			});

		// Triangle 2
		vertices.push(Vertex3DLine{
			currentPos,
			previousPos,
			nextPos,
			-thickness,
			packedColor
			});

		// NOTE: This uses nextNextPos to get the second half of the line segment
		vertices.push(Vertex3DLine{
			nextPos,
			currentPos,
			nextNextPos,
			thickness,
			packedColor
			});

		// NOTE: This uses nextNextPos to get the second half of the line segment
		vertices.push(Vertex3DLine{
			nextPos,
			currentPos,
			nextNextPos,
			-thickness,
			packedColor
			});
	}

	void DrawList3DLine::setupGraphicsBuffers()
	{
		// Create the batched vao
		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);

		// Allocate space for the batched vao
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3DLine) * vertices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

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

	void DrawList3DLine::render(const Shader& shader) const
	{
		if (vertices.size() == 0)
		{
			return;
		}

		glEnable(GL_DEPTH_TEST);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3DLine) * vertices.size(), vertices.data, GL_DYNAMIC_DRAW);

		shader.bind();
		shader.uploadMat4("uProjection", Renderer::perspCamera->calculateProjectionMatrix());
		shader.uploadMat4("uView", Renderer::perspCamera->calculateViewMatrix());
		shader.uploadFloat("uAspectRatio", Application::getOutputTargetAspectRatio());

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glDisable(GL_DEPTH_TEST);
	}

	void DrawList3DLine::reset()
	{
		vertices.softClear();
	}

	void DrawList3DLine::free()
	{
		if (vbo != UINT32_MAX)
		{
			glDeleteBuffers(1, &vbo);
		}

		if (vao != UINT32_MAX)
		{
			glDeleteVertexArrays(1, &vao);
		}

		vbo = UINT32_MAX;
		vao = UINT32_MAX;

		vertices.hardClear();
	}
	// ---------------------- End DrawList3DLine Functions ----------------------

	// ---------------------- Begin DrawList3D Functions ----------------------
	void DrawList3D::init()
	{
		vao = UINT32_MAX;
		ebo = UINT32_MAX;
		vbo = UINT32_MAX;

		vertices.init();
		indices.init();
		drawCommands.init();
		textureIdStack.init();
		setupGraphicsBuffers();
	}

	void DrawList3D::setupGraphicsBuffers()
	{

	}

	void DrawList3D::free()
	{
		if (vbo != UINT32_MAX)
		{
			glDeleteBuffers(1, &vbo);
		}

		if (ebo != UINT32_MAX)
		{
			glDeleteBuffers(1, &ebo);
		}

		if (vao != UINT32_MAX)
		{
			glDeleteVertexArrays(1, &vao);
		}

		vbo = UINT32_MAX;
		ebo = UINT32_MAX;
		vao = UINT32_MAX;

		vertices.hardClear();
		indices.hardClear();
		drawCommands.hardClear();
		textureIdStack.hardClear();
	}
	// ---------------------- End DrawList3D Functions ----------------------
}
