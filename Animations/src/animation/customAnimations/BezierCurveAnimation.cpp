#include "animation/customAnimations/BezierCurveAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace BezierCurveAnimation
	{
		static void plotPoints(std::initializer_list<glm::vec2> points, float yOffset)
		{
			Style pointStyle = Styles::defaultStyle;
			pointStyle.color = Colors::blue;

			int i = 0;
			for (glm::vec2 point : points)
			{
				point.y += yOffset;

				if (i == 1)
				{
					pointStyle.color = Colors::green;
				}
				else
				{
					pointStyle.color = Colors::blue;
				}
				AnimationManager::addFilledCircleAnimation(
					FilledCircleAnimationBuilder()
					.setPosition(point)
					.setRadius(0.06f)
					.setDuration(0.5f)
					.setNumSegments(40)
					.build(),
					pointStyle
				);

				i++;
			}
		}

		void init()
		{
			glm::vec2 p0(-3.0f, -0.5f);
			glm::vec2 p1(0.25f, -2.25f);
			glm::vec2 p2(4.0f, 2.5f);

			Style style = Styles::defaultStyle;
			AnimationManager::addBezier2Animation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(1.0f)
				.withPoints()
				.build(),
				style
			);

			p1 = glm::vec2(-1.0f, 1.0f);
			AnimationManager::addInterpolation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(2.0f)
				.setDelay(13.0f)
				.build()
			);

			p1 = glm::vec2(-4.0f, -2.5f);
			AnimationManager::addInterpolation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(2.0f)
				.setDelay(2.0f)
				.build()
			);

			p0 = glm::vec2(2.0f, -1.0f);
			AnimationManager::addInterpolation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(2.0f)
				.setDelay(2.0f)
				.build()
			);

			p2 = glm::vec2(-1.0f, 1.0f);
			AnimationManager::addInterpolation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(2.0f)
				.setDelay(2.0f)
				.build()
			);
		}
	}
}
