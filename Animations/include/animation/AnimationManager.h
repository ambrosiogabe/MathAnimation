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
	
	struct SceneSnapshot
	{
		// Frame of the animation that this correlates too
		uint32 absoluteFrame;
		// Current state of the scene is stored in here
		std::vector<AnimObjectState> objectStates;
		std::vector<AnimObject> objects;
		// Maps from AnimObjectId -> Index in objects vector
		std::unordered_map<AnimObjId, size_t> objectIdMap;

		Vec2 camera2DPos;
	};

	namespace AnimationManager
	{
		AnimationManagerData* create(OrthoCamera& camera);
		void free(AnimationManagerData* animManager);
		void endFrame(AnimationManagerData* am);
		SceneSnapshot buildSnapshot(AnimationManagerData* am, NVGcontext* vg, uint32 absoluteFrame);

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
		const AnimObject* getObjectFromSnapshot(const SceneSnapshot& snapshot, AnimObjId animObj);
		AnimObject* getMutableObject(AnimationManagerData* am, AnimObjId animObj);
		AnimObject* getMutableObjectFromSnapshot(SceneSnapshot& snapshot, AnimObjId animObj);
		const Animation* getAnimation(const AnimationManagerData* am, AnimId anim);
		Animation* getMutableAnimation(AnimationManagerData* am, AnimId anim);

		const std::vector<AnimObject>& getAnimObjects(const AnimationManagerData* am);
		const std::vector<Animation>& getAnimations(const AnimationManagerData* am);

		std::vector<AnimId> getAssociatedAnimations(const AnimationManagerData* am, AnimObjId obj);
		std::vector<AnimObjId> getChildren(const AnimationManagerData* am, AnimObjId obj);
		std::vector<AnimObjId> getChildrenFromSnapshot(const SceneSnapshot& snapshot, AnimObjId obj);

		const SceneSnapshot& getCurrentSnapshot(const AnimationManagerData* am);
		SceneSnapshot& getMutableCurrentSnapshot(AnimationManagerData* am);

		RawMemory serialize(const AnimationManagerData* am);
		void deserialize(AnimationManagerData* am, NVGcontext* vg, RawMemory& memory, int currentFrame);
		void sortAnimations(AnimationManagerData* am);

		const char* getAnimObjectName(AnimObjectTypeV1 type);
		const char* getAnimationName(AnimTypeV1 type);
	}
}

#endif 