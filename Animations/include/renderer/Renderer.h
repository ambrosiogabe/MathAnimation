#ifndef MATH_ANIM_RENDERER_H
#define MATH_ANIM_RENDERER_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	struct OrthoCamera;
	struct PerspectiveCamera;
	struct Shader;
	struct Framebuffer;
	struct Texture;
	struct Font;
	struct SizedFont;

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
		void free();

		// ----------- Render calls ----------- 
		void renderToFramebuffer(NVGcontext* vg, int frame, Framebuffer& framebuffer);
		void renderFramebuffer(const Framebuffer& framebuffer);

		// ----------- Styles ----------- 
		// TODO: Should this be push/pop calls, or more like nvgStroke calls with implicit pops?
		void pushStrokeWidth(float strokeWidth);
		void pushColor(const glm::u8vec4& color);
		void pushColor(const glm::vec4& color);
		void pushColor(const Vec4& color);
		void pushLineEnding(CapType lineEnding);
		void pushFont(const SizedFont* sizedFont);

		void popStrokeWidth(int numToPop = 1);
		void popColor(int numToPop = 1);
		void popLineEnding(int numToPop = 1);
		void popFont(int numToPop = 1);

		// ----------- 2D stuff ----------- 
		// TODO: Switch to using this when drawing completed objects to potentially
		// batch draw calls together and improve performance
		void drawSquare(const Vec2& start, const Vec2& size);
		void drawFilledSquare(const Vec2& start, const Vec2& size);
		void drawLine(const Vec2& start, const Vec2& end);
		void drawTexture(const RenderableTexture& renderable, const Vec4& color);
		void drawString(const std::string& string, const Vec2& start);
		void drawFilledCircle(const Vec2& position, float radius, int numSegments);
		void drawFilledTriangle(const Vec2& p0, const Vec2& p1, const Vec2& p2);

		// ----------- 3D Line stuff ----------- 
		void beginPath3D(const Vec3& start);
		void endPath3D(bool closePath = true);

		void lineTo3D(const Vec3& point, bool applyTransform = true);
		void bezier2To3D(const Vec3& p1, const Vec3& p2);
		void bezier3To3D(const Vec3& p1, const Vec3& p2, const Vec3& p3);

		void translate3D(const Vec3& translation);
		void rotate3D(const Vec3& eulerAngles);
		void resetTransform3D();

		// ----------- 3D stuff ----------- 
		void drawFilledCube(const Vec3& center, const Vec3& size);

		// ----------- Miscellaneous ----------- 
		const OrthoCamera* getOrthoCamera();
		OrthoCamera* getMutableOrthoCamera();

		const PerspectiveCamera* get3DCamera();
		PerspectiveCamera* getMutable3DCamera();

		void clearColor(const Vec4& color);
	}
}

#endif
