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
	struct AnimationManagerData;
	struct Path2DContext;

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
		void renderToFramebuffer(Framebuffer& framebuffer, const Vec4& clearColor, const OrthoCamera& orthoCamera, PerspectiveCamera& perspectiveCamera, bool shouldRenderPickingOutline);
		void renderFramebuffer(const Framebuffer& framebuffer);
		void endFrame();

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
		void drawFilledQuad(const Vec2& start, const Vec2& size, uint32 objId = UINT32_MAX);
		void drawTexturedQuad(const Texture& texture, const Vec2& size, const Vec2& uvMin, const Vec2& uvMax, const Vec4& color, AnimObjId objId, const glm::mat4& transform = glm::identity<glm::mat4>());
		void drawFilledTri(const Vec2& p0, const Vec2& p1, const Vec2& p2, uint32 objId = UINT32_MAX);
		void drawLine(const Vec2& start, const Vec2& end, const Vec2& startNormal = {FLT_MAX, FLT_MAX}, float startThickness = 0.0f, const Vec2& endNormal = {FLT_MAX, FLT_MAX}, float endThickness = 0.0f);
		void drawString(const std::string& string, const Vec2& start, uint32 objId);
		void drawFilledCircle(const Vec2& position, float radius, int numSegments);

		// ----------- 2D Line stuff ----------- 
		Path2DContext* beginPath(const Vec2& start, const glm::mat4& transform = glm::identity<glm::mat4>(), const Vec2& normal = Vec2{ FLT_MAX, FLT_MAX });
		void free(Path2DContext* path);
		void endPath(Path2DContext* path, bool closePath = true);

		void lineTo(Path2DContext* path, const Vec2& point, bool applyTransform = true, const Vec2& normal = Vec2{ FLT_MAX, FLT_MAX });
		void quadTo(Path2DContext* path, const Vec2& p1, const Vec2& p2);
		void cubicTo(Path2DContext* path, const Vec2& p1, const Vec2& p2, const Vec2& p3);

		void setTransform(Path2DContext* path, const glm::mat4& transform);

		// ----------- 3D Line stuff ----------- 
		void beginPath3D(const Vec3& start, const Vec3& normal = Vec3{FLT_MAX, FLT_MAX, FLT_MAX});
		void endPath3D(bool closePath = true);

		void lineTo3D(const Vec3& point, bool applyTransform = true, const Vec3& normal = Vec3{FLT_MAX, FLT_MAX, FLT_MAX});
		void bezier2To3D(const Vec3& p1, const Vec3& p2);
		void bezier3To3D(const Vec3& p1, const Vec3& p2, const Vec3& p3);

		void translate3D(const Vec3& translation);
		void rotate3D(const Vec3& eulerAngles);
		void setTransform(const glm::mat4& transform);
		void resetTransform3D();

		// ----------- 3D stuff ----------- 
		void drawFilledCube(const Vec3& center, const Vec3& size);
		void drawTexturedQuad3D(const Texture& texture, const Vec2& size, const Vec2& uvMin, const Vec2& uvMax, const glm::mat4& transform = glm::identity<glm::mat4>(), bool isTransparent = false);

		// ----------- Miscellaneous ----------- 
		void clearColor(const Vec4& color);
	}
}

#endif
