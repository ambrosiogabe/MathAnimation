#ifndef MATH_ANIM_IMGUI_EXTENDED_H
#define MATH_ANIM_IMGUI_EXTENDED_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#undef IMGUI_DEFINE_MATH_OPERATORS

namespace MathAnim
{
	namespace ImGuiExtended
	{
		bool ToggleButton(const char* string, bool* enabled, const ImVec2& size = ImVec2(0, 0));
		bool OutlineButton(const char* string, const ImVec2& size = ImVec2(0, 0));
		bool IconButton(const char* icon, const char* string, const ImVec2& size = ImVec2(0, 0));
		bool VerticalIconButton(const char* icon, const char* buttonText, float width);

		void Icon(const char* icon, bool solid = true, float lineHeight = 0.0f);

		bool RenamableIconSelectable(const char* icon, char* stringBuffer, size_t stringBufferSize, bool isSelected, float width);

		void CenteredWrappedText(ImVec2 textPosition, ImColor color, const char* text, float maxWidth);
	}
}

#endif 