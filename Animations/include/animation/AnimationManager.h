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
		void addAnimationTo(const Animation& animation, AnimObject& animObject);
		void addAnimationTo(AnimationManagerData* am, const Animation& animation, int animObjectId);

		bool removeAnimObject(AnimationManagerData* am, int animObjectId);
		bool removeAnimation(AnimationManagerData* am, int animObjectId, int animationId);

		bool setAnimObjectTime(AnimationManagerData* am, int animObjectId, int frameStart, int duration);
		bool setAnimationTime(AnimationManagerData* am, int animObjectId, int animationId, int frameStart, int duration);
		void setAnimObjectTrack(AnimationManagerData* am, int animObjectId, int track);

		Framebuffer prepareFramebuffer(int outputWidth, int outputHeight);
		void render(AnimationManagerData* am, NVGcontext* vg, int frame, Framebuffer& framebuffer);

		int lastAnimatedFrame(const AnimationManagerData* am);

		bool isObjectNull(int animObjectId);
		const AnimObject* getObject(const AnimationManagerData* am, int animObjectId);
		AnimObject* getMutableObject(AnimationManagerData* am, int animObjectId);
		Animation* getMutableAnimation(AnimationManagerData* am, int animationId);
		const std::vector<AnimObject>& getAnimObjects(const AnimationManagerData* am);

		const AnimObject* getNextAnimObject(const AnimationManagerData* am, int animObjectId);

		RawMemory serialize(const AnimationManagerData* am);
		void deserialize(AnimationManagerData* am, RawMemory& memory);
		void sortAnimObjects(AnimationManagerData* am);

		const char* getAnimObjectName(AnimObjectTypeV1 type);
		const char* getAnimationName(AnimTypeV1 type);
	}
}

#endif 