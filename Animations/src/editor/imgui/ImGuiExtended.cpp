#include "editor/timeline/Timeline.h"
#include "editor/imgui/ImGuiExtended.h"
#include "editor/imgui/ImGuiLayer.h"
#include "editor/panels/InspectorPanel.h"
#include "renderer/Colors.h"
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

	struct AdditionalEditState
	{
		bool isBeingEdited;
	};

	template<typename T>
	class ImGuiState
	{
	public:
		ImGuiState()
		{
			this->states = {};
		}

		/**
		 * @brief Finds the element indexed by key. If the element doesn't exist, it gets
		 *        initialized with `defaultValue` and returns a reference to the newly created
		 *        item.
		 *
		 * @param key
		 * @param defaultValue
		 * @return Reference to the item in the unordered_map
		*/
		T& findOrDefault(const std::string& label, const T& defaultValue)
		{
			if (auto iter = states.find(label); iter == states.end())
			{
				states[label] = defaultValue;
				return states[label];
			}
			else
			{
				return iter->second;
			}
		}

		void clear()
		{
			states.clear();
		}

	private:
		std::unordered_map<std::string, T> states;
	};

	namespace ImGuiExtended
	{
		// -------- Internal Vars --------
		static ImGuiState<AdditionalEditState> additionalEditStates;
		static ImGuiState<RenamableState> renamableStates;
		static ImGuiState<ToggleState> toggleStates;

		static constexpr bool drawDebugBoxes = false;
		static const char* filePayloadId = "DRAG_DROP_FILE_PAYLOAD";

		// Helper functions to wrap around ImGui function calls and return a tri-state instead of true/false
		template<typename Closure>
		static EditState undoableImGuiFunction(
			const std::string& label,
			Closure&& closure)
		{
			AdditionalEditState& state = additionalEditStates.findOrDefault(label, { false });

			bool editRes = closure();
			editRes = editRes || ImGui::IsItemActive();
			if (editRes)
			{
				state.isBeingEdited = true;
			}

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				state.isBeingEdited = false;
				return EditState::FinishedEditing;
			}

			return editRes
				? EditState::BeingEdited
				: EditState::NotEditing;
		}

		template<typename T>
		class ImGuiStateEx
		{
		public:
			ImGuiStateEx()
			{
				this->states = {};
			}

			template<typename Closure>
			ImGuiDataEx<T> undoableImGuiFunctionEx(const std::string& label, const T& defaultValue, Closure&& closure)
			{
				ImGuiDataEx<T>& state = states.findOrDefault(label, { defaultValue, EditState::NotEditing });

				if (state.editState == EditState::NotEditing)
				{
					state.ogData = defaultValue;
				}

				state.editState = closure();

				return {
					state.ogData,
					state.editState
				};
			}

			void clear()
			{
				states.clear();
			}

		private:
			ImGuiState<ImGuiDataEx<T>> states;
		};

		// ------------ Extra State Vars ------------
		static ImGuiStateEx<AnimObjId> animObjDragDropData;
		static ImGuiStateEx<glm::u8vec4> colorEditU84Data;
		static ImGuiStateEx<Vec4> colorEdit4Data;
		static ImGuiStateEx<int> comboData;
		static ImGuiStateEx<Vec2i> dragInt2Data;
		static ImGuiStateEx<float> dragFloatData;
		static ImGuiStateEx<Vec2> dragFloat2Data;
		static ImGuiStateEx<Vec3> dragFloat3Data;
		static ImGuiStateEx<bool> checkboxData;
		static ImGuiStateEx<std::string> inputTextData;
		static ImGuiStateEx<std::string> inputTextMultilineData;
		static ImGuiStateEx<std::string> fileDragDropData;

		bool ToggleButton(const char* string, bool* enabled, const ImVec2& size)
		{
			ToggleState& state = toggleStates.findOrDefault(string, { false });

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
			ImGui::PushStyleColor(
				ImGuiCol_Button,
				state.isToggled
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
				state.isToggled = !state.isToggled;
			}
			*enabled = state.isToggled;

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
			RenamableState& state = renamableStates.findOrDefault(mapName, { false, Vec2{0, 0} });

			ImGui::PushID(mapName.c_str());
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			bool buttonClicked = ImGui::Button(
				"##Selectable",
				ImVec2(state.groupSize.x, state.groupSize.y)
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

			state.groupSize = Vec2{ size.x, size.y };

			return buttonClicked;
		}

		const AnimObjectPayload* AnimObjectDragDropTarget()
		{
			const AnimObjectPayload* res = nullptr;

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(InspectorPanel::getAnimObjectPayloadId()))
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

		EditState AnimObjDragDropInputBox(const char* label, AnimationManagerData* am, AnimObjId* output, AnimId animation)
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
				return EditState::FinishedEditing;
			}

			return EditState::NotEditing;
		}

		ImGuiDataEx<AnimObjId> AnimObjDragDropInputBoxEx(const char* label, AnimationManagerData* am, AnimObjId* output, AnimId animation)
		{
			return animObjDragDropData.undoableImGuiFunctionEx(
				label,
				*output,
				[&]()
				{
					return AnimObjDragDropInputBox(label, am, output, animation);
				});
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

		EditState FileDragDropInputBox(const char* label, char* outBuffer, size_t outBufferSize)
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
					g_memory_copyMem((void*)outBuffer, (void*)objPayload->filepath, sizeof(char) * outBufferSize);
					outBuffer[outBufferSize - 1] = '\0';
					g_logger_warning("File drag drop target got filepath of length '{}' that was too long to fit into buffer of length '{}'. Truncating filename.", objPayload->filepathLength, outBufferSize);
				}

				return EditState::FinishedEditing;
			}

			return EditState::NotEditing;
		}

		ImGuiDataEx<std::string> FileDragDropInputBoxEx(const char* label, char* outBuffer, size_t outBufferSize)
		{
			return fileDragDropData.undoableImGuiFunctionEx(
				label,
				outBuffer,
				[&]()
				{
					return InputText(label, outBuffer, outBufferSize);
				});
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
			RenamableState& state = renamableStates.findOrDefault(iconName, { false, Vec2{0, 0} });

			ImGui::PushID(iconName.c_str());
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			bool isAlreadySelected = isSelected;
			bool selectionChanged = ImGui::Selectable(
				"##Selectable",
				&isSelected,
				0,
				ImVec2(state.groupSize.x, state.groupSize.y)
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
				if (selectableIsClicked && !state.isBeingRenamed)
				{
					state.isBeingRenamed = true;
				}
			}
			else
			{
				state.isBeingRenamed = false;
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
			if (!state.isBeingRenamed)
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
					state.isBeingRenamed = false;
					selectionChanged = true;
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !selectableIsClicked)
				{
					state.isBeingRenamed = false;
				}
			}

			ImGui::SetCursorScreenPos(cursor + ImVec2(size));
			ImGui::EndGroup();

			state.groupSize = Vec2{ size.x, size.y };

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

		bool SelectableImage(const char* internalName, const Texture& image, const ImVec2& size, const ImVec2& uvMin, const ImVec2& uvMax)
		{
			std::string id = std::string("##") + image.path.string() + internalName;
			bool res = ImGui::Selectable(id.c_str(), false, 0, size);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 rectMin = ImGui::GetItemRectMin();
			ImVec2 rectMax = ImGui::GetItemRectMax();
			drawList->AddImage((void*)(uintptr_t)image.graphicsId, rectMin, rectMax, uvMin, uvMax);

			return res;
		}

		bool BeginImageCombo(const char* internalName, const Texture& image, const ImVec2& size, const ImVec2& uvMin, const ImVec2& uvMax)
		{
			std::string id = std::string("##") + image.path.string() + internalName;
			ImGui::PushItemWidth(size.x);
			bool res = ImGui::BeginCombo(id.c_str(), "", ImGuiComboFlags_NoArrowButton);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 rectMin = ImGui::GetItemRectMin();
			ImVec2 rectMax = ImGui::GetItemRectMax();
			drawList->AddImage((void*)(uintptr_t)image.graphicsId, rectMin, rectMax, uvMin, uvMax);

			return res;
		}

		bool ProgressBar(const char* label, float value, float maxWidth)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			ImGuiContext& g = *GImGui;
			const ImGuiStyle& style = g.Style;
			const ImGuiID id = window->GetID(label);

			ImVec2 pos = window->DC.CursorPos;

			// Make room for the label
			ImVec2 labelSize = ImGui::CalcTextSize(label);

			ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, labelSize.y);
			size.x -= style.FramePadding.x * 2.0f;
			size.x -= (labelSize.x + style.FramePadding.x);
			size.y += style.FramePadding.y * 2.0f;
			if (maxWidth > 0.0f && size.x > maxWidth)
			{
				size.x = maxWidth;
			}

			const ImRect bb(pos, pos + size);
			ImGui::ItemSize(bb);
			if (!ImGui::ItemAdd(bb, id))
				return false;

			// Render
			ImU32 bg_col = ImGui::ColorConvertFloat4ToU32(Colors::Primary[6]);
			ImU32 fg_col = ImGui::ColorConvertFloat4ToU32(Colors::Primary[3]);
			window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + size.x, bb.Max.y), bg_col);
			window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + size.x * value, bb.Max.y), fg_col);

			constexpr size_t percentCompleteTextBufferSize = 24;
			static char percentCompleteText[percentCompleteTextBufferSize];
			sprintf_s(percentCompleteText, percentCompleteTextBufferSize, "%2.2f%%", value * 100.0f);
			ImVec2 strSize = ImGui::CalcTextSize(percentCompleteText);
			if (strSize.x < size.x)
			{
				ImVec2 strPos = ImVec2(size.x / 2.0f - strSize.x / 2.0f, size.y / 2.0f - strSize.y / 2.0f);
				ImU32 textCol = ImGui::ColorConvertFloat4ToU32(Colors::Primary[0]);
				window->DrawList->AddText(pos + strPos, textCol, percentCompleteText);
			}

			// Just let ImGui handle the label
			ImGui::SameLine();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y / 2.0f - labelSize.y / 2.0f));
			ImGui::Text(label);

			return false;
		}

		EditState Checkbox(const char* label, bool* v)
		{
			std::string fullLabel = label + std::string("##Checkbox");
			return undoableImGuiFunction(
				fullLabel,
				[&]()
				{
					return ImGui::Checkbox(label, v);
				});
		}

		ImGuiDataEx<bool> CheckboxEx(const char* label, bool* v)
		{
			return checkboxData.undoableImGuiFunctionEx(
				label,
				*v,
				[&]()
				{
					return Checkbox(label, v);
				});
		}

		EditState InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
		{
			std::string fullLabel = label + std::string("##InputText");
			return undoableImGuiFunction(
				fullLabel,
				[&]()
				{
					return ImGui::InputText(label, buf, buf_size, flags, callback, user_data);
				});
		}

		ImGuiDataEx<std::string> InputTextEx(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
		{
			return inputTextData.undoableImGuiFunctionEx(
				label,
				buf,
				[&]()
				{
					return InputText(label, buf, buf_size, flags, callback, user_data);
				});
		}

		EditState InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
		{
			std::string fullLabel = label + std::string("##InputTextMultiline");
			return undoableImGuiFunction(
				fullLabel,
				[&]()
				{
					return ImGui::InputTextMultiline(label, buf, buf_size, size, flags, callback, user_data);
				});
		}

		ImGuiDataEx<std::string> InputTextMultilineEx(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
		{
			return inputTextMultilineData.undoableImGuiFunctionEx(
				label,
				buf,
				[&]()
				{
					return InputTextMultiline(label, buf, buf_size, size, flags, callback, user_data);
				});
		}

		EditState DragInt2(const char* label, Vec2i* v, float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
		{
			std::string fullLabel = label + std::string("##DragInt2");
			return undoableImGuiFunction(
				fullLabel,
				[&]()
				{
					return ImGui::DragInt2(label, v->values, v_speed, v_min, v_max, format, flags);
				});
		}

		ImGuiDataEx<Vec2i> DragInt2Ex(const char* label, Vec2i* v, float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
		{
			return dragInt2Data.undoableImGuiFunctionEx(
				label,
				*v,
				[&]()
				{
					return DragInt2(label, v, v_speed, v_min, v_max, format, flags);
				});
		}

		EditState DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
		{
			std::string fullLabel = label + std::string("##DragFloat");
			return undoableImGuiFunction(
				fullLabel,
				[&]()
				{
					return ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags);
				});
		}

		ImGuiDataEx<float> DragFloatEx(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
		{
			return dragFloatData.undoableImGuiFunctionEx(
				label,
				*v,
				[&]()
				{
					return DragFloat(label, v, v_speed, v_min, v_max, format, flags);
				});
		}

		EditState DragFloat2(const char* label, Vec2* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
		{
			std::string fullLabel = label + std::string("##DragFloat2");
			return undoableImGuiFunction(
				fullLabel,
				[&]()
				{
					return ImGui::DragFloat2(label, (float*)v->values, v_speed, v_min, v_max, format, flags);
				});
		}

		ImGuiDataEx<Vec2> DragFloat2Ex(const char* label, Vec2* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
		{
			return dragFloat2Data.undoableImGuiFunctionEx(
				label,
				*v,
				[&]()
				{
					return DragFloat2(label, v, v_speed, v_min, v_max, format, flags);
				});
		}

		EditState DragFloat3(const char* label, Vec3* vec3, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
		{
			std::string fullLabel = label + std::string("##DragFloat3");
			return undoableImGuiFunction(
				fullLabel,
				[&]()
				{
					return ImGui::DragFloat3(label, (float*)vec3->values, v_speed, v_min, v_max, format, flags);
				});
		}

		ImGuiDataEx<Vec3> DragFloat3Ex(const char* label, Vec3* vec3, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
		{
			return dragFloat3Data.undoableImGuiFunctionEx(
				label,
				*vec3,
				[&]()
				{
					return DragFloat3(label, vec3, v_speed, v_min, v_max, format, flags);
				});
		}

		EditState Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items)
		{
			std::string fullLabel = label + std::string("##Combo");
			return ImGui::Combo(label, current_item, items, items_count, popup_max_height_in_items)
				? EditState::FinishedEditing
				: EditState::NotEditing;
		}

		ImGuiDataEx<int> ComboEx(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items)
		{
			return comboData.undoableImGuiFunctionEx(
				label,
				*current_item,
				[&]()
				{
					return Combo(label, current_item, items, items_count, popup_max_height_in_items);
				});
		}

		EditState ColorEdit4(const char* label, glm::u8vec4* color, ImGuiColorEditFlags flags)
		{
			std::string fullLabel = label + std::string("##ColorEdit4");
			AdditionalEditState& state = additionalEditStates.findOrDefault(fullLabel, { false });

			float fillColor[4] = {
				(float)color->r / 255.0f,
				(float)color->g / 255.0f,
				(float)color->b / 255.0f,
				(float)color->a / 255.0f,
			};

			bool editRes = ImGui::ColorEdit4(label, fillColor, flags);
			editRes = editRes || ImGui::IsItemActive();
			if (editRes)
			{
				state.isBeingEdited = true;

				color->r = (uint8)(fillColor[0] * 255.0f);
				color->g = (uint8)(fillColor[1] * 255.0f);
				color->b = (uint8)(fillColor[2] * 255.0f);
				color->a = (uint8)(fillColor[3] * 255.0f);
			}

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				state.isBeingEdited = false;
				return EditState::FinishedEditing;
			}

			return editRes
				? EditState::BeingEdited
				: EditState::NotEditing;
		}

		ImGuiDataEx<glm::u8vec4> ColorEdit4Ex(const char* label, glm::u8vec4* color, ImGuiColorEditFlags flags)
		{
			return colorEditU84Data.undoableImGuiFunctionEx(
				label,
				*color,
				[&]()
				{
					return ColorEdit4(label, color, flags);
				});
		}

		EditState ColorEdit4(const char* label, Vec4* color, ImGuiColorEditFlags flags)
		{
			std::string fullLabel = label + std::string("##ColorEdit4");
			AdditionalEditState& state = additionalEditStates.findOrDefault(fullLabel, { false });

			bool editRes = ImGui::ColorEdit4(label, color->values, flags);
			editRes = editRes || ImGui::IsItemActive();
			if (editRes)
			{
				state.isBeingEdited = true;
			}

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				state.isBeingEdited = false;
				return EditState::FinishedEditing;
			}

			return editRes
				? EditState::BeingEdited
				: EditState::NotEditing;
		}

		ImGuiDataEx<Vec4> ColorEdit4Ex(const char* label, Vec4* col, ImGuiColorEditFlags flags)
		{
			return colorEdit4Data.undoableImGuiFunctionEx(
				label,
				*col,
				[&]()
				{
					return ColorEdit4(label, col, flags);
				});
		}
	}
}