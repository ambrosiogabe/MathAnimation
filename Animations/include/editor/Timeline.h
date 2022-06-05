#ifndef MATH_ANIM_TIMELINE_H
#define MATH_ANIM_TIMELINE_H
#include "animation/Animation.h"

namespace MathAnim
{
	struct TimelinePayload
	{
		AnimObjectTypeV1 objectType;
		AnimTypeExV1 animType;
		bool isAnimObject;
	};

	namespace Timeline
	{
		void init();

		void update();

		void free();
	}
}

#endif 