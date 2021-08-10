#ifndef MANIM_ANIMATION_BUILDERS_H
#define MANIM_ANIMATION_BUILDERS_H
#include "core.h"
#include "animation/Animation.h"

namespace MathAnim
{
	class ParametricAnimationBuilder
	{
	public:
		ParametricAnimationBuilder();

		ParametricAnimationBuilder& setFunction(ParametricFunction function);
		ParametricAnimationBuilder& setDuration(float duration);
		ParametricAnimationBuilder& setDelay(float delay);
		ParametricAnimationBuilder& setStartT(float startT);
		ParametricAnimationBuilder& setEndT(float endT);
		ParametricAnimationBuilder& setGranularity(int32 granularity);

		ParametricAnimation build();

	private:
		ParametricAnimation animation;
	};

	class TextAnimationBuilder
	{
	public:
		TextAnimationBuilder();

		TextAnimationBuilder& setTypingTime(float typingTime);
		TextAnimationBuilder& setScale(float scale);
		TextAnimationBuilder& setPosition(const glm::vec2& position);
		TextAnimationBuilder& setFont(Font* font);
		TextAnimationBuilder& setText(const std::string& text);
		TextAnimationBuilder& setDelay(float delay);

		TextAnimation build();

	private:
		TextAnimation animation;
	};
}

#endif 
