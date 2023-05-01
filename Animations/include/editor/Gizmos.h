#ifndef MATH_ANIM_GIZMOS_H
#define MATH_ANIM_GIZMOS_H
#include "core.h"

namespace MathAnim
{
	struct Framebuffer;
	struct AnimationManagerData;
	struct Camera;
	struct Texture;

	enum class GizmoVariant : uint8
	{
		None       = 0,
		Free       = 0b0001,
		Horizontal = 0b0010,
		Vertical   = 0b0100,
		Forward    = 0b1000,
		All        = 0b1111
	};

	enum class GizmoType : uint8
	{
		None,
		Translation,
		Rotation,
		Scaling
	};

	namespace GizmoManager
	{
		void init();
		void update(AnimationManagerData* am);
		void render(AnimationManagerData* am);
		void renderOrientationGizmo(const Camera& editorCamera);
		void free();

		const Texture& getCameraOrientationTexture();

		bool anyGizmoActive();

		bool translateGizmo(const char* gizmoName, Vec3* position, GizmoVariant variant = GizmoVariant::All);
		bool rotateGizmo(const char* gizmoName, Vec3* rotation, GizmoVariant variant = GizmoVariant::All);
		bool scaleGizmo(const char* gizmoName, Vec3* scale, GizmoVariant variant = GizmoVariant::All);
	}
}

#endif 