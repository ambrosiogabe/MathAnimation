#ifndef MATH_ANIM_APPLICATION_H
#define MATH_ANIM_APPLICATION_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	class GlobalThreadPool;
	struct OrthoCamera;

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

		float getOutputTargetAspectRatio();
		glm::vec2 getOutputSize();
		glm::vec2 getViewportSize();

		void saveProject();
		void loadProject(const char* filepath);

		void setEditorPlayState(AnimState state);
		AnimState getEditorPlayState();

		void setFrameIndex(int frame);
		int getFrameIndex();
		int getFrameratePerSecond();

		void exportVideoTo(const std::string& filename);
		bool isExportingVideo();
		void endExport();

		// TODO: Ugly hack
		OrthoCamera* getEditorCamera();

		NVGcontext* getNvgContext();
		GlobalThreadPool* threadPool();
	}
}

#endif