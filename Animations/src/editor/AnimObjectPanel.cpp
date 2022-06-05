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
		constexpr ImVec2 buttonSize = ImVec2(120, 120);

		void init()
		{

		}

		void update()
		{
			ImGui::Begin("Animation Objects");

			for (uint32 i = 1; i < (uint32)AnimObjectTypeV1::Length; i++)
			{
				const char* name = AnimationManager::getAnimObjectName((AnimObjectTypeV1)i);
				ImGui::PushID(name);
				ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name);
				if (ImGui::BeginDragDropSource())
				{
					static TimelinePayload payloadData;
					payloadData.objectType = (AnimObjectTypeV1)i;
					payloadData.animType = AnimTypeV1::None;
					payloadData.isAnimObject = true;
					ImGui::SetDragDropPayload(ImGuiTimeline_DragDropSegmentPayloadId(), &payloadData, sizeof(payloadData), ImGuiCond_Once);
					ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name);
					ImGui::EndDragDropSource();
				}
				ImGui::PopID();
			}

			ImGui::End();


			ImGui::Begin("Animations");

			for (uint32 i = 1; i < (uint32)AnimTypeV1::Length; i++)
			{
				const char* name = AnimationManager::getAnimationName((AnimTypeV1)i);
				ImGui::PushID(name);
				ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name);
				if (ImGui::BeginDragDropSource())
				{
					static TimelinePayload payloadData;
					payloadData.animType = (AnimTypeV1)i;
					payloadData.objectType = AnimObjectTypeV1::None;
					payloadData.isAnimObject = false;
					ImGui::SetDragDropPayload(ImGuiTimeline_DragDropSubSegmentPayloadId(), &payloadData, sizeof(payloadData), ImGuiCond_Once);
					ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name);
					ImGui::EndDragDropSource();
				}
				ImGui::PopID();
			}

			ImGui::End();
		}

		void free()
		{

		}
	}
}