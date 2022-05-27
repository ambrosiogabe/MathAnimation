#include "editor/ImGuiTimeline.h"

#include "imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

namespace MathAnim
{
	// ----------- Internal Functions -----------
	static bool handleLegendSplitter(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, float* legendWidth);
	static bool handleCursor(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, float legendWidth, int* currentFrame, int numFrames, bool isHandlingWidget);

	bool ImGuiTimeline(Track* tracks, int numTracks, int* currentFrame, int* firstFrame, int numFrames, float* inZoom, ImGuiTimelineFlags flags)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();        // Resize canvas to what's available

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		static float legendWidth = 0.2f;
		ImVec4& legendBackground = style.Colors[ImGuiCol_FrameBg];
		ImVec2 legendSize = ImVec2 { canvasSize.x * legendWidth, canvasSize.y };
		drawList->AddRectFilled(canvasPos, canvasPos + legendSize, ImColor(legendBackground));

		bool result = false;
		bool isHandlingWidget = false;
		if (handleLegendSplitter(canvasPos, canvasSize, legendSize, &legendWidth))
		{
			isHandlingWidget = true;
		}

		static float defaultZoom = 1.0f;
		float* zoom = inZoom == nullptr 
			? &defaultZoom 
			: inZoom;
		constexpr int maxHighlightedSquares = 12;
		int numHighlightedSquares = glm::max((int)glm::ceil((1.0f - legendWidth) * (float)maxHighlightedSquares), 2);
		float squareWidth = canvasSize.x * (1.0f - legendWidth) / numHighlightedSquares;
		ImVec2 timelineStart = canvasPos + ImVec2(legendSize.x, 0.0f);
		for (int i = 1; i < numHighlightedSquares; i += 2)
		{
			ImVec2 rectStart = timelineStart + ImVec2(squareWidth * (float)i, 0.0f);
			ImU32 grayColor = IM_COL32(32, 32, 32, 255);
			float halfLineWidth = 1.5f;
			drawList->AddRectFilled(
				rectStart - ImVec2(halfLineWidth, 0.0f), 
				rectStart + ImVec2(halfLineWidth, canvasSize.y), 
				grayColor);
			drawList->AddRectFilled(
				rectStart - ImVec2(halfLineWidth, 0.0f) + ImVec2(squareWidth, 0.0f),
				rectStart + ImVec2(halfLineWidth, canvasSize.y) + ImVec2(squareWidth, 0.0f),
				grayColor);
		}

		if (handleCursor(canvasPos, canvasSize, legendSize, legendWidth, currentFrame, numFrames, isHandlingWidget))
		{
			result = true;
		}

		return result;
	}

	static bool handleLegendSplitter(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, float* legendWidth)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImGuiStyle& style = ImGui::GetStyle();

		constexpr float splitterWidth = 12.0f;
		ImVec2 splitterBegin = (canvasPos + ImVec2(legendSize.x, 0)) - ImVec2(splitterWidth / 2.0f, 0);

		static bool isHovering = false;
		static bool isActive = false;

		if (ImGui::IsMouseHoveringRect(splitterBegin, splitterBegin + ImVec2(splitterWidth, canvasSize.y)))
		{
			constexpr float splitterRenderWidth = 4.0f;
			const ImVec4& splitterBgColor = style.Colors[ImGuiCol_FrameBgHovered];
			drawList->AddRectFilled(splitterBegin, splitterBegin + ImVec2(splitterRenderWidth, canvasSize.y), ImColor(splitterBgColor));
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			isHovering = true;

			if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] && !isActive)
			{
				isActive = true;
			}
		}
		else
		{
			isHovering = false;
		}

		if (isActive && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
		{
			float dragDistance = ImGui::GetIO().MouseDelta.x;
			*legendWidth += (dragDistance / canvasSize.x);
			*legendWidth = glm::min(glm::max(0.2f, *legendWidth), 0.8f);

			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
		else if (isActive && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
		{
			isActive = false;
		}

		return isActive;
	}

	static bool handleCursor(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, float legendWidth, int* currentFrame, int numFrames, bool isHandlingWidget)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImGuiStyle& style = ImGui::GetStyle();

		float framePos = (float)*currentFrame / (float)numFrames;
		int paddingX = 6.0f;
		float cursorOffsetX = framePos * canvasSize.x * (1.0f - legendWidth) + paddingX;
		ImVec2 cursorStart = canvasPos + ImVec2(legendSize.x + cursorOffsetX, 0.0f);
		constexpr float hoverWidth = 6.0f;
		ImVec2 cursorSize = ImVec2(3.5f, canvasSize.y);
		ImU32 cursorColor = IM_COL32(214, 118, 111, 255);

		static bool isHovering = false;
		static bool isActive = false;

		if (!isHandlingWidget && ImGui::IsMouseHoveringRect(cursorStart, cursorStart + ImVec2(hoverWidth, canvasSize.y)))
		{
			const ImVec4& splitterBgColor = style.Colors[ImGuiCol_FrameBgHovered];
			cursorColor = IM_COL32(237, 113, 104, 255);
			isHovering = true;
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

			if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] && !isActive)
			{
				isActive = true;
			}
		}
		else
		{
			isHovering = false;
		}


		if (isActive && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
		{
			cursorColor = IM_COL32(237, 131, 123, 255);

			float mousePosX = ImGui::GetIO().MousePos.x;
			float normalizedPosX = (mousePosX - canvasPos.x - legendSize.x - paddingX) / ((1.0f - legendWidth) * canvasSize.x);
			*currentFrame = (int)(normalizedPosX * (float)numFrames);
			*currentFrame = glm::min(glm::max(0, *currentFrame), numFrames - 1);

			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}
		else if (isActive && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
		{
			isActive = false;
		}

		drawList->AddRectFilled(cursorStart, cursorStart + cursorSize, cursorColor);

		return isActive;
	}
}