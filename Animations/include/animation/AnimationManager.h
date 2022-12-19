#ifndef MATH_ANIM_ANIMATION_MANAGER_H
#define MATH_ANIM_ANIMATION_MANAGER_H
#include "core.h"
#include "animation/Animation.h"

namespace MathAnim
{
	struct AnimObject;
	struct Animation;
	struct Framebuffer;
	struct OrthoCamera;

	struct AnimationManagerData;

	namespace AnimationManager
	{
		AnimationManagerData* create();
		void free(AnimationManagerData* animManager);
		void endFrame(AnimationManagerData* am);
		void resetToFrame(AnimationManagerData* am, uint32 absoluteFrame);
		void calculateAnimationKeyFrames(AnimationManagerData* am);

		void addAnimObject(AnimationManagerData* am, const AnimObject& object);
		void addAnimation(AnimationManagerData* am, const Animation& animation);

		void removeAnimObject(AnimationManagerData* am, AnimObjId animObj);
		void removeAnimation(AnimationManagerData* am, AnimId anim);

		void addObjectToAnim(AnimationManagerData* am, AnimObjId animObj, AnimId animation);
		void removeObjectFromAnim(AnimationManagerData* am, AnimObjId animObj, AnimId animation);

		bool setAnimationTime(AnimationManagerData* am, AnimId anim, int frameStart, int duration);
		void setAnimationTrack(AnimationManagerData* am, AnimId anim, int track);

		Framebuffer prepareFramebuffer(int outputWidth, int outputHeight);
		void render(AnimationManagerData* am, int deltaFrame);

		int lastAnimatedFrame(const AnimationManagerData* am);
		const AnimObject* getActiveOrthoCamera(const AnimationManagerData* am);
		void setActiveOrthoCamera(AnimationManagerData* am, AnimObjId id);

		// NOTE: This function is slow, only use this as a backup if getObject fails
		const AnimObject* getPendingObject(const AnimationManagerData* am, AnimObjId animObj);
		const AnimObject* getObject(const AnimationManagerData* am, AnimObjId animObj);
		AnimObject* getMutableObject(AnimationManagerData* am, AnimObjId animObj);
		const Animation* getAnimation(const AnimationManagerData* am, AnimId anim);
		Animation* getMutableAnimation(AnimationManagerData* am, AnimId anim);

		const std::vector<AnimObject>& getAnimObjects(const AnimationManagerData* am);
		const std::vector<Animation>& getAnimations(const AnimationManagerData* am);

		std::vector<AnimId> getAssociatedAnimations(const AnimationManagerData* am, AnimObjId obj);
		std::vector<AnimObjId> getChildren(const AnimationManagerData* am, AnimObjId obj);

		RawMemory serialize(const AnimationManagerData* am);
		void deserialize(AnimationManagerData* am, RawMemory& memory, int currentFrame);
		void sortAnimations(AnimationManagerData* am);

		void applyGlobalTransforms(AnimationManagerData* am);
		void applyGlobalTransformsTo(AnimationManagerData* am, AnimObjId obj);
		void updateObjectState(AnimationManagerData* am, AnimObjId animObj);
	}
}

#endif 