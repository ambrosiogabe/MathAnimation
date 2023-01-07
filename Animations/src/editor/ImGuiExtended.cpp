#include "editor/Timeline.h"
#include "editor/ImGuiExtended.h"
#include "core/ImGuiLayer.h"
#include "core/Colors.h"
#include "animation/AnimationManager.h"

namespace MathAnim
{
	struct RenamableState
	{
		bool isBeingRenamed;
		Vec2 groupSize;
	};

	struct ToggleState
	{
		bool isToggled;
	};

	namespace ImGuiExtended
	{
		// -------- Internal Vars --------
		static std::unordered_map<std::string, RenamableState> renamableStates;
		static std::unordered_map<std::string, ToggleState> toggleStates;
		static constexpr bool drawDebugBoxes = false;
		static const char* filePayloadId = "DRAG_DROP_FILE_PAYLOAD";

		bool ToggleButton(const char* string, bool* enabled, const ImVec2& size)
		{
			auto toggleIter = toggleStates.find(string);
			ToggleState* state = nullptr;
			if (toggleIter == toggleStates.end())
			{
				toggleStates[string] = { false };
				state = &toggleStates[string];
			}
			else
			{
				state = &toggleIter->second;
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
			ImGui::PushStyleColor(
				ImGuiCol_Button,
				state->isToggled
				? Colors::Neutral[6]
				: Colors::Neutral[8]
			);
			ImGui::PushStyleColor(ImGuiCol_Border, Colors::Neutral[5]);
			bool clicked = ImGui::Button(string, size);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Apply property to all children");
			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar(2);

			if (clicked)
			{
				state->isToggled = !state->isToggled;
			}
			*enabled = state->isToggled;

			return clicked;
		}

		bool OutlineButton(const char* string, const ImVec2& size)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, Colors::Neutral[8]);
			ImGui::PushStyleColor(ImGuiCol_Border, Colors::Neutral[5]);
			bool res = ImGui::Button(string, size);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Apply property to all children");
			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar(2);
			return res;
		}

		bool IconButton(const char* icon, const char* string, const ImVec2& inSize)
		{
			ImVec2 size = inSize;

			ImVec2 stringSize = ImGui::CalcTextSize(string);
			ImVec2 iconSize = ImGui::CalcTextSize(icon);

			// TODO: Super hack FIXME. For now just multiply height by 3 for icons since
			// ImGui does not use the actual font height
			iconSize.y *= 2.0f;
			if (size.x == 0.0f)
			{
				size.x = ImGui::GetContentRegionAvail().x;
			}
			if (size.y == 0.0f)
			{
				size.y = glm::max(iconSize.y, stringSize.y) + ImGui::GetStyle().FramePadding.y * 4.0f;
			}

			ImVec2 buttonStart = ImGui::GetCursorScreenPos();

			ImGui::PushID(string);
			bool result = ImGui::Button("", size);
			ImGui::PopID();

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 iconPos = ImVec2(
				buttonStart.x + ImGui::GetStyle().FramePadding.x,
				buttonStart.y + ImGui::GetStyle().FramePadding.y + (iconSize.y - stringSize.y)
			);
			ImColor fontColor = ImGui::GetStyle().Colors[ImGuiCol_Text];
			drawList->AddText(iconPos, fontColor, icon);
			ImVec2 textPosition = ImVec2(
				buttonStart.x + (size.x / 2.0f) - (stringSize.x / 2.0f),
				buttonStart.y + (size.y / 2.0f) - (stringSize.y / 2.0f)
			);
			drawList->AddText(textPosition, fontColor, string);

			return result;
		}

