#include "animation/Animation.h"

#include "renderer/Renderer.h"
#include "animation/Styles.h"
#include "animation/AnimationBuilders.h"
#include "utils/CMath.h"

namespace MathAnim
{
	namespace AnimationManagerEx
	{
		// List of animations, sorted by start time
		static std::vector<AnimationEx> mAnimations;

		// List of animatable objects, sorted by start time
		static std::vector<AnimObject> mObjects;

		void addAnimObject(AnimObject object)
		{
			bool insertedObject = false;
			for (auto iter = mObjects.begin(); iter != mObjects.end(); iter++)
			{
				// Insert it here. The list will always be sorted
				if (object.startTime > iter->startTime)
				{
					mObjects.insert(iter, object);
					insertedObject = true;
					break;
				}
			}

			if (!insertedObject)
			{
				// If we didn't insert the object
				// that means it must start after all the
				// current animObject start times.
				mObjects.push_back(object);
			}
		}

		void addAnimation(AnimationEx animation)
		{
			animation.objectIndex = -1;
			for (auto iter = mObjects.begin(); iter != mObjects.end(); iter++)
			{
				if (iter->id == animation.objectId)
				{
					animation.objectIndex = iter - mObjects.begin();
				}
			}
			if (animation.objectIndex == -1)
			{
				g_logger_error("Could not find animation object with id: %d. Not adding this animation.", animation.objectId);
				return;
			}

			bool insertedObject = false;
			for (auto iter = mAnimations.begin(); iter != mAnimations.end(); iter++)
			{
				// Insert it here. The list will always be sorted
				if (animation.startTime > iter->startTime)
				{
					mAnimations.insert(iter, animation);
					insertedObject = true;
					break;
				}
			}

			if (!insertedObject)
			{
				// If we didn't insert the animation
				// that means it must start after all the
				// current animation start times.
				mAnimations.push_back(animation);
			}
		}

		void render(NVGcontext* vg, float time)
		{
			for (auto objectIter = mObjects.begin(); objectIter != mObjects.end(); objectIter++)
			{
				float objectDeathTime = objectIter->startTime + objectIter->duration;
				if (objectIter->startTime <= time && time <= objectDeathTime)
				{
					objectIter->render(vg);
				}
			}

			for (auto animIter = mAnimations.begin(); animIter != mAnimations.end(); animIter++)
			{
				float animDeathTime = animIter->startTime + animIter->duration;
				if (animIter->startTime <= time && time <= animDeathTime)
				{
					float interpolatedT = (time - animIter->startTime) / animIter->duration;
					animIter->render(vg, interpolatedT);
					// animIter->animationIsAlive = true;
				}
				//else if (time > animDeathTime && animIter->animationIsAlive)
				//{
				//	animIter->endAnimation();
				//	animIter->animationIsAlive = false;
				//}
			}
		}

		const AnimObject* getObject(int index)
		{
			if (index >= 0 && index < mObjects.size())
			{
				return &mObjects[index];
			}

			return nullptr;
		}
	}

	void AnimationEx::render(NVGcontext* vg, float t) const
	{
		switch (type)
		{
		case AnimTypeEx::WriteInText:
			getParent()->as.textObject.renderWriteInAnimation(vg, t, getParent());
			break;
		default:
			// TODO: Add magic_enum
			// g_logger_info("Unknown animation: '%s'", magic_enum::enum_name(type).data());
			g_logger_info("Unknown animation: %d", type);
			break;
		}
	}

	const AnimObject* AnimationEx::getParent() const
	{
		const AnimObject* res = AnimationManagerEx::getObject(objectIndex);
		g_logger_assert(res != nullptr, "Invalid anim object.");
		return res;
	}

	void AnimObject::render(NVGcontext* vg) const
	{
		switch (objectType)
		{
		case AnimObjectType::TextObject:
			this->as.textObject.render(vg, this);
			break;
		default:
			// TODO: Add magic_enum
			// g_logger_info("Unknown animation: '%s'", magic_enum::enum_name(objectType).data());
			g_logger_info("Unknown animation: %d", objectType);
			break;
		}
	}

