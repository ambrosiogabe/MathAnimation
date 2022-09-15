#ifndef MATH_ANIM_EDITOR_CAMERA_CONTROLLER_H
#define MATH_ANIM_EDITOR_CAMERA_CONTROLLER_H

namespace MathAnim
{
	struct OrthoCamera;
	struct PerspectiveCamera;

	namespace EditorCameraController
	{
		void updateOrtho(OrthoCamera& camera);
		void updatePerspective(PerspectiveCamera& camera);
	}
}

#endif 