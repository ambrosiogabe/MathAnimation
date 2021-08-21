#include "animation/customAnimations/MoreQuestions.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace MoreQuestions
	{
		static void hitJunction()
		{
			Vec2 p0{-1.0f, -2.0f};
			Vec2 p1{3.0f, -2.0f};
			Vec2 p2{1.0f, 2.0f};
			Vec2 p3{0.0f, 2.25f};

			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(1)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(p2)
				.setP1(p3)
				.setDuration(1.0f)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition({ -1.0f, 2.0f })
				.setDuration(0.32f)
				.setNumSegments(40)
				.setRadius(0.06f)
				.setDelay(0.5f)
				.build(),
				Styles::defaultStyle
			);
			Style arrowStyle = Styles::defaultStyle;
			arrowStyle.lineEnding = CapType::Arrow;
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0({ -1.0f, 2.0f })
				.setP1({ 2.0f, 2.0f })
				.setDelay(0.5f)
				.build(),
				arrowStyle
			);
		}

		static void straightLine()
		{
			Vec2 p0{-1.0f, 1.0f};
			Vec2 p1{2.0f, 1.0f};

			Vec2 point{-3.0f, 1.0f};

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setDuration(1.0f)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(point)
				.setRadius(0.06f)
				.setNumSegments(40)
				.setDuration(0.32f)
				.setDelay(0.5f)
				.build(),
				Styles::defaultStyle
			);
			Style arrowStyle = Styles::defaultStyle;
			arrowStyle.lineEnding = CapType::Arrow;
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(point)
				.setP1(point + Vec2{ 6.0f, 0.0f })
				.setDuration(1.0f)
				.build(),
				arrowStyle
			);
		}

		static void startsOnCurve()
		{
			Vec2 p0{0.0f, -1.0f};
			Vec2 p1{0.0f, 1.0f};

			Vec2 point{0.0f, 0.0f};

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setDuration(1.0f)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(point)
				.setRadius(0.06f)
				.setNumSegments(40)
				.setDuration(0.32f)
				.setDelay(0.5f)
				.build(),
				Styles::defaultStyle
			);
			Style arrowStyle = Styles::defaultStyle;
			arrowStyle.lineEnding = CapType::Arrow;
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(point)
				.setP1(point + Vec2{ 3.0f, 0.0f })
				.setDuration(1.0f)
				.build(),
				arrowStyle
			);
		}

		void init()
		{
			Style style = Styles::defaultStyle;
			style.strokeWidth = 0.08f;
			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0({ -2.0f, 0.0f })
				.setP1({ 2.0f, 1.0f })
				.setP2({3.0f, 0.0f})
				.setDuration(1.0f)
				.withPoints()
				.build(),
				style
			);

			style.strokeWidth = 0.03f;
			style.lineEnding = CapType::Arrow;
			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0({ -2.0f, -1.0f })
				.setP1({ 2.0f, 0.0f })
				.setP2({ 3.0f, -1.0f })
				.setDuration(1.0f)
				.withPoints()
				.build(),
				style
			);

			//straightLine();
			//startsOnCurve();
		}
	}
}