	namespace AnimationManager
	{
		static std::vector<Animation> mAnimations;
		static std::vector<Style> mStyles;

		static std::vector<PopAnimation> mAnimationPops = std::vector<PopAnimation>();

		static std::vector<TranslateAnimation> mTranslationAnimations = std::vector<TranslateAnimation>();

		static std::vector<Interpolation> mInterpolations = std::vector<Interpolation>();

		static float mTime = 0.0f;
		static float mLastAnimEndTime = 0.0f;

		void addAnimation(Animation& animation, const Style& style)
		{
			mLastAnimEndTime += animation.delay;
			animation.startTime = mLastAnimEndTime;
			mLastAnimEndTime += animation.duration;

			mAnimations.push_back(animation);
			mStyles.push_back(style);
		}

		void addInterpolation(Animation& animation)
		{
		//	Interpolation interpolation;
		//	interpolation.ogAnimIndex = mBezier2Animations.size() - 1;
		//	interpolation.ogAnim = mBezier2Animations[interpolation.ogAnimIndex];
		//	interpolation.ogP0Index = mFilledCircleAnimations.size() - 3;
		//	interpolation.ogP1Index = mFilledCircleAnimations.size() - 2;
		//	interpolation.ogP2Index = mFilledCircleAnimations.size() - 1;

		//	mLastAnimEndTime += animation.delay;
		//	animation.startTime = mLastAnimEndTime;
		//	mLastAnimEndTime += animation.duration;

		//	interpolation.newAnim = animation;
		//	mInterpolations.emplace_back(interpolation);
		}

		void popAnimation(AnimType animationType, float delay, float fadeOutTime)
		{
			PopAnimation pop;
			pop.animType = animationType;
			pop.startTime = mLastAnimEndTime + delay;
			pop.fadeOutTime = fadeOutTime;

			pop.index = mAnimations.size() - 1;

			mAnimationPops.emplace_back(pop);
		}

		void translateAnimation(AnimType animationType, const Vec2& translation, float duration, float delay)
		{
		//	TranslateAnimation anim;
		//	anim.animType = animationType;
		//	anim.duration = duration;
		//	anim.translation = translation;

		//	anim.startTime = mLastAnimEndTime + delay;

		//	switch (animationType)
		//	{
		//	case AnimType::Bezier1Animation:
		//		anim.index = mBezier1Animations.size() - 1;
		//		break;
		//	case AnimType::Bezier2Animation:
		//		anim.index = mBezier2Animations.size() - 1;
		//		break;
		//	case AnimType::BitmapAnimation:
		//		anim.index = mBitmapAnimations.size() - 1;
		//		break;
		//	case AnimType::ParametricAnimation:
		//		anim.index = mParametricAnimations.size() - 1;
		//		break;
		//	case AnimType::TextAnimation:
		//		anim.index = mTextAnimations.size() - 1;
		//		break;
		//	case AnimType::FilledBoxAnimation:
		//		anim.index = mFilledBoxAnimations.size() - 1;
		//		break;
		//	case AnimType::FilledCircleAnimation:
		//		anim.index = mFilledCircleAnimations.size() - 1;
		//		break;
		//	}

		//	mTranslationAnimations.emplace_back(anim);
		}

