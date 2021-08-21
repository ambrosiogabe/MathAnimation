#ifndef MATH_ANIM_ANIMATION_H
#define MATH_ANIM_ANIMATION_H
#include "core.h"

namespace MathAnim
{
	struct Style;
	struct Font;
	typedef glm::vec2(*ParametricFunction)(float t);

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

	enum class Direction
	{
		Up,
		Down,
		Right,
		Left
	};

	struct FilledCircleAnimation
	{
		glm::vec2 position;
		int numSegments;
		float radius;
		float duration;
		float delay;
		float startTime;
	};

	struct FilledBoxAnimation
	{
		glm::vec2 center;
		glm::vec2 size;
		Direction fillDirection;
		float delay;
		float duration;
		float startTime;
	};

	struct ParametricAnimation
	{
		int32 granularity;
		float startT;
		float endT;
		float delay;
		float startTime;
		float duration;
		glm::vec2 translation;
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

	struct BitmapAnimation
	{
		glm::vec4 bitmap[16][16];
		bool bitmapState[16][16];
		float startTime;
		float delay;
		float duration;
		float revealTime;
		int32 bitmapSquaresShowing;
		glm::vec2 canvasPosition;
		glm::vec2 canvasSize;
	};

	struct Bezier1Animation
	{
		glm::vec2 p0;
		glm::vec2 p1;
		float duration;
		float delay;
		float startTime;
		float granularity;
		bool withPoints;
	};

	struct Bezier2Animation
	{
		glm::vec2 p0;
		glm::vec2 p1;
		glm::vec2 p2;
		float duration;
		float delay;
		float startTime;
		float granularity;
		bool withPoints;
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
		glm::vec2 translation;
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

	namespace AnimationManager
	{
		void addBitmapAnimation(BitmapAnimation& animation, const Style& style);
		void addParametricAnimation(ParametricAnimation& animation, const Style& style);
		void addTextAnimation(TextAnimation& animation, const Style& style);
		void addBezier1Animation(Bezier1Animation& animation, const Style& style);
		void addBezier2Animation(Bezier2Animation& animation, const Style& style);
		void addFilledCircleAnimation(FilledCircleAnimation& animation, const Style& style);
		void addFilledBoxAnimation(FilledBoxAnimation& animation, const Style& style);

		// TODO: Make all these functions one templated function
		void addInterpolation(Bezier2Animation animation);

		void popAnimation(AnimType animationType, float delay, float fadeOutTime = 0.32f);
		void translateAnimation(AnimType animationType, const glm::vec2& translation, float duration, float delay);
		void update(float dt);
		void reset();
	}
}

#endif
