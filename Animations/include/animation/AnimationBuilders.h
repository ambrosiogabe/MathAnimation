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

		Animation build();

	private:
		Animation animation;
	};

	class TextAnimationBuilder
	{
	public:
		TextAnimationBuilder();

		TextAnimationBuilder& setTypingTime(float typingTime);
		TextAnimationBuilder& setScale(float scale);
		TextAnimationBuilder& setPosition(const Vec2& position);
		TextAnimationBuilder& setFont(Font* font);
		TextAnimationBuilder& setText(const std::string& text);
		TextAnimationBuilder& setDelay(float delay);

		Animation build();

	private:
		Animation animation;
	};

	class BitmapAnimationBuilder
	{
	public:
		BitmapAnimationBuilder();

		BitmapAnimationBuilder& setBitmap(Vec4 bitmap[16][16]);
		BitmapAnimationBuilder& setDelay(float delay);
		BitmapAnimationBuilder& setDuration(float duration);
		BitmapAnimationBuilder& setCanvasPosition(const Vec2& canvasPosition);
		BitmapAnimationBuilder& setCanvasSize(const Vec2& canvasSize);
		BitmapAnimationBuilder& setRevealTime(float revealTime);

		Animation build();

	private:
		Animation animation;
	};

	class Bezier1AnimationBuilder
	{
	public:
		Bezier1AnimationBuilder();

		Bezier1AnimationBuilder& setP0(const Vec2& point);
		Bezier1AnimationBuilder& setP1(const Vec2& point);
		Bezier1AnimationBuilder& setDelay(float delay);
		Bezier1AnimationBuilder& setDuration(float duration);
		Bezier1AnimationBuilder& withPoints();

		Animation build();

	private:
		Animation animation;
	};

	class Bezier2AnimationBuilder
	{
	public:
		Bezier2AnimationBuilder();

		Bezier2AnimationBuilder& setP0(const Vec2& point);
		Bezier2AnimationBuilder& setP1(const Vec2& point);
		Bezier2AnimationBuilder& setP2(const Vec2& point);
		Bezier2AnimationBuilder& setDelay(float delay);
		Bezier2AnimationBuilder& setDuration(float duration);
		Bezier2AnimationBuilder& withPoints();

		Animation build();

	private:
		Animation animation;
	};

	class FilledCircleAnimationBuilder
	{
	public:
		FilledCircleAnimationBuilder();

		FilledCircleAnimationBuilder& setPosition(const Vec2& point);
		FilledCircleAnimationBuilder& setRadius(float radius);
		FilledCircleAnimationBuilder& setNumSegments(int numSegments);
		FilledCircleAnimationBuilder& setDelay(float delay);
		FilledCircleAnimationBuilder& setDuration(float duration);

		Animation build();

	private:
		Animation animation;
	};

	class FilledBoxAnimationBuilder
	{
	public:
		FilledBoxAnimationBuilder();

		FilledBoxAnimationBuilder& setCenter(const Vec2& point);
		FilledBoxAnimationBuilder& setSize(const Vec2& size);
		FilledBoxAnimationBuilder& setFillDirection(Direction direction);
		FilledBoxAnimationBuilder& setDelay(float delay);
		FilledBoxAnimationBuilder& setDuration(float duration);

		Animation build();

	private:
		Animation animation;
	};
}

#endif 
