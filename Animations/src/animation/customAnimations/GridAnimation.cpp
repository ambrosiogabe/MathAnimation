#include "animation/customAnimations/GridAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace GridAnimation
	{
		void init(const glm::vec2& canvasPos, const glm::vec2& canvasSize, const glm::vec2& gridSize)
		{
			glm::vec2 pos{};
			float duration = 0.25f;
			float delay = -0.18f;

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(canvasPos + glm::vec2{ 0, 0 })
				.setP1(canvasPos + glm::vec2{ canvasSize.x, 0 })
				.setDuration(duration)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(canvasPos + glm::vec2{ canvasSize.x, 0 })
				.setP1(canvasPos + glm::vec2{ canvasSize.x, canvasSize.y })
				.setDuration(duration)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(canvasPos + glm::vec2{ canvasSize.x, canvasSize.y })
				.setP1(canvasPos + glm::vec2{ 0, canvasSize.y })
				.setDuration(duration)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0(canvasPos + glm::vec2{ 0, canvasSize.y })
				.setP1(canvasPos + glm::vec2{ 0, 0 })
				.setDuration(duration)
				.build(),
				Styles::defaultStyle
			);

			// Draw horizontal lines
			while (pos.x < canvasSize.x || pos.y < canvasSize.y)
			{
				if (pos.y < canvasSize.y)
				{
					AnimationManager::addBezier1Animation(
						Bezier1AnimationBuilder()
						.setP0(canvasPos + glm::vec2{ 0, pos.y })
						.setP1(canvasPos + glm::vec2{ canvasSize.x, pos.y })
						.setDuration(duration)
						.setDelay(pos.y == 0 ? 0 : delay)
						.build(),
						Styles::defaultStyle
					);
				}

				if (pos.x < canvasSize.x)
				{
					AnimationManager::addBezier1Animation(
						Bezier1AnimationBuilder()
						.setP0(canvasPos + glm::vec2{ pos.x, 0 })
						.setP1(canvasPos + glm::vec2{ pos.x, canvasSize.y })
						.setDuration(duration)
						.setDelay(delay)
						.build(),
						Styles::defaultStyle
					);
				}

				pos += gridSize;
			}
		}
	}
}

