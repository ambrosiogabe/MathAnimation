#ifndef MATH_ANIM_ANIMATION_MANAGER_H
#define MATH_ANIM_ANIMATION_MANAGER_H
#include "core.h"
#include "animation/Animation.h"

struct NVGcontext;

namespace MathAnim
{
	struct AnimObject;
	struct Animation;

	namespace AnimationManager
	{
		void addAnimObject(const AnimObject& object);
		void addAnimationTo(const Animation& animation, AnimObject& animObject);
		void addAnimationTo(const Animation& animation, int animObjectId);

		bool removeAnimObject(int animObjectId);
		bool removeAnimation(int animObjectId, int animationId);

		bool setAnimObjectTime(int animObjectId, int frameStart, int duration);
		bool setAnimationTime(int animObjectId, int animationId, int frameStart, int duration);
		void setAnimObjectTrack(int animObjectId, int track);

		void render(NVGcontext* vg, int frame);

		const AnimObject* getObject(int animObjectId);
		AnimObject* getMutableObject(int animObjectId);
		Animation* getMutableAnimation(int animationId);
		const std::vector<AnimObject>& getAnimObjects();

		const AnimObject* getNextAnimObject(int animObjectId);

		void serialize(const char* savePath = nullptr);
		void deserialize(const char* loadPath = nullptr);
		void sortAnimObjects();

		const char* getAnimObjectName(AnimObjectTypeV1 type);
		const char* getAnimationName(AnimTypeV1 type);
	}
}

#endif 