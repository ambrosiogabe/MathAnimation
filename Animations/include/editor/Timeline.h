#ifndef MATH_ANIM_TIMELINE_H
#define MATH_ANIM_TIMELINE_H
#include "animation/Animation.h"

namespace MathAnim
{
	struct TimelinePayload
	{
		AnimObjectType objectType;
		AnimTypeEx animType;
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