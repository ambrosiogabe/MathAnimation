#include "core.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/PerspectiveCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Fonts.h"
#include "renderer/Colors.h"
#include "renderer/Fonts.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "core/Application.h"

// TODO: Tmp remove me
#include "latex/LaTexLayer.h"

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

	struct DrawListFont2D
	{
		SimpleVector<Vertex2D> vertices;
		SimpleVector<uint16> indices;
		SimpleVector<DrawCmd> drawCommands;
		SimpleVector<uint32> textureIdStack;

		uint32 vao;
		uint32 vbo;
		uint32 ebo;

		void init();

		void addGlyph(const Vec2& posMin, const Vec2& posMax, const Vec2& uvMin, const Vec2& uvMax, const Vec4& color, int textureId);

		void setupGraphicsBuffers();
		void render(const Shader& shader);
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
		Vec3 normal;
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

		void addCubeFilled(const Vec3& position, const Vec3& size, const Vec4& color);
		// void addCubeFilledMulticolor(const Vec3& position, const Vec3& size, const Vec4* colors, int numColors);
		// void addCubeFilledMulticolor(const Vec3& position, const Vec3& size, Vec4 colors[6]);

		void setupGraphicsBuffers();
		void render(const Shader& opaqueShader, const Shader& transparentShader, const Shader& compositeShader, const Framebuffer& framebuffer) const;
		void reset();
		void free();
	};

	namespace Renderer
	{
		// Internal variables
		static DrawList2D drawList2D;
		static DrawListFont2D drawListFont2D;
		static DrawList3DLine drawList3DLine;
		static DrawList3D drawList3D;

		static OrthoCamera* orthoCamera;
		static PerspectiveCamera* perspCamera;

		static Shader shader2D;
		static Shader shaderFont2D;
		static Shader shader3DLine;
		static Shader screenShader;
		static Shader shader3DOpaque;
		static Shader shader3DTransparent;
		static Shader shader3DComposite;

		static constexpr int MAX_STACK_SIZE = 64;

		static glm::vec4 colorStack[MAX_STACK_SIZE];
		static float strokeWidthStack[MAX_STACK_SIZE];
		static CapType lineEndingStack[MAX_STACK_SIZE];
		static const SizedFont* fontStack[MAX_STACK_SIZE];

		static int colorStackPtr;
		static int strokeWidthStackPtr;
		static int lineEndingStackPtr;
		static int fontStackPtr;

		static constexpr glm::vec4 defaultColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		static constexpr float defaultStrokeWidth = 0.1f;
		static constexpr CapType defaultLineEnding = CapType::Flat;

		static glm::mat4 transform3D;
		static constexpr int max3DPathSize = 1'000;
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

			// Initialize default shader
#ifdef _DEBUG
			shader2D.compile("assets/shaders/default.glsl");
			shaderFont2D.compile("assets/shaders/shaderFont2D.glsl");
			screenShader.compile("assets/shaders/screen.glsl");
			shader3DLine.compile("assets/shaders/shader3DLine.glsl");
			shader3DOpaque.compile("assets/shaders/shader3DOpaque.glsl");
			shader3DTransparent.compile("assets/shaders/shader3DTransparent.glsl");
			shader3DComposite.compile("assets/shaders/shader3DComposite.glsl");
#elif defined(_RELEASE)
			// TODO: Replace these with hardcoded strings
			shader2D.compile("assets/shaders/default.glsl");
			shaderFont2D.compile("assets/shaders/shaderFont2D.glsl");
			screenShader.compile("assets/shaders/screen.glsl");
			shader3DLine.compile("assets/shaders/shader3DLine.glsl");
			shader3DOpaque.compile("assets/shaders/shader3DOpaque.glsl");
			shader3DTransparent.compile("assets/shaders/shader3DTransparent.glsl");
			shader3DComposite.compile("assets/shaders/shader3DComposite.glsl");
#endif

			drawList2D.init();
			drawListFont2D.init();
			drawList3DLine.init();
			drawList3D.init();
			setupScreenVao();
		}

		void free()
		{
			shaderFont2D.destroy();
			screenShader.destroy();
			shader3DLine.destroy();
			shader3DOpaque.destroy();
			shader3DTransparent.destroy();
			shader3DComposite.destroy();

			drawList2D.free();
			drawListFont2D.free();
			drawList3DLine.free();
			drawList3D.free();
		}

		// ----------- Render calls ----------- 
		void renderToFramebuffer(NVGcontext* vg, int frame, Framebuffer& framebuffer)
		{
			g_logger_assert(framebuffer.colorAttachments.size() == 3, "Invalid framebuffer. Should have 3 color attachments.");
			g_logger_assert(framebuffer.includeDepthStencil, "Invalid framebuffer. Should include depth and stencil buffers.");

			// Clear the framebuffer attachments and set it up
			framebuffer.bind();
			glViewport(0, 0, framebuffer.width, framebuffer.height);
			framebuffer.clearColorAttachmentRgba(0, colors[(uint8)Color::GreenBrown]);
			framebuffer.clearDepthStencil();

			// Reset the draw buffers to draw to FB_attachment_0
			GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE };
			glDrawBuffers(3, compositeDrawBuffers);

			// Collect all the render commands
			nvgBeginFrame(vg, (float)framebuffer.width, (float)framebuffer.height, 1.0f);
			AnimationManager::render(vg, frame, framebuffer);

			// Do all the draw calls
			drawList3DLine.render(shader3DLine);
			drawList3DLine.reset();

			drawListFont2D.render(shaderFont2D);
			drawListFont2D.reset();

			// Draw 3D objects after the lines so that we can do appropriate blending
			// using OIT
			drawList3D.render(shader3DOpaque, shader3DTransparent, shader3DComposite, framebuffer);
			drawList3D.reset();

			// Reset the draw buffers to draw to FB_attachment_0
			glDrawBuffers(3, compositeDrawBuffers);

			// Draw 2D stuff over 3D stuff so that 3D stuff is always "behind" the
			// 2D stuff like a HUD
			// These should be blended appropriately
			drawList2D.render(shader2D);
			drawList2D.reset();
			LaTexLayer::update();
			nvgEndFrame(vg);

			g_logger_assert(lineEndingStackPtr == 0, "Missing popLineEnding() call.");
			g_logger_assert(colorStackPtr == 0, "Missing popColor() call.");
			g_logger_assert(strokeWidthStackPtr == 0, "Missing popStrokeWidth() call.");
			g_logger_assert(fontStackPtr == 0, "Missing popFont() call.");
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

		void pushColor(const glm::u8vec4& color)
		{
			g_logger_assert(colorStackPtr < MAX_STACK_SIZE, "Ran out of room on the color stack.");
			glm::vec4 normalizedColor = glm::vec4{
				(float)color.r / 255.0f,
				(float)color.g / 255.0f,
				(float)color.b / 255.0f,
				(float)color.a / 255.0f
			};
			colorStack[colorStackPtr] = normalizedColor;
			colorStackPtr++;
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

		void pushFont(const SizedFont* font)
		{
			g_logger_assert(fontStackPtr < MAX_STACK_SIZE, "Ran out of room on the font stack.");
			fontStack[fontStackPtr] = font;
			fontStackPtr++;
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

		void popFont(int numToPop)
		{
			fontStackPtr -= numToPop;
			g_logger_assert(fontStackPtr >= 0, "Popped to many values off of font stack: %d", fontStackPtr);
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

		void drawString(const std::string& string, const Vec2& start)
		{
			g_logger_assert(fontStackPtr > 0, "Cannot draw string without a font provided. Did you forget a pushFont() call?");

			const SizedFont* font = fontStack[fontStackPtr - 1];
			const glm::vec4& colorGlm = getColor();
			Vec4 color = Vec4{ colorGlm.r, colorGlm.g, colorGlm.b, colorGlm.a };
			Vec2 cursorPos = start;

			for (int i = 0; i < string.length(); i++)
			{
				char c = string[i];
				const GlyphTexture& glyphTexture = font->getGlyphTexture(c);
				const GlyphOutline& glyphOutline = font->getGlyphInfo(c);
				float charWidth = glyphOutline.glyphWidth * (float)font->fontSizePixels;
				float charHeight = glyphOutline.glyphHeight * (float)font->fontSizePixels;
				float bearingX = glyphOutline.bearingX * (float)font->fontSizePixels;
				float descentY = glyphOutline.descentY * (float)font->fontSizePixels;
				float bearingY = glyphOutline.bearingY * (float)font->fontSizePixels;
				float lineHeight = font->unsizedFont->lineHeight * font->fontSizePixels;

				drawListFont2D.addGlyph(
					cursorPos + Vec2{ bearingX, -bearingY },
					cursorPos + Vec2{ bearingX + charWidth, descentY },
					glyphTexture.uvMin,
					glyphTexture.uvMax,
					color,
					font->texture.graphicsId
				);

				float kerning = 0.0f;
				if (i < string.length() - 1)
				{
					//kerning = font->getKerning((uint32)string[i], (uint32)string[i + 1]);
				}
				cursorPos.x += (glyphOutline.advanceX + kerning) * font->fontSizePixels;
			}
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

			glm::vec4 translatedPos = glm::vec4(start.x, start.y, start.z, 1.0f);
			translatedPos = transform3D * translatedPos;

			current3DPath[numVertsIn3DPath].currentPos = Vec3{ translatedPos.x, translatedPos.y, translatedPos.z };
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

		void lineTo3D(const Vec3& point, bool applyTransform)
		{
			g_logger_assert(isDrawing3DPath, "lineTo3D() cannot be called without calling beginPath3D(...) first.");
			if (numVertsIn3DPath >= max3DPathSize)
			{
				//g_logger_assert(numVertsIn3DPath < max3DPathSize, "Max path size exceeded. A 3D Path can only have up to %d points.", max3DPathSize);
				return;
			}

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

			glm::vec4 translatedPos = glm::vec4(point.x, point.y, point.z, 1.0f);
			if (applyTransform)
			{
				translatedPos = transform3D * translatedPos;
			}

			current3DPath[numVertsIn3DPath].currentPos = Vec3{ translatedPos.x, translatedPos.y, translatedPos.z };
			current3DPath[numVertsIn3DPath].color = packedColor;
			current3DPath[numVertsIn3DPath].thickness = strokeWidth;
			numVertsIn3DPath++;
		}

		void bezier2To3D(const Vec3& p1, const Vec3& p2)
		{
			g_logger_assert(numVertsIn3DPath > 0, "Cannot use bezier2To3D without beginning a path.");

			glm::vec4 tmpP1 = glm::vec4(p1.x, p1.y, p1.z, 1.0f);
			glm::vec4 tmpP2 = glm::vec4(p2.x, p2.y, p2.z, 1.0f);

			tmpP1 = transform3D * tmpP1;
			tmpP2 = transform3D * tmpP2;

			const Vec3& translatedP0 = current3DPath[numVertsIn3DPath - 1].currentPos;
			Vec3 translatedP1 = Vec3{ tmpP1.x, tmpP1.y, tmpP1.z };
			Vec3 translatedP2 = Vec3{ tmpP2.x, tmpP2.y, tmpP2.z };

			// Estimate the length of the bezier curve to get an approximate for the
			// number of line segments to use
			Vec3 chord1 = translatedP1 - translatedP0;
			Vec3 chord2 = translatedP2 - translatedP1;
			float chordLengthSq = CMath::lengthSquared(chord1) + CMath::lengthSquared(chord2);
			float lineLengthSq = CMath::lengthSquared(translatedP2 - translatedP0);
			float approxLength = glm::sqrt(lineLengthSq + chordLengthSq) / 2.0f;
			int numSegments = (int)(approxLength * 10.0f);
			float tInc = 1.0f / (float)numSegments;
			float t = 0.0f;
			for (int i = 0; i < numSegments; i++)
			{
				Vec3 interpPoint = CMath::bezier2(translatedP0, translatedP1, translatedP2, t);
				lineTo3D(interpPoint, false);
				t += tInc;
			}

			lineTo3D(translatedP2, false);
		}

		void bezier3To3D(const Vec3& p1, const Vec3& p2, const Vec3& p3)
		{
			g_logger_assert(numVertsIn3DPath > 0, "Cannot use bezier2To3D without beginning a path.");

			glm::vec4 tmpP1 = glm::vec4(p1.x, p1.y, p1.z, 1.0f);
			glm::vec4 tmpP2 = glm::vec4(p2.x, p2.y, p2.z, 1.0f);
			glm::vec4 tmpP3 = glm::vec4(p3.x, p3.y, p3.z, 1.0f);

			tmpP1 = transform3D * tmpP1;
			tmpP2 = transform3D * tmpP2;
			tmpP3 = transform3D * tmpP3;

			const Vec3& translatedP0 = current3DPath[numVertsIn3DPath - 1].currentPos;
			Vec3 translatedP1 = Vec3{ tmpP1.x, tmpP1.y, tmpP1.z };
			Vec3 translatedP2 = Vec3{ tmpP2.x, tmpP2.y, tmpP2.z };
			Vec3 translatedP3 = Vec3{ tmpP3.x, tmpP3.y, tmpP3.z };

			// Estimate the length of the bezier curve to get an approximate for the
			// number of line segments to use
			Vec3 chord1 = translatedP1 - translatedP0;
			Vec3 chord2 = translatedP2 - translatedP1;
			Vec3 chord3 = translatedP3 - translatedP2;
			float chordLengthSq = CMath::lengthSquared(chord1) + CMath::lengthSquared(chord2) + CMath::lengthSquared(chord3);
			float lineLengthSq = CMath::lengthSquared(translatedP3 - translatedP0);
			float approxLength = glm::sqrt(lineLengthSq + chordLengthSq) / 2.0f;
			int numSegments = (int)(approxLength * 10.0f);
			float tInc = 1.0f / (float)numSegments;
			float t = tInc;
			for (int i = 1; i < numSegments; i++)
			{
				Vec3 interpPoint = CMath::bezier3(translatedP0, translatedP1, translatedP2, translatedP3, t);
				lineTo3D(interpPoint, false);
				t += tInc;
			}

			lineTo3D(translatedP3, false);
		}

		void translate3D(const Vec3& translation)
		{
			transform3D = glm::translate(transform3D, glm::vec3(translation.x, translation.y, translation.z));
		}

		void rotate3D(const Vec3& eulerAngles)
		{
			transform3D *= glm::orientate4(glm::radians(glm::vec3(eulerAngles.x, eulerAngles.y, eulerAngles.z)));
		}

		void resetTransform3D()
		{
			transform3D = glm::identity<glm::mat4>();
		}

		// ----------- 3D stuff ----------- 
		void drawFilledCube(const Vec3& center, const Vec3& size)
		{
			const glm::vec4& color = getColor();
			drawList3D.addCubeFilled(center, size, Vec4{ color.r, color.g, color.b, color.a });
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

	// ---------------------- Begin DrawListFont2D Functions ----------------------
	void DrawListFont2D::init()
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

	void DrawListFont2D::addGlyph(const Vec2& posMin, const Vec2& posMax, const Vec2& uvMin, const Vec2& uvMax, const Vec4& color, int textureId)
	{
		// Check if we need to switch to a new batch
		if (drawCommands.size() == 0 || drawCommands.data[drawCommands.size() - 1].textureId != textureId)
		{
			DrawCmd newCommand;
			newCommand.elementCount = 0;
			newCommand.indexOffset = indices.size();
			newCommand.vertexOffset = vertices.size();
			newCommand.textureId = textureId;
			drawCommands.push(newCommand);
		}

		DrawCmd& cmd = drawCommands.data[drawCommands.size() - 1];

		int rectStartIndex = cmd.elementCount / 6 * 4;
		// Tri 1 indices
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 1); indices.push(rectStartIndex + 2);
		// Tri 2 indices
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 2); indices.push(rectStartIndex + 3);
		cmd.elementCount += 6;

		Vertex2D vert;
		vert.color = color;

		// Verts
		vert.position = posMin;
		vert.textureCoords = uvMin;
		vertices.push(vert);

		vert.position = Vec2{ posMin.x, posMax.y };
		vert.textureCoords = Vec2{ uvMin.x, uvMax.y };
		vertices.push(vert);

		vert.position = posMax;
		vert.textureCoords = uvMax;
		vertices.push(vert);

		vert.position = Vec2{ posMax.x, posMin.y };
		vert.textureCoords = Vec2{ uvMax.x, uvMin.y };
		vertices.push(vert);
	}

	// TODO: This is literally the exact same thing as the DrawList2D::setupGraphicsBuffers() function
	// maybe I should come up with some sort of CreateVao abstraction?
	void DrawListFont2D::setupGraphicsBuffers()
	{
		// Create the batched vao
		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);

		// Allocate space for the batched vao
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * vertices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

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

	void DrawListFont2D::render(const Shader& shader)
	{
		shader.bind();
		shader.uploadMat4("uProjection", Renderer::orthoCamera->calculateProjectionMatrix());
		shader.uploadMat4("uView", Renderer::orthoCamera->calculateViewMatrix());

		for (int i = 0; i < drawCommands.size(); i++)
		{
			// Bind the texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, drawCommands.data[i].textureId);
			shader.uploadInt("uTexture", 0);

			// Upload the verts
			// TODO: Figure out how to correctly do this stuff
			// I think this is crashing the GPU right now
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			int numVerts = drawCommands.data[i].elementCount / 6 * 4;
			glBufferData(
				GL_ARRAY_BUFFER,
				sizeof(Vertex2D) * numVerts,
				vertices.data + drawCommands.data[i].vertexOffset,
				GL_DYNAMIC_DRAW
			);

			// Buffer the elements
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint16) * drawCommands.data[i].elementCount,
				indices.data + drawCommands.data[i].indexOffset,
				GL_DYNAMIC_DRAW
			);

			// TODO: Swap this with glMultiDraw...
			// Make the draw call
			glBindVertexArray(vao);
			glDrawElements(
				GL_TRIANGLES,
				drawCommands.data[i].elementCount,
				GL_UNSIGNED_SHORT,
				nullptr
			);
		}
	}

	void DrawListFont2D::reset()
	{
		vertices.softClear();
		indices.softClear();
		drawCommands.softClear();
		g_logger_assert(textureIdStack.size() == 0, "Mismatched texture ID stack. Are you missing a drawListFont2D.popTexture()?");
	}

	void DrawListFont2D::free()
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
	// ---------------------- End DrawListFont2D Functions ----------------------

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
		glDisable(GL_CULL_FACE);

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

	void DrawList3D::addCubeFilled(const Vec3& position, const Vec3& size, const Vec4& color)
	{
		// Check if this can merge with the previous draw call
		// drawCommands.data[drawCommands.size() - 1]

		// First add the 8 unique vertices
		Vec3 halfSize = size / 2.0f;
		Vec3 backBottomLeft = position - halfSize;
		Vec3 backBottomRight = backBottomLeft + Vec3{ size.x, 0, 0 };
		Vec3 frontBottomLeft = backBottomLeft + Vec3{ 0, 0, size.z };
		Vec3 frontBottomRight = frontBottomLeft + Vec3{ size.x, 0, 0 };

		Vec3 backTopLeft = backBottomLeft + Vec3{ 0, size.y, 0 };
		Vec3 backTopRight = backBottomRight + Vec3{ 0, size.y, 0 };
		Vec3 frontTopLeft = frontBottomLeft + Vec3{ 0, size.y, 0 };
		Vec3 frontTopRight = frontBottomRight + Vec3{ 0, size.y, 0 };

		Vertex3D vertToAdd;
		vertToAdd.color = color;
		vertToAdd.textureCoords = { 0, 0 };
		int indexStart = vertices.size();

		// Bottom Face
		vertToAdd.position = backBottomLeft;
		vertToAdd.normal = Vec3{ 0, -1, 0 };
		vertices.push(vertToAdd);
		vertToAdd.position = backBottomRight;
		vertices.push(vertToAdd);
		vertToAdd.position = frontBottomRight;
		vertices.push(vertToAdd);
		vertToAdd.position = frontBottomLeft;
		vertices.push(vertToAdd);

		// Top Face
		vertToAdd.position = backTopLeft;
		vertToAdd.normal = Vec3{ 0, 1, 0 };
		vertices.push(vertToAdd);
		vertToAdd.position = backTopRight;
		vertices.push(vertToAdd);
		vertToAdd.position = frontTopRight;
		vertices.push(vertToAdd);
		vertToAdd.position = frontTopLeft;
		vertices.push(vertToAdd);

		// Left Face
		vertToAdd.position = backTopLeft;
		vertToAdd.normal = Vec3{ -1, 0, 0 };
		vertices.push(vertToAdd);
		vertToAdd.position = frontTopLeft;
		vertices.push(vertToAdd);
		vertToAdd.position = frontBottomLeft;
		vertices.push(vertToAdd);
		vertToAdd.position = backBottomLeft;
		vertices.push(vertToAdd);

		// Right Face
		vertToAdd.position = backTopRight;
		vertToAdd.normal = Vec3{ 1, 0, 0 };
		vertices.push(vertToAdd);
		vertToAdd.position = frontTopRight;
		vertices.push(vertToAdd);
		vertToAdd.position = frontBottomRight;
		vertices.push(vertToAdd);
		vertToAdd.position = backBottomRight;
		vertices.push(vertToAdd);

		// Back Face
		vertToAdd.position = backTopLeft;
		vertToAdd.normal = Vec3{ 0, 0, -1 };
		vertices.push(vertToAdd);
		vertToAdd.position = backTopRight;
		vertices.push(vertToAdd);
		vertToAdd.position = backBottomRight;
		vertices.push(vertToAdd);
		vertToAdd.position = backBottomLeft;
		vertices.push(vertToAdd);

		// Front Face
		vertToAdd.position = frontTopLeft;
		vertToAdd.normal = Vec3{ 0, 0, 1 };
		vertices.push(vertToAdd);
		vertToAdd.position = frontTopRight;
		vertices.push(vertToAdd);
		vertToAdd.position = frontBottomRight;
		vertices.push(vertToAdd);
		vertToAdd.position = frontBottomLeft;
		vertices.push(vertToAdd);

		// Add the indices. Gonna be 36 of them, very fun...
		for (int face = 0; face < 6; face++)
		{
			int faceIndex = indexStart + (face * 4);
			indices.push(faceIndex + 0); indices.push(faceIndex + 1); indices.push(faceIndex + 2);
			indices.push(faceIndex + 0); indices.push(faceIndex + 2); indices.push(faceIndex + 3);
		}
	}

	void DrawList3D::setupGraphicsBuffers()
	{
		// Vec3 position;
		// Vec4 color;
		// Vec2 textureCoords;
		// Create the batched vao
		glCreateVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Allocate space for the batched vbo
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3D) * vertices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * indices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		// Set up the batched vao attributes
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(offsetof(Vertex3D, position)));
		glEnableVertexAttribArray(0);

		// TODO: Change me to a packed color in 1 uint32
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(offsetof(Vertex3D, color)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(offsetof(Vertex3D, textureCoords)));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(offsetof(Vertex3D, normal)));
		glEnableVertexAttribArray(3);
	}

	void DrawList3D::render(const Shader& opaqueShader, const Shader& transparentShader,
		const Shader& compositeShader, const Framebuffer& framebuffer) const
	{
		if (vertices.size() == 0)
		{
			return;
		}

		// TODO: Upload a list of draw commands and do one render call
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3D) * vertices.size(), vertices.data, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16) * indices.size(), indices.data, GL_DYNAMIC_DRAW);

		framebuffer.bind();

		// Set up the transparent draw buffers
		GLenum drawBuffers[] = { GL_NONE, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, drawBuffers);

		// Set up GL state for transparent pass
		// Disable writing to the depth buffer
		glDepthMask(GL_FALSE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);

		// These values are obtained from http://casual-effects.blogspot.com/2015/03/implemented-weighted-blended-order.html
		// Under the 3D transparency pass table
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunci(1, GL_ONE, GL_ONE);
		glBlendFunci(2, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

		// Clear the buffers
		float accumulationClear[4] = { 0, 0, 0, 0 };
		float revealageClear[4] = { 1, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 1, accumulationClear);
		glClearBufferfv(GL_COLOR, 2, revealageClear);

		transparentShader.bind();
		transparentShader.uploadMat4("uProjection", Renderer::perspCamera->calculateProjectionMatrix());
		transparentShader.uploadMat4("uView", Renderer::perspCamera->calculateViewMatrix());
		transparentShader.uploadVec3("sunDirection", glm::vec3(0.3f, -0.2f, -0.8f));
		Vec4 sunColor = "#edc253"_hex;
		transparentShader.uploadVec3("sunColor", glm::vec3(sunColor.r, sunColor.g, sunColor.b));

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, NULL);

		// Composite the accumulation and revealage textures together
		// Render to the composite framebuffer attachment
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		compositeShader.bind();

		const Texture& accumulationTexture = framebuffer.getColorAttachment(1);
		const Texture& revealageTexture = framebuffer.getColorAttachment(2);

		glActiveTexture(GL_TEXTURE0);
		accumulationTexture.bind();
		compositeShader.uploadInt("uAccumTexture", 0);

		glActiveTexture(GL_TEXTURE1);
		revealageTexture.bind();
		compositeShader.uploadInt("uRevealageTexture", 1);

		// Set up the composite draw buffers
		GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE };
		glDrawBuffers(3, compositeDrawBuffers);

		glBindVertexArray(Renderer::screenVao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Reset GL state
		// Enable writing to the depth buffer again
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
	}

	void DrawList3D::reset()
	{
		vertices.softClear();
		indices.softClear();
		drawCommands.softClear();
		g_logger_assert(textureIdStack.size() == 0, "Mismatched texture ID stack. Are you missing a drawList2D.popTexture()?");
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
