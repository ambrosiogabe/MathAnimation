#ifndef MATH_ANIM_ANIMATION_MANAGER_H
#define MATH_ANIM_ANIMATION_MANAGER_H
#include "core.h"
#include "animation/Animation.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct AnimObject;
	struct Animation;
	struct Framebuffer;
	struct Camera;

	struct AnimationManagerData;

	namespace AnimationManager
	{
		AnimationManagerData* create();
		void free(AnimationManagerData* animManager);
		void endFrame(AnimationManagerData* am);
		void resetToFrame(AnimationManagerData* am, uint32 absoluteFrame);
		void calculateAnimationKeyFrames(AnimationManagerData* am);

		/**
		 * @brief Adds this animation object to the animation manager. 
		 *        IMPORTANT: This function takes ownership of this object, so any references
		 *        to the object after adding it using this function are undefined behavior.
		 * 
		 * @param am Animation Manager
		 * @param object The object to add
		*/
		void addAnimObject(AnimationManagerData* am, const AnimObject& object);
		void addAnimation(AnimationManagerData* am, const Animation& animation);

		void removeAnimObject(AnimationManagerData* am, AnimObjId animObj);
		void removeAnimation(AnimationManagerData* am, AnimId anim);

		void addObjectToAnim(AnimationManagerData* am, AnimObjId animObj, AnimId animation);
		void removeObjectFromAnim(AnimationManagerData* am, AnimObjId animObj, AnimId animation);

		bool setAnimationTime(AnimationManagerData* am, AnimId anim, int frameStart, int duration);
		void setAnimationTrack(AnimationManagerData* am, AnimId anim, int track);

		void render(AnimationManagerData* am, int deltaFrame);

		int lastAnimatedFrame(const AnimationManagerData* am);
		bool isPastLastFrame(const AnimationManagerData* am);

		const Camera& getActiveCamera2D(const AnimationManagerData* am);
		const Camera& getActiveCamera3D(const AnimationManagerData* am);
		bool hasActive2DCamera(const AnimationManagerData* am);
		bool hasActive3DCamera(const AnimationManagerData* am);
		void setActiveCamera2D(AnimationManagerData* am, AnimObjId cameraObj);
		void setActiveCamera3D(AnimationManagerData* am, AnimObjId cameraObj);
		void calculateCameraMatrices(AnimationManagerData* am);

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
		AnimObjId getNextSibling(const AnimationManagerData* am, AnimObjId obj);

		void serialize(const AnimationManagerData* am, nlohmann::json& j);
		void deserialize(AnimationManagerData* am, const nlohmann::json& j, int currentFrame, uint32 versionMajor, uint32 versionMinor);
		void sortAnimations(AnimationManagerData* am);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		void legacy_deserialize(AnimationManagerData* am, RawMemory& memory, int currentFrame);

		void retargetSvgScales(AnimationManagerData* am);

		void applyGlobalTransforms(AnimationManagerData* am);
		void applyGlobalTransformsTo(AnimationManagerData* am, AnimObjId obj);
		void calculateBBoxes(AnimationManagerData* am);
		void calculateBBoxFor(AnimationManagerData* am, AnimObjId obj);
		void updateObjectState(AnimationManagerData* am, AnimObjId animObj);
	}
}

#endif 