#ifndef MATH_ANIM_GIZMOS_H
#define MATH_ANIM_GIZMOS_H
#include "core.h"

namespace MathAnim
{
	struct Framebuffer;
	struct AnimationManagerData;
	struct OrthoCamera;
	struct PerspectiveCamera;

	enum class GizmoVariant : uint8
	{
		None = 0,
		Free = 0x1,
		Horizontal = 0x2,
		Vertical = 0x4,
		All = 0xF
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
		void free();

		bool anyGizmoActive();

		bool translateGizmo(const char* gizmoName, Vec3* position, GizmoVariant variant = GizmoVariant::All);
		bool rotateGizmo(const char* gizmoName, Vec3* rotation, GizmoVariant variant = GizmoVariant::All);
		bool scaleGizmo(const char* gizmoName, Vec3* scale, GizmoVariant variant = GizmoVariant::All);
	}
}

#endif 