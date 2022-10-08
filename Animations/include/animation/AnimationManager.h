#ifndef MATH_ANIM_ANIMATION_MANAGER_H
#define MATH_ANIM_ANIMATION_MANAGER_H
#include "core.h"
#include "animation/Animation.h"

struct NVGcontext;

namespace MathAnim
{
	struct AnimObject;
	struct Animation;
	struct Framebuffer;
	struct OrthoCamera;

	struct AnimationManagerData;

	namespace AnimationManager
	{
		AnimationManagerData* create(OrthoCamera& camera);
		void free(AnimationManagerData* animManager);
		void endFrame(AnimationManagerData* am);

		void addAnimObject(AnimationManagerData* am, const AnimObject& object);
		void addAnimation(AnimationManagerData* am, const Animation& animation);

		void removeAnimObject(AnimationManagerData* am, AnimObjId animObj);
		void removeAnimation(AnimationManagerData* am, AnimId anim);

		bool setAnimationTime(AnimationManagerData* am, AnimId anim, int frameStart, int duration);
		void setAnimationTrack(AnimationManagerData* am, AnimId anim, int track);

		Framebuffer prepareFramebuffer(int outputWidth, int outputHeight);
		void render(AnimationManagerData* am, NVGcontext* vg, int deltaFrame);

		int lastAnimatedFrame(const AnimationManagerData* am);

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
		void deserialize(AnimationManagerData* am, RawMemory& memory);
		void sortAnimations(AnimationManagerData* am);

		const char* getAnimObjectName(AnimObjectTypeV1 type);
		const char* getAnimationName(AnimTypeV1 type);
	}
}

#endif 