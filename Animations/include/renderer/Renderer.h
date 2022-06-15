#ifndef MATH_ANIM_RENDERER_H
#define MATH_ANIM_RENDERER_H
#include "core.h"

namespace MathAnim
{
	struct OrthoCamera;
	struct PerspectiveCamera;
	struct Shader;
	struct Framebuffer;
	struct Texture;
	struct Font;

	enum class CapType
	{
		Flat,
		Arrow
	};

	struct RenderableTexture
	{
		const Texture* texture;
		Vec2 start;
		Vec2 size;
		Vec2 texCoordStart;
		Vec2 texCoordSize;
	};

	namespace Renderer
	{
		void init(OrthoCamera& sceneCamera, PerspectiveCamera& camera);

		// ----------- Render calls ----------- 
		void render();
		void renderFramebuffer(const Framebuffer& framebuffer);

		// ----------- Styles ----------- 
		// TODO: Should this be push/pop calls, or more like nvgStroke calls with implicit pops?
		void pushStrokeWidth(float strokeWidth);
		void pushColor(const glm::vec4& color);
		void pushColor(const Vec4& color);
		void pushLineEnding(CapType lineEnding);

		void popStrokeWidth(int numToPop = 1);
		void popColor(int numToPop = 1);
		void popLineEnding(int numToPop = 1);

		// ----------- 2D stuff ----------- 
		// TODO: Switch to using this when drawing completed objects to potentially
		// batch draw calls together and improve performance
		void drawSquare(const Vec2& start, const Vec2& size);
		void drawFilledSquare(const Vec2& start, const Vec2& size);
		void drawLine(const Vec2& start, const Vec2& end);
		void drawTexture(const RenderableTexture& renderable, const Vec4& color);
		void drawString(const std::string& string, const Font& font, const Vec2& start, float scale, const Vec4& color);
		void drawFilledCircle(const Vec2& position, float radius, int numSegments);
		void drawFilledTriangle(const Vec2& p0, const Vec2& p1, const Vec2& p2);

		// ----------- 3D stuff ----------- 
		void drawLine3D(const Vec3& start, const Vec3& end);

		// ----------- Miscellaneous ----------- 
		const OrthoCamera* getOrthoCamera();
		OrthoCamera* getMutableOrthoCamera();

		const PerspectiveCamera* get3DCamera();
		PerspectiveCamera* getMutable3DCamera();
		
		void flushBatch();
		void flushBatch3D();

		void clearColor(const Vec4& color);
	}
}

#endif
