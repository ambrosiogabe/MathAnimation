#ifndef MATH_ANIM_EDITOR_CAMERA_CONTROLLER_H
#define MATH_ANIM_EDITOR_CAMERA_CONTROLLER_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct Camera;
	struct EditorCamera;

	namespace EditorCameraController
	{
		EditorCamera* init(const Camera& camera);
		void free(EditorCamera* editorCamera);

		void update(float deltaTime, EditorCamera* camera);
		void endFrame(EditorCamera* camera);

		const Camera& getCamera(const EditorCamera* editorCamera);

		void serialize(const EditorCamera* camera, nlohmann::json& j);
		EditorCamera* deserialize(const nlohmann::json& j, uint32 version);
	}
}

#endif 