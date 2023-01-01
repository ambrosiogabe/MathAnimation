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
#include "renderer/GLApi.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "core/Application.h"
#include "editor/Timeline.h"
#include "editor/EditorGui.h"
#include "editor/EditorSettings.h"
#include "svg/Svg.h"

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
		uint32 elementCounter;
		uint32 numVerts;
		uint32 numElements;
	};

	struct DrawCmd3D
	{
		uint32 textureId;
		uint32 vertexOffset;
		uint32 indexOffset;
		uint32 elementCount;
		bool isTransparent;
	};

	struct Vertex2D
	{
		Vec2 position;
		Vec4 color;
		Vec2 textureCoords;
		uint64 objId;
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
		void addTexturedQuad(const Texture& texture, const Vec2& min, const Vec2& max, const Vec2& uvMin, const Vec2& uvMax, const Vec4& color, AnimObjId objId, const glm::mat4& transform);
		void addColoredQuad(const Vec2& min, const Vec2& max, const Vec4& color, AnimObjId objId);
		void addColoredTri(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec4& color, AnimObjId objId);
		void addMultiColoredTri(const Vec2& p0, const Vec4& c0, const Vec2& p1, const Vec4& c1, const Vec2& p2, const Vec4& c2, AnimObjId objId);

		void setupGraphicsBuffers();
		void render(const Shader& shader, const OrthoCamera& orthoCamera) const;
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

		void addGlyph(const Vec2& posMin, const Vec2& posMax, const Vec2& uvMin, const Vec2& uvMax, const Vec4& color, int textureId, AnimObjId objId);

		void setupGraphicsBuffers();
		void render(const Shader& shader, const OrthoCamera& orthoCamera);
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
		Vec3 normal;
	};

	struct Path_Vertex2DLine
	{
		Vec2 position;
		Vec4 color;
		float thickness;

		Vec2 frontP1, frontP2;
		Vec2 backP1, backP2;
	};

	struct Path2DContext
	{
		std::vector<Vec4> colors;
		std::vector<Curve> rawCurves;
		std::vector<Path_Vertex2DLine> data;
		glm::mat4 transform;
		float approximateLength;
	};

	struct Path_Vertex3DLine
	{
		Vec3 position;
		Vec3 normal;
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
		void render(const Shader& shader, PerspectiveCamera& perspectiveCamera) const;
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
		SimpleVector<DrawCmd3D> drawCommands;
		SimpleVector<uint32> textureIdStack;

		uint32 vao;
		uint32 ebo;
		uint32 vbo;

		void init();

		void addCubeFilled(const Vec3& position, const Vec3& size, const Vec4& color);
		// void addCubeFilledMulticolor(const Vec3& position, const Vec3& size, const Vec4* colors, int numColors);
		// void addCubeFilledMulticolor(const Vec3& position, const Vec3& size, Vec4 colors[6]);
		void addTexturedQuad3D(const Texture& texture, const Vec3& bottomLeft, const Vec3& topLeft, const Vec3& topRight, const Vec3& bottomRight, const Vec2& uvMin, const Vec2& uvMax, const Vec3& faceNormal, bool isTransparent);

		void setupGraphicsBuffers();
		void render(const Shader& opaqueShader, const Shader& transparentShader, const Shader& compositeShader, const Framebuffer& framebuffer, PerspectiveCamera& perspectiveCamera) const;
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

		static int list2DNumDrawCalls = 0;
		static int listFont2DNumDrawCalls = 0;
		static int list3DNumDrawCalls = 0;

		static int list2DNumTris = 0;
		static int listFont2DNumTris = 0;
		static int list3DNumTris = 0;

		static Shader shader2D;
		static Shader shaderFont2D;
		static Shader shader3DLine;
		static Shader screenShader;
		static Shader shader3DOpaque;
		static Shader shader3DTransparent;
		static Shader shader3DComposite;
		static Shader pickingOutlineShader;

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
		static constexpr float defaultStrokeWidth = 0.02f;
		static constexpr CapType defaultLineEnding = CapType::Flat;

		static glm::mat4 transform3D;
		static constexpr int max3DPathSize = 1'000;
		static bool isDrawing3DPath;
		static Path_Vertex3DLine current3DPath[max3DPathSize];
		static int numVertsIn3DPath;
		static Texture defaultWhiteTexture;
		static int debugMsgId = 0;

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
		static void setupDefaultWhiteTexture();
		static void setupScreenVao();
		static void renderPickingOutline(const Framebuffer& mainFramebuffer);
		static uint32 getColorCompressed();
		static float getStrokeWidth();
		static void generateMiter3D(const Vec3& previousPoint, const Vec3& currentPoint, const Vec3& nextPoint, float strokeWidth, Vec2* outNormal, float* outStrokeWidth);
		static void lineToInternal(Path2DContext* path, const Vec2& point, bool addToRawCurve);
		static void lineToInternal(Path2DContext* path, const Path_Vertex2DLine& vert, bool addToRawCurve);

		void init()
		{
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
			pickingOutlineShader.compile("assets/shaders/pickingOutline.glsl");
#elif defined(_RELEASE)
			// TODO: Replace these with hardcoded strings
			shader2D.compile("assets/shaders/default.glsl");
			shaderFont2D.compile("assets/shaders/shaderFont2D.glsl");
			screenShader.compile("assets/shaders/screen.glsl");
			shader3DLine.compile("assets/shaders/shader3DLine.glsl");
			shader3DOpaque.compile("assets/shaders/shader3DOpaque.glsl");
			shader3DTransparent.compile("assets/shaders/shader3DTransparent.glsl");
			shader3DComposite.compile("assets/shaders/shader3DComposite.glsl");
			pickingOutlineShader.compile("assets/shaders/pickingOutline.glsl");
#else
#error Neither _DEBUG nor _RELEASE are defined
#endif

			drawList2D.init();
			drawListFont2D.init();
			drawList3DLine.init();
			drawList3D.init();
			setupScreenVao();
			setupDefaultWhiteTexture();
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
		void renderToFramebuffer(Framebuffer& framebuffer, const Vec4& clearColor, const OrthoCamera& orthoCamera, PerspectiveCamera& perspectiveCamera, bool shouldRenderPickingOutline)
		{
			g_logger_assert(framebuffer.colorAttachments.size() == 4, "Invalid framebuffer. Should have 3 color attachments.");
			g_logger_assert(framebuffer.includeDepthStencil, "Invalid framebuffer. Should include depth and stencil buffers.");

			debugMsgId = 0;
			GL::pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, debugMsgId++, -1, "Main_Framebuffer_Pass");

			// Clear the framebuffer attachments and set it up
			framebuffer.bind();
			GL::viewport(0, 0, framebuffer.width, framebuffer.height);
			framebuffer.clearColorAttachmentRgba(0, clearColor);
			framebuffer.clearColorAttachmentUint64(3, NULL_ANIM_OBJECT);
			framebuffer.clearDepthStencil();

			// Reset the draw buffers to draw to FB_attachment_0
			GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, GL_COLOR_ATTACHMENT3 };
			GL::drawBuffers(4, compositeDrawBuffers);

			// Do all the draw calls
			// Draw lines and strings
			drawList3DLine.render(shader3DLine, perspectiveCamera);
			drawListFont2D.render(shaderFont2D, orthoCamera);

			// Draw 3D objects after the lines so that we can do appropriate blending
			// using OIT
			drawList3D.render(shader3DOpaque, shader3DTransparent, shader3DComposite, framebuffer, perspectiveCamera);

			// Reset the draw buffers to draw to FB_attachment_0
			GL::drawBuffers(4, compositeDrawBuffers);

			// Draw 2D stuff over 3D stuff so that 3D stuff is always "behind" the
			// 2D stuff like a HUD
			// These should be blended appropriately
			drawList2D.render(shader2D, orthoCamera);

			// Draw outline around active anim object
			if (shouldRenderPickingOutline)
			{
				renderPickingOutline(framebuffer);
			}

			GL::popDebugGroup();
		}

		void renderToFramebuffer(Framebuffer& framebuffer, const Vec4& clearColor, AnimationManagerData* am, bool shouldRenderPickingOutline)
		{
			OrthoCamera orthoCamera = {};
			const AnimObject* orthoCameraObj = AnimationManager::getActiveOrthoCamera(am);
			if (orthoCameraObj)
			{
				orthoCamera = orthoCameraObj->as.camera.camera2D;
			}

			PerspectiveCamera perspCamera = {};
			// TODO: Get active perspective camera
			renderToFramebuffer(framebuffer, clearColor, orthoCamera, perspCamera, shouldRenderPickingOutline);
		}

		void renderFramebuffer(const Framebuffer& framebuffer)
		{
			screenShader.bind();

			const Texture& texture = framebuffer.getColorAttachment(0);
			GL::activeTexture(GL_TEXTURE0);
			texture.bind();
			screenShader.uploadInt("uTexture", 0);

			GL::bindVertexArray(screenVao);
			GL::drawArrays(GL_TRIANGLES, 0, 6);
		}

		void endFrame()
		{
			// Track metrics
			list2DNumDrawCalls = drawList2D.drawCommands.size();
			listFont2DNumDrawCalls = drawListFont2D.drawCommands.size();
			list3DNumDrawCalls = drawList3D.drawCommands.size();

			list2DNumTris = drawList2D.indices.size() / 3;
			listFont2DNumTris = drawListFont2D.indices.size() / 3;
			list3DNumTris = drawList3D.indices.size() / 3;

			// Do all the draw calls
			drawList3DLine.reset();
			drawListFont2D.reset();
			drawList3D.reset();
			drawList2D.reset();

			g_logger_assert(lineEndingStackPtr == 0, "Missing popLineEnding() call.");
			g_logger_assert(colorStackPtr == 0, "Missing popColor() call.");
			g_logger_assert(strokeWidthStackPtr == 0, "Missing popStrokeWidth() call.");
			g_logger_assert(fontStackPtr == 0, "Missing popFont() call.");
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

		Vec4 getColor()
		{
			const glm::vec4& color = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;
			return Vec4{ color.r, color.g, color.b, color.a };
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
			// Don't draw squares with non-negative sizes since that's an invalid input
			if (size.x <= 0.0f || size.y <= 0.0f)
			{
				return;
			}

			Path2DContext* path = beginPath(start);
			lineTo(path, start + Vec2{ size.x, 0 });
			lineTo(path, start + size);
			lineTo(path, start + Vec2{ 0, size.y });
			lineTo(path, start);
			endPath(path);
			free(path);
		}

		void drawFilledQuad(const Vec2& start, const Vec2& size, AnimObjId objId)
		{
			Vec2 min = start + (size * -0.5f);
			Vec2 max = start + (size * 0.5f);

			drawList2D.addColoredQuad(min, max, getColor(), objId);
		}

		void drawTexturedQuad(const Texture& texture, const Vec2& size, const Vec2& uvMin, const Vec2& uvMax, const Vec4& color, AnimObjId objId, const glm::mat4& transform)
		{
			drawList2D.addTexturedQuad(texture, size / -2.0f, size / 2.0f, uvMin, uvMax, color, objId, transform);
		}

		void drawFilledTri(const Vec2& p0, const Vec2& p1, const Vec2& p2, AnimObjId objId)
		{
			drawList2D.addColoredTri(p0, p1, p2, getColor(), objId);
		}

		void drawMultiColoredTri(const Vec2& p0, const Vec4& color0, const Vec2& p1, const Vec4& color1, const Vec2& p2, const Vec4& color2, AnimObjId objId)
		{
			drawList2D.addMultiColoredTri(p0, color0, p1, color1, p2, color2, objId);
		}

		void drawLine(const Vec2& start, const Vec2& end)
		{
			CapType lineEnding = lineEndingStackPtr > 0
				? lineEndingStack[lineEndingStackPtr - 1]
				: defaultLineEnding;

			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;

			Vec2 direction = end - start;
			Vec2 normalDirection = CMath::normalize(direction);
			Vec2 perpVector = Vec2{ -normalDirection.y, normalDirection.x };

			// Triangle 1
			// "Bottom-left" corner of line
			Vec2 bottomLeft = start - (perpVector * strokeWidth * 0.5f);
			Vec2 bottomRight = start + (perpVector * strokeWidth * 0.5f);
			Vec2 topLeft = end + (perpVector * strokeWidth * 0.5f);
			Vec2 topRight = end - (perpVector * strokeWidth * 0.5f);

			drawFilledTri(bottomLeft, bottomRight, topLeft);
			drawFilledTri(bottomLeft, topLeft, topRight);

			if (lineEnding == CapType::Arrow)
			{
				// Add arrow tip
				//Vec2 centerDot = end + (normalDirection * strokeWidth * 0.5f);
				//Vec2 vectorToCenter = CMath::normalize(centerDot - (end - perpVector * strokeWidth * 0.5f));
				//Vec2 oVectorToCenter = CMath::normalize(centerDot - (end + perpVector * strokeWidth * 0.5f));
				//Vec2 bottomLeft = centerDot - vectorToCenter * strokeWidth * 4.0f;
				//Vec2 bottomRight = centerDot - oVectorToCenter * strokeWidth * 4.0f;
				//Vec2 top = centerDot + normalDirection * strokeWidth * 4.0f;

				// Left Triangle
				//vertices[numVertices].position = centerDot;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;

				//vertices[numVertices].position = bottomLeft;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;

				//vertices[numVertices].position = top;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;

				//// Right triangle
				//vertices[numVertices].position = top;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;

				//vertices[numVertices].position = centerDot;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;

				//vertices[numVertices].position = bottomRight;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;

				//// Center triangle
				//vertices[numVertices].position = centerDot;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;

				//vertices[numVertices].position = end + perpVector * strokeWidth * 0.5f;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;

				//vertices[numVertices].position = end - perpVector * strokeWidth * 0.5f;
				//vertices[numVertices].color = color;
				//vertices[numVertices].textureId = 0;
				//numVertices++;
			}
		}

		void drawString(const std::string& string, const Vec2& start, AnimObjId objId)
		{
			g_logger_assert(fontStackPtr > 0, "Cannot draw string without a font provided. Did you forget a pushFont() call?");

			const SizedFont* font = fontStack[fontStackPtr - 1];
			const Vec4& color = getColor();
			Vec2 cursorPos = start;

			for (int i = 0; i < string.length(); i++)
			{
				char c = string[i];
				const GlyphTexture& glyphTexture = font->getGlyphTexture(c);
				const GlyphOutline& glyphOutline = font->getGlyphInfo(c);
				float charWidth = glyphOutline.glyphWidth * (float)font->fontSizePixels;
				float bearingX = glyphOutline.bearingX * (float)font->fontSizePixels;
				float descentY = glyphOutline.descentY * (float)font->fontSizePixels;
				float bearingY = glyphOutline.bearingY * (float)font->fontSizePixels;

				drawListFont2D.addGlyph(
					cursorPos + Vec2{ bearingX, -bearingY },
					cursorPos + Vec2{ bearingX + charWidth, descentY },
					glyphTexture.uvMin,
					glyphTexture.uvMax,
					color,
					font->texture.graphicsId,
					objId
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

				drawFilledTri(position, position + Vec2{ x, y }, position + Vec2{ nextX, nextY });

				t += sectorSize;
			}
		}

		// ----------- 2D Line stuff ----------- 
		Path2DContext* beginPath(const Vec2& start, const glm::mat4& transform)
		{
			Path2DContext* context = (Path2DContext*)g_memory_allocate(sizeof(Path2DContext));
			new(context)Path2DContext();

			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;

			glm::vec4 color = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;

			context->transform = transform;
			glm::vec4 translatedPos = glm::vec4(start.x, start.y, 0.0f, 1.0f);
			translatedPos = context->transform * translatedPos;

			Path_Vertex2DLine vert;
			vert.position = Vec2{ translatedPos.x, translatedPos.y };
			vert.color = Vec4{
				color.r,
				color.g,
				color.b,
				color.a
			};
			vert.thickness = strokeWidth;
			context->data.emplace_back(vert);
			context->colors.push_back(Vec4{ color.r, color.g, color.b, color.a });

			return context;
		}

		void free(Path2DContext* path)
		{
			path->~Path2DContext();
			g_memory_free(path);
		}

		void endPath(Path2DContext* path, bool closePath, AnimObjId objId)
		{
			g_logger_assert(path != nullptr, "Null path.");

			// NOTE: This is some weird shenanigans in order to get the path
			// to close correctly and join the last vertex to the first vertex
			// This hack is followed up in the second loop by a similar hack
			int endPoint = (int)path->data.size();
			if (closePath && current3DPath[0].position == current3DPath[numVertsIn3DPath - 1].position)
			{
				endPoint--;
			}

			// Do two loops:
			// 
			// The first loop extrudes all the vertices
			// and forms the tesselated path. It also creates 
			// any bevels/miters/rounded corners and adds
			// them immediately and just saves the connection
			// points for the second loop.
			// 
			// The second loop
			// connects the verts into quads to form the stroke

			for (int vertIndex = 0; vertIndex < path->data.size(); vertIndex++)
			{
				Path_Vertex2DLine& vertex = path->data[vertIndex];
				Vec2 currentPos = vertex.position;
				Vec2 nextPos = vertIndex + 1 < endPoint
					? path->data[vertIndex + 1].position
					: closePath
					? path->data[(vertIndex + 1) % endPoint].position
					: path->data[endPoint - 1].position;
				Vec2 previousPos = vertIndex > 0
					? path->data[vertIndex - 1].position
					: closePath
					? path->data[endPoint - 1].position
					: path->data[0].position;
				const Vec4& currentColor = vertex.color;

				// Generate a miter
				bool generateVerts = true;

				Vec2 dirA = CMath::normalize(currentPos - previousPos);
				Vec2 dirB = CMath::normalize(nextPos - currentPos);
				Vec2 bisectionPerp = CMath::normalize(dirA + dirB);
				Vec2 secondLinePerp = Vec2{ -dirB.y, dirB.x };
				// This is the miter
				Vec2 bisection = Vec2{ -bisectionPerp.y, bisectionPerp.x };
				Vec2 extrusionNormal = bisection;
				float bisectionDotProduct = CMath::dot(bisection, secondLinePerp);
				float miterThickness = vertex.thickness / bisectionDotProduct;
				if (CMath::compare(bisectionDotProduct, 0.0f, 0.01f))
				{
					// Clamp the miter if the joining curves are almost parallell
					miterThickness = vertex.thickness;
				}

				constexpr float strokeMiterLimit = 2.0f;
				bool shouldConvertToBevel = miterThickness / vertex.thickness > strokeMiterLimit;
				if (shouldConvertToBevel)
				{
					float firstBevelWidth = vertex.thickness / CMath::dot(bisection, dirA) * 0.5f;
					float secondBevelWidth = vertex.thickness / CMath::dot(bisection, dirB) * 0.5f;
					Vec2 firstPoint = currentPos + (bisectionPerp * firstBevelWidth);
					Vec2 secondPoint = currentPos + (bisectionPerp * secondBevelWidth);

					float centerBevelWidth = vertex.thickness / CMath::dot(bisectionPerp, CMath::normalize(currentPos - previousPos));
					centerBevelWidth = glm::min(centerBevelWidth, vertex.thickness);
					Vec2 centerPoint = currentPos + (bisection * centerBevelWidth);
					Renderer::drawMultiColoredTri(firstPoint, currentColor, secondPoint, currentColor, centerPoint, currentColor, objId);

					// Save the "front" and "back" for the connection loop
					vertex.frontP1 = centerPoint;
					vertex.frontP2 = secondPoint;

					vertex.backP1 = centerPoint;
					vertex.backP2 = firstPoint;
					generateVerts = false;
				}

				// If we're drawing the beginning/end of the path, just
				// do a straight cap on the line segment
				if (vertIndex == 0 && !closePath)
				{
					generateVerts = true;
					Vec2 normal = CMath::normalize(nextPos - currentPos);
					extrusionNormal = Vec2{ -normal.y, normal.x };
					miterThickness = vertex.thickness;
				}
				else if (vertIndex == endPoint - 1 && !closePath)
				{
					generateVerts = true;
					Vec2 normal = CMath::normalize(currentPos - previousPos);
					extrusionNormal = Vec2{ -normal.y, normal.x };
					miterThickness = vertex.thickness;
				}

				// If we're doing a bevel or something, that uses special vertices
				// to join segments. Otherwise we can just use the extrusion normal
				// and miterThickness to join the segments
				if (generateVerts)
				{
					vertex.frontP1 = vertex.position + extrusionNormal * miterThickness * 0.5f;
					vertex.frontP2 = vertex.position - extrusionNormal * miterThickness * 0.5f;

					vertex.backP1 = vertex.position + extrusionNormal * miterThickness * 0.5f;
					vertex.backP2 = vertex.position - extrusionNormal * miterThickness * 0.5f;
				}
			}

			// NOTE: This is some weird shenanigans in order to get the path
			// to close correctly and join the last vertex to the first vertex
			if (!closePath)
			{
				endPoint--;
			}

			for (int vertIndex = 0; vertIndex < endPoint; vertIndex++)
			{
				const Path_Vertex2DLine& vertex = path->data[vertIndex % path->data.size()];
				const Path_Vertex2DLine& nextVertex = path->data[(vertIndex + 1) % path->data.size()];

				drawMultiColoredTri(vertex.frontP1, vertex.color, vertex.frontP2, vertex.color, nextVertex.backP1, nextVertex.color, objId);
				drawMultiColoredTri(vertex.frontP2, vertex.color, nextVertex.backP2, nextVertex.color, nextVertex.backP1, nextVertex.color, objId);
			}
		}

		void renderOutline(Path2DContext* path, float startT, float endT, bool closePath, AnimObjId)
		{
			startT = glm::clamp(startT, 0.0f, 1.0f);
			endT = glm::clamp(endT, startT, 1.0f);

			// Start the fade in after 80% of the svg object is drawn
			const float lengthToDraw = (endT - startT) * (float)path->approximateLength;
			float currentT = 0.0f;

			if (lengthToDraw > 0 && path->rawCurves.size() > 0)
			{
				float lengthDrawn = 0.0f;

				Path2DContext* context = nullptr;

				for (int curvei = 0; curvei < path->rawCurves.size(); curvei++)
				{
					float lengthLeft = lengthToDraw - lengthDrawn;
					if (lengthLeft < 0.0f)
					{
						break;
					}

					Curve curve = path->rawCurves[curvei];
					float approxLength = curve.calculateApproximatePerimeter();
					float approxLengthTVal = approxLength / (float)path->approximateLength;

					if (context == nullptr)
					{
						Renderer::pushColor(path->colors[curvei]);
						if (currentT >= startT)
						{
							context = Renderer::beginPath(path->rawCurves[curvei].p0);
						}
						else if (currentT + approxLengthTVal >= startT)
						{
							float tSplit0 = 1.0f - ((currentT + approxLengthTVal - startT) / approxLengthTVal);
							curve = curve.split(tSplit0, 1.0f);
							currentT = startT;
							approxLength = curve.calculateApproximatePerimeter();
							approxLengthTVal = approxLength / (float)path->approximateLength;

							context = Renderer::beginPath(curve.p0);
						}
						Renderer::popColor();
					}

					const Vec2& p0 = curve.p0;
					Renderer::pushColor(path->colors[curvei + 1]);

					switch (curve.type)
					{
					case CurveType::Bezier3:
					{
						if (currentT < startT)
						{
							break;
						}

						if (lengthLeft < approxLength)
						{
							// Interpolate the curve
							float percentOfCurveToDraw = lengthLeft / approxLength;
							curve = curve.split(0.0f, percentOfCurveToDraw);
						}

						lengthDrawn += approxLength;
						Renderer::cubicTo(
							context,
							curve.as.bezier3.p1,
							curve.as.bezier3.p2,
							curve.as.bezier3.p3
						);
					}
					break;
					case CurveType::Bezier2:
					{
						if (currentT < startT)
						{
							break;
						}

						const Vec2& p1 = curve.as.bezier2.p1;
						const Vec2& p2 = curve.as.bezier2.p1;
						const Vec2& p3 = curve.as.bezier2.p2;

						// Degree elevated quadratic bezier curve
						Vec2 pr0 = p0;
						Vec2 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
						Vec2 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
						Vec2 pr3 = p3;

						if (lengthLeft < approxLength)
						{
							// Interpolate the curve
							float percentOfCurveToDraw = lengthLeft / approxLength;
							curve = curve.split(0.0f, percentOfCurveToDraw);

							pr1 = curve.as.bezier3.p1;
							pr2 = curve.as.bezier3.p2;
							pr3 = curve.as.bezier3.p3;
						}

						lengthDrawn += approxLength;
						Renderer::cubicTo(
							context,
							pr1,
							pr2,
							pr3
						);
					}
					break;
					case CurveType::Line:
					{
						if (currentT < startT)
						{
							break;
						}

						if (lengthLeft < approxLength)
						{
							float percentOfCurveToDraw = lengthLeft / approxLength;
							curve = curve.split(0.0f, percentOfCurveToDraw);
						}

						const Vec2& p1 = curve.as.line.p1;
						lengthDrawn += approxLength;
						Renderer::lineTo(
							context,
							p1
						);
					}
					break;
					case CurveType::None:
						break;
					}

					Renderer::popColor();
					currentT += approxLengthTVal;
				}

				if (endT < 1.0f)
				{
					Renderer::endPath(context, false);
					Renderer::free(context);
				}
				else
				{
					Renderer::endPath(context, closePath);
					Renderer::free(context);
				}
			}
		}

		void lineTo(Path2DContext* path, const Vec2& point, bool applyTransform)
		{
			g_logger_assert(path != nullptr, "Null path.");

			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;

			glm::vec4 color = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;

			glm::vec4 translatedPos = glm::vec4(point.x, point.y, 0.0f, 1.0f);
			if (applyTransform)
			{
				translatedPos = path->transform * translatedPos;
			}

			Path_Vertex2DLine vert;
			vert.position = Vec2{ translatedPos.x, translatedPos.y };
			vert.color = Vec4{
				color.r,
				color.g,
				color.b,
				color.a
			};
			vert.thickness = strokeWidth;

			lineToInternal(path, vert, true);
		}

		void quadTo(Path2DContext* path, const Vec2& p1, const Vec2& p2)
		{
			g_logger_assert(path != nullptr, "Null path.");
			g_logger_assert(path->data.size() > 0, "Cannot do a quadTo on an empty path.");

			glm::vec4 tmpP1 = glm::vec4(p1.x, p1.y, 0.0f, 1.0f);
			glm::vec4 tmpP2 = glm::vec4(p2.x, p2.y, 0.0f, 1.0f);

			tmpP1 = path->transform * tmpP1;
			tmpP2 = path->transform * tmpP2;

			Vec2 transformedP0 = path->data[path->data.size() - 1].position;
			Vec2 transformedP1 = Vec2{ tmpP1.x, tmpP1.y };
			Vec2 transformedP2 = Vec2{ tmpP2.x, tmpP2.y };

			{
				// Add raw curve data
				Curve rawCurve{};
				rawCurve.type = CurveType::Bezier2;
				rawCurve.p0 = transformedP0;
				rawCurve.as.bezier2.p1 = transformedP1;
				rawCurve.as.bezier2.p2 = transformedP2;

				path->approximateLength += rawCurve.calculateApproximatePerimeter();
				path->rawCurves.emplace_back(rawCurve);

				glm::vec4 color = colorStackPtr > 0
					? colorStack[colorStackPtr - 1]
					: defaultColor;
				path->colors.push_back(Vec4{ color.r, color.g, color.b, color.a });
			}

			// Estimate the length of the bezier curve to get an approximate for the
			// number of line segments to use
			Vec2 chord1 = transformedP1 - transformedP0;
			Vec2 chord2 = transformedP2 - transformedP1;
			float chordLengthSq = CMath::lengthSquared(chord1) + CMath::lengthSquared(chord2);
			float lineLengthSq = CMath::lengthSquared(transformedP2 - transformedP0);
			float approxLength = glm::sqrt(lineLengthSq + chordLengthSq) / 2.0f;
			int numSegments = (int)(approxLength * 40.0f);
			for (int i = 1; i < numSegments - 1; i++)
			{
				float t = (float)i / (float)numSegments;
				Vec2 interpPoint = CMath::bezier2(transformedP0, transformedP1, transformedP2, t);
				lineToInternal(path, interpPoint, false);
			}

			lineToInternal(path, transformedP2, false);
		}

		void cubicTo(Path2DContext* path, const Vec2& p1, const Vec2& p2, const Vec2& p3)
		{
			g_logger_assert(path != nullptr, "Null path.");
			g_logger_assert(path->data.size() > 0, "Cannot do a cubicTo on an empty path.");

			glm::vec4 tmpP1 = glm::vec4(p1.x, p1.y, 0.0f, 1.0f);
			glm::vec4 tmpP2 = glm::vec4(p2.x, p2.y, 0.0f, 1.0f);
			glm::vec4 tmpP3 = glm::vec4(p3.x, p3.y, 0.0f, 1.0f);

			tmpP1 = path->transform * tmpP1;
			tmpP2 = path->transform * tmpP2;
			tmpP3 = path->transform * tmpP3;

			Vec2 transformedP0 = path->data[path->data.size() - 1].position;
			Vec2 transformedP1 = Vec2{ tmpP1.x, tmpP1.y };
			Vec2 transformedP2 = Vec2{ tmpP2.x, tmpP2.y };
			Vec2 transformedP3 = Vec2{ tmpP3.x, tmpP3.y };

			{
				// Add raw curve data
				Curve rawCurve{};
				rawCurve.type = CurveType::Bezier3;
				rawCurve.p0 = transformedP0;
				rawCurve.as.bezier3.p1 = transformedP1;
				rawCurve.as.bezier3.p2 = transformedP2;
				rawCurve.as.bezier3.p3 = transformedP3;

				path->approximateLength += rawCurve.calculateApproximatePerimeter();
				path->rawCurves.emplace_back(rawCurve);

				glm::vec4 color = colorStackPtr > 0
					? colorStack[colorStackPtr - 1]
					: defaultColor;
				path->colors.push_back(Vec4{ color.r, color.g, color.b, color.a });
			}

			// Estimate the length of the bezier curve to get an approximate for the
			// number of line segments to use
			Vec2 chord1 = transformedP1 - transformedP0;
			Vec2 chord2 = transformedP2 - transformedP1;
			Vec2 chord3 = transformedP3 - transformedP2;
			float chordLengthSq = CMath::lengthSquared(chord1) + CMath::lengthSquared(chord2) + CMath::lengthSquared(chord3);
			float lineLengthSq = CMath::lengthSquared(transformedP3 - transformedP0);
			float approxLength = glm::sqrt(lineLengthSq + chordLengthSq) / 2.0f;
			int numSegments = (int)(approxLength * 40.0f);
			for (int i = 1; i < numSegments - 1; i++)
			{
				float t = (float)i / (float)numSegments;
				Vec2 interpPoint = CMath::bezier3(transformedP0, transformedP1, transformedP2, transformedP3, t);
				lineToInternal(path, interpPoint, false);
			}

			lineToInternal(path, transformedP3, false);
		}

		void setTransform(Path2DContext* path, const glm::mat4& transform)
		{
			g_logger_assert(path != nullptr, "Null path.");
			path->transform = transform;
		}

		// ----------- 3D stuff ----------- 
		// TODO: Consider just making these glm::vec3's. I'm not sure what kind
		// of impact, if any that will have
		void beginPath3D(const Vec3& start, const Vec3& normal)
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

			current3DPath[numVertsIn3DPath].position = Vec3{ translatedPos.x, translatedPos.y, translatedPos.z };
			current3DPath[numVertsIn3DPath].color = packedColor;
			current3DPath[numVertsIn3DPath].thickness = strokeWidth;
			current3DPath[numVertsIn3DPath].normal = normal;
			numVertsIn3DPath++;
		}

		void endPath3D(bool closePath)
		{
			if (closePath && current3DPath[0].position == current3DPath[numVertsIn3DPath - 1].position)
			{
				numVertsIn3DPath--;
			}

			for (int vert = 0; vert < numVertsIn3DPath; vert++)
			{
				Vec3 currentPos = current3DPath[vert].position;
				Vec3 nextPos = vert + 1 < numVertsIn3DPath
					? current3DPath[vert + 1].position
					: closePath
					? current3DPath[(vert + 1) % numVertsIn3DPath].position
					: current3DPath[numVertsIn3DPath - 1].position;
				Vec3 nextNextPos = vert + 2 < numVertsIn3DPath
					? current3DPath[vert + 2].position
					: closePath
					? current3DPath[(vert + 2) % numVertsIn3DPath].position
					: current3DPath[numVertsIn3DPath - 1].position;
				Vec3 previousPos = vert > 0
					? current3DPath[vert - 1].position
					: closePath
					? current3DPath[numVertsIn3DPath - 1].position
					: current3DPath[0].position;
				uint32 packedColor = current3DPath[vert].color;
				float thickness = current3DPath[vert].thickness;

				Vec4 unpackedColor = {
					(float)(packedColor >> 24 & 0xFF) / 255.0f,
					(float)(packedColor >> 16 & 0xFF) / 255.0f,
					(float)(packedColor >> 8 & 0xFF) / 255.0f,
					(float)(packedColor & 0xFF) / 255.0f
				};

				Renderer::pushStrokeWidth(thickness);
				Renderer::pushColor(unpackedColor);

				Vec3 currentNormal = current3DPath[vert].normal;
				Vec3 nextNormal = current3DPath[(vert + 1) % numVertsIn3DPath].normal;

				// NOTE: Previous method all the following code should be
				// used for 2D contexts only most likely...
				//drawList3DLine.addLine(previousPos, currentPos, nextPos, nextNextPos, packedColor, thickness);

				Vec2 firstNormal = CMath::vector2From3(currentNormal);
				Vec2 secondNormal = CMath::vector2From3(nextNormal);
				float firstThickness = 0.0f;
				float secondThickness = 0.0f;
				if (firstNormal == Vec2{ FLT_MAX, FLT_MAX })
				{
					generateMiter3D(previousPos, currentPos, nextPos, thickness, &firstNormal, &firstThickness);
				}
				if (secondNormal == Vec2{ FLT_MAX, FLT_MAX })
				{
					generateMiter3D(currentPos, nextPos, nextNextPos, thickness, &secondNormal, &secondThickness);
				}

				if (currentPos == previousPos)
				{
					// If we're drawing the beginning of the path, just
					// do a straight cap on the line segment
					firstNormal = CMath::normalize(CMath::vector2From3(nextPos - currentPos));
					firstThickness = thickness;
				}
				if (nextPos == nextNextPos)
				{
					// If we're drawing the end of the path, just
					// do a straight cap on the line segment
					secondNormal = CMath::normalize(CMath::vector2From3(nextPos - currentPos));
					secondThickness = thickness;
				}

				drawLine(CMath::vector2From3(currentPos), CMath::vector2From3(nextPos));

				Renderer::popStrokeWidth();
				Renderer::popColor();
			}

			isDrawing3DPath = false;
			numVertsIn3DPath = 0;
		}

		void lineTo3D(const Vec3& point, bool applyTransform, const Vec3& normal)
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

			current3DPath[numVertsIn3DPath].position = Vec3{ translatedPos.x, translatedPos.y, translatedPos.z };
			current3DPath[numVertsIn3DPath].color = packedColor;
			current3DPath[numVertsIn3DPath].thickness = strokeWidth;
			current3DPath[numVertsIn3DPath].normal = normal;
			numVertsIn3DPath++;
		}

		void bezier2To3D(const Vec3& p1, const Vec3& p2)
		{
			g_logger_assert(numVertsIn3DPath > 0, "Cannot use bezier2To3D without beginning a path.");

			glm::vec4 tmpP1 = glm::vec4(p1.x, p1.y, p1.z, 1.0f);
			glm::vec4 tmpP2 = glm::vec4(p2.x, p2.y, p2.z, 1.0f);

			tmpP1 = transform3D * tmpP1;
			tmpP2 = transform3D * tmpP2;

			const Vec3& translatedP0 = current3DPath[numVertsIn3DPath - 1].position;
			Vec3 translatedP1 = Vec3{ tmpP1.x, tmpP1.y, tmpP1.z };
			Vec3 translatedP2 = Vec3{ tmpP2.x, tmpP2.y, tmpP2.z };

			// Estimate the length of the bezier curve to get an approximate for the
			// number of line segments to use
			Vec3 chord1 = translatedP1 - translatedP0;
			Vec3 chord2 = translatedP2 - translatedP1;
			float chordLengthSq = CMath::lengthSquared(chord1) + CMath::lengthSquared(chord2);
			float lineLengthSq = CMath::lengthSquared(translatedP2 - translatedP0);
			float approxLength = glm::sqrt(lineLengthSq + chordLengthSq) / 2.0f;
			int numSegments = (int)(approxLength * 40.0f);
			for (int i = 0; i < numSegments; i++)
			{
				float t = (float)i / (float)numSegments;
				Vec3 interpPoint = CMath::bezier2(translatedP0, translatedP1, translatedP2, t);
				Vec3 normal = CMath::bezier2Normal(translatedP0, translatedP1, translatedP2, t);
				lineTo3D(interpPoint, false, normal);
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

			const Vec3& translatedP0 = current3DPath[numVertsIn3DPath - 1].position;
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
			int numSegments = (int)(approxLength * 40.0f);
			for (int i = 1; i < numSegments; i++)
			{
				float t = (float)i / (float)numSegments;
				Vec3 interpPoint = CMath::bezier3(translatedP0, translatedP1, translatedP2, translatedP3, t);
				Vec3 normal = CMath::bezier3Normal(translatedP0, translatedP1, translatedP2, translatedP3, t);
				lineTo3D(interpPoint, false, normal);
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

		void setTransform(const glm::mat4& transform)
		{
			transform3D = transform;
		}

		void resetTransform3D()
		{
			transform3D = glm::identity<glm::mat4>();
		}

		// ----------- 3D stuff ----------- 
		void drawFilledCube(const Vec3& center, const Vec3& size)
		{
			drawList3D.addCubeFilled(center, size, getColor());
		}

		void drawTexturedQuad3D(const Texture& texture, const Vec2& size, const Vec2& uvMin, const Vec2& uvMax, const glm::mat4& transform, bool isTransparent)
		{
			glm::vec4 tmpBottomLeft = transform * glm::vec4(-size.x / 2.0f, -size.y / 2.0f, 0.0f, 1.0f);
			glm::vec4 tmpTopLeft = transform * glm::vec4(-size.x / 2.0f, size.y / 2.0f, 0.0f, 1.0f);
			glm::vec4 tmpTopRight = transform * glm::vec4(size.x / 2.0f, size.y / 2.0f, 0.0f, 1.0f);
			glm::vec4 tmpBottomRight = transform * glm::vec4(size.x / 2.0f, -size.y / 2.0f, 0.0f, 1.0f);
			glm::vec4 tmpFaceNormal = transform * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
			Vec3 bottomLeft = { tmpBottomLeft.x, tmpBottomLeft.y, tmpBottomLeft.z };
			Vec3 topLeft = { tmpTopLeft.x, tmpTopLeft.y, tmpTopLeft.z };
			Vec3 topRight = { tmpTopRight.x, tmpTopRight.y, tmpTopRight.z };
			Vec3 bottomRight = { tmpBottomRight.x, tmpBottomRight.y, tmpBottomRight.z };
			Vec3 faceNormal = Vec3{ tmpFaceNormal.x, tmpFaceNormal.y, tmpFaceNormal.z };

			drawList3D.addTexturedQuad3D(texture, bottomLeft, topLeft, topRight, bottomRight, uvMin, uvMax, faceNormal, isTransparent);
		}

		// ----------- Miscellaneous ----------- 
		void clearColor(const Vec4& color)
		{
			GL::clearColor(color.r, color.g, color.b, color.a);
			GL::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		// ----------- Metrics ----------- 
		int getTotalNumDrawCalls()
		{
			return getDrawList2DNumDrawCalls() +
				getDrawListFont2DNumDrawCalls() +
				getDrawList3DNumDrawCalls();
		}

		int getDrawList2DNumDrawCalls()
		{
			return list2DNumDrawCalls;
		}

		int getDrawListFont2DNumDrawCalls()
		{
			return listFont2DNumDrawCalls;
		}

		int getDrawList3DNumDrawCalls()
		{
			return list3DNumDrawCalls;
		}

		int getTotalNumTris()
		{
			return getDrawList2DNumTris() +
				getDrawListFont2DNumTris() +
				getDrawList3DNumTris();
		}

		int getDrawList2DNumTris()
		{
			return list2DNumTris;
		}

		int getDrawListFont2DNumTris()
		{
			return listFont2DNumTris;
		}

		int getDrawList3DNumTris()
		{
			return list3DNumTris;
		}

		// ---------------------- Begin Internal Functions ----------------------
		static void setupDefaultWhiteTexture()
		{
			defaultWhiteTexture = TextureBuilder()
				.setWidth(1)
				.setHeight(1)
				.setMagFilter(FilterMode::Nearest)
				.setMinFilter(FilterMode::Nearest)
				.setFormat(ByteFormat::RGBA8_UI)
				.generate();
			uint32 whitePixel = 0xFFFFFFFF;
			defaultWhiteTexture.uploadSubImage(0, 0, 1, 1, (uint8*)&whitePixel, sizeof(uint32));
		}

		static void setupScreenVao()
		{
			// Create the screen vao
			GL::createVertexArray(&screenVao);
			GL::bindVertexArray(screenVao);

			uint32 screenVbo;
			GL::genBuffers(1, &screenVbo);

			// Allocate space for the screen vao
			GL::bindBuffer(GL_ARRAY_BUFFER, screenVbo);
			GL::bufferData(GL_ARRAY_BUFFER, sizeof(defaultScreenQuad), defaultScreenQuad, GL_STATIC_DRAW);

			// Set up the screen vao attributes
			// The position doubles as the texture coordinates so we can use the same floats for that
			GL::vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
			GL::enableVertexAttribArray(0);

			GL::vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
			GL::enableVertexAttribArray(1);
		}

		static void renderPickingOutline(const Framebuffer& mainFramebuffer)
		{
			GL::pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, debugMsgId++, -1, "Active_Object_Outline_Pass");

			GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, GL_NONE };
			GL::drawBuffers(4, compositeDrawBuffers);
			pickingOutlineShader.bind();

			const Texture& objIdTexture = mainFramebuffer.getColorAttachment(3);
			GL::activeTexture(GL_TEXTURE0);
			objIdTexture.bind();
			pickingOutlineShader.uploadInt("uObjectIdTexture", 0);

			const Texture& colorTexture = mainFramebuffer.getColorAttachment(0);
			GL::activeTexture(GL_TEXTURE1);
			colorTexture.bind();
			pickingOutlineShader.uploadInt("uColorTexture", 1);

			pickingOutlineShader.uploadU64AsUVec2("uActiveObjectId", Timeline::getActiveAnimObject());
			glm::vec2 textureSize = glm::vec2((float)objIdTexture.width, (float)objIdTexture.height);
			pickingOutlineShader.uploadVec2("uResolution", textureSize);

			//pickingOutlineShader.uploadFloat("uThreshold", EditorGui::getThreshold());

			GL::bindVertexArray(screenVao);
			GL::drawArrays(GL_TRIANGLES, 0, 6);

			GL::popDebugGroup();
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

		static float getStrokeWidth()
		{
			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;
			return strokeWidth;
		}

		static void generateMiter3D(const Vec3& previousPoint, const Vec3& currentPoint, const Vec3& nextPoint, float strokeWidth, Vec2* outNormal, float* outStrokeWidth)
		{
			Vec2 dirA = CMath::normalize(CMath::vector2From3(currentPoint - previousPoint));
			Vec2 dirB = CMath::normalize(CMath::vector2From3(nextPoint - currentPoint));
			Vec2 bisection = CMath::normalize(dirA + dirB);
			Vec2 secondLinePerp = Vec2{ -dirB.y, dirB.x };
			// This is the miter
			Vec2 bisectionPerp = Vec2{ -bisection.y, bisection.x };
			*outNormal = bisection;
			*outStrokeWidth = strokeWidth / CMath::dot(bisectionPerp, secondLinePerp);
			//*outStrokeWidth = CMath::max(CMath::min(CMath::abs(*outStrokeWidth), strokeWidth), -CMath::abs(strokeWidth));
		}

		static void lineToInternal(Path2DContext* path, const Vec2& point, bool addRawCurve)
		{
			g_logger_assert(path != nullptr, "Null path.");

			float strokeWidth = strokeWidthStackPtr > 0
				? strokeWidthStack[strokeWidthStackPtr - 1]
				: defaultStrokeWidth;

			glm::vec4 color = colorStackPtr > 0
				? colorStack[colorStackPtr - 1]
				: defaultColor;

			Path_Vertex2DLine vert;
			vert.position = point;
			vert.color = Vec4{
				color.r,
				color.g,
				color.b,
				color.a
			};
			vert.thickness = strokeWidth;

			// Don't add the vertex if it's going to itself
			if (path->data.size() > 0 && path->data[path->data.size() - 1].position != vert.position)
			{
				if (addRawCurve)
				{
					Curve rawCurve;
					rawCurve.type = CurveType::Line;
					rawCurve.p0 = path->data[path->data.size() - 1].position;
					rawCurve.as.line.p1 = vert.position;

					path->approximateLength += rawCurve.calculateApproximatePerimeter();
					path->rawCurves.emplace_back(rawCurve);
				}

				path->data.emplace_back(vert);
			}
		}

		static void lineToInternal(Path2DContext* path, const Path_Vertex2DLine& vert, bool addRawCurve)
		{
			g_logger_assert(path != nullptr, "Null path.");

			// Don't add the vertex if it's going to itself
			if (path->data.size() > 0 && path->data[path->data.size() - 1].position != vert.position)
			{
				if (addRawCurve)
				{
					Curve rawCurve;
					rawCurve.type = CurveType::Line;
					rawCurve.p0 = path->data[path->data.size() - 1].position;
					rawCurve.as.line.p1 = vert.position;

					path->approximateLength += rawCurve.calculateApproximatePerimeter();
					path->rawCurves.emplace_back(rawCurve);

					glm::vec4 color = colorStackPtr > 0
						? colorStack[colorStackPtr - 1]
						: defaultColor;
					path->colors.push_back(Vec4{ color.r, color.g, color.b, color.a });
				}

				path->data.emplace_back(vert);
			}
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
	void DrawList2D::addTexturedQuad(const Texture& texture, const Vec2& min, const Vec2& max, const Vec2& uvMin, const Vec2& uvMax, const Vec4& color, AnimObjId objId, const glm::mat4& transform)
	{
		// Check if we need to switch to a new batch
		if (drawCommands.size() == 0 || drawCommands.data[drawCommands.size() - 1].textureId != texture.graphicsId)
		{
			DrawCmd newCommand;
			newCommand.elementCounter = 0;
			newCommand.indexOffset = indices.size();
			newCommand.vertexOffset = vertices.size();
			newCommand.textureId = texture.graphicsId;
			newCommand.numElements = 0;
			newCommand.numVerts = 0;
			drawCommands.push(newCommand);
		}

		DrawCmd& cmd = drawCommands.data[drawCommands.size() - 1];

		glm::vec4 bottomLeft = transform * glm::vec4(min.x, min.y, 0.0f, 1.0f);
		glm::vec4 topLeft = transform * glm::vec4(min.x, max.y, 0.0f, 1.0f);
		glm::vec4 topRight = transform * glm::vec4(max.x, max.y, 0.0f, 1.0f);
		glm::vec4 bottomRight = transform * glm::vec4(max.x, min.y, 0.0f, 1.0f);

		int rectStartIndex = cmd.elementCounter;
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 1); indices.push(rectStartIndex + 2);
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 2); indices.push(rectStartIndex + 3);
		cmd.elementCounter += 4;
		cmd.numElements += 6;

		Vertex2D vert;
		vert.color = color;
		vert.objId = objId;
		vert.position = Vec2{ bottomLeft.x, bottomLeft.y };
		vert.textureCoords = uvMin;
		vertices.push(vert);

		vert.position = Vec2{ topLeft.x, topLeft.y };
		vert.textureCoords = Vec2{ uvMin.x, uvMax.y };
		vertices.push(vert);

		vert.position = Vec2{ topRight.x, topRight.y };
		vert.textureCoords = uvMax;
		vertices.push(vert);

		vert.position = Vec2{ bottomRight.x, bottomRight.y };
		vert.textureCoords = Vec2{ uvMax.x, uvMin.y };
		vertices.push(vert);
		cmd.numVerts += 4;
	}

	void DrawList2D::addColoredQuad(const Vec2& min, const Vec2& max, const Vec4& color, AnimObjId objId)
	{
		// Check if we need to switch to a new batch
		if (drawCommands.size() == 0 || drawCommands.data[drawCommands.size() - 1].textureId != UINT32_MAX)
		{
			DrawCmd newCommand;
			newCommand.elementCounter = 0;
			newCommand.indexOffset = indices.size();
			newCommand.vertexOffset = vertices.size();
			newCommand.textureId = UINT32_MAX;
			newCommand.numElements = 0;
			newCommand.numVerts = 0;
			drawCommands.push(newCommand);
		}

		DrawCmd& cmd = drawCommands.data[drawCommands.size() - 1];

		int rectStartIndex = cmd.elementCounter;
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 1); indices.push(rectStartIndex + 2);
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 2); indices.push(rectStartIndex + 3);
		cmd.elementCounter += 4;
		cmd.numElements += 6;

		Vertex2D vert;
		vert.color = color;
		vert.objId = objId;
		vert.position = min;
		vert.textureCoords = Vec2{ 0, 0 };
		vertices.push(vert);

		vert.position = Vec2{ min.x, max.y };
		vert.textureCoords = Vec2{ 0, 1 };
		vertices.push(vert);

		vert.position = max;
		vert.textureCoords = Vec2{ 1, 1 };
		vertices.push(vert);

		vert.position = Vec2{ max.x, min.y };
		vert.textureCoords = Vec2{ 1, 0 };
		vertices.push(vert);
		cmd.numVerts += 4;
	}

	void DrawList2D::addColoredTri(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec4& color, AnimObjId objId)
	{
		// Check if we need to switch to a new batch
		if (drawCommands.size() == 0 || drawCommands.data[drawCommands.size() - 1].textureId != UINT32_MAX)
		{
			DrawCmd newCommand;
			newCommand.elementCounter = 0;
			newCommand.indexOffset = indices.size();
			newCommand.vertexOffset = vertices.size();
			newCommand.numElements = 0;
			newCommand.numVerts = 0;
			newCommand.textureId = UINT32_MAX;
			drawCommands.push(newCommand);
		}

		DrawCmd& cmd = drawCommands.data[drawCommands.size() - 1];

		int rectStartIndex = cmd.elementCounter;
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 1); indices.push(rectStartIndex + 2);
		cmd.elementCounter += 3;
		cmd.numElements += 3;

		Vertex2D vert;
		vert.color = color;
		vert.objId = objId;
		vert.position = p0;
		vert.textureCoords = Vec2{ 0, 0 };
		vertices.push(vert);

		vert.position = p1;
		vert.textureCoords = Vec2{ 0, 1 };
		vertices.push(vert);

		vert.position = p2;
		vert.textureCoords = Vec2{ 1, 1 };
		vertices.push(vert);

		cmd.numVerts += 3;
	}

	void DrawList2D::addMultiColoredTri(const Vec2& p0, const Vec4& c0, const Vec2& p1, const Vec4& c1, const Vec2& p2, const Vec4& c2, AnimObjId objId)
	{
		// Check if we need to switch to a new batch
		if (drawCommands.size() == 0 || drawCommands.data[drawCommands.size() - 1].textureId != UINT32_MAX)
		{
			DrawCmd newCommand;
			newCommand.elementCounter = 0;
			newCommand.indexOffset = indices.size();
			newCommand.vertexOffset = vertices.size();
			newCommand.numElements = 0;
			newCommand.numVerts = 0;
			newCommand.textureId = UINT32_MAX;
			drawCommands.push(newCommand);
		}

		DrawCmd& cmd = drawCommands.data[drawCommands.size() - 1];

		int rectStartIndex = cmd.elementCounter;
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 1); indices.push(rectStartIndex + 2);
		cmd.elementCounter += 3;
		cmd.numElements += 3;

		Vertex2D vert;
		vert.color = c0;
		vert.objId = objId;
		vert.position = p0;
		vert.textureCoords = Vec2{ 0, 0 };
		vertices.push(vert);

		vert.position = p1;
		vert.color = c1;
		vert.textureCoords = Vec2{ 0, 1 };
		vertices.push(vert);

		vert.position = p2;
		vert.color = c2;
		vert.textureCoords = Vec2{ 1, 1 };
		vertices.push(vert);

		cmd.numVerts += 3;
	}

	void DrawList2D::setupGraphicsBuffers()
	{
		// Create the batched vao
		GL::createVertexArray(&vao);
		GL::bindVertexArray(vao);

		GL::genBuffers(1, &vbo);

		// Allocate space for the batched vao
		GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
		GL::bufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * vertices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		GL::genBuffers(1, &ebo);
		GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		GL::bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * indices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		// Set up the batched vao attributes
		GL::vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, position)));
		GL::enableVertexAttribArray(0);

		GL::vertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, color)));
		GL::enableVertexAttribArray(1);

		GL::vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, textureCoords)));
		GL::enableVertexAttribArray(2);

		GL::vertexAttribIPointer(3, 2, GL_UNSIGNED_INT, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, objId)));
		GL::enableVertexAttribArray(3);
	}

	void DrawList2D::render(const Shader& shader, const OrthoCamera& camera) const
	{
		if (vertices.size() == 0)
		{
			return;
		}

		GL::pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, Renderer::debugMsgId++, -1, "2D_General_Pass");
		GL::enable(GL_BLEND);
		GL::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		shader.bind();
		shader.uploadMat4("uProjection", camera.calculateProjectionMatrix());
		shader.uploadMat4("uView", camera.calculateViewMatrix());
		shader.uploadInt("uWireframeOn", EditorSettings::getSettings().viewMode == ViewMode::WireMesh);

		for (int i = 0; i < drawCommands.size(); i++)
		{
			if (drawCommands.data[i].textureId != UINT32_MAX)
			{
				// Bind the texture
				GL::activeTexture(GL_TEXTURE0);
				GL::bindTexture(GL_TEXTURE_2D, drawCommands.data[i].textureId);
				shader.uploadInt("uTexture", 0);
			}
			else
			{
				GL::activeTexture(GL_TEXTURE0);
				GL::bindTexture(GL_TEXTURE_2D, Renderer::defaultWhiteTexture.graphicsId);
				shader.uploadInt("uTexture", 0);
			}

			GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
			int numVerts = drawCommands.data[i].numVerts;
			GL::bufferData(
				GL_ARRAY_BUFFER,
				sizeof(Vertex2D) * numVerts,
				vertices.data + drawCommands.data[i].vertexOffset,
				GL_DYNAMIC_DRAW
			);

			// Buffer the elements
			GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			GL::bufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint16) * drawCommands.data[i].numElements,
				indices.data + drawCommands.data[i].indexOffset,
				GL_DYNAMIC_DRAW
			);

			// TODO: Swap this with glMultiDraw...
			// Make the draw call
			GL::bindVertexArray(vao);
			GL::drawElements(
				GL_TRIANGLES,
				drawCommands.data[i].numElements,
				GL_UNSIGNED_SHORT,
				nullptr
			);
		}

		GL::popDebugGroup();
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
			GL::deleteBuffers(1, &vbo);
		}

		if (ebo != UINT32_MAX)
		{
			GL::deleteBuffers(1, &ebo);
		}

		if (vao != UINT32_MAX)
		{
			GL::deleteVertexArrays(1, &vao);
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

	void DrawListFont2D::addGlyph(const Vec2& posMin, const Vec2& posMax, const Vec2& uvMin, const Vec2& uvMax, const Vec4& color, int textureId, AnimObjId objId)
	{
		// Check if we need to switch to a new batch
		if (drawCommands.size() == 0 || drawCommands.data[drawCommands.size() - 1].textureId != textureId)
		{
			DrawCmd newCommand;
			newCommand.elementCounter = 0;
			newCommand.indexOffset = indices.size();
			newCommand.vertexOffset = vertices.size();
			newCommand.textureId = textureId;
			newCommand.numElements = 0;
			newCommand.numVerts = 0;
			drawCommands.push(newCommand);
		}

		DrawCmd& cmd = drawCommands.data[drawCommands.size() - 1];

		int rectStartIndex = cmd.elementCounter;
		// Tri 1 indices
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 1); indices.push(rectStartIndex + 2);
		// Tri 2 indices
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 2); indices.push(rectStartIndex + 3);
		cmd.elementCounter += 4;
		cmd.numElements += 6;

		Vertex2D vert;
		vert.color = color;
		vert.objId = objId;

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
		cmd.numVerts += 4;
	}

	// TODO: This is literally the exact same thing as the DrawList2D::setupGraphicsBuffers() function
	// maybe I should come up with some sort of CreateVao abstraction?
	void DrawListFont2D::setupGraphicsBuffers()
	{
		// Create the batched vao
		GL::createVertexArray(&vao);
		GL::bindVertexArray(vao);

		GL::genBuffers(1, &vbo);

		// Allocate space for the batched vao
		GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
		GL::bufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * vertices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		GL::genBuffers(1, &ebo);
		GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		GL::bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * indices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		// Set up the batched vao attributes
		GL::vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, position)));
		GL::enableVertexAttribArray(0);

		GL::vertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, color)));
		GL::enableVertexAttribArray(1);

		GL::vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, textureCoords)));
		GL::enableVertexAttribArray(2);

		GL::vertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(Vertex2D), (void*)(offsetof(Vertex2D, objId)));
		GL::enableVertexAttribArray(3);
	}

	void DrawListFont2D::render(const Shader& shader, const OrthoCamera& camera)
	{
		if (drawCommands.size() == 0)
		{
			return;
		}

		GL::pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, Renderer::debugMsgId++, -1, "2D_Font_Pass");

		shader.bind();
		shader.uploadMat4("uProjection", camera.calculateProjectionMatrix());
		shader.uploadMat4("uView", camera.calculateViewMatrix());

		for (int i = 0; i < drawCommands.size(); i++)
		{
			// Bind the texture
			GL::activeTexture(GL_TEXTURE0);
			GL::bindTexture(GL_TEXTURE_2D, drawCommands.data[i].textureId);
			shader.uploadInt("uTexture", 0);

			// Upload the verts
			// TODO: Figure out how to correctly do this stuff
			// I think this is crashing the GPU right now
			GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
			int numVerts = drawCommands.data[i].numVerts;
			GL::bufferData(
				GL_ARRAY_BUFFER,
				sizeof(Vertex2D) * numVerts,
				vertices.data + drawCommands.data[i].vertexOffset,
				GL_DYNAMIC_DRAW
			);

			// Buffer the elements
			GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			GL::bufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint16) * drawCommands.data[i].numElements,
				indices.data + drawCommands.data[i].indexOffset,
				GL_DYNAMIC_DRAW
			);

			// TODO: Swap this with glMultiDraw...
			// Make the draw call
			GL::bindVertexArray(vao);
			GL::drawElements(
				GL_TRIANGLES,
				drawCommands.data[i].numElements,
				GL_UNSIGNED_SHORT,
				nullptr
			);
		}

		GL::popDebugGroup();
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
			GL::deleteBuffers(1, &vbo);
		}

		if (ebo != UINT32_MAX)
		{
			GL::deleteBuffers(1, &ebo);
		}

		if (vao != UINT32_MAX)
		{
			GL::deleteVertexArrays(1, &vao);
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
		GL::createVertexArray(&vao);
		GL::bindVertexArray(vao);

		GL::genBuffers(1, &vbo);

		// Allocate space for the batched vao
		GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
		GL::bufferData(GL_ARRAY_BUFFER, sizeof(Vertex3DLine) * vertices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		// Set up the batched vao attributes
		GL::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, currentPos)));
		GL::enableVertexAttribArray(0);

		GL::vertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, previousPos)));
		GL::enableVertexAttribArray(1);

		GL::vertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, nextPos)));
		GL::enableVertexAttribArray(2);

		GL::vertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, thickness)));
		GL::enableVertexAttribArray(3);

		GL::vertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(Vertex3DLine), (void*)(offsetof(Vertex3DLine, color)));
		GL::enableVertexAttribArray(4);
	}

	void DrawList3DLine::render(const Shader& shader, PerspectiveCamera& camera) const
	{
		if (vertices.size() == 0)
		{
			return;
		}

		GL::pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, Renderer::debugMsgId++, -1, "3D_Line_Pass");

		GL::enable(GL_DEPTH_TEST);
		GL::disable(GL_CULL_FACE);

		GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
		GL::bufferData(GL_ARRAY_BUFFER, sizeof(Vertex3DLine) * vertices.size(), vertices.data, GL_DYNAMIC_DRAW);

		shader.bind();
		shader.uploadMat4("uProjection", camera.calculateProjectionMatrix());
		shader.uploadMat4("uView", camera.calculateViewMatrix());
		shader.uploadFloat("uAspectRatio", Application::getOutputTargetAspectRatio());

		GL::bindVertexArray(vao);
		GL::drawArrays(GL_TRIANGLES, 0, vertices.size());

		GL::disable(GL_DEPTH_TEST);

		GL::popDebugGroup();
	}

	void DrawList3DLine::reset()
	{
		vertices.softClear();
	}

	void DrawList3DLine::free()
	{
		if (vbo != UINT32_MAX)
		{
			GL::deleteBuffers(1, &vbo);
		}

		if (vao != UINT32_MAX)
		{
			GL::deleteVertexArrays(1, &vao);
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

	void DrawList3D::addTexturedQuad3D(const Texture& texture, const Vec3& bottomLeft, const Vec3& topLeft, const Vec3& topRight, const Vec3& bottomRight, const Vec2& uvMin, const Vec2& uvMax, const Vec3& faceNormal, bool isTransparent)
	{
		if (drawCommands.size() == 0 ||
			drawCommands.data[drawCommands.size() - 1].textureId != texture.graphicsId ||
			drawCommands.data[drawCommands.size() - 1].isTransparent != isTransparent)
		{
			DrawCmd3D newCommand;
			newCommand.elementCount = 0;
			newCommand.indexOffset = indices.size();
			newCommand.vertexOffset = vertices.size();
			newCommand.textureId = texture.graphicsId;
			newCommand.isTransparent = isTransparent;
			drawCommands.push(newCommand);
		}

		DrawCmd3D& cmd = drawCommands.data[drawCommands.size() - 1];

		int rectStartIndex = cmd.elementCount / 6 * 4;
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 1); indices.push(rectStartIndex + 2);
		indices.push(rectStartIndex + 0); indices.push(rectStartIndex + 2); indices.push(rectStartIndex + 3);
		cmd.elementCount += 6;

		Vertex3D vert;
		vert.color = Vec4{ 1, 1, 1, 1 };
		vert.position = bottomLeft;
		vert.textureCoords = uvMin;
		vert.normal = faceNormal;
		vertices.push(vert);

		vert.position = topLeft;
		vert.textureCoords = Vec2{ uvMin.x, uvMax.y };
		vertices.push(vert);

		vert.position = topRight;
		vert.textureCoords = uvMax;
		vertices.push(vert);

		vert.position = bottomRight;
		vert.textureCoords = Vec2{ uvMax.x, uvMin.y };
		vertices.push(vert);
	}

	void DrawList3D::setupGraphicsBuffers()
	{
		// Vec3 position;
		// Vec4 color;
		// Vec2 textureCoords;
		// Create the batched vao
		GL::createVertexArray(&vao);
		GL::bindVertexArray(vao);

		// Allocate space for the batched vbo
		GL::genBuffers(1, &vbo);
		GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
		GL::bufferData(GL_ARRAY_BUFFER, sizeof(Vertex3D) * vertices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		GL::genBuffers(1, &ebo);
		GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		GL::bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32) * indices.maxCapacity, NULL, GL_DYNAMIC_DRAW);

		// Set up the batched vao attributes
		GL::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(offsetof(Vertex3D, position)));
		GL::enableVertexAttribArray(0);

		// TODO: Change me to a packed color in 1 uint32
		GL::vertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(offsetof(Vertex3D, color)));
		GL::enableVertexAttribArray(1);

		GL::vertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(offsetof(Vertex3D, textureCoords)));
		GL::enableVertexAttribArray(2);

		GL::vertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(offsetof(Vertex3D, normal)));
		GL::enableVertexAttribArray(3);
	}

	void DrawList3D::render(const Shader& opaqueShader, const Shader& transparentShader,
		const Shader& compositeShader, const Framebuffer& framebuffer, PerspectiveCamera& camera) const
	{
		if (vertices.size() == 0)
		{
			return;
		}

		GL::pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, Renderer::debugMsgId++, -1, "3D_OIT_Pass");

		Vec4 sunColor = "#ffffffff"_hex;

		// Enable depth testing and depth buffer writes
		GL::depthMask(GL_TRUE);
		GL::enable(GL_DEPTH_TEST);

		// First render opaque objects
		opaqueShader.bind();
		opaqueShader.uploadMat4("uProjection", camera.calculateProjectionMatrix());
		opaqueShader.uploadMat4("uView", camera.calculateViewMatrix());
		//opaqueShader.uploadVec3("sunDirection", glm::vec3(0.3f, -0.2f, -0.8f));
		//opaqueShader.uploadVec3("sunColor", glm::vec3(sunColor.r, sunColor.g, sunColor.b));

		for (int i = 0; i < drawCommands.size(); i++)
		{
			if (drawCommands.data[i].isTransparent)
			{
				continue;
			}

			GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
			int numVerts = drawCommands.data[i].elementCount / 6 * 4;
			GL::bufferData(
				GL_ARRAY_BUFFER,
				sizeof(Vertex3D) * numVerts,
				vertices.data + drawCommands.data[i].vertexOffset,
				GL_DYNAMIC_DRAW
			);

			GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			GL::bufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint16) * drawCommands.data[i].elementCount,
				indices.data + drawCommands.data[i].indexOffset,
				GL_DYNAMIC_DRAW
			);

			GL::activeTexture(GL_TEXTURE0);
			GL::bindTexture(GL_TEXTURE_2D, drawCommands.data[i].textureId);
			transparentShader.uploadInt("uTexture", 0);

			GL::bindVertexArray(vao);
			GL::drawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, NULL);

			// TODO: Swap this with glMultiDraw...
			// Make the draw call
			GL::bindVertexArray(vao);
			GL::drawElements(
				GL_TRIANGLES,
				drawCommands.data[i].elementCount,
				GL_UNSIGNED_SHORT,
				nullptr
			);
		}

		framebuffer.bind();

		// Set up the transparent draw buffers
		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, GL_COLOR_ATTACHMENT3 };
		GL::drawBuffers(4, drawBuffers);

		// Set up GL state for transparent pass
		// Disable writing to the depth buffer
		GL::depthMask(GL_FALSE);
		GL::enable(GL_DEPTH_TEST);
		GL::disable(GL_CULL_FACE);
		GL::enable(GL_BLEND);

		// These values are obtained from http://casual-effects.blogspot.com/2015/03/implemented-weighted-blended-order.html
		// Under the 3D transparency pass table
		GL::blendEquation(GL_FUNC_ADD);
		GL::blendFunci(1, GL_ONE, GL_ONE);
		GL::blendFunci(2, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

		// Clear the buffers
		float accumulationClear[4] = { 0, 0, 0, 0 };
		float revealageClear[4] = { 1, 0, 0, 0 };
		GL::clearBufferfv(GL_COLOR, 1, accumulationClear);
		GL::clearBufferfv(GL_COLOR, 2, revealageClear);

		// Then render the transparent surfaces
		transparentShader.bind();
		transparentShader.uploadMat4("uProjection", camera.calculateProjectionMatrix());
		transparentShader.uploadMat4("uView", camera.calculateViewMatrix());
		//transparentShader.uploadVec3("sunDirection", glm::vec3(0.3f, -0.2f, -0.8f));
		//transparentShader.uploadVec3("sunColor", glm::vec3(sunColor.r, sunColor.g, sunColor.b));

		for (int i = 0; i < drawCommands.size(); i++)
		{
			if (!drawCommands.data[i].isTransparent)
			{
				continue;
			}

			GL::bindBuffer(GL_ARRAY_BUFFER, vbo);
			int numVerts = drawCommands.data[i].elementCount / 6 * 4;
			GL::bufferData(
				GL_ARRAY_BUFFER,
				sizeof(Vertex3D) * numVerts,
				vertices.data + drawCommands.data[i].vertexOffset,
				GL_DYNAMIC_DRAW
			);

			GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			GL::bufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint16) * drawCommands.data[i].elementCount,
				indices.data + drawCommands.data[i].indexOffset,
				GL_DYNAMIC_DRAW
			);

			GL::activeTexture(GL_TEXTURE0);
			GL::bindTexture(GL_TEXTURE_2D, drawCommands.data[i].textureId);
			transparentShader.uploadInt("uTexture", 0);

			GL::bindVertexArray(vao);
			GL::drawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, NULL);

			// TODO: Swap this with glMultiDraw...
			// Make the draw call
			GL::bindVertexArray(vao);
			GL::drawElements(
				GL_TRIANGLES,
				drawCommands.data[i].elementCount,
				GL_UNSIGNED_SHORT,
				nullptr
			);
		}

		// Composite the accumulation and revealage textures together
		// Render to the composite framebuffer attachment
		GL::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		compositeShader.bind();

		const Texture& accumulationTexture = framebuffer.getColorAttachment(1);
		const Texture& revealageTexture = framebuffer.getColorAttachment(2);

		GL::activeTexture(GL_TEXTURE0);
		accumulationTexture.bind();
		compositeShader.uploadInt("uAccumTexture", 0);

		GL::activeTexture(GL_TEXTURE1);
		revealageTexture.bind();
		compositeShader.uploadInt("uRevealageTexture", 1);

		// Set up the composite draw buffers
		GLenum compositeDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE, GL_NONE };
		GL::drawBuffers(4, compositeDrawBuffers);

		GL::bindVertexArray(Renderer::screenVao);
		GL::drawArrays(GL_TRIANGLES, 0, 6);

		// Reset GL state
		// Enable writing to the depth buffer again
		GL::depthMask(GL_TRUE);

		GL::popDebugGroup();
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
			GL::deleteBuffers(1, &vbo);
		}

		if (ebo != UINT32_MAX)
		{
			GL::deleteBuffers(1, &ebo);
		}

		if (vao != UINT32_MAX)
		{
			GL::deleteVertexArrays(1, &vao);
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
