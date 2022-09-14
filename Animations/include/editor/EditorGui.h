#ifndef MATH_ANIM_EDITOR_GUI_H
#define MATH_ANIM_EDITOR_GUI_H
#include "core.h"

namespace MathAnim
{
	struct TimelineData;
	struct AnimationManagerData;
	struct Framebuffer;

	namespace EditorGui
	{
		void init(AnimationManagerData* am);

		void update(const Framebuffer& mainFramebuffer, const Framebuffer& editorFramebuffer, AnimationManagerData* am);
		void onGizmo(AnimationManagerData* am);
		Vec2 mouseToNormalizedViewport();
		Vec2 mouseToViewportCoords();

		void free(AnimationManagerData* am);

		const TimelineData& getTimelineData();
		void setTimelineData(const TimelineData& data);

		bool mainViewportActive();
		bool editorViewportActive();
	}
}

#endif 