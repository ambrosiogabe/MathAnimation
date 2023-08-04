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
	struct Texture;
	
	struct FilePayload
	{
		const char* filepath;
		size_t filepathLength;
	};

	enum class EditState : uint8
	{
		NotEditing = 0,
		BeingEdited,
		FinishedEditing,
	};

	template<typename T>
	struct ImGuiDataEx
	{
		T ogData;
		EditState editState;
	};

	namespace ImGuiExtended
	{
		bool ToggleButton(const char* string, bool* enabled, const ImVec2& size = ImVec2(0, 0));
		bool OutlineButton(const char* string, const ImVec2& size = ImVec2(0, 0));
		bool IconButton(const char* icon, const char* string, const ImVec2& size = ImVec2(0, 0));
		bool VerticalIconButton(const char* icon, const char* buttonText, float width);

		const AnimObjectPayload* AnimObjectDragDropTarget();
		EditState AnimObjDragDropInputBox(const char* label, AnimationManagerData* am, AnimObjId* output, AnimId animation = NULL_ANIM);
		ImGuiDataEx<AnimObjId> AnimObjDragDropInputBoxEx(const char* label, AnimationManagerData* am, AnimObjId* output, AnimId animation = NULL_ANIM);

		const FilePayload* FileDragDropTarget();
		bool FileDragDropInputBox(const char* label, char* outBuffer, size_t outBufferSize);
		const char* getFileDragDropPayloadId();

		void Icon(const char* icon, bool solid = true, float lineHeight = 0.0f);
		void MediumIcon(const char* icon, const Vec4& color = Vec4{1.0f, 1.0f, 1.0f, 1.0f}, bool solid = true);
		void LargeIcon(const char* icon, const Vec4& color = Vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, bool solid = true);

		bool RenamableIconSelectable(const char* icon, char* stringBuffer, size_t stringBufferSize, bool isSelected, float width);

		void CenteredWrappedText(ImVec2 textPosition, ImColor color, const char* text, float maxWidth);

		bool SelectableImage(const char* internalName, const Texture& image, const ImVec2& size, const ImVec2& uvMin = ImVec2(0, 0), const ImVec2& uvMax = ImVec2(1, 1));

		bool BeginImageCombo(const char* internalName, const Texture& image, const ImVec2& size, const ImVec2& uvMin = ImVec2(0, 0), const ImVec2& uvMax = ImVec2(1, 1));

		bool ProgressBar(const char* label, float value, float maxWidth = -1.0f);

		EditState InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
		ImGuiDataEx<std::string> InputTextEx(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
		EditState InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
		ImGuiDataEx<std::string> InputTextMultilineEx(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);

		EditState DragInt2(const char* label, Vec2i* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);
		ImGuiDataEx<Vec2i> DragInt2Ex(const char* label, Vec2i* v, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0);

		EditState DragFloat(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
		ImGuiDataEx<float> DragFloatEx(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
		EditState DragFloat2(const char* label, Vec2* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
		ImGuiDataEx<Vec2> DragFloat2Ex(const char* label, Vec2* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
		EditState DragFloat3(const char* label, Vec3* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);
		ImGuiDataEx<Vec3> DragFloat3Ex(const char* label, Vec3* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0);

		EditState Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
		ImGuiDataEx<int> ComboEx(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);

		EditState ColorEdit4(const char* label, glm::u8vec4* color, ImGuiColorEditFlags flags = 0);
		ImGuiDataEx<glm::u8vec4> ColorEdit4Ex(const char* label, glm::u8vec4* color, ImGuiColorEditFlags flags = 0);

		EditState ColorEdit4(const char* label, Vec4* color, ImGuiColorEditFlags flags = 0);
		ImGuiDataEx<Vec4> ColorEdit4Ex(const char* label, Vec4* col, ImGuiColorEditFlags flags = 0);
	}
}

#endif 