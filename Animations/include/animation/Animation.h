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
		TextAnimation
	};

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

	struct BitmapAnimation
	{
		bool bitmap[16][16];
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
	};

	struct PopAnimation
	{
		AnimType animType;
		float startTime;
		int index;
	};

	namespace AnimationManager
	{
		void addBitmapAnimation(BitmapAnimation& animation, const Style& style);
		void addParametricAnimation(ParametricAnimation& animation, const Style& style);
		void addTextAnimation(TextAnimation& animation, const Style& style);
		void addBezier1Animation(Bezier1Animation& animation, const Style& style);
		void addBezier2Animation(Bezier2Animation& animation, const Style& style);

		void popAnimation(AnimType animationType, float delay);
		void update(float dt);
		void reset();
	}
}

#endif
