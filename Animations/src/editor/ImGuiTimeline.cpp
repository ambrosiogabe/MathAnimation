#include "editor/ImGuiTimeline.h"
#include "core/ImGuiLayer.h"

#include "imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

enum class DragState : uint8
{
	None,
	Hover,
	Active,
};

enum class ResizeFlags : uint8
{
	EastWest = 0x1,
	NorthSouth = 0x2
};

namespace MathAnim
{
	// ----------- Internal Variables -----------
	// Config Values
	constexpr float timelineHorizontalScrollSensitivity = 12.0f;
	constexpr int fps = 60;

	// Dimensional values
	constexpr int maxHighlightedSquares = 12;
	constexpr float timelineRulerHeight = 65.0f;
	constexpr float timelineRulerBorderHeight = 3.0f;
	constexpr float timelineRulerTickSpacing = 9.0f;
	constexpr float minDistanceBetweenRulerTimecodes = 320.0f;

	constexpr int tickWidth = 2.0f;
	constexpr int smallTickHeight = 7.5f;
	constexpr int mediumTickHeight = 15.0f;
	constexpr int largeTickHeight = 30.0f;
	constexpr int boundaryTickHeight = 45.0f;

	// Colors
	constexpr ImU32 boundaryTickColor = IM_COL32(135, 135, 135, 255);
	constexpr ImU32 largeTickColor = IM_COL32(105, 105, 105, 255);
	constexpr ImU32 defaultTickColor = IM_COL32(85, 85, 85, 255);

	// ----------- Internal Functions -----------
	static bool handleLegendSplitter(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, float* legendWidth);
	static bool handleResizeElement(float* currentValue, DragState* state, const ImVec2& valueBounds, const ImVec2& mouseBounds, const ImVec2& hoverRectStart, const ImVec2& hoverRectEnd, ResizeFlags flags);
	static void framesToTimeStr(char* strBuffer, size_t strBufferLength, int frame);
	static float getScrollFromFrame(float amountOfTimeVisibleInTimeline, int firstFrame, const ImVec2& timelineRulerEnd, const ImVec2& timelineRulerBegin);

