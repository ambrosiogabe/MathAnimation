#include "editor/AnimObjectPanel.h"
#include "editor/ImGuiExtended.h"
#include "editor/ImGuiTimeline.h"
#include "editor/Timeline.h"
#include "animation/Animation.h"
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

			for (uint32 i = 1; i < (uint32)AnimObjectType::Length; i++)
			{
				const char* name = AnimationManagerEx::getAnimObjectName((AnimObjectType)i);
				ImGui::PushID(name);
				ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name);
				if (ImGui::BeginDragDropSource())
				{
					static TimelinePayload payloadData;
					payloadData.objectType = (AnimObjectType)i;
					payloadData.animType = AnimTypeEx::None;
					payloadData.isAnimObject = true;
					ImGui::SetDragDropPayload(ImGuiTimeline_DragDropSegmentPayloadId(), &payloadData, sizeof(payloadData), ImGuiCond_Once);
					ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name);
					ImGui::EndDragDropSource();
				}
				ImGui::PopID();
			}

			ImGui::End();


			ImGui::Begin("Animations");

			for (uint32 i = 1; i < (uint32)AnimTypeEx::Length; i++)
			{
				const char* name = AnimationManagerEx::getAnimationName((AnimTypeEx)i);
				ImGui::PushID(name);
				ImGuiExtended::IconButton(ICON_FA_BOOK_DEAD, name);
				if (ImGui::BeginDragDropSource())
				{
					static TimelinePayload payloadData;
					payloadData.animType = (AnimTypeEx)i;
					payloadData.objectType = AnimObjectType::None;
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