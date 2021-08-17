#include "animation/customAnimations/PhoneAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"
#include "renderer/Renderer.h"
#include "renderer/Fonts.h"
#include "utils/CMath.h"

namespace MathAnim
{
	namespace PhoneAnimation
	{
		void init()
		{
			Style style = Styles::defaultStyle;

			float duration1 = 0.1f;
			float duration2 = 0.2f;
			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0({ -1.5f, -2.0f })
				.setP1({ -1.5f, 2.0f })
				.setDuration(duration1)
				.build(),
				style
			);

			AnimationManager::addBezier2Animation(
				Bezier2AnimationBuilder()
				.setP0({ -1.5f, 2.0f })
				.setP1({ -1.5f, 2.5f })
				.setP2({ -1.25f, 2.5f })
				.setDuration(duration2)
				.build(),
				style
			);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0({ -1.25f, 2.5f })
				.setP1({ 1.25f, 2.5f })
				.setDuration(duration1)
				.build(),
				style
			);

			AnimationManager::addBezier2Animation(
				Bezier2AnimationBuilder()
				.setP0({ 1.25f, 2.5f })
				.setP1({ 1.5f, 2.5f })
				.setP2({ 1.5f, 2.0f })
				.setDuration(duration2)
				.build(),
				style
			);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0({ 1.5f, 2.0f })
				.setP1({ 1.5f, -2.0f })
				.setDuration(duration1)
				.build(),
				style
			);

			AnimationManager::addBezier2Animation(
				Bezier2AnimationBuilder()
				.setP0({ 1.5f, -2.0f })
				.setP1({ 1.5f, -2.5f })
				.setP2({ 1.25f, -2.5f })
				.setDuration(duration2)
				.build(),
				style
			);

			AnimationManager::addBezier1Animation(
				Bezier1AnimationBuilder()
				.setP0({ 1.25f, -2.5f })
				.setP1({ -1.25f, -2.5f })
				.setDuration(duration1)
				.build(),
				style
			);

			AnimationManager::addBezier2Animation(
				Bezier2AnimationBuilder()
				.setP0({ -1.25f, -2.5f })
				.setP1({ -1.5f, -2.5f })
				.setP2({ -1.5f, -2.0f })
				.setDuration(duration2)
				.build(),
				style
			);
		}
	}
}