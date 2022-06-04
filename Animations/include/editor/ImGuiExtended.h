#ifndef MATH_ANIM_IMGUI_EXTENDED_H
#define MATH_ANIM_IMGUI_EXTENDED_H

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#undef IMGUI_DEFINE_MATH_OPERATORS

namespace MathAnim
{
	namespace ImGuiExtended
	{
		bool IconButton(const char* icon, const char* string, const ImVec2& size = ImVec2(0, 0));
	}
}

#endif 