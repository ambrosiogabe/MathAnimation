#include "animation/customAnimations/PlottingLettersWithPoints.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace PlottingLettersWithPoints
	{
		static void addPointsBulk(std::initializer_list<glm::vec2> points, float yOffset)
		{
			Style pointStyle = Styles::defaultStyle;
			pointStyle.color = Colors::blue;

			for (glm::vec2 point : points)
			{
				point.y += yOffset;
				AnimationManager::addFilledCircleAnimation(
					FilledCircleAnimationBuilder()
					.setPosition(point)
					.setRadius(0.06f)
					.setDuration(0.16f)
					.setNumSegments(40)
					.build(),
					pointStyle
				);
			}
		}

		static void addLinesBulk(std::initializer_list<std::tuple<glm::vec2, glm::vec2>> lines, float yOffset, float duration)
		{
			Style lineStyle = Styles::defaultStyle;
			lineStyle.color = Colors::offWhite;

			float longestLength = 0;
			for (const std::tuple<glm::vec2, glm::vec2>& line : lines)
			{
				float length = glm::length2(std::get<0>(line) - std::get<1>(line));
				longestLength = glm::max(longestLength, length);
			}
			longestLength *= 0.8f;

			for (std::tuple<glm::vec2, glm::vec2> line : lines)
			{
				std::get<0>(line).y += yOffset;
				std::get<1>(line).y += yOffset;

				float adjustedDuration = duration * (glm::length2(std::get<0>(line) - std::get<1>(line)) / longestLength);
				AnimationManager::addBezier1Animation(
					Bezier1AnimationBuilder()
					.setP0(std::get<0>(line))
					.setP1(std::get<1>(line))
					.setDuration(adjustedDuration)
					.build(),
					lineStyle
				);
			}
		}

		static void plotLetterA()
		{
			glm::vec2 p0(-2.0f, -2.0f);
			glm::vec2 p1(-0.75f, 2.5f);
			glm::vec2 p2(0.75f, 2.5f);
			glm::vec2 p3(2.0f, -2.0f);
			glm::vec2 p4(1.5f, -2.0f);
			glm::vec2 p5(1.0f, 0.0f);
			glm::vec2 p6(-1.0f, 0.0f);
			glm::vec2 p7(-1.5f, -2.0f);

			glm::vec2 p8(-0.875f, 0.5f);
			glm::vec2 p9(-0.5f, 2.0f);
			glm::vec2 p10(0.5f, 2.0f);
			glm::vec2 p11(0.875f, 0.5f);

			Style pointStyle = Styles::defaultStyle;
			pointStyle.color = Colors::blue;

			addPointsBulk({ p0, p1, p2, p3, p4, p5, p6, p7 }, -0.25f);
			addLinesBulk({ {p0, p1}, {p1, p2}, {p2, p3}, {p3, p4}, {p4, p5}, {p5, p6}, {p6, p7}, {p7, p0} }, -0.25f, 1.0f);

			addPointsBulk({ p8, p9, p10, p11 }, -0.25f);
			addLinesBulk({ {p8, p9}, {p9, p10}, {p10, p11}, {p11, p8} }, -0.25f, 1.0f);
		}

		static void plotLetterC()
		{
			glm::vec2 p0(0.5f, -0.5f);
			glm::vec2 p1(1.0f, -1.0f);
			glm::vec2 p2(0.5f, -1.5f);
			glm::vec2 p3(0, -1.75f);
			glm::vec2 p4(-0.5f, -1.75f);
			glm::vec2 p5(-1.0f, -1.5f);
			glm::vec2 p6(-1.5f, -1.0f);
			glm::vec2 p7(-1.625f, -0.5f);

			// Line of y symmetry
			glm::vec2 p8(-1.75f, 0.0f);
			
			glm::vec2 p9(p7.x, -p7.y);
			glm::vec2 p10(p6.x, -p6.y);
			glm::vec2 p11(p5.x, -p5.y);
			glm::vec2 p12(p4.x, -p4.y);
			glm::vec2 p13(p3.x, -p3.y);
			glm::vec2 p14(p2.x, -p2.y);
			glm::vec2 p15(p1.x, -p1.y);
			glm::vec2 p16(p0.x, -p0.y);

			glm::vec2 p17(0, 1);
			glm::vec2 p18(-0.5f, 1);
			glm::vec2 p19(-0.75f, 0.5f);

			// Line of y symmetry
			glm::vec2 p20(-0.75f, 0.0f);

			glm::vec2 p21(p19.x, -p19.y);
			glm::vec2 p22(p18.x, -p18.y);
			glm::vec2 p23(p17.x, -p17.y);

			addPointsBulk({ p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14,
				p15, p16, p17, p18, p19, p20, p21, p22, p23 }, 0.0f);
			addLinesBulk({ {p0, p1}, {p1, p2}, {p2, p3}, {p3, p4}, {p4, p5}, {p5, p6}, {p6, p7}, {p7, p8},
				{p8, p9}, {p9, p10}, {p10, p11}, {p11, p12}, {p12, p13}, {p13, p14}, {p14, p15}, {p15, p16},
				{p16, p17}, {p17, p18}, {p18, p19}, {p19, p20}, {p20, p21}, {p21, p22}, {p22, p23}, {p23, p0}}, 0.0f, 0.25f);
		}

		void init()
		{
			//plotLetterA();
			plotLetterC();
		}
	}
}