		bool VerticalIconButton(const char* icon, const char* buttonText, float width)
		{
			std::string iconName = std::string(buttonText);
			std::string mapName = "Button_" + iconName;
			auto renamableIter = renamableStates.find(mapName);
			RenamableState* state = nullptr;
			if (renamableIter == renamableStates.end())
			{
				renamableStates[mapName] = { false, Vec2{0, 0} };
				state = &renamableStates[mapName];
			}
			else
			{
				state = &renamableIter->second;
			}

			ImGui::PushID(mapName.c_str());
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			bool buttonClicked = ImGui::Button(
				"##Selectable",
				ImVec2(state->groupSize.x, state->groupSize.y)
			);
			ImGui::PopStyleColor();
			ImGui::PopID();

			ImGui::SetCursorScreenPos(cursor);

			ImGui::BeginGroup();

			ImVec2 size = ImVec2(width, 0.0f);

			float wrapWidth = width - ImGui::GetStyle().FramePadding.x * 2.0f;
			ImVec2 stringSize = ImGui::CalcTextSize(buttonText, 0, false, wrapWidth);
			stringSize.x = wrapWidth;
			ImVec2 iconSize = ImGui::CalcTextSize(icon);

			// TODO: Super hack FIXME. For now just multiply height by 2 for icons since
			// ImGui does not use the actual font height
			iconSize.y *= 2.0f;
			size.y = iconSize.y + stringSize.y + ImGui::GetStyle().FramePadding.y * 4.0f;

			float iconCenteredX = cursor.x + (width / 2.0f) - (iconSize.x / 2.0f);
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 iconPos = ImVec2(
				iconCenteredX,
				cursor.y + ImGui::GetStyle().FramePadding.y
			);
			ImColor fontColor = ImGui::GetStyle().Colors[ImGuiCol_Text];
			drawList->AddText(iconPos + ImVec2(0.0f, (iconSize.y - ImGui::GetFontSize())), fontColor, icon);
			if (drawDebugBoxes)
			{
				drawList->AddRect(iconPos, iconPos + iconSize, IM_COL32(128, 4, 4, 255));
			}

			float textCenteredX = cursor.x + (width / 2.0f) - (stringSize.x / 2.0f);
			float iconEndY = iconPos.y + iconSize.y + ImGui::GetStyle().FramePadding.y;
			ImVec2 textPosition = ImVec2(
				textCenteredX,
				iconEndY + ImGui::GetStyle().FramePadding.y
			);
			ImGui::SetCursorScreenPos(textPosition);
			CenteredWrappedText(textPosition, fontColor, buttonText, wrapWidth);
			if (drawDebugBoxes)
			{
				drawList->AddRect(textPosition, textPosition + stringSize, IM_COL32(4, 128, 4, 255));
			}

			ImGui::SetCursorScreenPos(cursor + ImVec2(size));
			ImGui::EndGroup();

			state->groupSize = Vec2{ size.x, size.y };

			return buttonClicked;
		}

