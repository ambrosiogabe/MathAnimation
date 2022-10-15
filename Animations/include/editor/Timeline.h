#ifndef MATH_ANIM_TIMELINE_H
#define MATH_ANIM_TIMELINE_H
#include "animation/Animation.h"

namespace MathAnim
{
	struct AnimationManagerData;

	struct TimelinePayload
	{
		AnimObjectTypeV1 objectType;
		AnimTypeV1 animType;
		bool isAnimObject;
	};

	struct AnimObjectPayload
	{
		AnimObjId animObjectId;
		int32 sceneHierarchyIndex;
	};

	struct TimelineData
	{
		uint8* audioSourceFile;
		size_t audioSourceFileLength;
		int32 firstFrame;
		int32 currentFrame;
		float zoomLevel;
	};

	namespace Timeline
	{
		TimelineData initInstance();
		void init(AnimationManagerData* am);

		void update(TimelineData& data, AnimationManagerData* am);

		void freeInstance(TimelineData& data);
		void free(AnimationManagerData* am);

		void setActiveAnimObject(AnimObjId animObjectId);
		AnimObjId getActiveAnimObject();
		AnimId getActiveAnimation();

		const char* getAnimObjectPayloadId();

		RawMemory serialize(const TimelineData& data);
		TimelineData deserialize(RawMemory& memory);
	}
}

#endif 