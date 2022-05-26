#ifndef MATH_ANIM_APPLICATION_H
#define MATH_ANIM_APPLICATION_H
#include "core.h"

namespace MathAnim
{
	enum class AnimState : uint8
	{
		PlayForward,
		PlayReverse,
		Pause,
	};

	namespace Application
	{
		void init();

		void run();

		void free();

		float getOutputTargetAspectRatio();

		void setEditorPlayState(AnimState state);
	}
}

#endif