#include "core.h"
#include "animation/Sandbox.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"
#include "renderer/Renderer.h"
#include "renderer/Fonts.h"

namespace MathAnim
{
	namespace Sandbox
	{
		float mapRange(const glm::vec2& inputRange, const glm::vec2& outputRange, float value)
		{
			return (value - inputRange.x) / (inputRange.y - inputRange.x) * (outputRange.y - outputRange.x) + outputRange.x;
		}

		glm::vec2 triangle(float t)
		{
			if (t >= 0 && t < 1)
			{
				float newT = mapRange({ 0.0f, 1.0f }, { -1.0f, 0.0f }, t);
				return glm::vec2{
					newT,
					newT + 2
				};
			}
			else if (t >= 1 && t < 2)
			{
				float newT = mapRange({ 1.0f, 2.0f }, { 0.0f, 1.0f }, t);
				return glm::vec2{
					newT,
					-newT + 2
				};
			} 
			else if (t >= 2 && t < 3)
			{
				float newT = mapRange({ 2.0f, 3.0f }, { 1.0f, -1.0f }, t);
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
				(-(t * t) / 10.0f) - 2.0f
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
			Style style = Styles::defaultStyle;
			style.color = Colors::blue;
			ParametricAnimation animation1 = ParametricAnimationBuilder()
				.setFunction(parabola)
				.setDuration(0.6f)
				.setStartT(-4)
				.setEndT(4)
				.setGranularity(100)
				.build();
			AnimationManager::addParametricAnimation(animation1, style);

			style.color = Colors::yellow;
			ParametricAnimation animation2 = ParametricAnimationBuilder()
				.setFunction(hyperbolic)
				.setDuration(1.0f)
				.setStartT(-6)
				.setEndT(-0.1f)
				.setGranularity(100)
				.build();
			AnimationManager::addParametricAnimation(animation2, style);

			style.color = Colors::lightOrange;
			ParametricAnimation animation3 = ParametricAnimationBuilder()
				.setFunction(circle)
				.setDuration(3.0f)
				.setStartT(0.0f)
				.setEndT(glm::pi<float>() * 2.0f)
				.setGranularity(100)
				.build();
			AnimationManager::addParametricAnimation(animation3, style);

			style.color = Colors::purple;
			ParametricAnimation animation4 = ParametricAnimationBuilder()
				.setFunction(triangle)
				.setDuration(3.0f)
				.setStartT(0.0f)
				.setEndT(3.0f)
				.setGranularity(100)
				.build();
			AnimationManager::addParametricAnimation(animation4, style);

			font = Fonts::loadFont("C:/Windows/Fonts/Arial.ttf", 64);

			TextAnimation textAnim1 = TextAnimationBuilder()
				.setFont(font)
				.setPosition(glm::vec2(-2, -2))
				.setScale(0.1f)
				.setText("Hello there, how are you doing?")
				.setDelay(-2.0f)
				.build();
			style.color = Colors::offWhite;
			AnimationManager::addTextAnimation(textAnim1, style);

			AnimationManager::addTextAnimation(
				TextAnimationBuilder()
					.setFont(font)
					.setPosition(glm::vec2(-2, 2))
					.setScale(0.11f)
					.setText("And this is more stuff...")
					.setDelay(-1.5f)
					.build(),
				style
			);
		}

		void update(float dt)
		{
			static Style style = Styles::defaultStyle;
			style.color = Colors::lightOrange;
			Renderer::drawFilledSquare(glm::vec2(-1, 0), glm::vec2(1, 1), style);
			style.color = Colors::red;
			Renderer::drawSquare(glm::vec2(-1.025f, -0.025f), glm::vec2(1.045f, 1.045f), style);
		}
	}
}