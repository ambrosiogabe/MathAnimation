#include "animation/customAnimations/BitmapLetterAnimations.h"
#include "animation/customAnimations/GridAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace BitmapLetterAnimations
	{
		void init(const glm::vec2& canvasPos, const glm::vec2& canvasSize, const glm::vec2& gridSize)
		{
			GridAnimation::init({ -2.0f, -2.0f }, { 4.0f, 4.0f }, { 0.25f, 0.25f });
			glm::vec4 black = "#000000"_hex;
			glm::vec4 white = "#ffffff"_hex;

			glm::vec4 bitmap1[16][16] = {
						{black, black, black, black, black, white, white, white, white, white, white, black, black, black, black, black},
						{black, black, black, black, black, white, white, white, white, white, white, black, black, black, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, white, white, white, white, white, white, white, white, white, black, black},
						{black, black, white, white, white, white, white, white, white, white, white, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
						{black, black, white, white, white, black, black, black, black, black, black, white, white, white, black, black},
			};
			AnimationManager::addBitmapAnimation(
				BitmapAnimationBuilder()
				.setBitmap(bitmap1)
				.setRevealTime(0.01f)
				.setDuration(2.0f)
				.setDelay(17.0f)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::popAnimation(AnimType::BitmapAnimation, 2.5f);

			glm::vec4 bitmap2[16][16] = {
				{black, black, white, white, white, white, white, white, white, white, white, white, black, black, black, black},
				{black, black, white, white, white, white, white, white, white, white, white, white, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, white, white, white, white, white, white, white, black, black, black, black},
				{black, black, white, white, white, white, white, white, white, white, white, white, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, white, white, white, white, white, white, white, black, black, black, black},
				{black, black, white, white, white, white, white, white, white, white, white, white, black, black, black, black},
			};
			AnimationManager::addBitmapAnimation(
				BitmapAnimationBuilder()
				.setBitmap(bitmap2)
				.setDuration(2.0f)
				.setRevealTime(0.01f)
				.setDelay(0.5f)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::popAnimation(AnimType::BitmapAnimation, 0.5f);

			glm::vec4 bitmap3[16][16] = {
				{black, black, black, white, white, white, white, white, white, white, white, white, black, black, black, black},
				{black, black, white, white, white, white, white, white, white, white, white, white, black, black, black, black},
				{black, black, white, white, white, white, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, black, black, black, black}, 
				{black, black, white, white, white, black, black, black, black, black, black, black, black, black, black, black},
				{black, black, white, white, white, black, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, white, black, black, black, black, black, black, white, white, black, black},
				{black, black, white, white, white, white, white, white, white, white, white, white, black, black, black, black},
				{black, black, black, white, white, white, white, white, white, white, white, white, black, black, black, black},
			};
			AnimationManager::addBitmapAnimation(
				BitmapAnimationBuilder()
				.setBitmap(bitmap3)
				.setDuration(2.0f)
				.setRevealTime(0.01f)
				.setDelay(0.5f)
				.build(),
				Styles::defaultStyle
			);
		}
	}
}