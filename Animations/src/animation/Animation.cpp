#include "renderer/Renderer.h"
#include "animation/Animation.h"
#include "animation/Styles.h"
#include "utils/CMath.h"

namespace MathAnim
{
	namespace AnimationManager
	{
		static std::vector<ParametricAnimation> mParametricAnimations = std::vector<ParametricAnimation>();
		static std::vector<Style> mParametricStyles = std::vector<Style>();

		static std::vector<TextAnimation> mTextAnimations = std::vector<TextAnimation>();
		static std::vector<Style> mTextStyles = std::vector<Style>();

		static std::vector<BitmapAnimation> mBitmapAnimations = std::vector<BitmapAnimation>();
		static std::vector<Style> mBitmapStyles = std::vector<Style>();

		static std::vector<Bezier1Animation> mBezier1Animations = std::vector<Bezier1Animation>();
		static std::vector<Style> mBezier1Styles = std::vector<Style>();

		static std::vector<Bezier2Animation> mBezier2Animations = std::vector<Bezier2Animation>();
		static std::vector<Style> mBezier2Styles = std::vector<Style>();

		static std::vector<FilledCircleAnimation> mFilledCircleAnimations = std::vector<FilledCircleAnimation>();
		static std::vector<Style> mFilledCircleStyles = std::vector<Style>();

		static std::vector<PopAnimation> mAnimationPops = std::vector<PopAnimation>();

		static std::vector<TranslateAnimation> mTranslationAnimations = std::vector<TranslateAnimation>();

		static float mTime = 0.0f;
		static float mLastAnimEndTime = 0.0f;

		void addBitmapAnimation(BitmapAnimation& animation, const Style& style)
		{
			mLastAnimEndTime += animation.delay;
			animation.startTime = mLastAnimEndTime;
			mLastAnimEndTime += animation.duration;

			mBitmapAnimations.push_back(animation);
			mBitmapStyles.push_back(style);
		}

		void addParametricAnimation(ParametricAnimation& animation, const Style& style)
		{
			mLastAnimEndTime += animation.delay;
			animation.startTime = mLastAnimEndTime;
			mLastAnimEndTime += animation.duration;

			mParametricAnimations.push_back(animation);
			mParametricStyles.push_back(style);
		}

		void addTextAnimation(TextAnimation& animation, const Style& style)
		{
			mLastAnimEndTime += animation.delay;
			animation.startTime = mLastAnimEndTime;
			mLastAnimEndTime += animation.typingTime * animation.text.length();

			mTextAnimations.push_back(animation);
			mTextStyles.push_back(style);
		}

		void addBezier1Animation(Bezier1Animation& animation, const Style& style)
		{
			mLastAnimEndTime += animation.delay;
			animation.startTime = mLastAnimEndTime;
			mLastAnimEndTime += animation.duration;

			mBezier1Animations.push_back(animation);
			mBezier1Styles.push_back(style);
		}

		void addBezier2Animation(Bezier2Animation& animation, const Style& style)
		{
			mLastAnimEndTime += animation.delay;
			animation.startTime = mLastAnimEndTime;
			mLastAnimEndTime += animation.duration;

			mBezier2Animations.push_back(animation);
			mBezier2Styles.push_back(style);
		}

		void addFilledCircleAnimation(FilledCircleAnimation& animation, const Style& style)
		{
			mLastAnimEndTime += animation.delay;
			animation.startTime = mLastAnimEndTime;
			mLastAnimEndTime += animation.duration;

			mFilledCircleAnimations.push_back(animation);
			mFilledCircleStyles.push_back(style);
		}

		void popAnimation(AnimType animationType, float delay)
		{
			PopAnimation pop;
			pop.animType = animationType;
			mLastAnimEndTime += delay;
			pop.startTime = mLastAnimEndTime;

			switch (animationType)
			{
			case AnimType::Bezier1Animation:
				pop.index = mBezier1Animations.size() - 1;
				break;
			case AnimType::Bezier2Animation:
				pop.index = mBezier2Animations.size() - 1;
				break;
			case AnimType::BitmapAnimation:
				pop.index = mBitmapAnimations.size() - 1;
				break;
			case AnimType::ParametricAnimation:
				pop.index = mParametricAnimations.size() - 1;
				break;
			case AnimType::TextAnimation:
				pop.index = mTextAnimations.size() - 1;
				break;
			}

			mAnimationPops.emplace_back(pop);
		}

