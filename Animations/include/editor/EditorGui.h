#ifndef MATH_ANIM_EDITOR_GUI_H
#define MATH_ANIM_EDITOR_GUI_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	struct TimelineData;
	struct AnimationManagerData;
	struct Framebuffer;

	namespace EditorGui
	{
		void init(AnimationManagerData* am);

		void update(const Framebuffer& mainFramebuffer, const Framebuffer& editorFramebuffer, AnimationManagerData* am, NVGcontext* vg);
		void onGizmo(AnimationManagerData* am, NVGcontext* vg);
		Vec2 mouseToNormalizedViewport();
		Vec2 mouseToViewportCoords();

		void free(AnimationManagerData* am);

		const TimelineData& getTimelineData();
		void setTimelineData(const TimelineData& data);

		bool mainViewportActive();
		bool editorViewportActive();
		bool mouseHoveredEditorViewport();
	}
}

#endif 