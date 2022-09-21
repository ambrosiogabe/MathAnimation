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

		void addAnimObject(AnimationManagerData* am, const AnimObject& object);
		void addAnimation(AnimationManagerData* am, const Animation& animation);

		bool removeAnimObject(AnimationManagerData* am, int animObjectId);
		bool removeAnimation(AnimationManagerData* am, int animationId);

		bool setAnimationTime(AnimationManagerData* am, int animationId, int frameStart, int duration);
		void setAnimationTrack(AnimationManagerData* am, int animationId, int track);

		Framebuffer prepareFramebuffer(int outputWidth, int outputHeight);
		void render(AnimationManagerData* am, NVGcontext* vg, int deltaFrame);

		int lastAnimatedFrame(const AnimationManagerData* am);

		bool isObjectNull(int animObjectId);
		const AnimObject* getObject(const AnimationManagerData* am, int animObjectId);
		AnimObject* getMutableObject(AnimationManagerData* am, int animObjectId);
		const Animation* getAnimation(const AnimationManagerData* am, int animationId);
		Animation* getMutableAnimation(AnimationManagerData* am, int animationId);

		const std::vector<AnimObject>& getAnimObjects(const AnimationManagerData* am);
		const std::vector<Animation>& getAnimations(const AnimationManagerData* am);

		std::vector<int32> getAssociatedAnimations(const AnimationManagerData* am, const AnimObject* obj);
		std::vector<int32> getChildren(const AnimationManagerData* am, const AnimObject* obj);

		RawMemory serialize(const AnimationManagerData* am);
		void deserialize(AnimationManagerData* am, RawMemory& memory);
		void sortAnimations(AnimationManagerData* am);

		const char* getAnimObjectName(AnimObjectTypeV1 type);
		const char* getAnimationName(AnimTypeV1 type);
	}
}

#endif 