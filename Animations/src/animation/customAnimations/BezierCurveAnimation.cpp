#include "animation/customAnimations/BezierCurveAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace BezierCurveAnimation
	{
		static void plotPoints(std::initializer_list<Vec2> points, float yOffset)
		{
			Style pointStyle = Styles::defaultStyle;
			pointStyle.color = Colors::blue;

			int i = 0;
			for (Vec2 point : points)
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
				AnimationManager::addAnimation(
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
			Vec2 p0{-3.0f, -0.5f};
			Vec2 p1{0.25f, -2.25f};
			Vec2 p2{4.0f, 2.5f};

			Style style = Styles::defaultStyle;
			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(1.0f)
				.withPoints()
				.build(),
				style
			);

			p1 = Vec2{-1.0f, 1.0f};
			AnimationManager::addInterpolation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(2.0f)
				.setDelay(13.0f)
				.build()
			);

			p1 = Vec2{-4.0f, -2.5f};
			AnimationManager::addInterpolation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(2.0f)
				.setDelay(2.0f)
				.build()
			);

			p0 = Vec2{2.0f, -1.0f};
			AnimationManager::addInterpolation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(2.0f)
				.setDelay(2.0f)
				.build()
			);

			p2 = Vec2{-1.0f, 1.0f};
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
