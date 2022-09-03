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

	namespace AnimationManager
	{
		void init(OrthoCamera& camera);

		void addAnimObject(const AnimObject& object);
		void addAnimationTo(const Animation& animation, AnimObject& animObject);
		void addAnimationTo(const Animation& animation, int animObjectId);

		bool removeAnimObject(int animObjectId);
		bool removeAnimation(int animObjectId, int animationId);

		bool setAnimObjectTime(int animObjectId, int frameStart, int duration);
		bool setAnimationTime(int animObjectId, int animationId, int frameStart, int duration);
		void setAnimObjectTrack(int animObjectId, int track);

		Framebuffer prepareFramebuffer(int outputWidth, int outputHeight);
		void render(NVGcontext* vg, int frame, Framebuffer& framebuffer);

		int lastAnimatedFrame();

		const AnimObject* getObject(int animObjectId);
		AnimObject* getMutableObject(int animObjectId);
		Animation* getMutableAnimation(int animationId);
		const std::vector<AnimObject>& getAnimObjects();

		const AnimObject* getNextAnimObject(int animObjectId);

		RawMemory serialize();
		void deserialize(RawMemory& memory);
		void sortAnimObjects();

		const char* getAnimObjectName(AnimObjectTypeV1 type);
		const char* getAnimationName(AnimTypeV1 type);
	}
}

#endif 