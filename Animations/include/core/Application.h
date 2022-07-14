#ifndef MATH_ANIM_APPLICATION_H
#define MATH_ANIM_APPLICATION_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	class GlobalThreadPool;

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

		void setEditorPlayState(AnimState state);
		AnimState getEditorPlayState();

		void setFrameIndex(int frame);
		int getFrameIndex();
		int getFrameratePerSecond();

		NVGcontext* getNvgContext();
		GlobalThreadPool* threadPool();
	}
}

#endif