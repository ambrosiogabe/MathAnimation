#include "animation/AnimationBuilders.h"

namespace MathAnim
{
	static Vec2 defaultEquation(float t)
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
		animation.as.parametric.startT = 0.0f;
		animation.as.parametric.endT = 1.0f;
		animation.as.parametric.granularity = 1;
		animation.as.parametric.parametricEquation = defaultEquation;
		animation.startTime = 0;
		animation.as.parametric.translation = { 0, 0 };
		animation.drawAnimation = AnimationManager::drawParametricAnimation;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setFunction(ParametricFunction function)
	{
		animation.as.parametric.parametricEquation = function;
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
		animation.as.parametric.startT = startT;
		return *this;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setEndT(float endT)
	{
		animation.as.parametric.endT = endT;
		return *this;
	}

	ParametricAnimationBuilder& ParametricAnimationBuilder::setGranularity(int32 granularity)
	{
		animation.as.parametric.granularity = granularity;
		return *this;
	}

	Animation ParametricAnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Text Animation Builder
	// ========================================================================================================
	TextAnimationBuilder::TextAnimationBuilder()
	{
		animation.startTime = 0.0f;
		animation.as.text.font = nullptr;
		animation.as.text.position.x = 0;
		animation.as.text.position.y = 0;
		animation.as.text.scale = 1.0f;
		animation.as.text.text = "";
		animation.as.text.typingTime = 0.1f;
		animation.delay = 0;
		animation.drawAnimation = AnimationManager::drawTextAnimation;
	}

	TextAnimationBuilder& TextAnimationBuilder::setTypingTime(float typingTime)
	{
		animation.as.text.typingTime = typingTime;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setScale(float scale)
	{
		animation.as.text.scale = scale;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setPosition(const Vec2& position)
	{
		animation.as.text.position.x = position.x;
		animation.as.text.position.y = position.y;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setFont(Font* font)
	{
		animation.as.text.font = font;
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setText(const std::string& text)
	{
		char* strMemory = (char*)g_memory_allocate(sizeof(char) * (text.length() + 1));
		strMemory[text.length()] = '\0';
		animation.as.text.text = std::strcpy(strMemory, text.c_str());
		return *this;
	}

	TextAnimationBuilder& TextAnimationBuilder::setDelay(float delay)
	{
		animation.delay = delay;
		return *this;
	}

	Animation TextAnimationBuilder::build()
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
				animation.as.bitmap.bitmap[y][x] = "#000000"_hex;
				animation.as.bitmap.bitmapState[y][x] = false;
			}
		}

		animation.delay = 0;
		animation.duration = 1.0f;
		animation.as.bitmap.revealTime = 0.01f;
		animation.as.bitmap.bitmapSquaresShowing = 0;
		animation.startTime = 0;
		animation.as.bitmap.canvasPosition = { -2, -2 };
		animation.as.bitmap.canvasSize = { 4, 4 };
		animation.drawAnimation = AnimationManager::drawBitmapAnimation;
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setBitmap(Vec4 bitmap[16][16])
	{
		for (int y = 0; y < 16; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				animation.as.bitmap.bitmap[y][x] = bitmap[16 - y - 1][x];
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

	BitmapAnimationBuilder& BitmapAnimationBuilder::setCanvasPosition(const Vec2& canvasPosition)
	{
		animation.as.bitmap.canvasPosition = {canvasPosition.x, canvasPosition.x};
		return *this;
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setCanvasSize(const Vec2& canvasSize)
	{
		animation.as.bitmap.canvasSize = {canvasSize.x, canvasSize.x};
		return *this;
	}

	BitmapAnimationBuilder& BitmapAnimationBuilder::setRevealTime(float revealTime)
	{
		animation.as.bitmap.revealTime = revealTime;
		return *this;
	}

	Animation BitmapAnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Bezier 1 Animation Builder
	// ========================================================================================================
	Bezier1AnimationBuilder::Bezier1AnimationBuilder()
	{
		animation.as.bezier1.p0 = {0, 0};
		animation.as.bezier1.p1 = {0, 0};
		animation.duration = 1.0f;
		animation.delay = 0.0f;
		animation.startTime = 0.0f;
		animation.as.bezier1.granularity = 2;
		animation.as.bezier1.withPoints = false;
		animation.drawAnimation = AnimationManager::drawBezier1Animation;
	}

	Bezier1AnimationBuilder& Bezier1AnimationBuilder::setP0(const Vec2& point)
	{
		animation.as.bezier1.p0 = {point.x, point.y};
		return *this;
	}

	Bezier1AnimationBuilder& Bezier1AnimationBuilder::setP1(const Vec2& point)
	{
		animation.as.bezier1.p1 = {point.x, point.y};
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
		animation.as.bezier1.withPoints = true;
		return *this;
	}

	Animation Bezier1AnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Bezier 2 Animation Builder
	// ========================================================================================================
	Bezier2AnimationBuilder::Bezier2AnimationBuilder()
	{
		animation.as.bezier2.p0 = {0, 0};
		animation.as.bezier2.p1 = {0, 0};
		animation.as.bezier2.p2 = {0, 0};
		animation.duration = 1.0f;
		animation.delay = 0.0f;
		animation.startTime = 0.0f;
		animation.as.bezier2.granularity = 100;
		animation.as.bezier2.withPoints = false;
		animation.drawAnimation = AnimationManager::drawBezier2Animation;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::setP0(const Vec2& point)
	{
		animation.as.bezier2.p0 = {point.x, point.y};
		return *this;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::setP1(const Vec2& point)
	{
		animation.as.bezier2.p1 = {point.x, point.y};
		return *this;
	}

	Bezier2AnimationBuilder& Bezier2AnimationBuilder::setP2(const Vec2& point)
	{
		animation.as.bezier2.p2 = {point.x, point.y};
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
		animation.as.bezier2.withPoints = true;
		return *this;
	}

	Animation Bezier2AnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Filled Circle Animation Builder
	// ========================================================================================================
	FilledCircleAnimationBuilder::FilledCircleAnimationBuilder()
	{
		animation.as.filledCircle.position = {0, 0};
		animation.as.filledCircle.radius = 1.0f;
		animation.startTime = 0.0f;
		animation.delay = 0.0f;
		animation.as.filledCircle.numSegments = 50;
		animation.duration = 1.0f;
		animation.drawAnimation = AnimationManager::drawFilledCircleAnimation;
	}

	FilledCircleAnimationBuilder& FilledCircleAnimationBuilder::setPosition(const Vec2& point)
	{
		animation.as.filledCircle.position = {point.x, point.y};
		return *this;
	}

	FilledCircleAnimationBuilder& FilledCircleAnimationBuilder::setRadius(float radius)
	{
		animation.as.filledCircle.radius = radius;
		return *this;
	}

	FilledCircleAnimationBuilder& FilledCircleAnimationBuilder::setNumSegments(int numSegments)
	{
		animation.as.filledCircle.numSegments = numSegments;
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

	Animation FilledCircleAnimationBuilder::build()
	{
		return animation;
	}

	// ========================================================================================================
	// 	   Filled Box Animation Builder
	// ========================================================================================================
	FilledBoxAnimationBuilder::FilledBoxAnimationBuilder()
	{
		animation.as.filledBox.center = {0, 0};
		animation.as.filledBox.size = {0, 0};
		animation.as.filledBox.fillDirection = Direction::Up;
		animation.duration = 1.0f;
		animation.startTime = 0.0f;
		animation.delay = 0.0f;
		animation.drawAnimation = AnimationManager::drawFilledBoxAnimation;
	}

	FilledBoxAnimationBuilder& FilledBoxAnimationBuilder::setCenter(const Vec2& point)
	{
		animation.as.filledBox.center = { point.x, point.y };
		return *this;
	}

	FilledBoxAnimationBuilder& FilledBoxAnimationBuilder::setSize(const Vec2& size)
	{
		animation.as.filledBox.size = { size.x, size.y };
		return *this;
	}

	FilledBoxAnimationBuilder& FilledBoxAnimationBuilder::setFillDirection(Direction direction)
	{
		animation.as.filledBox.fillDirection = direction;
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

	Animation FilledBoxAnimationBuilder::build()
	{
		return animation;
	}
}