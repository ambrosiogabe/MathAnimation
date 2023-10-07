#ifndef MATH_ANIM_GIZMOS_H
#define MATH_ANIM_GIZMOS_H
#include "core.h"

#include "editor/imgui/ImGuiExtended.h"

namespace MathAnim
{
	struct Framebuffer;
	struct AnimationManagerData;
	struct Camera;
	struct Texture;

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
		void changeVisualMode(GizmoType type);
		const char* getVisualModeStr();
		GizmoType getVisualMode();

		ImGuiDataEx<Vec3> translateGizmo(const char* gizmoName, Vec3* position, bool drawCubeVisualization = false);
		ImGuiDataEx<Vec3> rotateGizmo(const char* gizmoName, const Vec3& gizmoPosition, Vec3* rotation);
		ImGuiDataEx<Vec3> scaleGizmo(const char* gizmoName, const Vec3& gizmoPosition, Vec3* scale);
	}
}

#endif 