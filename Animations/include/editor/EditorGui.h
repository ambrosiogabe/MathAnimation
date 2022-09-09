#ifndef MATH_ANIM_EDITOR_GUI_H
#define MATH_ANIM_EDITOR_GUI_H
#include "core.h"

namespace MathAnim
{
	struct TimelineData;
	struct AnimationManagerData;

	namespace EditorGui
	{
		void init(AnimationManagerData* am);

		void update(uint32 sceneTextureId, AnimationManagerData* am);
		void onGizmo(AnimationManagerData* am);

		void free(AnimationManagerData* am);

		const TimelineData& getTimelineData();
		void setTimelineData(const TimelineData& data);
	}
}

#endif 