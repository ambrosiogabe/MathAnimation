#ifndef MATH_ANIM_TIMELINE_H
#define MATH_ANIM_TIMELINE_H
#include "animation/Animation.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct AnimationManagerData;

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
		void init(AnimationManagerData* am);

		void update(TimelineData& data, AnimationManagerData* am);

		void freeInstance(TimelineData& data);
		void free(AnimationManagerData* am);

		void serialize(const TimelineData& data, nlohmann::json& j);
		TimelineData deserialize(const nlohmann::json& j);

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		TimelineData legacy_deserialize(RawMemory& memory);
	}
}

#endif 