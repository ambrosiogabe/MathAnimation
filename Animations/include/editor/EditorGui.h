#ifndef MATH_ANIM_EDITOR_GUI_H
#define MATH_ANIM_EDITOR_GUI_H
#include "core.h"

namespace MathAnim
{
	struct TimelineData;

	namespace EditorGui
	{
		void init();

		void update(uint32 sceneTextureId);

		void free();

		const TimelineData& getTimelineData();
		void setTimelineData(const TimelineData& data);
	}
}

#endif 