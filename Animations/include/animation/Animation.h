#ifndef MATH_ANIM_ANIMATION_H
#define MATH_ANIM_ANIMATION_H
#include "core.h"

namespace MathAnim
{
	struct Style;
	struct Font;
	typedef glm::vec2(*ParametricFunction)(float t);

	struct ParametricAnimation
	{
		int32 granularity;
		float startT;
		float endT;
		float delay;
		float startTime;
		float duration;
		ParametricFunction parametricEquation;
	};

	struct TextAnimation
	{
		float typingTime;
		float startTime;
		float scale;
		float delay;
		glm::vec2 position;
		Font* font;
		std::string text;
	};

	namespace AnimationManager
	{
		void addParametricAnimation(ParametricAnimation& animation, const Style& style);
		void addTextAnimation(TextAnimation& animation, const Style& style);
		void update(float dt);
		void reset();
	}
}

#endif
