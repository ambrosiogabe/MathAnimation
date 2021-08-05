#ifndef MATH_ANIM_ANIMATION_H
#define MATH_ANIM_ANIMATION_H
#include "core.h"

namespace MathAnim
{
	struct Style;
	struct Font;
	typedef glm::vec2(*Function)(float t);

	struct ParametricAnimation
	{
		int32 granularity;
		float startT;
		float endT;
		float startTime;
		float duration;
		Function parametricEquation;
	};

	struct TextAnimation
	{
		float typingTime;
		float startTime;
		float scale;
		glm::vec2 position;
		Font* font;
		std::string text;
	};

	namespace AnimationManager
	{
		void addParametricAnimation(const ParametricAnimation& animation, const Style& style);
		void addTextAnimation(const TextAnimation& animation, const Style& style);
		void update(float dt);
		void reset();
	}
}

#endif