		void translateAnimation(AnimType animationType, const glm::vec2& translation, float duration, float delay)
		{
			TranslateAnimation anim;
			anim.animType = animationType;
			anim.duration = duration;
			anim.translation = translation;

			mLastAnimEndTime += delay;
			anim.startTime = mLastAnimEndTime;
			mLastAnimEndTime += anim.duration;

			switch (animationType)
			{
			case AnimType::Bezier1Animation:
				anim.index = mBezier1Animations.size() - 1;
				break;
			case AnimType::Bezier2Animation:
				anim.index = mBezier2Animations.size() - 1;
				break;
			case AnimType::BitmapAnimation:
				anim.index = mBitmapAnimations.size() - 1;
				break;
			case AnimType::ParametricAnimation:
				anim.index = mParametricAnimations.size() - 1;
				break;
			case AnimType::TextAnimation:
				anim.index = mTextAnimations.size() - 1;
				break;
			}

			mTranslationAnimations.emplace_back(anim);
		}

		static void drawParametricAnimation(const ParametricAnimation& anim, const Style& style)
		{
			if (mTime < anim.startTime) return;

			float lerp = glm::clamp((mTime - anim.startTime) / anim.duration, 0.0f, 1.0f);
			float percentToDo = ((anim.endT - anim.startT) * lerp) / (anim.endT - anim.startT);

			float t = anim.startT;
			const float granularity = ((anim.endT - anim.startT) * percentToDo) / anim.granularity;
			for (int i = 0; i < anim.granularity; i++)
			{
				glm::vec2 coord = anim.parametricEquation(t) + anim.translation;
				float nextT = t + granularity;
				glm::vec2 nextCoord = anim.parametricEquation(nextT) + anim.translation;
				Renderer::drawLine(glm::vec2(coord.x, coord.y), glm::vec2(nextCoord.x, nextCoord.y), style);
				t = nextT;
			}
		}

		static void drawTextAnimation(const TextAnimation& anim, const Style& style)
		{
			if (mTime < anim.startTime) return;

			int numCharsToShow = glm::clamp((int)((mTime - anim.startTime) / anim.typingTime), 0, (int)anim.text.length());
			Renderer::drawString(anim.text.substr(0, numCharsToShow), *anim.font, anim.position, anim.scale, style.color);
		}

