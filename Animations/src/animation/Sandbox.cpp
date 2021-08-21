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
#include "animation/customAnimations/PlottingLettersWithPoints.h"
#include "animation/customAnimations/LagrangeInterpolation.h"
#include "animation/customAnimations/BezierCurveAnimation.h"
#include "animation/customAnimations/FillingInLetters.h"
#include "animation/customAnimations/WindingContours.h"
#include "animation/customAnimations/DrawRandomCurves.h"
#include "animation/customAnimations/TranslatingCurves.h"
#include "animation/customAnimations/HowBezierWorks.h"
#include "animation/customAnimations/CharacterAnimation.h"
#include "animation/customAnimations/PixelsOnAScreen.h"
#include "animation/customAnimations/MoreQuestions.h"

namespace MathAnim
{
	namespace Sandbox
	{
		static Font* font = nullptr;

		void init()
		{
			//PhoneAnimation::init();
			//GridAnimation::init({ -2.0f, -2.0f }, { 4.0f, 4.0f }, { 0.25f, 0.25f });
			//BitmapLetterAnimations::init({ -2.0f, -2.0f }, { 4.0f, 4.0f }, { 0.25f, 0.25f });
			//DrawRandomCurves::init();
			//TranslatingCurves::init();
			//PlottingLettersWithPoints::init();
			//LagrangeInterpolation::init();
			//BezierCurveAnimation::init();
			//FillingInLetters::init();
			//WindingContours::init();
			//HowBezierWorks::init();
			//CharacterAnimation::init();
			//PixelsOnAScreen::init();
			MoreQuestions::init();
		}

		void update(float dt)
		{
			HowBezierWorks::update(dt);
		}
	}
}