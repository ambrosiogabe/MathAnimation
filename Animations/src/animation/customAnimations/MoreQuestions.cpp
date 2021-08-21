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
			glm::vec2 p0(-1.0f, -2.0f);
			glm::vec2 p1(3.0f, -2.0f);
			glm::vec2 p2(1.0f, 2.0f);
			glm::vec2 p3(0.0f, 2.25f);

			AnimationManager::addBezier2Animation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(1)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(p2)
				.setP1(p3)
				.setDuration(1.0f)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addFilledCircleAnimation(
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
			AnimationManager::addBezier1Animation(
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
			glm::vec2 p0(-1.0f, 1.0f);
			glm::vec2 p1(2.0f, 1.0f);

			glm::vec2 point(-3.0f, 1.0f);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setDuration(1.0f)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addFilledCircleAnimation(
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
			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(point)
				.setP1(point + glm::vec2{ 6.0f, 0.0f })
				.setDuration(1.0f)
				.build(),
				arrowStyle
			);
		}

		static void startsOnCurve()
		{
			glm::vec2 p0(0.0f, -1.0f);
			glm::vec2 p1(0.0f, 1.0f);

			glm::vec2 point(0.0f, 0.0f);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setDuration(1.0f)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addFilledCircleAnimation(
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
			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(point)
				.setP1(point + glm::vec2{ 3.0f, 0.0f })
				.setDuration(1.0f)
				.build(),
				arrowStyle
			);
		}

		void init()
		{
			//straightLine();
			startsOnCurve();
		}
	}
}