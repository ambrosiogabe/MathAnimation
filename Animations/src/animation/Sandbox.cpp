#include "core.h"
#include "animation/Sandbox.h"
#include "animation/Animation.h"
#include "animation/Styles.h"
#include "renderer/Renderer.h"

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

		void init()
		{
			Style style = Styles::defaultStyle;
			style.color = Colors::blue;
			Animation animation1;
			animation1.parametricEquation = parabola;
			animation1.duration = 0.6f;
			animation1.startT = -4;
			animation1.endT = 4;
			animation1.granularity = 100.0f;
			animation1.startTime = 0.0f;
			AnimationManager::addAnimation(animation1, style);

			style.color = Colors::yellow;
			Animation animation2;
			animation2.parametricEquation = hyperbolic;
			animation2.duration = 1.0f;
			animation2.startT = -6;
			animation2.endT = -0.1f;
			animation2.granularity = 100.0f;
			animation2.startTime = 0.8f;
			AnimationManager::addAnimation(animation2, style);

			style.color = Colors::lightOrange;
			Animation animation3;
			animation3.parametricEquation = circle;
			animation3.duration = 3;
			animation3.startT = 0;
			animation3.endT = glm::pi<float>() * 2.0f;
			animation3.granularity = 100;
			animation3.startTime = 2.0f;
			AnimationManager::addAnimation(animation3, style);

			style.color = Colors::purple;
			Animation animation4;			
			animation4.parametricEquation = triangle;
			animation4.duration = 3;
			animation4.startT = 0;
			animation4.endT = 3;
			animation4.granularity = 100;
			animation4.startTime = 5.0f;
			AnimationManager::addAnimation(animation4, style);
		}

		void update(float dt)
		{
			Renderer::clearColor(Colors::greenBrown);
		}
	}
}