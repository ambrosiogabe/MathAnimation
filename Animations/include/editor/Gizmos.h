#ifndef MATH_ANIM_GIZMOS_H
#define MATH_ANIM_GIZMOS_H
#include "core.h"

namespace MathAnim
{
	struct Framebuffer;
	struct AnimationManagerData;

	enum class FreeMoveVariant : uint8
	{
		None = 0,
		FreeMove = 0x1,
		HorizontalMove = 0x2,
		VerticalMove = 0x4,
		All = 0xF
	};

	namespace GizmoManager
	{
		void init();
		void update(AnimationManagerData* am);
		void render(const Framebuffer& framebuffer);
		void free();

		void freeMoveGizmo(const char* gizmoName, Vec3& position, FreeMoveVariant variant = FreeMoveVariant::All);
	}
}

#endif 