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
		animation.translation = { 0, 0 };
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

	// ========================================================================================================
	// 	   Bitmap Animation Builder
	// ========================================================================================================
	BitmapAnimationBuilder::BitmapAnimationBuilder()
	{
		for (int y = 0; y < 16; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				animation.bitmap[y][x] = "#000000"_hex;
				animation.bitmapState[y][x] = false;
			}
		}

		animation.delay = 0;
		animation.duration = 1.0f;
		animation.revealTime = 0.01f;
		animation.bitmapSquaresShowing = 0;
		animation.startTime = 0;
		animation.canvasPosition = glm::vec2({ -2, -2 });
		animation.canvasSize = glm::vec2({ 4, 4 });
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setBitmap(glm::vec4 bitmap[16][16])
	{
		for (int y = 0; y < 16; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				animation.bitmap[y][x] = bitmap[16 - y - 1][x];
			}
		}
		return *this;
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setDelay(float delay)
	{
		animation.delay = delay;
		return *this;
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setDuration(float duration)
	{
		animation.duration = duration;
		return *this;
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setCanvasPosition(const glm::vec2& canvasPosition)
	{
		animation.canvasPosition = canvasPosition;
		return *this;
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setCanvasSize(const glm::vec2& canvasSize)
	{
		animation.canvasSize = canvasSize;
		return *this;
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setRevealTime(float revealTime)
	{
		animation.revealTime = revealTime;
		return *this;
	}

	BitmapAnimation BitmapAnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Bezier 1 Animation Builder
	// ========================================================================================================
	Bezier1AnimationBuilder::Bezier1AnimationBuilder()
	{
		animation.p0 = glm::vec2();
		animation.p1 = glm::vec2();
		animation.duration = 1.0f;
		animation.delay = 0.0f;
		animation.startTime = 0.0f;
		animation.granularity = 2;
		animation.withPoints = false;
	}

	Bezier1AnimationBuilder& Bezier1AnimationBuilder::setP0(const glm::vec2& point)
	{
		animation.p0 = point;
		return *this;
	}

	Bezier1AnimationBuilder& Bezier1AnimationBuilder::setP1(const glm::vec2& point)
	{
		animation.p1 = point;
		return *this;
	}

	Bezier1AnimationBuilder& Bezier1AnimationBuilder::setDelay(float delay)
	{
		animation.delay = delay;
		return *this;
	}

	Bezier1AnimationBuilder& Bezier1AnimationBuilder::setDuration(float duration)
	{
		animation.duration = duration;
		return *this;
	}

	Bezier1AnimationBuilder& Bezier1AnimationBuilder::withPoints()
	{
		animation.withPoints = true;
		return *this;
	}

	Bezier1Animation Bezier1AnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Bezier 2 Animation Builder
	// ========================================================================================================
	Bezier2AnimationBuilder::Bezier2AnimationBuilder()
	{
		animation.p0 = glm::vec2();
		animation.p1 = glm::vec2();
		animation.p2 = glm::vec2();
		animation.duration = 1.0f;
		animation.delay = 0.0f;
		animation.startTime = 0.0f;
		animation.granularity = 100;
		animation.withPoints = false;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::setP0(const glm::vec2& point)
	{
		animation.p0 = point;
		return *this;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::setP1(const glm::vec2& point)
	{
		animation.p1 = point;
		return *this;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::setP2(const glm::vec2& point)
	{
		animation.p2 = point;
		return *this;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::setDelay(float delay)
	{
		animation.delay = delay;
		return *this;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::setDuration(float duration)
	{
		animation.duration = duration;
		return *this;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::withPoints()
	{
		animation.withPoints = true;
		return *this;
	}

	Bezier2Animation Bezier2AnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Filled Circle Animation Builder
	// ========================================================================================================
	FilledCircleAnimationBuilder::FilledCircleAnimationBuilder()
	{
		animation.position = glm::vec2();
		animation.radius = 1.0f;
		animation.startTime = 0.0f;
		animation.delay = 0.0f;
		animation.numSegments = 50;
		animation.duration = 1.0f;
	}

	FilledCircleAnimationBuilder& FilledCircleAnimationBuilder::setPosition(const glm::vec2& point)
	{
		animation.position = point;
		return *this;
	}

	FilledCircleAnimationBuilder& FilledCircleAnimationBuilder::setRadius(float radius)
	{
		animation.radius = radius;
		return *this;
	}

	FilledCircleAnimationBuilder& FilledCircleAnimationBuilder::setNumSegments(int numSegments)
	{
		animation.numSegments = numSegments;
		return *this;
	}

	FilledCircleAnimationBuilder& FilledCircleAnimationBuilder::setDelay(float delay)
	{
		animation.delay = delay;
		return *this;
	}

	FilledCircleAnimationBuilder& FilledCircleAnimationBuilder::setDuration(float duration)
	{
		animation.duration = duration;
		return *this;
	}

	FilledCircleAnimation FilledCircleAnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Filled Box Animation Builder
	// ========================================================================================================
	FilledBoxAnimationBuilder::FilledBoxAnimationBuilder()
	{
		animation.center = glm::vec2();
		animation.size = glm::vec2();
		animation.fillDirection = Direction::Up;
		animation.duration = 1.0f;
		animation.startTime = 0.0f;
		animation.delay = 0.0f;
	}

	FilledBoxAnimationBuilder& FilledBoxAnimationBuilder::setCenter(const glm::vec2& point)
	{
		animation.center = point;
		return *this;
	}

	FilledBoxAnimationBuilder& FilledBoxAnimationBuilder::setSize(const glm::vec2& size)
	{
		animation.size = size;
		return *this;
	}

	FilledBoxAnimationBuilder& FilledBoxAnimationBuilder::setFillDirection(Direction direction)
	{
		animation.fillDirection = direction;
		return *this;
	}

	FilledBoxAnimationBuilder& FilledBoxAnimationBuilder::setDelay(float delay)
	{
		animation.delay = delay;
		return *this;
	}

	FilledBoxAnimationBuilder& FilledBoxAnimationBuilder::setDuration(float duration)
	{
		animation.duration = duration;
		return *this;
	}

	FilledBoxAnimation FilledBoxAnimationBuilder::build()
	{
		return animation;
	}
}