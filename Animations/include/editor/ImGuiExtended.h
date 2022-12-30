#ifndef MATH_ANIM_IMGUI_EXTENDED_H
#define MATH_ANIM_IMGUI_EXTENDED_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#undef IMGUI_DEFINE_MATH_OPERATORS

#include "core.h"

namespace MathAnim
{
	struct AnimObjectPayload;
	struct AnimationManagerData;
	
	struct FilePayload
	{
		const char* filepath;
		size_t filepathLength;
	};

	namespace ImGuiExtended
	{
		bool ToggleButton(const char* string, bool* enabled, const ImVec2& size = ImVec2(0, 0));
		bool OutlineButton(const char* string, const ImVec2& size = ImVec2(0, 0));
		bool IconButton(const char* icon, const char* string, const ImVec2& size = ImVec2(0, 0));
		bool VerticalIconButton(const char* icon, const char* buttonText, float width);

		const AnimObjectPayload* AnimObjectDragDropTarget();
		bool AnimObjDragDropInputBox(const char* label, AnimationManagerData* am, AnimObjId* output, AnimId animation = NULL_ANIM);

		const FilePayload* FileDragDropTarget();
		bool FileDragDropInputBox(const char* label, AnimationManagerData* am, char* outBuffer, size_t outBufferSize);
		const char* getFileDragDropPayloadId();

		void Icon(const char* icon, bool solid = true, float lineHeight = 0.0f);
		void MediumIcon(const char* icon, const Vec4& color = Vec4{1.0f, 1.0f, 1.0f, 1.0f}, bool solid = true);
		void LargeIcon(const char* icon, const Vec4& color = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, bool solid = true);

		bool RenamableIconSelectable(const char* icon, char* stringBuffer, size_t stringBufferSize, bool isSelected, float width);

		void CenteredWrappedText(ImVec2 textPosition, ImColor color, const char* text, float maxWidth);
	}
}

#endif 