		void drawParametricAnimation(Animation& genericAnimation, const Style& style)
		{
			if (mTime < genericAnimation.startTime) return;
			Style styleToUse = style;

			float lerp = glm::clamp((mTime - genericAnimation.startTime) / genericAnimation.duration, 0.0f, 1.0f);

			ParametricAnimation& anim = genericAnimation.as.parametric;
			float percentToDo = ((anim.endT - anim.startT) * lerp) / (anim.endT - anim.startT);

			float t = anim.startT;
			const float granularity = ((anim.endT - anim.startT) * percentToDo) / anim.granularity;
			for (int i = 0; i < anim.granularity; i++)
			{
				if (i < anim.granularity - 1 && style.lineEnding == CapType::Arrow)
				{
					styleToUse.lineEnding = CapType::Flat;
				}
				else if (style.lineEnding == CapType::Arrow)
				{
					styleToUse.lineEnding = CapType::Arrow;
				}

				Vec2 coord = anim.parametricEquation(t) + anim.translation;
				float nextT = t + granularity;
				Vec2 nextCoord = anim.parametricEquation(nextT) + anim.translation;
				Renderer::drawLine(Vec2{coord.x, coord.y}, Vec2{nextCoord.x, nextCoord.y}, styleToUse);
				t = nextT;
			}
		}

		void drawTextAnimation(Animation& genericAnimation, const Style& style)
		{
			if (mTime < genericAnimation.startTime) return;

			TextAnimation& anim = genericAnimation.as.text;
			std::string tmpStr = std::string(anim.text);
			int numCharsToShow = glm::clamp((int)((mTime - genericAnimation.startTime) / anim.typingTime), 0, (int)tmpStr.length());
			Renderer::drawString(tmpStr.substr(0, numCharsToShow), *anim.font, Vec2{anim.position.x, anim.position.y}, anim.scale, style.color);
		}

		void drawBitmapAnimation(Animation& genericAnimation, const Style& style)
		{
			if (mTime < genericAnimation.startTime) return;
			int numSquaresToShow = glm::clamp((int)(((mTime - genericAnimation.startTime) / genericAnimation.duration) * 16 * 16), 0, 16 * 16);
			BitmapAnimation& anim = genericAnimation.as.bitmap;

			int randNumber = rand() % 100;
			const Vec2 gridSize = Vec2{
				anim.canvasSize.x / 16.0f,
				anim.canvasSize.y / 16.0f
			};
			for (int y = 0; y < 16; y++)
			{
				for (int x = 0; x < 16; x++)
				{
					if (anim.bitmapState[y][x])
					{
						Vec2 position = anim.canvasPosition + Vec2{(float)x * gridSize.x, (float)y * gridSize.y};
						Style newStyle = style;
						newStyle.color = anim.bitmap[y][x];
						Renderer::drawFilledSquare(position, gridSize, newStyle);
					}

					while (numSquaresToShow > anim.bitmapSquaresShowing && randNumber >= 49)
					{
						int randX = rand() % 16;
						int randY = rand() % 16;
						while (anim.bitmapState[randY][randX])
						{
							randX = rand() % 16;
							randY = rand() % 16;
						}

						anim.bitmapState[randY][randX] = true;
						anim.bitmapSquaresShowing++;
					}
				}
			}
		}

		void drawBezier1Animation(Animation& genericAnimation, const Style& style)
		{
			if (mTime < genericAnimation.startTime) return;
			Style styleToUse = style;

			// 0, 1 
			float lerp = glm::clamp((mTime - genericAnimation.startTime) / genericAnimation.duration, 0.0f, 1.0f);
			float percentToDo = lerp;
			Bezier1Animation& anim = genericAnimation.as.bezier1;

			float t = 0;
			const float granularity = percentToDo / anim.granularity;
			for (int i = 0; i < anim.granularity; i++)
			{
				if (i < anim.granularity - 1 && style.lineEnding == CapType::Arrow)
				{
					styleToUse.lineEnding = CapType::Flat;
				}
				else if (style.lineEnding == CapType::Arrow)
				{
					styleToUse.lineEnding = CapType::Arrow;
				}

				Vec2 coord = CMath::bezier1(anim.p0 + Vec2{}, anim.p1 + Vec2{}, t);
				float nextT = t + granularity;
				Vec2 nextCoord = CMath::bezier1(anim.p0 + Vec2{}, anim.p1 + Vec2{}, nextT);
				Renderer::drawLine(Vec2{coord.x, coord.y}, Vec2{nextCoord.x, nextCoord.y}, styleToUse);
				t = nextT;
			}
		}

