#include "animation/customAnimations/WindingContours.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace WindingContours
	{
		static void addPointsBulk(std::initializer_list<Vec2> points, float scale = 1.0f, std::initializer_list<Vec2> greenPoints = {})
		{
			Style pointStyle = Styles::defaultStyle;
			pointStyle.color = Colors::blue;

			for (Vec2 point : points)
			{
				if (std::find(greenPoints.begin(), greenPoints.end(), point) != greenPoints.end())
				{
					pointStyle.color = Colors::green;
				}
				else
				{
					pointStyle.color = Colors::blue;
				}

				point *= scale;
				AnimationManager::addAnimation(
					FilledCircleAnimationBuilder()
					.setPosition(point)
					.setRadius(0.04f)
					.setDuration(0.16f)
					.setNumSegments(40)
					.build(),
					pointStyle
				);
			}
		}

		static void addLinesBulk(std::initializer_list<std::tuple<Vec2, Vec2, Vec2>> lines, float duration, float scale = 1.0f,
			std::initializer_list<int> bezierCurves = {})
		{
			Style lineStyle = Styles::defaultStyle;
			lineStyle.color = Colors::offWhite;
			lineStyle.color.a = 0.4f;

			float longestLength = 0;
			for (const std::tuple<Vec2, Vec2, Vec2>& line : lines)
			{
				float length = CMath::lengthSquared(std::get<0>(line) * 0.75f - std::get<1>(line) * 0.75f);
				longestLength = glm::max(longestLength, length);
			}
			longestLength *= 0.5f;

			int index = 0;
			for (std::tuple<Vec2, Vec2, Vec2> line : lines)
			{
				std::get<0>(line) *= scale;
				std::get<1>(line) *= scale;
				std::get<2>(line) *= scale;

				float adjustedDuration = duration * (CMath::lengthSquared(std::get<0>(line) - std::get<1>(line)) / longestLength);

				if (std::find(bezierCurves.begin(), bezierCurves.end(), index) == bezierCurves.end())
				{
					AnimationManager::addAnimation(
						Bezier1AnimationBuilder()
						.setP0(std::get<0>(line))
						.setP1(std::get<1>(line))
						.setDuration(adjustedDuration)
						.build(),
						lineStyle
					);
				}
				else
				{
					AnimationManager::addAnimation(
						Bezier2AnimationBuilder()
						.setP0(std::get<0>(line))
						.setP1(std::get<1>(line))
						.setP2(std::get<2>(line))
						.setDuration(duration * 0.5f)
						.build(),
						lineStyle
					);
				}
				index++;
			}
		}

		void init()
		{
			Vec2 p0{1.0f, 0.0f};
			Vec2 p1{2.0f, -0.5f};
			Vec2 p2{2.0f, -1.5f};
			Vec2 p3{2.0f, -2.75f};
			Vec2 p4{1.0f, -3.0f};
			Vec2 p5{0.0f, -3.0f};
			Vec2 p6{-1.0f, -3.0f};

			Vec2 p7{p6.x, -p6.y};
			Vec2 p8{p5.x, -p5.y};
			Vec2 p9{p4.x, -p4.y};
			Vec2 p10{p3.x, -p3.y};
			Vec2 p11{p2.x, -p2.y};
			Vec2 p12{p1.x, -p1.y};

			addLinesBulk({ {p0, p1, p2}, {p2, p3, p4}, {p4, p5, Vec2()}, {p5, p6, Vec2()}, {p6, p7, Vec2()},
				{p7, p8, Vec2()}, {p8, p9, Vec2()}, {p9, p10, p11},
				{p11, p12, p0} }, 0.01f, 0.75f, { 0, 1, 7, 8 });

			Vec2 p13{0, -0.75f};
			Vec2 p14{0.5f, -0.75f};
			Vec2 p15{1.0f, -0.875f};
			Vec2 p16{1.0f, -1.5f};
			Vec2 p17{1.0f, -2.125f};
			Vec2 p18{0.5f, -2.25f};
			Vec2 p19{0.0f, -2.25f};

			Vec2 p20{p13.x, -p13.y};
			Vec2 p21{p14.x, -p14.y};
			Vec2 p22{p15.x, -p15.y};
			Vec2 p23{p16.x, -p16.y};
			Vec2 p24{p17.x, -p17.y};
			Vec2 p25{p18.x, -p18.y};
			Vec2 p26{p19.x, -p19.y};

			addLinesBulk({
				{p13, p14, Vec2()}, {p14, p15, p16}, {p16, p17, p18}, {p18, p19, Vec2()}, {p19, p13, Vec2()},
				{p20, p21, Vec2()}, {p21, p22, p23}, {p23, p24, p25}, {p25, p26, Vec2()}, {p26, p20, Vec2()}
				},
				0.01f, 0.75f, { 1, 2, 6, 7 });			

			Style arrowStyle = Styles::defaultStyle;
			arrowStyle.lineEnding = CapType::Arrow;
			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0({ 1.0f, 2.75f })
				.setP1({ 2.25f, 2.5f })
				.setP2({ 2.0f, 1.25f })
				.setDuration(1.0f)
				.setDelay(6.0f)
				.build(),
				arrowStyle
			);

			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0({ 1.0f, 1.25f })
				.setP1({ 1.0f, 1.75f + 0.125f })
				.setP2({ 0.5f, 1.75f + 0.125f })
				.setDuration(1.0f)
				.setDelay(4.0f)
				.build(),
				arrowStyle
			);
		}
	}
}
