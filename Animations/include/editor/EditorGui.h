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
		Vec2 toNormalizedViewportCoords(const Vec2& screenCoords);
		Vec2 mouseToNormalizedViewport();
		Vec2 mouseToViewportCoords();
		Vec2 toViewportCoords(const Vec2& screenCoords);

		void free(AnimationManagerData* am);

		const TimelineData& getTimelineData();
		void setTimelineData(const TimelineData& data);

		void displayActionText(const std::string& actionText);

		bool mainViewportActive();
		bool editorViewportActive();
		bool mouseHoveredEditorViewport();
		bool anyEditorItemActive();

		BBox getViewportBounds();

		void copyObjectToClipboard(const AnimationManagerData* am, AnimObjId obj);

		/**
		 * @return Returns the newly copied object id
		*/
		AnimObjId duplicateObject(AnimationManagerData* am, AnimObjId obj);

		/**
		 * @return Returns the newly copied object id 
		*/
		AnimObjId pasteObjectFromClipboardToParent(AnimationManagerData* am, AnimObjId newParent);

		/**
		 * @return Returns the newly copied object id
		*/
		AnimObjId pasteObjectFromClipboard(AnimationManagerData* am);
	}
}

#endif 