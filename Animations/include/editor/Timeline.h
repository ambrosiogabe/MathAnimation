#ifndef MATH_ANIM_TIMELINE_H
#define MATH_ANIM_TIMELINE_H
#include "animation/Animation.h"

namespace MathAnim
{
	struct TimelinePayload
	{
		AnimObjectTypeV1 objectType;
		AnimTypeV1 animType;
		bool isAnimObject;
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
		void init();

		void update(TimelineData& data);

		void freeInstance(TimelineData& data);
		void free();

		RawMemory serialize(const TimelineData& data);
		TimelineData deserialize(RawMemory& memory);
	}
}

#endif 