		void drawBezier2Animation(Animation& genericAnimation, const Style& style)
		{
			if (mTime < genericAnimation.startTime) return;
			Style styleToUse = style;

			float lerp = glm::clamp((mTime - genericAnimation.startTime) / genericAnimation.duration, 0.0f, 1.0f);
			float percentToDo = (genericAnimation.duration * lerp) / genericAnimation.duration;
			Bezier2Animation& anim = genericAnimation.as.bezier2;

			float t = 0;
			const float granularity = percentToDo / anim.granularity;
			for (int i = 0; i < anim.granularity; i++)
			{
				if (i < anim.granularity - 1 && style.lineEnding == CapType::Arrow)
				{
					styleToUse.lineEnding = CapType::Flat;
				}
				else if (style.lineEnding == CapType::Arrow)
				{
					styleToUse.lineEnding = CapType::Arrow;
				}

				Vec2 coord = CMath::bezier2(anim.p0 + Vec2{}, anim.p1 + Vec2{}, anim.p2 + Vec2{}, t);
				float nextT = t + granularity;
				Vec2 nextCoord = CMath::bezier2(anim.p0 + Vec2{}, anim.p1 + Vec2{}, anim.p2 + Vec2{}, nextT);
				Renderer::drawLine(Vec2{coord.x, coord.y}, Vec2{nextCoord.x, nextCoord.y}, styleToUse);
				t = nextT;
			}
		}

		static void interpolate(Interpolation& anim)
		{
		//	if (mTime <= anim.newAnim.startTime) return;

		//	float lerp = glm::clamp((mTime - anim.newAnim.startTime) / anim.newAnim.duration, 0.0f, 1.0f);
		//	Bezier2Animation& ogAnim = mBezier2Animations.at(anim.ogAnimIndex);
		//	ogAnim.p0 += (anim.newAnim.p0 - ogAnim.p0) * lerp;
		//	ogAnim.p1 += (anim.newAnim.p1 - ogAnim.p1) * lerp;
		//	ogAnim.p2 += (anim.newAnim.p2 - ogAnim.p2) * lerp;

		//	mFilledCircleAnimations.at(anim.ogP0Index).position = ogAnim.p0;
		//	mFilledCircleAnimations.at(anim.ogP1Index).position = ogAnim.p1;
		//	mFilledCircleAnimations.at(anim.ogP2Index).position = ogAnim.p2;
		}

		void drawFilledCircleAnimation(Animation& genericAnimation, const Style& style)
		{
			if (mTime < genericAnimation.startTime) return;

			float percentToDo = glm::clamp((mTime - genericAnimation.startTime) / genericAnimation.duration, 0.0f, 1.0f);
			FilledCircleAnimation& anim = genericAnimation.as.filledCircle;

			float t = 0;
			float sectorSize = (percentToDo * 360.0f) / (float)anim.numSegments;
			for (int i = 0; i < anim.numSegments; i++)
			{
				float x = anim.radius * glm::cos(glm::radians(t));
				float y = anim.radius * glm::sin(glm::radians(t));
				float nextT = t + sectorSize;
				float nextX = anim.radius * glm::cos(glm::radians(nextT));
				float nextY = anim.radius * glm::sin(glm::radians(nextT));

				Renderer::drawFilledTriangle(anim.position + Vec2{}, anim.position + Vec2{x, y}, anim.position + Vec2{nextX, nextY}, style);

				t += sectorSize;
			}
		}

