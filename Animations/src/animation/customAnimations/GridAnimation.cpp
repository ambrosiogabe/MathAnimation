#include "animation/customAnimations/GridAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace GridAnimation
	{
		void init(const Vec2& canvasPos, const Vec2& canvasSize, const Vec2& gridSize)
		{
			Vec2 pos{};
			float duration = 0.25f;
			float delay = -0.18f;

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(canvasPos + Vec2{ 0, 0 })
				.setP1(canvasPos + Vec2{ canvasSize.x, 0 })
				.setDuration(duration)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(canvasPos + Vec2{ canvasSize.x, 0 })
				.setP1(canvasPos + Vec2{ canvasSize.x, canvasSize.y })
				.setDuration(duration)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(canvasPos + Vec2{ canvasSize.x, canvasSize.y })
				.setP1(canvasPos + Vec2{ 0, canvasSize.y })
				.setDuration(duration)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(canvasPos + Vec2{ 0, canvasSize.y })
				.setP1(canvasPos + Vec2{ 0, 0 })
				.setDuration(duration)
				.build(),
				Styles::defaultStyle
			);

			// Draw horizontal lines
			while (pos.x < canvasSize.x || pos.y < canvasSize.y)
			{
				if (pos.y < canvasSize.y)
				{
					AnimationManager::addAnimation(
						Bezier1AnimationBuilder()
						.setP0(canvasPos + Vec2{ 0, pos.y })
						.setP1(canvasPos + Vec2{ canvasSize.x, pos.y })
						.setDuration(duration)
						.setDelay(pos.y == 0 ? 0 : delay)
						.build(),
						Styles::defaultStyle
					);
				}

				if (pos.x < canvasSize.x)
				{
					AnimationManager::addAnimation(
						Bezier1AnimationBuilder()
						.setP0(canvasPos + Vec2{ pos.x, 0 })
						.setP1(canvasPos + Vec2{ pos.x, canvasSize.y })
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