	ImGuiTimelineResult ImGuiTimeline(ImGuiTimeline_Track* tracks, int numTracks, int* currentFrame, int* firstFrame, float* inZoom, float* inScrollOffsetX, ImGuiTimelineFlags flags)
	{
		ImGuiTimelineResult res;
		res.flags = ImGuiTimelineResultFlags_None;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();        // Resize canvas to what's available

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		// ---------------------- Setup and Handle Legend ------------------------------
		static float legendWidth = 0.2f;
		ImVec4& legendBackground = style.Colors[ImGuiCol_FrameBg];
		ImVec2 legendSize = ImVec2{ canvasSize.x * legendWidth, canvasSize.y };
		bool result = false;
		handleLegendSplitter(canvasPos, canvasSize, legendSize, &legendWidth);
		// ---------------------- Setup and Handle Legend ------------------------------

		// ---------------------- Draw Timeline Ruler ------------------------------		
		// Zoom ranges from [1.0 - 10.0]
		static float defaultZoom = 3.0f;
		float* zoom = inZoom == nullptr
			? &defaultZoom
			: inZoom;
		ImU32 grayColor = IM_COL32(32, 32, 32, 255);

		static float defaultScrollOffsetX = 0.0f;
		float* scrollOffsetX = inScrollOffsetX == nullptr
			? &defaultScrollOffsetX
			: inScrollOffsetX;

		ImVec2 timelineRulerBegin = canvasPos + ImVec2(legendSize.x, 0.0f);
		ImVec2 timelineRulerEnd = canvasPos + ImVec2(canvasSize.x, timelineRulerHeight);
		drawList->AddRectFilled(
			timelineRulerBegin + ImVec2(0.0f, timelineRulerHeight),
			timelineRulerEnd + ImVec2(0.0f, timelineRulerBorderHeight),
			grayColor);

		// Draw the border top
		drawList->AddRectFilled(
			timelineRulerBegin,
			ImVec2(timelineRulerEnd.x, timelineRulerBegin.y - 1.5f),
			IM_COL32(0, 0, 0, 255)
		);

		// The Zoom level tells us how many frames are visible in a single
		// pixel. The minimum number
		// So default zoom should go 2 seconds, 4 seconds, 6 seconds ... 
		// 
		// It should also stay consistently spaced apart and go: 
		//      big small big small ... halfway small big small ... time tick

		constexpr int numTicksBetweenBoundaries = fps / 2;
		constexpr int tickMidpoint = numTicksBetweenBoundaries / 2;
		constexpr float distanceBetweenSmallTicks = minDistanceBetweenRulerTimecodes / (float)numTicksBetweenBoundaries;
		float amountOfTimeVisibleInTimeline = ((timelineRulerEnd.x - timelineRulerBegin.x) / (float)distanceBetweenSmallTicks);

		float firstTimecodePosition = glm::floor((*scrollOffsetX / minDistanceBetweenRulerTimecodes)) * minDistanceBetweenRulerTimecodes - *scrollOffsetX;
		// Zoom will effect this soon
		int numTimecodesThatFit = glm::ceil((timelineRulerEnd.x - timelineRulerBegin.x) / minDistanceBetweenRulerTimecodes);
		for (int i = 0; i <= numTimecodesThatFit; i++)
		{
			ImVec2 tickStart = ImVec2(firstTimecodePosition + i * minDistanceBetweenRulerTimecodes, 0.0f);
			tickStart += timelineRulerBegin;
			ImVec2 tickEnd = tickStart + ImVec2(tickWidth, boundaryTickHeight);
			if (tickStart.x >= timelineRulerBegin.x)
			{
				drawList->AddRectFilled(tickStart, tickEnd, boundaryTickColor);
			}

			{
				// Draw the time code string
				int firstTickPos = (int)glm::floor((*scrollOffsetX / minDistanceBetweenRulerTimecodes));
				int numFramesToThisPos = (firstTickPos + i) * numTicksBetweenBoundaries;

				ImVec4& fontColor = style.Colors[ImGuiCol_Text];
				// Max formatted string length is HH:MM:SS:ff
				static char strBuffer[] = "HH:MM:SS:ff";
				size_t strBufferSize = sizeof(strBuffer) / sizeof(strBuffer[0]);
				framesToTimeStr(strBuffer, strBufferSize, numFramesToThisPos);

				ImVec2 textSize = ImGui::CalcTextSize(strBuffer, strBuffer + strBufferSize - 1);
				ImVec2 textPos = tickEnd + ImVec2(3.0f, -textSize.y + 3.0f);
				if (textPos.x + textSize.x >= timelineRulerBegin.x)
				{
					drawList->AddText(textPos, ImColor(fontColor), strBuffer, strBuffer + strBufferSize - 1);
				}
			}

			{
				ImVec2 rectStart = ImVec2(tickStart.x, canvasPos.y + timelineRulerHeight);
				// Draw the boundary lines on the main timeline
				drawList->AddRectFilled(
					rectStart,
					rectStart + ImVec2(tickWidth, canvasSize.y),
					grayColor);
			}

			ImVec2 smallTickStart = tickStart;
			for (int i = 0; i < numTicksBetweenBoundaries; i++)
			{
				smallTickStart.x += distanceBetweenSmallTicks;
				ImVec2 smallTickEnd = smallTickStart + ImVec2(tickWidth, 0.0f);
				smallTickEnd.y += (i + 1) == tickMidpoint
					? largeTickHeight
					: i % 2 == 1
					? smallTickHeight
					: mediumTickHeight;
				const ImU32& color = (i + 1) == tickMidpoint
					? largeTickColor
					: defaultTickColor;

				if (smallTickStart.x > timelineRulerBegin.x)
				{
					drawList->AddRectFilled(smallTickStart, smallTickEnd, color);
				}
			}
		}
		// ---------------------- End Draw Timeline Ruler ------------------------------

		// ---------------------- Draw Timeline Elements ------------------------------
		int dummy;

		// ---------------------- End Draw Timeline Elements ------------------------------

		// ---------------------- Handle Timeline Cursor ------------------------------
		// Handle scrolling while over the timeline
		if (ImGui::IsMouseHoveringRect(timelineRulerBegin, ImVec2(timelineRulerEnd.x, canvasPos.y + canvasSize.y)))
		{
			if (io.MouseWheel != 0.0f && io.KeyCtrl)
			{
				*scrollOffsetX -= io.MouseWheel * timelineHorizontalScrollSensitivity;
				*scrollOffsetX = glm::max(*scrollOffsetX, 0.0f);

				float normalizedScrollDistance = *scrollOffsetX / (timelineRulerEnd.x - timelineRulerBegin.x);
				*firstFrame = (int)(amountOfTimeVisibleInTimeline * normalizedScrollDistance);
				res.flags |= ImGuiTimelineResultFlags_FirstFrameChanged;
			}
		}

		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImGuiStyle& style = ImGui::GetStyle();

			static DragState timelineDragging = DragState::None;
			if (ImGui::IsMouseHoveringRect(timelineRulerBegin, timelineRulerEnd))
			{
				if (timelineDragging == DragState::None && !io.MouseDown[ImGuiMouseButton_Left])
				{
					timelineDragging = DragState::Hover;
				}

				if (timelineDragging == DragState::Hover && io.MouseDown[ImGuiMouseButton_Left])
				{
					timelineDragging = DragState::Active;
				}
			}
			else if (timelineDragging == DragState::Hover)
			{
				timelineDragging = DragState::None;
			}

			if (timelineDragging == DragState::Active && io.MouseDown[ImGuiMouseButton_Left])
			{
				timelineDragging = DragState::Active;
				float mouseOffsetX = io.MousePos.x - timelineRulerBegin.x;
				float normalMouseX = mouseOffsetX / (timelineRulerEnd.x - timelineRulerBegin.x);
				*currentFrame = (int)glm::floor((normalMouseX * amountOfTimeVisibleInTimeline)) + (*firstFrame);

				if (*currentFrame < *firstFrame)
				{
					*firstFrame -= 3;
					*firstFrame = glm::max(*firstFrame, 0);
					*scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame, timelineRulerEnd, timelineRulerBegin);
					res.flags |= ImGuiTimelineResultFlags_FirstFrameChanged;
				}
				else if (*currentFrame > *firstFrame + (int)amountOfTimeVisibleInTimeline)
				{
					*firstFrame += 3;
					*scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame, timelineRulerEnd, timelineRulerBegin);
					res.flags |= ImGuiTimelineResultFlags_FirstFrameChanged;
				}

