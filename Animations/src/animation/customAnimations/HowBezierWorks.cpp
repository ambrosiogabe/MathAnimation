#include "animation/customAnimations/HowBezierWorks.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"
#include "renderer/Renderer.h"
#include "utils/CMath.h"

namespace MathAnim
{
	namespace HowBezierWorks
	{
		static void testPixel()
		{
			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0({ 0.0f, -1.0f })
				.setP1({ 1.25f, -1.0f })
				.setP2({ 3.0f, 2.0f })
				.setDuration(1.0f)
				.withPoints()
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition({ -1.0f, -0.0f })
				.setDuration(0.32f)
				.setRadius(0.06f)
				.setNumSegments(40)
				.setDelay(1.5f)
				.build(),
				Styles::defaultStyle
			);
			Style arrowStyle = Styles::defaultStyle;
			arrowStyle.lineEnding = CapType::Arrow;
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0({ -1.0f, 0.0f })
				.setP1({ 3.5f, 0.0f })
				.setDuration(1.5f)
				.build(),
				arrowStyle
			);

			Style redStyle = Styles::defaultStyle;
			redStyle.color = Colors::red;
			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition({ 1.6f, 0.0f })
				.setDuration(0.32f)
				.setNumSegments(40)
				.setRadius(0.06f)
				.setDelay(-0.5f)
				.build(),
				redStyle
			);
		}

		static void simpleBezierCurve()
		{
			Style blue = Styles::defaultStyle;
			blue.color = Colors::blue;
			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition({ -2.0f, -0.0f })
				.setDuration(0.32f)
				.setRadius(0.06f)
				.setNumSegments(40)
				.build(),
				blue
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition({ 2.0f, -0.0f })
				.setDuration(0.32f)
				.setRadius(0.06f)
				.setNumSegments(40)
				.setDelay(0.5f)
				.build(),
				blue
			);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0({ -2.0f, 0.0f })
				.setP1({ 2.0f, 0.0f })
				.setDuration(2.0f)
				.setDelay(0.5f)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0({ 0.0f, 0.125f })
				.setP1({ 0.0f, -0.125f })
				.setDelay(5.0f)
				.setDuration(0.25f)
				.build(),
				blue
			);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0({ 1.0f, 0.125f })
				.setP1({ 1.0f, -0.125f })
				.setDelay(1.0f)
				.setDuration(0.25f)
				.build(),
				blue
			);
		}

		void addingAThirdPoint()
		{
			Vec2 p0{-2.0f, -1.0f};
			Vec2 p1{-0.5f, 2.0f};
			Vec2 p2{2.0f, -1.5f};

			Style blueStyle = Styles::defaultStyle;
			blueStyle.color = Colors::blue;
			Style greenStyle = Styles::defaultStyle;
			greenStyle.color = Colors::green;

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(p0)
				.setRadius(0.06f)
				.setDuration(0.32f)
				.setNumSegments(40)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(p1)
				.setRadius(0.06f)
				.setDuration(0.32f)
				.setNumSegments(40)
				.build(),
				greenStyle
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(p2)
				.setRadius(0.06f)
				.setDuration(0.32f)
				.setNumSegments(40)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(1.0f)
				.setDelay(4.0f)
				.build(),
				Styles::defaultStyle
			);
			AnimationManager::popAnimation(AnimType::Bezier2Animation, 15.0f);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setDelay(16.0f)
				.setDuration(1.0f)
				.build(),
				greenStyle
			);
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(p1)
				.setP1(p2)
				.setDuration(1.0f)
				.setDelay(1.0f)
				.build(),
				greenStyle
			);

			// Another 10 seconds
		}

		void imaginePixelAsAxis()
		{
			Vec2 p0{-2.0f, -1.0f};
			Vec2 p1{0.0f, 2.0f};
			Vec2 p2{2.0f, -1.0f};

			Vec2 pixelPos{-3.0f, 0.0f};

			Style blueStyle = Styles::defaultStyle;
			blueStyle.color = Colors::blue;
			Style greenStyle = Styles::defaultStyle;
			greenStyle.color = Colors::green;
			Style redStyle = Styles::defaultStyle;
			redStyle.color = Colors::red;

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(p0)
				.setRadius(0.06f)
				.setDuration(0.32f)
				.setNumSegments(40)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(p1)
				.setRadius(0.06f)
				.setDuration(0.32f)
				.setNumSegments(40)
				.build(),
				greenStyle
			);

			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(p2)
				.setRadius(0.06f)
				.setDuration(0.32f)
				.setNumSegments(40)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				Bezier2AnimationBuilder()
				.setP0(p0)
				.setP1(p1)
				.setP2(p2)
				.setDuration(1.0f)
				.build(),
				Styles::defaultStyle
			);
			AnimationManager::addAnimation(
				FilledCircleAnimationBuilder()
				.setPosition(pixelPos)
				.setRadius(0.06f)
				.setDuration(0.32f)
				.setNumSegments(40)
				.setDelay(0.5f)
				.build(),
				blueStyle
			);
			Style arrowStyle = Styles::defaultStyle;
			arrowStyle.lineEnding = CapType::Arrow;
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0(pixelPos)
				.setP1(pixelPos + Vec2{5.0f, 0.0f})
				.setDuration(1.5f)
				.build(),
				arrowStyle
			);
			AnimationManager::popAnimation(AnimType::Bezier1Animation, 3.0f);

			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0({ -6.0f, pixelPos.y })
				.setP1({ 6.0f, pixelPos.y })
				.setDuration(1.0f)
				.setDelay(4.0f)
				.build(),
				redStyle
			);
			AnimationManager::addAnimation(
				Bezier1AnimationBuilder()
				.setP0({ pixelPos.x, -4.0f })
				.setP1({ pixelPos.x, 4.0f })
				.setDuration(1.0f)
				.setDelay(-0.8f)
				.build(),
				greenStyle
			);
		}

		void init()
		{
			//testPixel();
			//simpleBezierCurve();
			//addingAThirdPoint();
			imaginePixelAsAxis();
		}

		static float mTimeToStartBezierAnim = 39.0f;
		static float mTime = 0.0f;
		void updateBezierAnim(float dt)
		{
			if (mTime < mTimeToStartBezierAnim) return;

			static Vec2 p0{-2.0f, -1.0f};
			static Vec2 p1{-0.5f, 2.0f};
			static Vec2 p2{2.0f, -1.5f};

			static float duration = 4.0f;
			static int numSegments = 100;
			float percent = glm::clamp((mTime - mTimeToStartBezierAnim) / duration, 0.0f, 1.0f);
			float t = 0;
			float segmentSize = percent / numSegments;
			static Style blueStyle = Styles::defaultStyle;
			blueStyle.color = Colors::blue;

			Vec2 q0 = p1 * percent + p0 * (1.0f - percent);
			Vec2 q1 = p2 * percent + p1 * (1.0f - percent);
			if (percent != 0.0f && percent != 1.0f) 
				Renderer::drawLine(q0, q1, Styles::defaultStyle);
			Vec2 bezierPoint = q1 * percent + q0 * (1.0f - percent);
			Renderer::drawFilledCircle(bezierPoint, 0.06f, 40, blueStyle);

			Vec2 currentPoint;
			for (int i = 0; i < numSegments; i++)
			{
				currentPoint = CMath::bezier2(p0, p1, p2, t);
				Vec2 nextPoint = CMath::bezier2(p0, p1, p2, t + segmentSize);

				Renderer::drawLine(currentPoint, nextPoint, Styles::defaultStyle);

				currentPoint = nextPoint;
				t = t + segmentSize;
			}
		}
		
		void update(float dt)
		{
			mTime += dt;

			//updateBezierAnim(dt);
		}
	}
}