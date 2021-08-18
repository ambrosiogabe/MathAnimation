#include "animation/customAnimations/FillingInLetters.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace FillingInLetters
	{
		static void addPointsBulk(std::initializer_list<glm::vec2> points, float scale = 1.0f, std::initializer_list<glm::vec2> greenPoints = {})
		{
			Style pointStyle = Styles::defaultStyle;
			pointStyle.color = Colors::blue;

			for (glm::vec2 point : points)
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
				AnimationManager::addFilledCircleAnimation(
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

		static void addLinesBulk(std::initializer_list<std::tuple<glm::vec2, glm::vec2, glm::vec2>> lines, float duration, float scale = 1.0f,
			std::initializer_list<int> bezierCurves = {})
		{
			Style lineStyle = Styles::defaultStyle;
			lineStyle.color = Colors::offWhite;

			float longestLength = 0;
			for (const std::tuple<glm::vec2, glm::vec2, glm::vec2>& line : lines)
			{
				float length = glm::length2(std::get<0>(line) * 0.75f - std::get<1>(line) * 0.75f);
				longestLength = glm::max(longestLength, length);
			}
			longestLength *= 0.5f;

			int index = 0;
			for (std::tuple<glm::vec2, glm::vec2, glm::vec2> line : lines)
			{
				std::get<0>(line) *= scale;
				std::get<1>(line) *= scale;
				std::get<2>(line) *= scale;

				float adjustedDuration = duration * (glm::length2(std::get<0>(line) - std::get<1>(line)) / longestLength);

				if (std::find(bezierCurves.begin(), bezierCurves.end(), index) == bezierCurves.end())
				{
					AnimationManager::addBezier1Animation(
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
					AnimationManager::addBezier2Animation(
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

		void fillingInE()
		{
			glm::vec2 p0(0.5f, -0.25f);
			glm::vec2 p1(-0.5f, -0.25f);
			glm::vec2 p2(-0.5f, -1.25f);
			glm::vec2 p3(1.0f, -1.25f);
			glm::vec2 p4(1.0f, -1.75f);
			glm::vec2 p5(-1.125f, -1.75f);

			glm::vec2 p6(p5.x, -p5.y);
			glm::vec2 p7(p4.x, -p4.y);
			glm::vec2 p8(p3.x, -p3.y);
			glm::vec2 p9(p2.x, -p2.y);
			glm::vec2 p10(p1.x, -p1.y);
			glm::vec2 p11(p0.x, -p0.y);

			addPointsBulk({ p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11 });

			addLinesBulk({ {p0, p1, glm::vec2()}, {p1, p2, glm::vec2()}, {p2, p3, glm::vec2()}, {p3, p4, glm::vec2()}, {p4, p5, glm::vec2()},
				{p5, p6, glm::vec2()}, {p6, p7, glm::vec2()}, {p7, p8, glm::vec2()}, {p8, p9, glm::vec2()}, {p9, p10, glm::vec2()}, 
				{p10, p11, glm::vec2()}, {p11, p0, glm::vec2()} }, 1.0f);

			Style pointStyle = Styles::defaultStyle;
			pointStyle.color = Colors::blue;
			AnimationManager::addFilledCircleAnimation(
				FilledCircleAnimationBuilder()
				.setPosition({ 0.0f, 0.0f })
				.setRadius(0.06f)
				.setDuration(0.32f)
				.setNumSegments(40)
				.setDelay(8.0f)
				.build(),
				pointStyle
			);

			AnimationManager::popAnimation(AnimType::FilledCircleAnimation, 0.5f);

			Style fillStyle = Styles::defaultStyle;
			AnimationManager::addFilledBoxAnimation(
				FilledBoxAnimationBuilder()
				.setCenter({ 0.0f, 0.125f })
				.setSize({ 1.0f, 0.25f })
				.setFillDirection(Direction::Up)
				.setDuration(1.0f)
				.build(),
				fillStyle
			);

			AnimationManager::addFilledBoxAnimation(
				FilledBoxAnimationBuilder()
				.setCenter({ 0.0f, -0.125f })
				.setSize({ 1.0f, 0.25f })
				.setFillDirection(Direction::Down)
				.setDuration(1.0f)
				.setDelay(-1.0f)
				.build(),
				fillStyle
			);

			AnimationManager::addFilledBoxAnimation(
				FilledBoxAnimationBuilder()
				.setCenter({ -0.8125f, 0.0f })
				.setSize({ 0.625, 0.5f })
				.setFillDirection(Direction::Left)
				.setDuration(0.5f)
				.build(),
				fillStyle
			);

			AnimationManager::addFilledBoxAnimation(
				FilledBoxAnimationBuilder()
				.setCenter({ -0.8125f, 1.0f })
				.setSize({ 0.625, 1.5f })
				.setFillDirection(Direction::Up)
				.setDuration(1.0f)
				.build(),
				fillStyle
			);

			AnimationManager::addFilledBoxAnimation(
				FilledBoxAnimationBuilder()
				.setCenter({ -0.8125f, -1.0f })
				.setSize({ 0.625, 1.5f })
				.setFillDirection(Direction::Down)
				.setDuration(1.0f)
				.setDelay(-1.0f)
				.build(),
				fillStyle
			);

			AnimationManager::addFilledBoxAnimation(
				FilledBoxAnimationBuilder()
				.setCenter({ 0.25f, 1.5f })
				.setSize({ 1.5, 0.5f })
				.setFillDirection(Direction::Right)
				.setDuration(1.0f)
				.build(),
				fillStyle
			);

			AnimationManager::addFilledBoxAnimation(
				FilledBoxAnimationBuilder()
				.setCenter({ 0.25f, -1.5f })
				.setSize({ 1.5, 0.5f })
				.setFillDirection(Direction::Right)
				.setDuration(1.0f)
				.setDelay(-1.0f)
				.build(),
				fillStyle
			);
		}

		void fillingInB()
		{
			glm::vec2 p0(1.0f, 0.0f);
			glm::vec2 p1(2.0f, -0.5f);
			glm::vec2 p2(2.0f, -1.5f);
			glm::vec2 p3(2.0f, -2.75f);
			glm::vec2 p4(1.0f, -3.0f);
			glm::vec2 p5(0.0f, -3.0f);
			glm::vec2 p6(-1.0f, -3.0f);

			glm::vec2 p7(p6.x, -p6.y);
			glm::vec2 p8(p5.x, -p5.y);
			glm::vec2 p9(p4.x, -p4.y);
			glm::vec2 p10(p3.x, -p3.y);
			glm::vec2 p11(p2.x, -p2.y);
			glm::vec2 p12(p1.x, -p1.y);

			addPointsBulk({ p0, p1, p2, p3, p4, p5, p6, p7, p8,
				p9, p10, p11, p12 }, 0.75f, { p1, p3, p12, p10 });

			addLinesBulk({ {p0, p1, p2}, {p2, p3, p4}, {p4, p5, glm::vec2()}, {p5, p6, glm::vec2()}, {p6, p7, glm::vec2()}, 
				{p7, p8, glm::vec2()}, {p8, p9, glm::vec2()}, {p9, p10, p11},
				{p11, p12, p0} }, 1.5f, 0.75f, { 0, 1, 7, 8 });

			glm::vec2 p13(0, -0.75f);
			glm::vec2 p14(0.5f, -0.75f);
			glm::vec2 p15(1.0f, -0.875f);
			glm::vec2 p16(1.0f, -1.5f);
			glm::vec2 p17(1.0f, -2.125f);
			glm::vec2 p18(0.5f, -2.25f);
			glm::vec2 p19(0.0f, -2.25f);

			glm::vec2 p20(p13.x, -p13.y);
			glm::vec2 p21(p14.x, -p14.y);
			glm::vec2 p22(p15.x, -p15.y);
			glm::vec2 p23(p16.x, -p16.y);
			glm::vec2 p24(p17.x, -p17.y);
			glm::vec2 p25(p18.x, -p18.y);
			glm::vec2 p26(p19.x, -p19.y);

			addPointsBulk({ p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26 }, 0.75f, { p15, p17, p22, p24 });

			addLinesBulk({ 
				{p13, p14, glm::vec2()}, {p14, p15, p16}, {p16, p17, p18}, {p18, p19, glm::vec2()}, {p19, p13, glm::vec2()},
				{p20, p21, glm::vec2()}, {p21, p22, p23}, {p23, p24, p25}, {p25, p26, glm::vec2()}, {p26, p20, glm::vec2()}
				},
				0.5f, 0.75f, {1, 2, 6, 7});
		}

		void init()
		{
			fillingInB();
		}
	}
}