		static void drawBitmapAnimation(BitmapAnimation& anim, const Style& style)
		{
			if (mTime < anim.startTime) return;
			int numSquaresToShow = glm::clamp((int)((mTime - anim.startTime) / anim.revealTime), 0, 16 * 16);

			int randNumber = rand() % 100;
			const glm::vec2 gridSize = glm::vec2{
				anim.canvasSize.x / 16.0f,
				anim.canvasSize.y / 16.0f
			};
			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 16; x++)
				{
					if (anim.bitmapState[y][x] && anim.bitmap[y][x])
					{
						glm::vec2 position = anim.canvasPosition + glm::vec2((float)x * gridSize.x, (float)y * gridSize.y);
						Renderer::drawFilledSquare(position, gridSize, style);
					}

					if (numSquaresToShow > anim.bitmapSquaresShowing && randNumber >= 49)
					{
						int randX = rand() % 16;
						int randY = rand() % 16;
						while (anim.bitmapState[randY][randX])
						{
							randX = rand() % 16;
							randY = rand() % 16;
						}

						if (anim.bitmap[randY][randX])
						{
							anim.bitmapState[randY][randX] = true;
							anim.bitmapSquaresShowing++;
						}
					}
				}
			}
		}

		static void drawBezier1Animation(Bezier1Animation& anim, const Style& style)
		{
			if (mTime < anim.startTime) return;

			float lerp = glm::clamp((mTime - anim.startTime) / anim.duration, 0.0f, 1.0f);
			float percentToDo = (anim.duration * lerp) / anim.duration;

			float t = 0;
			const float granularity = percentToDo / anim.granularity;
			for (int i = 0; i < anim.granularity; i++)
			{
				glm::vec2 coord = CMath::bezier1(anim.p0, anim.p1, t);
				float nextT = t + granularity;
				glm::vec2 nextCoord = CMath::bezier1(anim.p0, anim.p1, nextT);
				Renderer::drawLine(glm::vec2(coord.x, coord.y), glm::vec2(nextCoord.x, nextCoord.y), style);
				t = nextT;
			}
		}

		static void drawBezier2Animation(Bezier2Animation& anim, const Style& style)
		{
			if (mTime < anim.startTime) return;

			float lerp = glm::clamp((mTime - anim.startTime) / anim.duration, 0.0f, 1.0f);
			float percentToDo = (anim.duration * lerp) / anim.duration;

			float t = 0;
			const float granularity = percentToDo / anim.granularity;
			for (int i = 0; i < anim.granularity; i++)
			{
				glm::vec2 coord = CMath::bezier2(anim.p0, anim.p1, anim.p2, t);
				float nextT = t + granularity;
				glm::vec2 nextCoord = CMath::bezier2(anim.p0, anim.p1, anim.p2, nextT);
				Renderer::drawLine(glm::vec2(coord.x, coord.y), glm::vec2(nextCoord.x, nextCoord.y), style);
				t = nextT;
			}
		}

		static void drawFilledCircleAnimation(FilledCircleAnimation& anim, const Style& style)
		{
			if (mTime < anim.startTime) return;

			float percentToDo = glm::clamp((mTime - anim.startTime) / anim.duration, 0.0f, 1.0f);
			float t = 0;
			float sectorSize = (percentToDo * 360.0f) / (float)anim.numSegments;
			for (int i = 0; i < anim.numSegments; i++)
			{
				float x = anim.radius * glm::cos(glm::radians(t));
				float y = anim.radius * glm::sin(glm::radians(t));
				float nextT = t + sectorSize;
				float nextX = anim.radius * glm::cos(glm::radians(nextT));
				float nextY = anim.radius * glm::sin(glm::radians(nextT));

				Renderer::drawFilledTriangle(anim.position, anim.position + glm::vec2(x, y), anim.position + glm::vec2(nextX, nextY), style);

				t += sectorSize;
			}
		}

		void update(float dt)
		{
			mTime += dt;
			for (int i = 0; i < mParametricAnimations.size(); i++)
			{
				const ParametricAnimation& anim = mParametricAnimations[i];
				const Style& style = mParametricStyles.at(i);
				drawParametricAnimation(anim, style);
			}

			for (int i = 0; i < mTextAnimations.size(); i++)
			{
				const TextAnimation& anim = mTextAnimations[i];
				const Style& style = mTextStyles.at(i);
				drawTextAnimation(anim, style);
			}

			for (int i = 0; i < mBitmapAnimations.size(); i++)
			{
				BitmapAnimation& anim = mBitmapAnimations[i];
				const Style& style = mBitmapStyles.at(i);
				drawBitmapAnimation(anim, style);
			}

			for (int i = 0; i < mBezier1Animations.size(); i++)
			{
				Bezier1Animation& anim = mBezier1Animations[i];
				const Style& style = mBezier1Styles.at(i);
				drawBezier1Animation(anim, style);
			}

			for (int i = 0; i < mBezier2Animations.size(); i++)
			{
				Bezier2Animation& anim = mBezier2Animations[i];
				const Style& style = mBezier2Styles.at(i);
				drawBezier2Animation(anim, style);
			}

			for (int i = 0; i < mFilledCircleAnimations.size(); i++)
			{
				FilledCircleAnimation& anim = mFilledCircleAnimations[i];
				const Style& style = mFilledCircleStyles.at(i);
				drawFilledCircleAnimation(anim, style);
			}

			for (int i = 0; i < mAnimationPops.size(); i++)
			{
				PopAnimation& anim = mAnimationPops[i];
				if (mTime >= anim.startTime)
				{
					if (anim.index >= 0)
					{
						switch (anim.animType)
						{
						case AnimType::Bezier1Animation:
							mBezier1Animations.at(anim.index).startTime = FLT_MAX;
							break;
						case AnimType::Bezier2Animation:
							mBezier2Animations.at(anim.index).startTime = FLT_MAX;
							break;
						case AnimType::BitmapAnimation:
							mBitmapAnimations.at(anim.index).startTime = FLT_MAX;
							break;
						case AnimType::ParametricAnimation:
							mParametricAnimations.at(anim.index).startTime = FLT_MAX;
							break;
						case AnimType::TextAnimation:
							mTextAnimations.at(anim.index).startTime = FLT_MAX;
							break;
						}
					}

					mAnimationPops.erase(mAnimationPops.begin() + i);
					i--;
				}
			}
			for (int i = 0; i < mTranslationAnimations.size(); i++)
			{
				TranslateAnimation& anim = mTranslationAnimations[i];
				if (mTime >= anim.startTime)
				{
					if (anim.index >= 0)
					{
						float delta = glm::clamp(dt / anim.duration, 0.0f, 1.0f);
						float overallDelta = glm::clamp((mTime - anim.startTime) / anim.duration, 0.0f, 1.0f);
						switch (anim.animType)
						{
						case AnimType::Bezier1Animation:
							mBezier1Animations.at(anim.index).p0 += delta * anim.translation;
							mBezier1Animations.at(anim.index).p1 += delta * anim.translation;
							break;
						case AnimType::Bezier2Animation:
							mBezier2Animations.at(anim.index).p0 += delta * anim.translation;
							mBezier2Animations.at(anim.index).p1 += delta * anim.translation;
							mBezier2Animations.at(anim.index).p2 += delta * anim.translation;
							break;
						case AnimType::BitmapAnimation:
							mBitmapAnimations.at(anim.index).canvasPosition += delta * anim.translation;
							break;
						case AnimType::ParametricAnimation:
							mParametricAnimations.at(anim.index).translation = overallDelta * anim.translation;
							break;
						case AnimType::TextAnimation:
							mTextAnimations.at(anim.index).position += delta * anim.translation;
							break;
						}
					}
				}
			}
		}

		void reset()
		{
			mTime = 0.0f;
		}
	}
}