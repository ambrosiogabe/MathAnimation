#ifndef MATH_ANIM_APPLICATION_H
#define MATH_ANIM_APPLICATION_H
#include "core.h"

namespace MathAnim
{
	class GlobalThreadPool;
	struct OrthoCamera;
	class SvgCache;

	enum class AnimState : uint8
	{
		PlayForward,
		PlayReverse,
		Pause,
	};

	namespace Application
	{
		void init(const char* projectFile);

		void run();

		void free();

		float getDeltaTime();
		float getOutputTargetAspectRatio();
		glm::vec2 getOutputSize();
		glm::vec2 getViewportSize();
		glm::vec2 getAppWindowSize();

		void saveProject();
		void saveCurrentScene();
		void loadProject(const std::string& projectRoot);
		void loadScene(const std::string& sceneName);
		void deleteScene(const std::string& sceneName);
		void changeSceneTo(const std::string& sceneName, bool saveCurrentScene = true);

		void setEditorPlayState(AnimState state);
		AnimState getEditorPlayState();

		// TODO: Consolidate and remove some of these?
		void setFrameIndex(int frame);
		int getFrameIndex();
		int getFrameratePerSecond();
		void resetToFrame(int frame);

		void exportVideoTo(const std::string& filename);
		bool isExportingVideo();
		void endExport();

		// TODO: Ugly hack
		OrthoCamera* getEditorCamera();
		// TODO: Ugly hack
		SvgCache* getSvgCache();

		GlobalThreadPool* threadPool();
	}
}

#endif