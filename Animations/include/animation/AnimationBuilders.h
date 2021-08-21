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

	class BitmapAnimationBuilder
	{
	public:
		BitmapAnimationBuilder();

		BitmapAnimationBuilder& setBitmap(glm::vec4 bitmap[16][16]);
		BitmapAnimationBuilder& setDelay(float delay);
		BitmapAnimationBuilder& setDuration(float duration);
		BitmapAnimationBuilder& setCanvasPosition(const glm::vec2& canvasPosition);
		BitmapAnimationBuilder& setCanvasSize(const glm::vec2& canvasSize);
		BitmapAnimationBuilder& setRevealTime(float revealTime);

		BitmapAnimation build();

	private:
		BitmapAnimation animation;
	};

	class Bezier1AnimationBuilder
	{
	public:
		Bezier1AnimationBuilder();

		Bezier1AnimationBuilder& setP0(const glm::vec2& point);
		Bezier1AnimationBuilder& setP1(const glm::vec2& point);
		Bezier1AnimationBuilder& setDelay(float delay);
		Bezier1AnimationBuilder& setDuration(float duration);
		Bezier1AnimationBuilder& withPoints();

		Bezier1Animation build();

	private:
		Bezier1Animation animation;
	};

	class Bezier2AnimationBuilder
	{
	public:
		Bezier2AnimationBuilder();

		Bezier2AnimationBuilder& setP0(const glm::vec2& point);
		Bezier2AnimationBuilder& setP1(const glm::vec2& point);
		Bezier2AnimationBuilder& setP2(const glm::vec2& point);
		Bezier2AnimationBuilder& setDelay(float delay);
		Bezier2AnimationBuilder& setDuration(float duration);
		Bezier2AnimationBuilder& withPoints();

		Bezier2Animation build();

	private:
		Bezier2Animation animation;
	};

	class FilledCircleAnimationBuilder
	{
	public:
		FilledCircleAnimationBuilder();

		FilledCircleAnimationBuilder& setPosition(const glm::vec2& point);
		FilledCircleAnimationBuilder& setRadius(float radius);
		FilledCircleAnimationBuilder& setNumSegments(int numSegments);
		FilledCircleAnimationBuilder& setDelay(float delay);
		FilledCircleAnimationBuilder& setDuration(float duration);

		FilledCircleAnimation build();

	private:
		FilledCircleAnimation animation;
	};

	class FilledBoxAnimationBuilder
	{
	public:
		FilledBoxAnimationBuilder();

		FilledBoxAnimationBuilder& setCenter(const glm::vec2& point);
		FilledBoxAnimationBuilder& setSize(const glm::vec2& size);
		FilledBoxAnimationBuilder& setFillDirection(Direction direction);
		FilledBoxAnimationBuilder& setDelay(float delay);
		FilledBoxAnimationBuilder& setDuration(float duration);

		FilledBoxAnimation build();

	private:
		FilledBoxAnimation animation;
	};
}

#endif 