		const AnimObjectPayload* AnimObjectDragDropTarget()
		{
			const AnimObjectPayload* res = nullptr;

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Timeline::getAnimObjectPayloadId()))
				{
					if (payload->DataSize == sizeof(AnimObjectPayload))
					{
						res = (const AnimObjectPayload*)payload->Data;
					}
				}

				ImGui::EndDragDropTarget();
			}

			return res;
		}

		bool AnimObjDragDropInputBox(const char* label, AnimationManagerData* am, AnimObjId* output, AnimId animation)
		{
			const AnimObject* srcObj = AnimationManager::getObject(am, *output);

			ImGui::BeginDisabled();
			if (srcObj == nullptr)
			{
				const char dummyInputText[] = "Drag Object Here";
				size_t dummyInputTextSize = sizeof(dummyInputText);
				ImGui::InputText(label, (char*)dummyInputText, dummyInputTextSize, ImGuiInputTextFlags_ReadOnly);
			}
			else
			{
				ImGui::InputText(label, (char*)srcObj->name, srcObj->nameLength, ImGuiInputTextFlags_ReadOnly);
			}
			ImGui::EndDisabled();

			if (auto objPayload = ImGuiExtended::AnimObjectDragDropTarget(); objPayload != nullptr)
			{
				if (!isNull(animation))
				{
					// If the animation reference is not null, then update
					// remove the reference from the old object and add it to
					// the new one
					AnimObject* objRef = AnimationManager::getMutableObject(am, *output);
					if (objRef)
					{
						objRef->referencedAnimations.erase(animation);
					}

					AnimObject* newObjRef = AnimationManager::getMutableObject(am, objPayload->animObjectId);
					if (newObjRef)
					{
						newObjRef->referencedAnimations.insert(animation);
					}
				}
				*output = objPayload->animObjectId;
				return true;
			}

			return false;
		}

		const FilePayload* FileDragDropTarget()
		{
			const FilePayload* res = nullptr;

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(filePayloadId))
				{
					if (payload->DataSize == sizeof(FilePayload))
					{
						res = (const FilePayload*)payload->Data;
					}
				}

				ImGui::EndDragDropTarget();
			}

			return res;
		}

		bool FileDragDropInputBox(const char* label, AnimationManagerData* am, char* outBuffer, size_t outBufferSize)
		{
			ImGui::BeginDisabled();
			ImGui::InputText(label, outBuffer, outBufferSize, ImGuiInputTextFlags_ReadOnly);
			ImGui::EndDisabled();

			if (auto objPayload = ImGuiExtended::FileDragDropTarget(); objPayload != nullptr)
			{
				if (outBufferSize >= (objPayload->filepathLength + 1))
				{
					g_memory_copyMem((void*)outBuffer, (void*)objPayload->filepath, sizeof(char) * objPayload->filepathLength);
					outBuffer[objPayload->filepathLength] = '\0';
				}
				else
				{
					g_logger_error("File drag drop target got filepath of length '%d' that was too long to fit into buffer of length '%d'.", (uint32)objPayload->filepathLength, (uint32)outBufferSize);
					return false;
				}

				return true;
			}

			return false;
		}

		const char* getFileDragDropPayloadId()
		{
			return filePayloadId;
		}

		void Icon(const char* icon, bool solid, float lineHeight)
		{
			if (lineHeight == 0.0f)
			{
				lineHeight = ImGui::GetFont()->FontSize;
			}

			if (solid)
			{
				float fontHeightDifference = ImGui::GetFont()->FontSize;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (lineHeight - fontHeightDifference) / 2.0f);
				ImGui::Text("%s", icon);
			}
			else
			{
				ImGui::PushFont(ImGuiLayer::getMediumRegularIconFont());
				float fontSize = ImGuiLayer::getMediumRegularIconFont()->FontSize;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (lineHeight - fontSize) / 2.0f - ImGui::GetStyle().FramePadding.y);
				ImGui::Text("%s", icon);
				ImGui::PopFont();
			}
		}

		void MediumIcon(const char* icon, const Vec4& color, bool solid)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			if (solid)
			{
				ImGui::PushFont(ImGuiLayer::getMediumSolidIconFont());
			}
			else
			{
				ImGui::PushFont(ImGuiLayer::getMediumRegularIconFont());
			}

			ImGui::Text("%s", icon);

			ImGui::PopStyleColor();
			ImGui::PopFont();
		}

		void LargeIcon(const char* icon, const Vec4& color, bool solid)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			if (solid)
			{
				ImGui::PushFont(ImGuiLayer::getLargeSolidIconFont());
			}
			else
			{
				ImGui::PushFont(ImGuiLayer::getLargeRegularIconFont());
			}

			ImGui::Text("%s", icon);

			ImGui::PopStyleColor();
			ImGui::PopFont();
		}

		bool RenamableIconSelectable(const char* icon, char* stringBuffer, size_t stringBufferSize, bool isSelected, float width)
		{
			std::string iconName = std::string(stringBuffer);
			auto renamableIter = renamableStates.find(iconName);
			RenamableState* state = nullptr;
			if (renamableIter == renamableStates.end())
			{
				renamableStates[iconName] = { false, Vec2{0, 0} };
				state = &renamableStates[iconName];
			}
			else
			{
				state = &renamableIter->second;
			}

			ImGui::PushID(iconName.c_str());
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			bool isAlreadySelected = isSelected;
			bool selectionChanged = ImGui::Selectable(
				"##Selectable",
				&isSelected,
				0,
				ImVec2(state->groupSize.x, state->groupSize.y)
			);
			bool selectableIsClicked = ImGui::IsItemClicked();
			ImGuiItemStatusFlags itemStatusFlags = ImGui::GetItemStatusFlags();
			ImGuiItemFlags itemFlags = ImGui::GetItemFlags();
			ImGuiID itemId = ImGui::GetItemID();
			ImRect itemRect;
			itemRect.Min = ImGui::GetItemRectMin();
			itemRect.Max = ImGui::GetItemRectMax();

			if (isAlreadySelected)
			{
				if (selectableIsClicked && !state->isBeingRenamed)
				{
					state->isBeingRenamed = true;
				}
			}
			else
			{
				state->isBeingRenamed = false;
			}

			ImGui::PopID();
			ImGui::SetCursorScreenPos(cursor);

			ImGui::BeginGroup();

			ImVec2 size = ImVec2(width, 0.0f);

			float wrapWidth = width - ImGui::GetStyle().FramePadding.x * 2.0f;
			ImVec2 stringSize = ImGui::CalcTextSize(stringBuffer, 0, false, wrapWidth);
			stringSize.x = wrapWidth;
			ImVec2 iconSize = ImGui::CalcTextSize(icon);

			// TODO: Super hack FIXME. For now just multiply height by 2 for icons since
			// ImGui does not use the actual font height
			iconSize.y *= 2.0f;
			size.y = iconSize.y + stringSize.y + ImGui::GetStyle().FramePadding.y * 4.0f;

			float iconCenteredX = cursor.x + (width / 2.0f) - (iconSize.x / 2.0f);
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 iconPos = ImVec2(
				iconCenteredX,
				cursor.y + ImGui::GetStyle().FramePadding.y
			);
			ImColor fontColor = ImGui::GetStyle().Colors[ImGuiCol_Text];
			drawList->AddText(iconPos + ImVec2(0.0f, (iconSize.y - ImGui::GetFontSize())), fontColor, icon);
			if (drawDebugBoxes)
			{
				drawList->AddRect(iconPos, iconPos + iconSize, IM_COL32(128, 4, 4, 255));
			}

			float textCenteredX = cursor.x + (width / 2.0f) - (stringSize.x / 2.0f);
			float iconEndY = iconPos.y + iconSize.y + ImGui::GetStyle().FramePadding.y;
			ImVec2 textPosition = ImVec2(
				textCenteredX,
				iconEndY + ImGui::GetStyle().FramePadding.y
			);
			ImGui::SetCursorScreenPos(textPosition);
			if (!state->isBeingRenamed)
			{
				CenteredWrappedText(textPosition, fontColor, stringBuffer, wrapWidth);
				if (drawDebugBoxes)
				{
					drawList->AddRect(textPosition, textPosition + stringSize, IM_COL32(4, 128, 4, 255));
				}
			}
			else
			{
				// Render input text if the icon is being renamed
				ImGui::SetNextItemWidth(wrapWidth);
				ImGui::SetKeyboardFocusHere();
				if (ImGui::InputText(
					"##InputText",
					stringBuffer,
					stringBufferSize,
					ImGuiInputTextFlags_EnterReturnsTrue
					| ImGuiInputTextFlags_AutoSelectAll
				))
				{
					state->isBeingRenamed = false;
					selectionChanged = true;
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !selectableIsClicked)
				{
					state->isBeingRenamed = false;
				}
			}

			ImGui::SetCursorScreenPos(cursor + ImVec2(size));
			ImGui::EndGroup();

			state->groupSize = Vec2{ size.x, size.y };

			ImGui::SetLastItemData(itemId, itemFlags, itemStatusFlags, itemRect);

			return selectionChanged;
		}

		void CenteredWrappedText(ImVec2 pos, ImColor color, const char* text, float wrap_width)
		{
			ImFont* font = ImGui::GetFont();
			const char* s = text;
			const char* text_end = text + strlen(text);
			const char* word_wrap_eol = NULL;
			const float scale = ImGui::GetFontSize() / font->FontSize;
			const float line_height = ImGui::GetFontSize();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const float start_x = pos.x;
			float x = pos.x;
			float y = pos.y;

			while (s < text_end)
			{
				// Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
				if (!word_wrap_eol)
				{
					word_wrap_eol = font->CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - start_x));
					if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
						word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
				}

				if (s >= word_wrap_eol)
				{
					x = start_x;
					y += line_height;
					word_wrap_eol = NULL;

					// Wrapping skips upcoming blanks
					while (s < text_end)
					{
						const char c = *s;
						if (ImCharIsBlankA(c)) { s++; }
						else if (c == '\n') { s++; break; }
						else { break; }
					}
					continue;
				}

				// Render text centered
				ImVec2 textSize = font->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0, s, word_wrap_eol, NULL);
				x = start_x + (wrap_width / 2.0f) - (textSize.x / 2.0f);
				drawList->AddText(ImVec2(x, y), color, s, word_wrap_eol);
				s = word_wrap_eol;
				if (drawDebugBoxes)
				{
					drawList->AddRect(ImVec2(x, y), ImVec2(x, y) + textSize, IM_COL32(204, 220, 12, 255));
				}
			}

			// End CenteredWrapText
		}
	}
}