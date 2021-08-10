#include "animation/AnimationBuilders.h"

namespace MathAnim
{
	static glm::vec2 defaultEquation(float t)
	{
		return {
			t,
			t
		};
	}

	// ========================================================================================================
	// 	   Parametric Animation Builder
	// ========================================================================================================
	ParametricAnimationBuilder::ParametricAnimationBuilder()
	{
		animation.delay = 0;
		animation.duration = 1.0f;
		animation.startT = 0.0f;
		animation.endT = 1.0f;
		animation.granularity = 1;
		animation.parametricEquation = defaultEquation;
		animation.startTime = 0;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setFunction(ParametricFunction function)
	{
		animation.parametricEquation = function;
		return *this;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setDuration(float duration)
	{
		animation.duration = duration;
		return *this;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setDelay(float delay)
	{
		animation.delay = delay;
		return *this;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setStartT(float startT)
	{
		animation.startT = startT;
		return *this;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setEndT(float endT)
	{
		animation.endT = endT;
		return *this;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setGranularity(int32 granularity)
	{
		animation.granularity = granularity;
		return *this;
	}

	ParametricAnimation ParametricAnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Text Animation Builder
	// ========================================================================================================
	TextAnimationBuilder::TextAnimationBuilder()
	{
		animation.startTime = 0.0f;
		animation.font = nullptr;
		animation.position = glm::vec2();
		animation.scale = 1.0f;
		animation.text = "";
		animation.typingTime = 0.1f;
		animation.delay = 0;
	}

	TextAnimationBuilder& TextAnimationBuilder::setTypingTime(float typingTime)
	{
		animation.typingTime = typingTime;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setScale(float scale)
	{
		animation.scale = scale;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setPosition(const glm::vec2& position)
	{
		animation.position = position;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setFont(Font* font)
	{
		animation.font = font;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setText(const std::string& text)
	{
		animation.text = text;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setDelay(float delay)
	{
		animation.delay = delay;
		return *this;
	}

	TextAnimation TextAnimationBuilder::build()
	{
		return animation;
	}
}