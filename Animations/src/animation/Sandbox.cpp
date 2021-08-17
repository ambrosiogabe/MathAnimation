#include "core.h"
#include "animation/Sandbox.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"
#include "renderer/Renderer.h"
#include "renderer/Fonts.h"
#include "utils/CMath.h"

#include "animation/customAnimations/PhoneAnimation.h"
#include "animation/customAnimations/GridAnimation.h"
#include "animation/customAnimations/BitmapLetterAnimations.h"

namespace MathAnim
{
	namespace Sandbox
	{
		glm::vec2 triangle(float t)
		{
			if (t >= 0 && t < 1)
			{
				float newT = CMath::mapRange({ 0.0f, 1.0f }, { -1.0f, 0.0f }, t);
				return glm::vec2{
					newT,
					newT + 2
				};
			}
			else if (t >= 1 && t < 2)
			{
				float newT = CMath::mapRange({ 1.0f, 2.0f }, { 0.0f, 1.0f }, t);
				return glm::vec2{
					newT,
					-newT + 2
				};
			} 
			else if (t >= 2 && t < 3)
			{
				float newT = CMath::mapRange({ 2.0f, 3.0f }, { 1.0f, -1.0f }, t);
				return glm::vec2{
					newT,
					1
				};
			}
		}

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

		static Font* font = nullptr;

		void init()
		{
			//PhoneAnimation::init();
			//GridAnimation::init({ -2.0f, -2.0f }, { 4.0f, 4.0f }, { 0.25f, 0.25f });
			//BitmapLetterAnimations::init({ -2.0f, -2.0f }, { 4.0f, 4.0f }, { 0.25f, 0.25f });

			Style style = Styles::defaultStyle;
			style.color = Colors::green;
			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setDuration(1)
				.setP0({ 0.0f, -5.0f })
				.setP1({ 0.0f, 5.0f })
				.build(),
				style
			);

			style.color = Colors::red;
			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setDuration(1)
				.setP0({ -6.0f, 0.0f })
				.setP1({ 6.0f, 0.0f })
				.setDelay(-0.5f)
				.build(),
				style
			);

			style.color = Colors::offWhite;
			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setDelay(8.0f)
				.setDuration(1.0f)
				.setStartT(-3.0f)
				.setEndT(3.0f)
				.setGranularity(100)
				.setFunction(parabola)
				.build(),
				style
			);

			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.0f);

			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setDelay(0.5f)
				.setDuration(1.0f)
				.setStartT(-3.0f)
				.setEndT(3.0f)
				.setGranularity(100)
				.setFunction(cubic)
				.build(),
				style
			);

			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.0f);

			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setDelay(0.5f)
				.setDuration(1.0f)
				.setStartT(0.01f)
				.setEndT(6.0f)
				.setGranularity(100)
				.setFunction(logarithm)
				.build(),
				style
			);

			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.0f);

			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setDelay(0.5f)
				.setDuration(1.0f)
				.setStartT(-6.0f)
				.setEndT(-0.01f)
				.setGranularity(100)
				.setFunction(hyperbolic)
				.build(),
				style
			);
			AnimationManager::addParametricAnimation(
				ParametricAnimationBuilder()
				.setDuration(1.0f)
				.setStartT(0.01f)
				.setEndT(6.0f)
				.setGranularity(100)
				.setFunction(hyperbolic)
				.build(),
				style
			);
		}

		void update(float dt)
		{

		}
	}
}