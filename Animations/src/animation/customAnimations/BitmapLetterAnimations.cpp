#include "animation/customAnimations/BitmapLetterAnimations.h"
#include "animation/customAnimations/GridAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace BitmapLetterAnimations
	{
		void init(const Vec2& canvasPos, const Vec2& canvasSize, const Vec2& gridSize)
		{
			GridAnimation::init({ -2.0f, -2.0f }, { 4.0f, 4.0f }, { 0.25f, 0.25f });
			Vec4 black = "#000000"_hex;
			Vec4 white = "#ffffff"_hex;

			Vec4 bitmap1[16][16] = {
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
			AnimationManager::addAnimation(
				BitmapAnimationBuilder()
				.setBitmap(bitmap1)
				.setRevealTime(0.01f)
				.setDuration(2.0f)
				.setDelay(17.0f)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::popAnimation(AnimType::BitmapAnimation, 2.5f);

			Vec4 bitmap2[16][16] = {
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
			AnimationManager::addAnimation(
				BitmapAnimationBuilder()
				.setBitmap(bitmap2)
				.setDuration(2.0f)
				.setRevealTime(0.01f)
				.setDelay(0.5f)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::popAnimation(AnimType::BitmapAnimation, 0.5f);

			Vec4 bitmap3[16][16] = {
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
			AnimationManager::addAnimation(
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