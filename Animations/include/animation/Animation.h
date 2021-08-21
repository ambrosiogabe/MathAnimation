#ifndef MATH_ANIM_ANIMATION_H
#define MATH_ANIM_ANIMATION_H
#include "core.h"

namespace MathAnim
{
	struct Style;
	struct Font;

	enum class Direction
	{
		Up,
		Down,
		Right,
		Left
	};

	typedef Vec2(*ParametricFunction)(float t);

	struct FilledCircleAnimation
	{
		Vec2 position;
		int numSegments;
		float radius;
	};

	struct FilledBoxAnimation
	{
		Vec2 center;
		Vec2 size;
		Direction fillDirection;
	};

	struct ParametricAnimation
	{
		int32 granularity;
		float startT;
		float endT;
		Vec2 translation;
		ParametricFunction parametricEquation;
	};

	struct TextAnimation
	{
		float typingTime;
		float scale;
		Vec2 position;
		Font* font;
		const char* text;
	};

	struct BitmapAnimation
	{
		Vec4 bitmap[16][16];
		bool bitmapState[16][16];
		float revealTime;
		int32 bitmapSquaresShowing;
		Vec2 canvasPosition;
		Vec2 canvasSize;
	};

	struct Bezier1Animation
	{
		Vec2 p0;
		Vec2 p1;
		float granularity;
		bool withPoints;
	};

	struct Bezier2Animation
	{
		Vec2 p0;
		Vec2 p1;
		Vec2 p2;
		float granularity;
		bool withPoints;
	};

	enum class AnimType
	{
		ParametricAnimation,
		Bezier1Animation,
		Bezier2Animation,
		BitmapAnimation,
		TextAnimation,
		FilledCircleAnimation,
		FilledBoxAnimation
	};

	struct PopAnimation
	{
		AnimType animType;
		float startTime;
		float fadeOutTime;
		int index;
	};

	struct TranslateAnimation
	{
		AnimType animType;
		float startTime;
		int index;
		float duration;
		Vec2 translation;
	};

	struct Interpolation
	{
		int ogAnimIndex;
		int ogP0Index;
		int ogP1Index;
		int ogP2Index;
		Bezier2Animation ogAnim;
		Bezier2Animation newAnim;
	};

	struct Animation;
	typedef void(*DrawAnimationFn)(Animation& animation, const Style& style);

	struct Animation
	{
		AnimType type;
		float startTime;
		float delay;
		float duration;
		DrawAnimationFn drawAnimation;
		union
		{
			Bezier1Animation bezier1;
			Bezier2Animation bezier2;
			FilledCircleAnimation filledCircle;
			FilledBoxAnimation filledBox;
			ParametricAnimation parametric;
			TextAnimation text;
			BitmapAnimation bitmap;
		} as;
	};

	namespace AnimationManager
	{
		void addAnimation(Animation& animation, const Style& style);

		// TODO: Make all these functions one templated function
		void addInterpolation(Animation& animation);

		void drawParametricAnimation(Animation& genericAnimation, const Style& style);
		void drawTextAnimation(Animation& genericAnimation, const Style& style);
		void drawBitmapAnimation(Animation& genericAnimation, const Style& style);
		void drawBezier1Animation(Animation& genericAnimation, const Style& style);
		void drawBezier2Animation(Animation& genericAnimation, const Style& style);
		void drawFilledCircleAnimation(Animation& genericAnimation, const Style& style);
		void drawFilledBoxAnimation(Animation& genericAnimation, const Style& style);

		void popAnimation(AnimType animationType, float delay, float fadeOutTime = 0.32f);
		void translateAnimation(AnimType animationType, const Vec2& translation, float duration, float delay);
		void update(float dt);
		void reset();
	}
}

#endif
