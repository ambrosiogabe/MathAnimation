#include "editor/ImGuiExtended.h"
#include "core.h"

#include "imgui.h"

namespace MathAnim
{
	namespace ImGuiExtended
	{
		bool IconButton(const char* icon, const char* string, const ImVec2& inSize)
		{
			ImVec2 size = inSize;

			ImVec2 stringSize = ImGui::CalcTextSize(string);
			ImVec2 iconSize = ImGui::CalcTextSize(icon);

			// TODO: Super hack FIXME. For now just multiply height by 3 for icons since
			// ImGui does not use the actual font height
			iconSize.y *= 3.0f;
			if (size.x == 0.0f && size.y == 0.0f)
			{
				size.x = glm::max(stringSize.x, iconSize.x) + ImGui::GetStyle().FramePadding.x * 2.0f;
				size.y = iconSize.y + stringSize.y + ImGui::GetStyle().FramePadding.y * 4.0f;
			}

			ImVec2 buttonStart = ImGui::GetCursorScreenPos();

			ImGui::PushID(string);
			bool result = ImGui::Button("", size);
			ImGui::PopID();

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 iconPos = ImVec2(
				buttonStart.x + (size.x / 2.0f) - (iconSize.x / 2.0f),
				buttonStart.y + ImGui::GetStyle().FramePadding.y + (iconSize.y - stringSize.y)
			);
			ImColor fontColor = ImGui::GetStyle().Colors[ImGuiCol_Text];
			drawList->AddText(iconPos, fontColor, icon);
			ImVec2 textPosition = ImVec2(
				buttonStart.x + (size.x / 2.0f) - (stringSize.x / 2.0f),
				buttonStart.y + iconSize.y + (ImGui::GetStyle().FramePadding.y * 2.0f)
			);
			drawList->AddText(textPosition, fontColor, string);

			return result;
		}
	}
}