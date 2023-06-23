#ifndef MATH_ANIM_APPLICATION_H
#define MATH_ANIM_APPLICATION_H
#include "core.h"

namespace MathAnim
{
	class GlobalThreadPool;
	class SvgCache;
	struct Framebuffer;
	struct Window;
	struct Camera;
	struct UndoSystemData;

	enum class AnimState : uint8
	{
		PlayForward,
		PlayReverse,
		PlayForwardFixedFrameTime,
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
		const Window& getWindow();

		void saveProject();
		void saveCurrentScene();
		void loadProject(const std::filesystem::path& projectRoot);
		void loadScene(const std::string& sceneName);
		void deleteScene(const std::string& sceneName);
		void changeSceneTo(const std::string& sceneName, bool saveCurrentScene = true);

		void setEditorPlayState(AnimState state);
		AnimState getEditorPlayState();

		// TODO: Consolidate and remove some of these?
		void setFrameIndex(int frame);
		int getFrameIndex();
		void resetToFrame(int frame);

		const Framebuffer& getMainFramebuffer();
		const std::filesystem::path& getCurrentProjectRoot();
		const std::filesystem::path& getTmpDir();

		const Camera* getEditorCamera();
		// TODO: Ugly hack
		SvgCache* getSvgCache();
		UndoSystemData* getUndoSystem();

		GlobalThreadPool* threadPool();
	}
}

#endif