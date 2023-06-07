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
		void init(AnimationManagerData* am, const std::filesystem::path& projectRoot, uint32 outputWidth, uint32 outputHeight);

		void update(const Framebuffer& mainFramebuffer, const Framebuffer& editorFramebuffer, AnimationManagerData* am, float deltaTime);
		void onGizmo(AnimationManagerData* am);
		Vec2 mouseToNormalizedViewport();
		Vec2 mouseToViewportCoords();

		void free(AnimationManagerData* am);

		const TimelineData& getTimelineData();
		void setTimelineData(const TimelineData& data);

		void displayActionText(const std::string& actionText);

		bool mainViewportActive();
		bool editorViewportActive();
		bool mouseHoveredEditorViewport();
		bool anyEditorItemActive();
	}
}

#endif 