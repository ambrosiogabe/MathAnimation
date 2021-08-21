#include "animation/customAnimations/DrawRandomCurves.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace DrawRandomCurves
	{
		glm::vec2 parabola(float t)
		{
			return glm::vec2{
				t,
				t * t
			};
		}

		glm::vec2 cubic(float t)
		{
			return glm::vec2{
				t,
				t * t * t
			};
		}

		glm::vec2 logarithm(float t)
		{
			return glm::vec2{
				t,
				glm::log(t)
			};
		}

		glm::vec2 hyperbolic(float t)
		{
			return glm::vec2{
				t,
				1.0f / t
			};
		}

		glm::vec2 line(float t)
		{
			return glm::vec2{
				t,
				(1.0f / 3.0f) * t
			};
		}

		glm::vec2 circle(float t)
		{
			return glm::vec2{
				1 * sin(t),
				1 * cos(t)
			};
		}

		void init()
		{
			Style xAxisStyle = Styles::defaultStyle;
			xAxisStyle.color = Colors::red;
			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0({ -6.0f, 0.0f })
				.setP1({ 6.0f, 0.0f })
				.setDuration(0.5f)
				.build(),
				xAxisStyle
			);

			Style yAxisStyle = Styles::defaultStyle;
			yAxisStyle.color = Colors::green;
			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0({ 0.0f, -4.0f })
				.setP1({ 0.0f, 4.0f })
				.setDuration(0.5f)
				.setDelay(0.2f)
				.build(),
				yAxisStyle
			);

			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setFunction(parabola)
				.setStartT(-2.0f)
				.setEndT(2.0f)
				.setDuration(0.75f)
				.setGranularity(100)
				.setDelay(10.0f)
				.build(),
				Styles::defaultStyle
			);
			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.0f);

			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setFunction(cubic)
				.setStartT(-2.0f)
				.setEndT(2.0f)
				.setDuration(0.75f)
				.setGranularity(100)
				.setDelay(2.2f)
				.build(),
				Styles::defaultStyle
			);
			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.0f);

			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setFunction(logarithm)
				.setStartT(0.01f)
				.setEndT(6.0f)
				.setDuration(0.75f)
				.setGranularity(100)
				.setDelay(2.2f)
				.build(),
				Styles::defaultStyle
			);
			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.0f);

			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setFunction(hyperbolic)
				.setStartT(-6.0f)
				.setEndT(-0.001f)
				.setDuration(0.35f)
				.setGranularity(100)
				.setDelay(2.2f)
				.build(),
				Styles::defaultStyle
			);
			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.0f);
			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setFunction(hyperbolic)
				.setStartT(0.001f)
				.setEndT(6.0f)
				.setDuration(0.35f)
				.setGranularity(100)
				.build(),
				Styles::defaultStyle
			);
			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.0f - 0.35f);
		}
	}
}