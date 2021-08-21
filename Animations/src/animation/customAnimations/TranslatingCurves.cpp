#include "animation/customAnimations/TranslatingCurves.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace TranslatingCurves
	{
		Vec2 parabola(float t)
		{
			return Vec2{
				t,
				t * t
			};
		}

		void init()
		{
			Style style = Styles::defaultStyle;
			style.color = Colors::green;
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setDuration(1)
				.setP0({ 0.0f, -5.0f })
				.setP1({ 0.0f, 5.0f })
				.build(),
				style
			);

			style.color = Colors::red;
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setDuration(1)
				.setP0({ -6.0f, 0.0f })
				.setP1({ 6.0f, 0.0f })
				.setDelay(-0.5f)
				.build(),
				style
			);

			style.color = Colors::offWhite;
			AnimationManager::addAnimation(
				ParametricAnimationBuilder()
				.setDelay(0.5f)
				.setDuration(1.0f)
				.setStartT(-3.0f)
				.setEndT(3.0f)
				.setGranularity(100)
				.setFunction(parabola)
				.build(),
				style
			);
			AnimationManager::addAnimation(
				ParametricAnimationBuilder()
				.setDelay(-1.0f)
				.setDuration(1.0f)
				.setStartT(-3.0f)
				.setEndT(3.0f)
				.setGranularity(100)
				.setFunction(parabola)
				.build(),
				style
			);
			AnimationManager::translateAnimation(AnimType::ParametricAnimation, { 0.0f, 1.0f }, 1.0f, 0.5f);
			AnimationManager::popAnimation(AnimType::ParametricAnimation, 2.5f);

			AnimationManager::addAnimation(
				ParametricAnimationBuilder()
				.setDelay(2.5f)
				.setDuration(1.0f)
				.setStartT(-3.0f)
				.setEndT(3.0f)
				.setGranularity(100)
				.setFunction(parabola)
				.build(),
				style
			);
			AnimationManager::translateAnimation(AnimType::ParametricAnimation, { 1.0f, 0.0f }, 1.0f, 0.5f);
		}
	}
}