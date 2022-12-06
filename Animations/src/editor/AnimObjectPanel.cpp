#include "editor/AnimObjectPanel.h"
#include "editor/ImGuiExtended.h"
#include "editor/ImGuiTimeline.h"
#include "editor/Timeline.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "utils/FontAwesome.h"

#include <imgui.h>

namespace MathAnim
{
	namespace AnimObjectPanel
	{
		constexpr float buttonHeight = 90.0f;
		constexpr float animPreviewIconWidth = 115.0f;

		void init()
		{

		}

		void update()
		{
			ImGui::Begin("Animations");

			ImVec2 availableRegion = ImGui::GetContentRegionAvail();
			for (uint32 i = 1; i < (uint32)AnimTypeV1::Length; i++)
			{
				const char* name = Animation::getAnimationName((AnimTypeV1)i);
				ImGui::PushID(name);

				ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name, ImVec2(availableRegion.x - animPreviewIconWidth, 0.0f));

				if (ImGui::BeginDragDropSource())
				{
					static TimelinePayload payloadData;
					payloadData.objectType = AnimObjectTypeV1::None;
					payloadData.animType = (AnimTypeV1)i;
					payloadData.isAnimObject = false;
					ImGui::SetDragDropPayload(ImGuiTimeline_DragDropSegmentPayloadId(), &payloadData, sizeof(payloadData), ImGuiCond_Once);
					ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name, ImVec2(availableRegion.x - animPreviewIconWidth, 0.0f));
					ImGui::EndDragDropSource();
				}
				ImGui::PopID();

				// Get button size
				ImVec2 buttonSize = ImGui::GetItemRectSize();
				ImGui::SameLine();
				ImGuiExtended::Icon(ICON_FA_QUESTION_CIRCLE, false, buttonSize.y);
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text("TODO: Implement a preview window to show what this does.");
					ImGui::EndTooltip();
				}
			}

			ImGui::End();
		}

		void free()
		{

		}
	}
}