				*currentFrame = glm::min(glm::max(*currentFrame, *firstFrame), *firstFrame + (int)amountOfTimeVisibleInTimeline);
				res.flags |= ImGuiTimelineResultFlags_CurrentFrameChanged;
			}
			else if (timelineDragging == DragState::Active && !io.MouseDown[ImGuiMouseButton_Left])
			{
				timelineDragging = DragState::None;
			}

			ImVec2 cursorSize = ImVec2(5.5f, canvasSize.y);
			ImU32 cursorColor = IM_COL32(214, 118, 111, 255);
			if (timelineDragging == DragState::Hover)
			{
				cursorColor = IM_COL32(237, 113, 104, 255);
			}
			else if (timelineDragging == DragState::Active)
			{
				cursorColor = IM_COL32(237, 131, 123, 255);
			}

			ImVec2 cursorStart = timelineRulerBegin;
			cursorStart.x += (timelineRulerEnd.x - timelineRulerBegin.x) * (((float)*currentFrame - (float)*firstFrame) / amountOfTimeVisibleInTimeline);

			float triangleWidth = 15.0f;
			float triangleHeight = 15.0f;
			float triangleOffsetY = 12.0f;
			ImVec2 cursorCenterX = cursorStart + (ImVec2(cursorSize.x, 0.0f) * 0.5f);
			ImVec2 p1 = cursorCenterX + ImVec2(-triangleWidth, triangleOffsetY);
			ImVec2 p2 = cursorCenterX + ImVec2(triangleWidth, triangleOffsetY);
			ImVec2 p3 = cursorCenterX + ImVec2(0.0f, triangleHeight + triangleOffsetY);
			ImVec2 triangleRectBegin = cursorCenterX + ImVec2(-triangleWidth, 0.0f);
			ImVec2 triangleRectEnd = cursorCenterX + ImVec2(triangleWidth, triangleOffsetY);

			// Don't draw the timeline cursor if it's scrolled off the edge
			if (*currentFrame >= *firstFrame)
			{
				// TODO: Add stroke color
				//drawList->AddRect(triangleRectBegin, triangleRectEnd, IM_COL32(0, 0, 0, 255), 0.5f, 0, 6.0f);
				//drawList->AddTriangle(p1, p2, p3, IM_COL32(0, 0, 0, 255), 6.0f);
				drawList->AddRectFilled(triangleRectBegin, triangleRectEnd, cursorColor);
				drawList->AddTriangleFilled(p1, p2, p3, cursorColor);
				drawList->AddRectFilled(cursorStart, cursorStart + cursorSize, cursorColor);
			}

			if (timelineDragging == DragState::Active)
			{
				result = true;
			}
		}
		// ---------------------- End Handle Timeline Cursor ------------------------------

		// ---------------------- Follow Timeline Cursor Logic ------------------------------
		// If we want to follow the cursor, adjust our current frame to be within the bounds by a certain threshold
		if ((uint8)flags & ImGuiTimelineFlags_FollowTimelineCursor)
		{
			bool changed = false;
			if (*currentFrame < *firstFrame)
			{
				*firstFrame = *currentFrame - 30;
				changed = true;
			}
			else if (*currentFrame > (*firstFrame + (int)amountOfTimeVisibleInTimeline))
			{
				*firstFrame = *currentFrame - 30;
				changed = true;
			}

			if (changed)
			{
				*scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame, timelineRulerEnd, timelineRulerBegin);
				res.flags |= ImGuiTimelineResultFlags_FirstFrameChanged;
			}
		}
		// ---------------------- End Follow Timeline Cursor Logic ------------------------------

		// ---------------------- Draw Legend ------------------------------
		{
			drawList->AddRectFilled(canvasPos, canvasPos + legendSize, ImColor(legendBackground.x, legendBackground.y, legendBackground.z));
			// Draw the current time in hours:minutes:seconds.milliseconds
			ImVec2 timecodeRectSize = ImVec2(legendSize.x, timelineRulerHeight);
			drawList->AddRect(canvasPos, canvasPos + timecodeRectSize, IM_COL32(0, 0, 0, 255), 0, 0, 1.5f);

			// Max formatted string length is HH:MM:SS:ff
			static char strBuffer[] = "HH:MM:SS:ff";
			size_t strBufferSize = sizeof(strBuffer) / sizeof(strBuffer[0]);
			framesToTimeStr(strBuffer, strBufferSize, *currentFrame);

			ImGui::PushFont(ImGuiLayer::getLargeFont());
			ImVec2 textSize = ImGui::CalcTextSize(strBuffer, strBuffer + strBufferSize - 1);
			ImVec2 textPos = canvasPos + ((timecodeRectSize - textSize) / 2.0f);
			ImVec4& fontColor = style.Colors[ImGuiCol_Text];
			drawList->AddText(textPos, ImColor(fontColor), strBuffer, strBuffer + strBufferSize - 1);
			ImGui::PopFont();

		}
		// ---------------------- End Legend ------------------------------

		return res;
	}

	static bool handleLegendSplitter(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, float* legendWidth)
	{
		static DragState state = DragState::None;

		constexpr float splitterWidth = 12.0f;
		ImVec2 splitterBegin = (canvasPos + ImVec2(legendSize.x, 0)) - ImVec2(splitterWidth / 2.0f, 0);
		ImVec2 splitterSize = ImVec2(splitterWidth, canvasSize.y);
		ImVec2 mouseBounds = ImVec2(canvasPos.x, canvasPos.x + canvasSize.x);

		if (handleResizeElement(legendWidth, &state, ImVec2(0.0f, 1.0f), mouseBounds, splitterBegin, splitterBegin + splitterSize, ResizeFlags::EastWest))
		{
			*legendWidth = glm::min(glm::max(*legendWidth, 0.2f), 0.5f);
		}

		if (state == DragState::Hover)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImGuiStyle& style = ImGui::GetStyle();
			constexpr float splitterRenderWidth = 4.0f;
			const ImVec4& splitterBgColor = style.Colors[ImGuiCol_FrameBgHovered];
			drawList->AddRectFilled(splitterBegin, splitterBegin + ImVec2(splitterRenderWidth, canvasSize.y), ImColor(splitterBgColor));
		}

		return state == DragState::Active;
	}

	static bool handleResizeElement(float* currentValue, DragState* state, const ImVec2& valueBounds, const ImVec2& mouseBounds, const ImVec2& hoverRectStart, const ImVec2& hoverRectEnd, ResizeFlags flags)
	{
		float normalizedPos = ((float)(*currentValue) - (float)valueBounds.x) / ((float)valueBounds.y - (float)valueBounds.x);

		if (ImGui::IsMouseHoveringRect(hoverRectStart, hoverRectEnd))
		{
			if (*state == DragState::None && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
			{
				*state = DragState::Hover;
			}

			if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] && *state == DragState::Hover)
			{
				*state = DragState::Active;
			}
		}
		else if (*state == DragState::Hover)
		{
			*state = DragState::None;
		}

		bool currentValueChanged = false;
		if (*state == DragState::Active && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
		{
			float mousePos = ((uint8)flags & (uint8)ResizeFlags::EastWest)
				? (float)ImGui::GetIO().MousePos.x
				: ((uint8)flags & (uint8)ResizeFlags::NorthSouth)
				? (float)ImGui::GetIO().MousePos.y
				: -1;
			float normalizedPos = (mousePos - (float)mouseBounds.x) / ((float)mouseBounds.y - (float)mouseBounds.x);
			// Un-normalize the value and mark it as changed
			*currentValue = (normalizedPos * ((float)valueBounds.y - (float)valueBounds.x)) + valueBounds.x;
			// Clamp it to the range
			*currentValue = glm::min(glm::max(valueBounds.x, *currentValue), valueBounds.y);
			currentValueChanged = true;
		}
		else if (*state == DragState::Active && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
		{
			*state = DragState::None;
		}

		if (*state != DragState::None)
		{
			ImGuiMouseCursor cursor = ((uint8)flags & (uint8)ResizeFlags::EastWest)
				? ImGuiMouseCursor_ResizeEW
				: ((uint8)flags & (uint8)ResizeFlags::NorthSouth)
				? ImGuiMouseCursor_ResizeNS
				: ImGuiMouseCursor_Arrow;
			ImGui::SetMouseCursor(cursor);
		}

		return currentValueChanged;
	}

	static void framesToTimeStr(char* strBuffer, size_t strBufferLength, int frame)
	{
		int relativeFrame = frame % 60;
		int numSeconds = frame / 60;
		int numMinutes = numSeconds / 60;
		int numHours = numMinutes / 60;
		// Max formatted string length is HH:MM:SS:ff
		constexpr char requiredBuffer[] = "HH:MM:SS:ff";
		constexpr size_t requiredBufferSize = sizeof(requiredBuffer) / sizeof(strBuffer[0]);
		g_logger_assert(strBufferLength >= requiredBufferSize, "String buffer length must be at least '%d' characters.", requiredBufferSize);
		sprintf_s(strBuffer, strBufferLength, "%02d:%02d:%02d.%02d", numHours, numMinutes, numSeconds, relativeFrame);
	}

	static float getScrollFromFrame(float amountOfTimeVisibleInTimeline, int firstFrame, const ImVec2& timelineRulerEnd, const ImVec2& timelineRulerBegin)
	{
		float normalizedScrollDistance = (float)firstFrame / amountOfTimeVisibleInTimeline;
		float res = normalizedScrollDistance * (timelineRulerEnd.x - timelineRulerBegin.x);
		res = glm::max(res, 0.0f);
		return res;
	}
}