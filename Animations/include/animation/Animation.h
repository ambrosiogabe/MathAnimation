#ifndef MATH_ANIM_ANIMATION_H
#define MATH_ANIM_ANIMATION_H
#include "core.h"

namespace MathAnim
{
	struct Style;
	typedef glm::vec2(*Function)(float t);

	struct Animation
	{
		int32 granularity;
		float startT;
		float endT;
		float startTime;
		float duration;
		Function parametricEquation;
	};

	namespace AnimationManager
	{
		void addAnimation(const Animation& animation, const Style& style);
		void update(float dt);
		void reset();
	}
}

#endif