		void drawFilledBoxAnimation(Animation& genericAnimation, const Style& style)
		{
			if (mTime < genericAnimation.startTime) return;

			float percentToDo = glm::clamp((mTime - genericAnimation.startTime) / genericAnimation.duration, 0.0f, 1.0f);
			FilledBoxAnimation& anim = genericAnimation.as.filledBox;

			switch (anim.fillDirection)
			{
			case Direction::Up:
				Renderer::drawFilledSquare(anim.center - (anim.size / 2.0f), Vec2{anim.size.x, anim.size.y * percentToDo}, style);
				break;
			case Direction::Down:
				Renderer::drawFilledSquare(anim.center + (anim.size / 2.0f) - Vec2{anim.size.x, anim.size.y * percentToDo},
					Vec2{anim.size.x, anim.size.y * percentToDo}, style);
				break;
			case Direction::Right:
				Renderer::drawFilledSquare(anim.center - (anim.size / 2.0f), Vec2{anim.size.x * percentToDo, anim.size.y}, style);
				break;
			case Direction::Left:
				Renderer::drawFilledSquare(anim.center + (anim.size / 2.0f) - Vec2{anim.size.x * percentToDo, anim.size.y},
					Vec2{anim.size.x * percentToDo, anim.size.y}, style);
				break;
			}
		}

		static void popAnim(PopAnimation& anim)
		{
			if (anim.startTime - mTime < 0)
			{
				mAnimations.at(anim.index).startTime = FLT_MAX;
			}
			else
			{
				float percent = (anim.startTime - mTime) / anim.fadeOutTime;
				mStyles.at(anim.index).color.a = percent;
			}
		}

		void update(float dt)
		{
			mTime += dt;
			for (int i = 0; i < mAnimations.size(); i++)
			{
				Animation& anim = mAnimations[i];
				const Style& style = mStyles.at(i);
				anim.drawAnimation(anim, style);
			}

			//for (int i = 0; i < mInterpolations.size(); i++)
			//{
			//	Interpolation& interpolation = mInterpolations[i];
			//	interpolate(interpolation);
			//}

			for (int i = 0; i < mAnimationPops.size(); i++)
			{
				PopAnimation& anim = mAnimationPops[i];
				if (anim.index >= 0 && mTime >= anim.startTime - anim.fadeOutTime)
				{
					popAnim(anim);
				}
			}
			//for (int i = 0; i < mTranslationAnimations.size(); i++)
			//{
			//	TranslateAnimation& anim = mTranslationAnimations[i];
			//	if (mTime >= anim.startTime)
			//	{
			//		if (anim.index >= 0)
			//		{
			//			float delta = glm::clamp(dt / anim.duration, 0.0f, 1.0f);
			//			float overallDelta = glm::clamp((mTime - anim.startTime) / anim.duration, 0.0f, 1.0f);
			//			switch (anim.animType)
			//			{
			//			case AnimType::Bezier1Animation:
			//				mBezier1Animations.at(anim.index).p0 += delta * anim.translation;
			//				mBezier1Animations.at(anim.index).p1 += delta * anim.translation;
			//				break;
			//			case AnimType::Bezier2Animation:
			//				mBezier2Animations.at(anim.index).p0 += delta * anim.translation;
			//				mBezier2Animations.at(anim.index).p1 += delta * anim.translation;
			//				mBezier2Animations.at(anim.index).p2 += delta * anim.translation;
			//				break;
			//			case AnimType::BitmapAnimation:
			//				mBitmapAnimations.at(anim.index).canvasPosition += delta * anim.translation;
			//				break;
			//			case AnimType::ParametricAnimation:
			//				mParametricAnimations.at(anim.index).translation = overallDelta * anim.translation;
			//				break;
			//			case AnimType::TextAnimation:
			//				mTextAnimations.at(anim.index).position += delta * anim.translation;
			//				break;
			//			case AnimType::FilledBoxAnimation:
			//				mFilledBoxAnimations.at(anim.index).center += delta * anim.translation;
			//				break;
			//			case AnimType::FilledCircleAnimation:
			//				mFilledCircleAnimations.at(anim.index).position += delta * anim.translation;
			//				break;
			//			}
			//		}
			//	}
			//}
		}

		void reset()
		{
			mTime = 0.0f;
		}
	}
}