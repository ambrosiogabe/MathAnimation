#include "editor/ImGuiTimeline.h"
#include "core/ImGuiLayer.h"
#include "core/Colors.h"

#include "imgui.h"
#include "utils/FontAwesome.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

enum class DragState : uint8
{
	None,
	Hover,
	Active,
};

enum class SegmentChangeType : uint8
{
	None,
	Resized,
	Moved
};

enum class ResizeFlags : uint8
{
	EastWest = 0x1,
	NorthSouth = 0x2
};

struct ImTimelineWindowData
{
	ImGuiID windowID;
	ImVec2 Scroll;
};

static constexpr int maxNumTimelines = 16;
static int numTimelinesActive = 0;
static ImTimelineWindowData windowData[maxNumTimelines];

static ImU32 U32Color(const MathAnim::Vec4& c)
{
	return ImGui::ColorConvertFloat4ToU32(c);
}

namespace MathAnim
{
	// ----------- Internal Variables -----------
	// Config Values
	constexpr float timelineHorizontalScrollSensitivity = 12.0f;
	constexpr float timelineVerticalScrollSensitivity = 10.0f;
	constexpr int fps = 60;
	const char* TIMELINE_DRAG_DROP_SEGMENT_PAYLOAD_ID = "TIMELINE_SEGMENT_PAYLOAD_ID";
	const char* TIMELINE_DRAG_DROP_SUB_SEGMENT_PAYLOAD_ID = "TIMELINE_SUB_SEGMENT_PAYLOAD_ID";

	// State Values
	static ImGuiID segmentDragID = UINT32_MAX;

	// Dimensional values
	constexpr int maxHighlightedSquares = 12;
	constexpr float timelineRulerHeight = 65.0f;
	constexpr float timelineRulerBorderHeight = 3.0f;
	constexpr float timelineRulerTickSpacing = 9.0f;
	constexpr float minDistanceBetweenRulerTimecodes = 320.0f;
	// If a segment is 20 pixels next to another segment while dragging it, snap them together
	constexpr float magnetThreshold = 30.0f;

	constexpr int tickWidth = 2;
	constexpr int smallTickHeight = 7;
	constexpr int mediumTickHeight = 15;
	constexpr int largeTickHeight = 30;
	constexpr int boundaryTickHeight = 45;

	constexpr int trackHeight = 145;
	constexpr int trackNamePadding = 15;

	constexpr ImVec2 segmentTextAreaPadding = ImVec2(10.0f, 10.0f);
	constexpr ImVec2 expandButtonSize = ImVec2(18.0f, 10.0f);
	constexpr int segmentTextAreaHeight = 40;

	constexpr float ZOOM_MIN = 0.1f;
	constexpr float ZOOM_MAX = 300.0f;
	constexpr float ZOOM_DEFAULT = 5.0f;

	// Colors
	const ImU32 boundaryTickColor = U32Color(Colors::Neutral[4]);
	const ImU32 largeTickColor = U32Color(Colors::Neutral[5]);
	const ImU32 defaultTickColor = U32Color(Colors::Neutral[6]);

	const ImU32 dropdownArrowColor = U32Color(Colors::Neutral[1]);
	const ImU32 legendBorderColor = U32Color(Colors::Neutral[8]);
	const ImU32 legendBorderHighlightColor = U32Color(Colors::Neutral[5]);

	const ImU32 canvasColor = U32Color(Colors::Neutral[8]);
	const ImU32 legendBackground = U32Color(Colors::Neutral[7]);
	const ImU32 timelineTrackDark = U32Color(Colors::Neutral[9]);
	const ImU32 cursorColor = U32Color(Colors::AccentRed[3]);

	const ImU32 segmentColor = U32Color(Colors::Primary[5]);
	const ImU32 segmentDarkColor = U32Color(Colors::Primary[6]);
	const ImU32 segmentBoldColor = U32Color(Colors::Primary[8]);
	const ImU32 subSegmentColor = U32Color(Colors::AccentGreen[3]);
	const ImU32 subSegmentDarkColor = U32Color(Colors::AccentGreen[4]);

	// Commonly used vars
	static ImVec2 timelineRulerEnd;
	static ImVec2 timelineRulerBegin;
	static ImVec2 canvasPos;
	static ImVec2 canvasSize;
	static ImVec2 legendSize;

	static ImVec2* audioCache = nullptr;
	static int lengthAudioCache = 0;
	static int maxLengthAudioCache = 0;

	// ----------- Internal Functions -----------
	static bool handleLegendSplitter(float* legendWidth, bool mouseHovering);
	static bool handleResizeElement(float* currentValue, DragState* state, const ImVec2& valueBounds, const ImVec2& mouseBounds, const ImVec2& hoverRectStart, const ImVec2& hoverRectEnd, ResizeFlags flags);
	static bool handleSegment(const ImVec2& segmentStart, const ImVec2& segmentEnd, ImGuiTimeline_Segment* segment, SegmentChangeType* changeType, ImGuiID segmentID, const ImVec2& timelineSize, float amountOfTimeVisibleInTimeline);
	static bool handleSubSegment(const ImVec2& segmentStart, const ImVec2& segmentEnd, ImGuiTimeline_SubSegment* segment, SegmentChangeType* changeType, ImGuiID segmentID, const ImVec2& timelineSize, float amountOfTimeVisibleInTimeline);
	static void framesToTimeStr(char* strBuffer, size_t strBufferLength, int frame);
	static float getScrollFromFrame(float amountOfTimeVisibleInTimeline, int firstFrame);
	static bool handleDragState(const ImVec2& start, const ImVec2& end, DragState* state);
	static bool beginPopupContextTimelineItem(const char* str_id, const ImVec2& rectBegin, const ImVec2& rectEnd, ImGuiPopupFlags popup_flags = 1);
	static int findSegmentFromFrame(const ImGuiTimeline_Track* tracks, int trackIndex, int firstAbsoluteFrame);
	static void snapToCursor(int* currentFrame, int* firstFrame, int* segmentFrameStart, int* segmentFrameDuration, float* offsetX, float* width, float amountOfTimeVisibleInTimeline, SegmentChangeType changeType);
	static void snapToPreviousSegment(int* firstFrame, float* offsetX, int* segmentFrameStart, int* segmentFrameDuration, int lastSegmentFrameStart, int lastSegmentFrameDuration, SegmentChangeType changeType, float amountOfTimeVisibleInTimeline);
	static void snapToNextSegment(int* firstFrame, float* offsetX, float* width, int* segmentFrameStart, int* segmentFrameDuration, int nextSegmentFrameStart, int nextSegmentFrameDuration, SegmentChangeType changeType, float amountOfTimeVisibleInTimeline);
	static void checkAudioCacheResize(int numElementsToAdd);

	static inline float calculateSegmentOffset(int segmentFrameStart, int firstFrame, float amountOfTimeVisibleInTimeline)
	{
		return (segmentFrameStart - (float)firstFrame) / amountOfTimeVisibleInTimeline * (canvasSize.x - legendSize.x);
	}

	static inline float calculateSegmentWidth(int segmentFrameDuration, float amountOfTimeVisibleInTimeline)
	{
		return (segmentFrameDuration / amountOfTimeVisibleInTimeline) * (canvasSize.x - legendSize.x);
	}

	static inline float calculateCursorOffset(int currentFrame, int firstFrame, float amountOfTimeVisibleInTimeline)
	{
		return (timelineRulerEnd.x - timelineRulerBegin.x) * (((float)currentFrame - (float)firstFrame) / amountOfTimeVisibleInTimeline);
	}

	ImGuiTimelineResult ImGuiTimeline(ImGuiTimeline_Track* tracks, int numTracks, int* currentFrame, int* firstFrame, float* inZoom, const ImGuiTimeline_AudioData* audioData, ImGuiTimelineFlags flags)
	{
		static float defaultZoom = ZOOM_DEFAULT;
		float* zoom = inZoom == nullptr
			? &defaultZoom
			: inZoom;
		static bool magnetEnabled = false;
		static bool mouseHoveringOverTimelineWindow = false;
		mouseHoveringOverTimelineWindow = ImGui::IsWindowHovered();

		// --------------------------- Handle Timeline Controls ---------------------------
		{
			// Draw any enabled controls first
			static float totalControlsWidth = 0.0f;
			float controlsBeginX = ImGui::GetCursorPos().x;

			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalControlsWidth) / 2.0f);

			if ((flags & ImGuiTimelineFlags_EnableMagnetControl) || (flags & ImGuiTimelineFlags_EnableZoomControl))
			{
				// Offset by some vertical padding if drawing timeline controls
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
			}

			if (flags & ImGuiTimelineFlags_EnableMagnetControl)
			{
				bool popColor = false;
				if (magnetEnabled)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, cursorColor);
					popColor = true;
				}

				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
				if (ImGui::Button(ICON_FA_MAGNET))
				{
					magnetEnabled = !magnetEnabled;
				}
				ImGui::PopStyleColor(3);
				ImGui::SameLine();

				if (popColor)
				{
					ImGui::PopStyleColor();
				}
			}

			if (flags & ImGuiTimelineFlags_EnableZoomControl)
			{
				if (ImGui::Button("-"))
				{
					*zoom -= 0.5f;
					*zoom = glm::clamp(*zoom, ZOOM_MIN, ZOOM_MAX);
				}
				ImGui::SameLine();

				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
				ImGui::SliderFloat("##ImGuiTimeline_ZoomControl", zoom, ZOOM_MIN, ZOOM_MAX, "%2.3f", ImGuiSliderFlags_Logarithmic);
				ImGui::SameLine();

				if (ImGui::Button("+"))
				{
					*zoom += 0.5f;
				}
				ImGui::SameLine();

				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
				if (ImGui::Button(ICON_FA_REPLY_ALL))
				{
					*zoom = ZOOM_DEFAULT;
				}
				ImGui::PopStyleColor(3);
			}

			float controlsEndX = ImGui::GetCursorPos().x;
			totalControlsWidth = controlsEndX - controlsBeginX;
		}
		// --------------------------- End Handle Timeline Controls ---------------------------

		ImGuiTimelineResult res;
		res.flags = ImGuiTimelineResultFlags_None;
		res.segmentIndex = -1;
		res.trackIndex = -1;
		res.subSegmentIndex = -1;
		res.activeObjectIsSubSegment = false;

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4& fontColor = style.Colors[ImGuiCol_Text];

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		canvasPos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
		canvasSize = ImGui::GetContentRegionAvail();        // Resize canvas to what's available

		// ---------------------- Check if timeline has been added to timeline data ------------------------------
		ImTimelineWindowData* data = nullptr;
		{
			for (int i = 0; i < numTimelinesActive; i++)
			{
				if (windowData[i].windowID == ImGui::GetCurrentWindow()->ID)
				{
					data = &windowData[i];
					break;
				}
			}

			if (!data)
			{
				g_logger_assert(numTimelinesActive < maxNumTimelines, "Ran out of timeline room.");
				data = &windowData[numTimelinesActive];
				numTimelinesActive++;
				g_memory_zeroMem(data, sizeof(ImTimelineWindowData));
				data->windowID = ImGui::GetCurrentWindow()->ID;
			}
		}
		// ---------------------- End Check if timeline has been added to timeline data ------------------------------

		// Draw background
		drawList->AddRectFilled(canvasPos, canvasPos + canvasSize, canvasColor);

		// ---------------------- Setup and Handle Legend ------------------------------
		static float legendWidth = 0.2f;
		legendSize = ImVec2{ canvasSize.x * legendWidth, canvasSize.y };
		bool result = false;
		handleLegendSplitter(&legendWidth, mouseHoveringOverTimelineWindow);
		// ---------------------- Setup and Handle Legend ------------------------------

		// ---------------------- Draw Timeline Ruler ------------------------------	
		// TODO: Abstract this into one of the colors
		ImU32 grayColor = IM_COL32(32, 32, 32, 255);

		float scrollOffsetX = data->Scroll.x;
		float scrollOffsetY = data->Scroll.y;

		timelineRulerBegin = canvasPos + ImVec2(legendSize.x, 0.0f);
		timelineRulerEnd = canvasPos + ImVec2(canvasSize.x, timelineRulerHeight);
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
		float distanceBetweenSmallTicks = minDistanceBetweenRulerTimecodes / (float)numTicksBetweenBoundaries;
		float amountOfTimeVisibleInTimeline = (*zoom) * ((timelineRulerEnd.x - timelineRulerBegin.x) / (float)distanceBetweenSmallTicks);
		float amountOfFramesVisibleBetweenSmallTicks = (*zoom);

		float firstTimecodePosition = glm::floor((scrollOffsetX / minDistanceBetweenRulerTimecodes)) * minDistanceBetweenRulerTimecodes - scrollOffsetX;
		// Zoom will effect this soon
		int numTimecodesThatFit = (int)glm::ceil((timelineRulerEnd.x - timelineRulerBegin.x) / minDistanceBetweenRulerTimecodes);
		for (int timecodei = 0; timecodei <= numTimecodesThatFit; timecodei++)
		{
			ImVec2 tickStart = ImVec2(firstTimecodePosition + timecodei * minDistanceBetweenRulerTimecodes, 0.0f);
			tickStart += timelineRulerBegin;
			ImVec2 tickEnd = tickStart + ImVec2(tickWidth, boundaryTickHeight);
			if (tickStart.x >= timelineRulerBegin.x)
			{
				drawList->AddRectFilled(tickStart, tickEnd, boundaryTickColor);
			}

			{
				// Draw the time code string
				int firstTickPos = (int)glm::floor((scrollOffsetX / minDistanceBetweenRulerTimecodes));
				int numFramesToThisPos = (int)((float)((firstTickPos + timecodei) * numTicksBetweenBoundaries) * amountOfFramesVisibleBetweenSmallTicks);

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

			ImVec2 smallTickStart = tickStart;
			for (int tickBoundaryi = 0; tickBoundaryi < numTicksBetweenBoundaries; tickBoundaryi++)
			{
				smallTickStart.x += distanceBetweenSmallTicks;
				ImVec2 smallTickEnd = smallTickStart + ImVec2(tickWidth, 0.0f);
				smallTickEnd.y += (tickBoundaryi + 1) == tickMidpoint
					? largeTickHeight
					: tickBoundaryi % 2 == 1
					? smallTickHeight
					: mediumTickHeight;
				const ImU32& color = (tickBoundaryi + 1) == tickMidpoint
					? largeTickColor
					: defaultTickColor;

				if (smallTickStart.x > timelineRulerBegin.x)
				{
					drawList->AddRectFilled(smallTickStart, smallTickEnd, color);
				}
			}
		}
		// ---------------------- End Draw Timeline Ruler ------------------------------

		// ---------------------- Handle horizontal/vertical scrolling ----------------------
		// Handle horizontal scrolling while over the timeline or timeline ruler
		if (ImGui::IsMouseHoveringRect(timelineRulerBegin, ImVec2(timelineRulerEnd.x, canvasPos.y + canvasSize.y)) && mouseHoveringOverTimelineWindow)
		{
			if (io.MouseWheel != 0.0f && io.KeyCtrl)
			{
				scrollOffsetX -= io.MouseWheel * timelineHorizontalScrollSensitivity;
				scrollOffsetX = glm::max(scrollOffsetX, 0.0f);

				float normalizedScrollDistance = scrollOffsetX / (timelineRulerEnd.x - timelineRulerBegin.x);
				*firstFrame = (int)(amountOfTimeVisibleInTimeline * normalizedScrollDistance);
				res.flags |= ImGuiTimelineResultFlags_FirstFrameChanged;
			}
		}

		// Handle vertical scrolling while over the tracks area
		if (ImGui::IsMouseHoveringRect(canvasPos + ImVec2(0, timelineRulerHeight), ImVec2(timelineRulerEnd.x, canvasPos.y + canvasSize.y)) && mouseHoveringOverTimelineWindow)
		{
			if (io.MouseWheel != 0.0f && !io.KeyCtrl)
			{
				scrollOffsetY -= io.MouseWheel * timelineVerticalScrollSensitivity;
				scrollOffsetY = glm::max(scrollOffsetY, 0.0f);
			}
		}
		// ---------------------- End Handle horizontal/vertical scrolling ----------------------

		// ---------------------- Draw Timeline Elements ------------------------------
		// Draw all the track backgrounds giving a darker background to any sub-tracks
		{
			float absTracksTop = canvasPos.y + timelineRulerHeight;
			float currentTrackTop = canvasPos.y + timelineRulerHeight - scrollOffsetY;
			for (int i = 0; i < numTracks; i++)
			{
				currentTrackTop += trackHeight;

				if (tracks[i].isExpanded)
				{
					if (currentTrackTop + trackHeight > absTracksTop)
					{
						float realTop = glm::max(currentTrackTop, absTracksTop);
						drawList->AddRectFilled(
							ImVec2(canvasPos.x + legendSize.x, realTop),
							ImVec2(canvasPos.x + canvasSize.x, currentTrackTop + trackHeight),
							timelineTrackDark
						);
					}

					currentTrackTop += trackHeight;
				}
			}
		}

		for (int i = 0; i <= numTimecodesThatFit; i++)
		{
			ImVec2 tickStart = ImVec2(firstTimecodePosition + i * minDistanceBetweenRulerTimecodes, 0.0f);
			tickStart += timelineRulerBegin;
			ImVec2 tickEnd = tickStart + ImVec2(tickWidth, boundaryTickHeight);

			ImVec2 rectStart = ImVec2(tickStart.x, canvasPos.y + timelineRulerHeight);
			// Draw the boundary lines on the main timeline
			drawList->AddRectFilled(
				rectStart,
				rectStart + ImVec2(tickWidth, canvasSize.y),
				grayColor);
		}

		// Draw/Handle the segments and their logic
		ImVec2 timelineSize = (canvasPos + canvasSize) - (timelineRulerBegin + ImVec2(0.0f, timelineRulerHeight));
		drawList->PushClipRect(timelineRulerBegin + ImVec2(0.0f, timelineRulerHeight), canvasPos + canvasSize, true);
		{
			bool mouseClickedSomewhereOnTimeline = ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
				ImGui::IsMouseHoveringRect(
					timelineRulerBegin + ImVec2(0.0f, timelineRulerHeight),
					canvasPos + canvasSize
				) && mouseHoveringOverTimelineWindow;

			float absTracksTop = canvasPos.y + timelineRulerHeight;
			float currentTrackTop = canvasPos.y + timelineRulerHeight - scrollOffsetY;
			for (int i = 0; i < numTracks; i++)
			{
				float trackTopY = currentTrackTop;
				float trackBottomY = currentTrackTop + trackHeight;

				bool shouldDrawTrack = true;
				if (trackBottomY <= absTracksTop)
				{
					shouldDrawTrack = false;
				}
				else if (trackTopY < absTracksTop)
				{
					trackTopY = absTracksTop;
				}

				float realTrackHeight = trackBottomY - trackTopY;

				for (int si = 0; si < tracks[i].numSegments; si++)
				{
					ImGuiTimeline_Segment& segment = tracks[i].segments[si];

					std::string strId = "Track_" + std::to_string(i) + "Segment_" + std::to_string(si);
					ImGuiID segmentID = ImHashStr(strId.c_str(), strId.size());
					static ImGuiID activeSegmentID = UINT32_MAX;

					float offsetX = calculateSegmentOffset(segment.frameStart, *firstFrame, amountOfTimeVisibleInTimeline);
					float width = calculateSegmentWidth(segment.frameDuration, amountOfTimeVisibleInTimeline);
					// Check if segment intersects with the timeline
					// We can check if:
					//    segmentEnd >= timelineBegin && segmentBegin <= timelineEnd
					if (shouldDrawTrack &&
						segment.frameStart <= ((int)amountOfTimeVisibleInTimeline + *firstFrame) &&
						segment.frameStart + segment.frameDuration >= *firstFrame)
					{
						// Clamp values as necessary
						if (offsetX < 0.0f)
						{
							width = width + offsetX;
							offsetX = 0.0f;
						}

						// Calculate the width, and truncate if necessary
						if (offsetX + width > canvasSize.x - legendSize.x)
						{
							width = canvasSize.x - legendSize.x - offsetX;
						}

						ImVec2 segmentStart = ImVec2(canvasPos.x + legendSize.x + offsetX, trackTopY);
						ImVec2 segmentEnd = ImVec2(segmentStart.x + width, trackBottomY);
						SegmentChangeType changeType;

						// Only handleSegment logic if the window is actually hovered
						if (mouseHoveringOverTimelineWindow && handleSegment(segmentStart, segmentEnd, &segment, &changeType, segmentID, timelineSize, amountOfTimeVisibleInTimeline))
						{
							offsetX = calculateSegmentOffset(segment.frameStart, *firstFrame, amountOfTimeVisibleInTimeline);
							width = calculateSegmentWidth(segment.frameDuration, amountOfTimeVisibleInTimeline);

							// ------------ Handle magnet snapping segments ------------
							if (magnetEnabled && si > 0)
							{
								const ImGuiTimeline_Segment& lastSegment = tracks[i].segments[si - 1];
								snapToPreviousSegment(firstFrame, &offsetX,
									&segment.frameStart, &segment.frameDuration,
									lastSegment.frameStart, lastSegment.frameDuration,
									changeType, amountOfTimeVisibleInTimeline);
							}

							if (magnetEnabled && si < tracks[i].numSegments - 1)
							{
								const ImGuiTimeline_Segment& nextSegment = tracks[i].segments[si + 1];
								snapToNextSegment(firstFrame, &offsetX, &width,
									&segment.frameStart, &segment.frameDuration,
									nextSegment.frameStart, nextSegment.frameDuration,
									changeType, amountOfTimeVisibleInTimeline);
							}

							if (magnetEnabled)
							{
								snapToCursor(currentFrame, firstFrame,
									&segment.frameStart, &segment.frameDuration,
									&offsetX, &width, amountOfTimeVisibleInTimeline, changeType);
							}
							// ------------ End Magnet handling stuff ------------

							g_logger_assert(res.segmentIndex == -1, "Invalid result. User somehow modified two segments at once.");
							res.flags |= ImGuiTimelineResultFlags_SegmentTimeChanged;
							res.segmentIndex = si;
							res.trackIndex = i;
							if (activeSegmentID != segmentID)
							{
								res.flags |= ImGuiTimelineResultFlags_ActiveObjectChanged;
								res.activeObjectIsSubSegment = false;
								activeSegmentID = segmentID;
							}

							// Adjust the segment start and end to the new positions
							offsetX = calculateSegmentOffset(segment.frameStart, *firstFrame, amountOfTimeVisibleInTimeline);
							width = calculateSegmentWidth(segment.frameDuration, amountOfTimeVisibleInTimeline);
							segmentStart = ImVec2(canvasPos.x + legendSize.x + offsetX, trackTopY);
							segmentEnd = ImVec2(segmentStart.x + width, trackBottomY);
						}
						else if (activeSegmentID == segmentID && mouseClickedSomewhereOnTimeline)
						{
							res.flags |= ImGuiTimelineResultFlags_ActiveObjectDeselected;
							activeSegmentID = UINT32_MAX;
						}

						// Draw the segment
						drawList->AddRectFilled(segmentStart, segmentEnd, segmentColor, 10.0f);
						if (activeSegmentID == segmentID)
						{
							drawList->AddRect(segmentStart, segmentEnd, cursorColor, 10.0f, 0, 4.0f);

							if (mouseHoveringOverTimelineWindow && ImGui::IsKeyPressed(ImGuiKey_Delete))
							{
								res.flags |= ImGuiTimelineResultFlags_DeleteActiveObject;
								res.flags |= ImGuiTimelineResultFlags_ActiveObjectDeselected;
								res.activeObjectIsSubSegment = false;
								res.segmentIndex = si;
								res.trackIndex = i;
								activeSegmentID = UINT32_MAX;
								segmentDragID = UINT32_MAX;
							}
						}
						ImVec2 borderStart = segmentStart + ImVec2(0.0f, realTrackHeight - segmentTextAreaHeight);
						ImVec2 borderEnd = ImVec2(segmentEnd.x, borderStart.y + 3.0f);
						drawList->AddRectFilled(borderStart, borderEnd, segmentDarkColor);
						ImVec2 textPosition = segmentStart + ImVec2(0.0f, realTrackHeight - segmentTextAreaHeight) + segmentTextAreaPadding;
						// Vertically center the text
						ImVec2 segmentTextSize = ImGui::CalcTextSize(segment.segmentName);
						textPosition.y += (segmentTextAreaHeight - segmentTextSize.y - segmentTextAreaPadding.y * 2.0f) / 2.0f;
						drawList->AddText(textPosition, ImColor(fontColor), segment.segmentName);
					} // End segment intersects with timeline check

					if (tracks[i].isExpanded)
					{
						float subTrackTopY = trackTopY + (float)realTrackHeight;
						float subTrackBottomY = subTrackTopY + (float)trackHeight;

						bool shouldDrawSubtrack = true;
						if (subTrackBottomY <= absTracksTop)
						{
							shouldDrawSubtrack = false;
						}
						else if (subTrackTopY < absTracksTop)
						{
							subTrackTopY = absTracksTop;
						}

						float realSubTrackHeight = subTrackBottomY - subTrackTopY;

						// Draw/handle the expanded segments and their logic
						for (int subSegmenti = 0; subSegmenti < segment.numSubSegments; subSegmenti++)
						{
							ImGuiTimeline_SubSegment& subSegment = segment.subSegments[subSegmenti];

							int subSegmentAbsFrameStart = subSegment.frameStart + segment.frameStart;
							float subOffsetX = calculateSegmentOffset(subSegmentAbsFrameStart, *firstFrame, amountOfTimeVisibleInTimeline);
							float subWidth = calculateSegmentWidth(subSegment.frameDuration, amountOfTimeVisibleInTimeline);
							// Check if segment intersects with the timeline
							// We can check if:
							//    segmentEnd >= timelineBegin && segmentBegin <= timelineEnd
							if (shouldDrawSubtrack &&
								subSegmentAbsFrameStart <= ((int)amountOfTimeVisibleInTimeline + *firstFrame) &&
								subSegmentAbsFrameStart + subSegment.frameDuration >= *firstFrame)
							{
								// Clamp values as necessary
								if (subOffsetX < 0.0f)
								{
									subWidth = subWidth + subOffsetX;
									subOffsetX = 0.0f;
								}

								// Calculate the width, and truncate if necessary
								if (subOffsetX + subWidth > canvasSize.x - legendSize.x)
								{
									subWidth = canvasSize.x - legendSize.x - subOffsetX;
								}

								ImVec2 subSegmentStart = ImVec2(canvasPos.x + legendSize.x + subOffsetX, subTrackTopY);
								ImVec2 subSegmentEnd = ImVec2(subSegmentStart.x + subWidth, subTrackBottomY);

								std::string subStrId = std::string("Track_") + std::to_string(i) + "_SubSegment_" + std::to_string(si) + "_" + std::to_string(subSegmenti);
								ImGuiID subSegmentID = ImHashStr(subStrId.c_str(), subStrId.size());
								SegmentChangeType subChangeType;

								// Same as above: Only handleSubSegment logic if the mouse is over the window
								if (mouseHoveringOverTimelineWindow && handleSubSegment(subSegmentStart, subSegmentEnd, &subSegment, &subChangeType, subSegmentID, timelineSize, amountOfTimeVisibleInTimeline))
								{
									// ------------ Handle magnet snapping segments ------------
									if (magnetEnabled && subSegmenti > 0)
									{
										const ImGuiTimeline_SubSegment& lastSubSegment = tracks[i].segments[si].subSegments[subSegmenti - 1];
										snapToPreviousSegment(firstFrame, &subOffsetX,
											&subSegment.frameStart, &subSegment.frameDuration,
											lastSubSegment.frameStart, lastSubSegment.frameDuration,
											subChangeType, amountOfTimeVisibleInTimeline);
									}

									if (magnetEnabled && subSegmenti < tracks[i].segments[si].numSubSegments - 1)
									{
										const ImGuiTimeline_SubSegment& nextSubSegment = tracks[i].segments[si].subSegments[subSegmenti + 1];
										snapToNextSegment(firstFrame, &subOffsetX, &subWidth,
											&subSegment.frameStart, &subSegment.frameDuration,
											nextSubSegment.frameStart, nextSubSegment.frameDuration,
											subChangeType, amountOfTimeVisibleInTimeline);
									}

									if (magnetEnabled)
									{
										snapToCursor(currentFrame, firstFrame,
											&subSegment.frameStart, &subSegment.frameDuration,
											&subOffsetX, &subWidth, amountOfTimeVisibleInTimeline, subChangeType);
									}
									// ------------ End Magnet handling stuff ------------

									// If subsegment is dragged beyond the end of it's parent segment, re-adjust the start
									// so that it clamps it to the end
									if (subSegment.frameStart + subSegment.frameDuration > segment.frameDuration)
									{
										subSegment.frameStart = segment.frameDuration - subSegment.frameDuration;
									}

									g_logger_assert(res.segmentIndex == -1, "Invalid result. User somehow modified two segments at once.");
									res.flags |= ImGuiTimelineResultFlags_SubSegmentTimeChanged;
									res.segmentIndex = si;
									res.subSegmentIndex = subSegmenti;
									res.trackIndex = i;
									if (activeSegmentID != subSegmentID)
									{
										res.flags |= ImGuiTimelineResultFlags_ActiveObjectChanged;
										res.activeObjectIsSubSegment = true;
										activeSegmentID = subSegmentID;
									}
								}
								else if (activeSegmentID == subSegmentID && mouseClickedSomewhereOnTimeline)
								{
									res.flags |= ImGuiTimelineResultFlags_ActiveObjectDeselected;
									activeSegmentID = UINT32_MAX;
								}

								if (shouldDrawSubtrack)
								{
									// Draw the segment
									drawList->AddRectFilled(subSegmentStart, subSegmentEnd, subSegmentColor, 10.0f);
									drawList->AddRect(subSegmentStart, subSegmentEnd, subSegmentDarkColor, 10.0f);
									if (activeSegmentID == subSegmentID)
									{
										drawList->AddRect(subSegmentStart, subSegmentEnd, cursorColor, 10.0f, 0, 4.0f);

										if (mouseHoveringOverTimelineWindow && ImGui::IsKeyPressed(ImGuiKey_Delete))
										{
											res.flags |= ImGuiTimelineResultFlags_DeleteActiveObject;
											res.flags |= ImGuiTimelineResultFlags_ActiveObjectDeselected;
											res.activeObjectIsSubSegment = true;
											res.segmentIndex = si;
											res.subSegmentIndex = subSegmenti;
											res.trackIndex = i;
											activeSegmentID = UINT32_MAX;
											segmentDragID = UINT32_MAX;
										}
									}
									ImVec2 borderStart = subSegmentStart + ImVec2(0.0f, realSubTrackHeight - segmentTextAreaHeight);
									ImVec2 borderEnd = ImVec2(subSegmentEnd.x, borderStart.y + 3.0f);
									drawList->AddRectFilled(borderStart, borderEnd, subSegmentDarkColor);
									ImVec2 textPosition = subSegmentStart + ImVec2(0.0f, realSubTrackHeight - segmentTextAreaHeight) + segmentTextAreaPadding;
									// Vertically center the text
									ImVec2 segmentTextSize = ImGui::CalcTextSize(subSegment.segmentName);
									textPosition.y += (segmentTextAreaHeight - segmentTextSize.y - segmentTextAreaPadding.y * 2.0f) / 2.0f;
									drawList->AddText(textPosition, ImColor(fontColor), subSegment.segmentName);
								}
							} // End sub-segment intersects with timeline check
						} // End sub-segment loop
					} // End segment.isExpanded check
				} // End segment loop

				if (tracks[i].isExpanded)
				{
					// Draw/handle the expanded segments and their logic
					currentTrackTop += trackHeight;
				}

				currentTrackTop += trackHeight;
			} // End track loop
		}
		drawList->PopClipRect();

		// ---------------------- End Draw Timeline Elements ------------------------------


		// ---------------------- Draw Preview Audio Waveform ------------------------------
		if (audioData != nullptr)
		{
			// TODO: Make this configurable
			// 60 FPS
			float amountOfSecondsVisibleInTimeline = amountOfTimeVisibleInTimeline / 60.0f;
			float currentSecond = (float)(*firstFrame) / 60.0f;
			// The divide by blockAlignment * blockAlignment makes sure we're properly byte aligned
			uint32 firstSampleByte = (uint32)(((float)audioData->bytesPerSec * currentSecond) / (float)audioData->blockAlignment) * (uint32)(audioData->blockAlignment);
			uint32 bytesPerSample = audioData->bitsPerSample / 8;
			uint32 numBytesVisible = (uint32)((float)audioData->bytesPerSec * amountOfSecondsVisibleInTimeline);
			uint32 end = glm::min(firstSampleByte + numBytesVisible, audioData->dataSize);
			g_logger_assert(bytesPerSample == 1 || bytesPerSample == 2, "Need 1 or 2 bytes per sample for audio data. Instead got %d bytes per sample", bytesPerSample);

			constexpr float distanceBetweenLineSegments = 1.0f;
			// TODO: Make this configurable
			constexpr float amplitudeAdjustment = 1.3f;
			float amtTimeVisibleInLineSegment = amountOfSecondsVisibleInTimeline * (distanceBetweenLineSegments / (timelineRulerEnd.x - timelineRulerBegin.x));
			uint32 numBytesVisibleInLineSegment = (uint32)((float)amtTimeVisibleInLineSegment * (float)audioData->bytesPerSec);

			ImVec2 lastMaxSegmentPos = ImVec2(0.0f, (float)trackHeight / 2.0f);
			ImVec2 lastMinSegmentPos = lastMaxSegmentPos;

			// Draw background for the audio track preview
			drawList->AddRectFilled(
				canvasPos + canvasSize - ImVec2(timelineRulerEnd.x - timelineRulerBegin.x, (float)trackHeight),
				canvasPos + canvasSize,
				canvasColor
			);

			// TODO: Spreading out the processing of audio data is pretty complex
			// maybe I should come up with some sort of multithreading promise system?
			// That way I could process in the background and when the promise is ready
			// switch to the new cache
			static int cachedFirstFrame = -1;
			static float cachedZoomLevel = -1.0f;
			static float cachedAmtTimeVisibleInTimeline = -1.0f;
			static bool isProcessingAudioWaveform = false;
			static ImVec2 lastMaxSegmentPosPrevFrame;
			static ImVec2 lastMinSegmentPosPrevFrame;
			static uint32 byteFromLastFrame;
			bool firstLine = !isProcessingAudioWaveform;

			if (cachedFirstFrame != *firstFrame || cachedZoomLevel != *zoom || cachedAmtTimeVisibleInTimeline != amountOfTimeVisibleInTimeline)
			{
				isProcessingAudioWaveform = true;
				cachedFirstFrame = *firstFrame;
				cachedAmtTimeVisibleInTimeline = amountOfTimeVisibleInTimeline;
				cachedZoomLevel = *zoom;
				lengthAudioCache = 0;
				byteFromLastFrame = firstSampleByte;
			}
			else if (isProcessingAudioWaveform)
			{
				lastMaxSegmentPos = lastMaxSegmentPosPrevFrame;
				lastMinSegmentPos = lastMinSegmentPosPrevFrame;
			}

			if (isProcessingAudioWaveform)
			{
				bool processedAllData = true;

				//g_logger_info("Recaching audio data.");
				// When we cache the data, if there's a lot of audio to process
				// spread the processing over a few frames to minimize lag
				constexpr int maxBytesToProcessInOneFrame = 500'000;
				int numBytesProcessed = 0;

				// Average the sound samples over the preview period and cache the results
				for (uint32 byte = byteFromLastFrame; byte < end; byte += audioData->blockAlignment)
				{
					uint32 endByte = glm::min(byte + numBytesVisibleInLineSegment, audioData->dataSize);
					float maxSample = 0.0f;
					float minSample = 0.0f;
					for (; byte < endByte; byte += audioData->blockAlignment)
					{
						int16 sample = bytesPerSample == 1
							? (int16)(audioData->data[byte])
							: *(int16*)(&audioData->data[byte]);
						float normalizedSample = (float)(sample) / (float)(INT16_MAX);
						maxSample = glm::max(maxSample, normalizedSample);
						minSample = glm::min(normalizedSample, minSample);

						numBytesProcessed += audioData->blockAlignment;
					}

					maxSample = glm::clamp(amplitudeAdjustment * maxSample, 0.0f, 1.0f);
					minSample = glm::clamp(amplitudeAdjustment * minSample, -1.0f, 0.0f);

					maxSample = 1.0f - ((maxSample + 1.0f) / 2.0f);
					minSample = 1.0f - ((minSample + 1.0f) / 2.0f);

					// Draw the line segment
					ImVec2 nextMaxPos = ImVec2(lastMaxSegmentPos.x + distanceBetweenLineSegments, maxSample * (trackHeight / 1.2f));
					ImVec2 nextMinPos = ImVec2(lastMinSegmentPos.x + distanceBetweenLineSegments, minSample * (trackHeight / 1.2f));
					if (firstLine)
					{
						lastMaxSegmentPos.y = nextMaxPos.y;
						lastMinSegmentPos.y = nextMinPos.y;
						firstLine = false;
					}

					// Get all the points necessary to fill the shape
					float baselineY = ((trackHeight / 1.2f) / 2.0f);
					// Draw max waveform point triangles
					{
						ImVec2 p1 = lastMaxSegmentPos;
						ImVec2 p2 = nextMaxPos;
						if (p1.y < baselineY && p2.y < baselineY && !glm::epsilonEqual(p1.y, baselineY, 1.0f) && !glm::epsilonEqual(p2.y, baselineY, 1.0f))
						{
							ImVec2 p0 = ImVec2(lastMaxSegmentPos.x, baselineY);
							ImVec2 p3 = ImVec2(nextMaxPos.x, baselineY);

							checkAudioCacheResize(6);
							audioCache[lengthAudioCache + 0] = p0;
							audioCache[lengthAudioCache + 1] = p1;
							audioCache[lengthAudioCache + 2] = p2;

							audioCache[lengthAudioCache + 3] = p0;
							audioCache[lengthAudioCache + 4] = p2;
							audioCache[lengthAudioCache + 5] = p3;
							lengthAudioCache += 6;
						}
					}
					// Draw min waveform point triangles
					{
						ImVec2 p1 = lastMinSegmentPos;
						ImVec2 p2 = nextMinPos;
						if (p1.y > baselineY && p2.y > baselineY && !glm::epsilonEqual(p1.y, baselineY, 1.0f) && !glm::epsilonEqual(p2.y, baselineY, 1.0f))
						{
							ImVec2 p0 = ImVec2(lastMinSegmentPos.x, baselineY);
							ImVec2 p3 = ImVec2(nextMinPos.x, baselineY);

							checkAudioCacheResize(6);
							audioCache[lengthAudioCache + 0] = p2;
							audioCache[lengthAudioCache + 1] = p1;
							audioCache[lengthAudioCache + 2] = p0;

							audioCache[lengthAudioCache + 3] = p3;
							audioCache[lengthAudioCache + 4] = p2;
							audioCache[lengthAudioCache + 5] = p0;
							lengthAudioCache += 6;
						}
					}

					// Keep track of the last points
					lastMaxSegmentPos = nextMaxPos;
					lastMinSegmentPos = nextMinPos;

					if (numBytesProcessed >= maxBytesToProcessInOneFrame)
					{
						lastMaxSegmentPosPrevFrame = lastMaxSegmentPos;
						lastMinSegmentPosPrevFrame = lastMinSegmentPos;
						processedAllData = false;
						byteFromLastFrame = byte + audioData->blockAlignment;
						break;
					}
				}

				isProcessingAudioWaveform = !processedAllData;
			}

			// Draw the cache 
			// TODO: I'm not sure if it looks cool or weird drawing the half-cached audio data
			// Should we display a loading screen here instead while it's loading the cache?
			ImVec2 audioTrackPos = (canvasPos + canvasSize) - ImVec2(timelineRulerEnd.x - timelineRulerBegin.x, trackHeight);
			for (int audioi = 0; audioi < lengthAudioCache - 3; audioi++)
			{
				drawList->AddTriangleFilled(
					audioTrackPos + audioCache[audioi],
					audioTrackPos + audioCache[audioi + 1],
					audioTrackPos + audioCache[audioi + 2],
					subSegmentColor
				);
			}
		}
		// ---------------------- End Draw Preview Audio Waveform ------------------------------

		// ---------------------- Handle Timeline Cursor ------------------------------
		{
			static DragState timelineDragging = DragState::None;
			if (mouseHoveringOverTimelineWindow && ImGui::IsMouseHoveringRect(timelineRulerBegin, timelineRulerEnd))
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
					*firstFrame -= glm::max((int)(3.0f * (*zoom)), 1);
					*firstFrame = glm::max(*firstFrame, 0);
					scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame);
					res.flags |= ImGuiTimelineResultFlags_FirstFrameChanged;
				}
				else if (*currentFrame > *firstFrame + (int)amountOfTimeVisibleInTimeline)
				{
					*firstFrame += glm::max((int)(3.0f * (*zoom)), 1);
					scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame);
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
			ImVec2 cursorStart = timelineRulerBegin;
			cursorStart.x += calculateCursorOffset(*currentFrame, *firstFrame, amountOfTimeVisibleInTimeline);

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
			// Advance 3/4 every time
			int numFramesToAdvance = (int)(amountOfTimeVisibleInTimeline * 3.0f / 4.0f);
			if (*currentFrame < *firstFrame)
			{
				*firstFrame -= numFramesToAdvance;
				changed = true;
			}
			else if (*currentFrame > (*firstFrame + (int)amountOfTimeVisibleInTimeline))
			{
				*firstFrame += numFramesToAdvance;
				changed = true;
			}

			if (changed)
			{
				scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame);
				res.flags |= ImGuiTimelineResultFlags_FirstFrameChanged;
			}
		}
		// ---------------------- End Follow Timeline Cursor Logic ------------------------------

		// ---------------------- Draw/Handle Legend ------------------------------
		{
			drawList->AddRectFilled(canvasPos, canvasPos + legendSize, legendBackground);
			// Draw the current time in hours:minutes:seconds.milliseconds
			ImVec2 timecodeRectSize = ImVec2(legendSize.x, timelineRulerHeight);
			drawList->AddRect(canvasPos, canvasPos + timecodeRectSize, IM_COL32(0, 0, 0, 255), 0, 0, 1.5f);

			// Max formatted string length is HH:MM:SS:ff
			{
				static char strBuffer[] = "HH:MM:SS:ff";
				size_t strBufferSize = sizeof(strBuffer) / sizeof(strBuffer[0]);
				framesToTimeStr(strBuffer, strBufferSize, *currentFrame);

				ImGui::PushFont(ImGuiLayer::getMediumFont());
				ImVec2 textSize = ImGui::CalcTextSize(strBuffer, strBuffer + strBufferSize - 1);
				ImVec2 textPos = canvasPos + ((timecodeRectSize - textSize) / 2.0f);
				drawList->AddText(textPos, ImColor(fontColor), strBuffer, strBuffer + strBufferSize - 1);
				ImGui::PopFont();
			}

			// Draw all the track labels
			float currentTrackTop = canvasPos.y + timelineRulerHeight - scrollOffsetY;
			ImGui::PushClipRect(canvasPos + ImVec2(0.0f, timelineRulerHeight), canvasPos + legendSize, true);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
			for (int i = 0; i < numTracks; i++)
			{
				ImVec2 textSize = ImGui::CalcTextSize(tracks[i].trackName);
				float offsetY = currentTrackTop + (trackHeight - textSize.y) / 2.0f;

				drawList->AddText(ImVec2(canvasPos.x + trackNamePadding, offsetY), ImColor(fontColor), tracks[i].trackName);

				ImVec2 legendTrackNameTop = ImVec2(canvasPos.x, currentTrackTop);
				ImVec2 legendTrackNameBottom = ImVec2(
					canvasPos.x + legendSize.x - trackNamePadding * 2.0f - expandButtonSize.x,
					currentTrackTop + trackHeight + 1.5f
				);
				ImVec2 fullTracknameBottom = legendTrackNameBottom + ImVec2(trackNamePadding * 2.0f + expandButtonSize.x, 0.0f);

				if (mouseHoveringOverTimelineWindow && ImGui::IsMouseHoveringRect(legendTrackNameTop, fullTracknameBottom))
				{
					// NOTE(voxel): Very Subtle Hover Highlight
					drawList->AddRect(legendTrackNameTop, fullTracknameBottom, legendBorderHighlightColor);
				}
				else
				{
					// Draw border top and bottom
					drawList->AddRect(
						legendTrackNameTop,
						ImVec2(canvasPos.x + legendSize.x, currentTrackTop + 1.5f),
						legendBorderColor
					);

					drawList->AddRect(
						ImVec2(canvasPos.x, currentTrackTop + trackHeight),
						fullTracknameBottom,
						legendBorderColor
					);
				}

				// Handle dropdown arrow logic
				float legendTrackNameWidth = legendTrackNameBottom.x - legendTrackNameTop.x;
				ImVec2 expandArrowRectStart = legendTrackNameTop + ImVec2(legendTrackNameWidth, 0.0f);
				ImVec2 expandArrowRectEnd = legendTrackNameTop + ImVec2(legendSize.x, trackHeight);
				ImVec2 expandArrowRectSize = expandArrowRectEnd - expandArrowRectStart;
				ImVec2 expandButtonStart = expandArrowRectStart + (expandArrowRectSize - expandButtonSize) / 2.0f;
				if (tracks[i].isExpanded)
				{
					drawList->AddTriangleFilled(
						expandButtonStart + ImVec2(0.0f, expandButtonSize.y),
						expandButtonStart + expandButtonSize,
						expandButtonStart + ImVec2(expandButtonSize.x / 2.0f, 0.0f),
						dropdownArrowColor
					);
				}
				else
				{
					drawList->AddTriangleFilled(
						expandButtonStart,
						expandButtonStart + ImVec2(expandButtonSize.x, 0.0f),
						expandButtonStart + ImVec2(expandButtonSize.x / 2.0f, expandButtonSize.y),
						dropdownArrowColor
					);
				}

				// Check if the user clicked the segment arrow to expand or un-expand it
				if (mouseHoveringOverTimelineWindow && ImGui::IsMouseHoveringRect(legendTrackNameTop, fullTracknameBottom) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					tracks[i].isExpanded = !tracks[i].isExpanded;
				}

				// Do this check *after* handling logic so there's no frame lagging when you click
				if (tracks[i].isExpanded)
				{
					currentTrackTop += trackHeight;
				}

				currentTrackTop += trackHeight;

				// Handle right clicking on the legend by popping up context menu
				std::string id = std::string("TrackName_") + std::to_string(i) + tracks[i].trackName;

				if (beginPopupContextTimelineItem(id.c_str(), legendTrackNameTop, legendTrackNameBottom))
				{
					if (ImGui::MenuItem("Add Track Above"))
					{
						res.flags |= ImGuiTimelineResultFlags_AddTrackClicked;
						res.trackIndex = glm::max(i - 1, 0);
					}
					if (ImGui::MenuItem("Add Track Below"))
					{
						res.flags |= ImGuiTimelineResultFlags_AddTrackClicked;
						res.trackIndex = i + 1;
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Delete Track"))
					{
						res.flags |= ImGuiTimelineResultFlags_DeleteTrackClicked;
						res.trackIndex = i;
					}

					ImGui::EndPopup();
				}
			}
			ImGui::PopClipRect();


			// Check if they right clicked in the empty area below track names
			ImVec2 legendTrackNameTop = ImVec2(canvasPos.x, currentTrackTop);
			ImVec2 legendTrackNameBottom = ImVec2(canvasPos.x + legendSize.x, canvasPos.y + canvasSize.y);
			if (beginPopupContextTimelineItem("Track_Empty_Legend_Area", legendTrackNameTop, legendTrackNameBottom))
			{
				if (ImGui::MenuItem("Add Track"))
				{
					res.flags |= ImGuiTimelineResultFlags_AddTrackClicked;
					res.trackIndex = numTracks;
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Add Audio Source"))
				{
					res.flags |= ImGuiTimelineResultFlags_AddAudioSource;
				}

				if (ImGui::MenuItem("Delete Audio Source"))
				{
					res.flags |= ImGuiTimelineResultFlags_DeleteAudioSource;
				}

				ImGui::EndPopup();
			}
			ImGui::PopStyleVar();
		}
		// ---------------------- End Legend ------------------------------

		// ---------------------- Handle Drag Drop Target ------------------------------
		{
			float absTracksTop = canvasPos.y + timelineRulerHeight;
			float currentTrackTop = canvasPos.y + timelineRulerHeight - scrollOffsetY;
			for (int i = 0; i < numTracks; i++)
			{
				float trackTopY = currentTrackTop;
				float trackBottomY = currentTrackTop + (float)trackHeight;

				bool shouldDrawTrack = true;
				if (trackBottomY <= absTracksTop)
				{
					shouldDrawTrack = false;
				}
				else if (trackTopY < absTracksTop)
				{
					trackTopY = absTracksTop;
				}

				std::string trackId = std::string("ImGuiTimelineTrack_ID_") + std::to_string(i);
				ImGuiID id = ImGui::GetID(trackId.c_str());
				ImVec2 trackTopLeft = ImVec2(canvasPos.x + legendSize.x, trackTopY);
				ImVec2 trackBottomRight = ImVec2(canvasPos.x + canvasSize.x, trackBottomY);
				ImGui::PushID(id);
				if (ImGui::IsMouseHoveringRect(trackTopLeft, trackBottomRight))
				{
					ImGui::SetLastItemData(id, ImGuiItemFlags_None, ImGuiItemStatusFlags_HoveredRect, ImRect(trackTopLeft, trackBottomRight));
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(TIMELINE_DRAG_DROP_SEGMENT_PAYLOAD_ID))
						{
							res.flags |= ImGuiTimelineResultFlags_DragDropPayloadHit;
							res.dragDropPayloadData = payload->Data;
							res.dragDropPayloadDataSize = payload->DataSize;
							float normalizedMousePos = (ImGui::GetMousePos().x - canvasPos.x - legendSize.x) / (timelineRulerEnd.x - timelineRulerBegin.x);
							res.dragDropPayloadFirstFrame = (int)(amountOfTimeVisibleInTimeline * normalizedMousePos) + *firstFrame;
							res.trackIndex = i;
							res.activeObjectIsSubSegment = false;
						}
						ImGui::EndDragDropTarget();
					}
				}
				ImGui::PopID();

				if (tracks[i].isExpanded)
				{
					// Draw/handle the expanded segments and their logic
					currentTrackTop += trackHeight;

					float subTrackTopY = currentTrackTop;
					float subTrackBottomY = subTrackTopY + (float)trackHeight;

					if (subTrackTopY < absTracksTop)
					{
						subTrackTopY = absTracksTop;
					}

					// TODO: Figure out a way to de-duplicate logic for sub-segments and segments
					std::string subTrackStr = std::string("ImGuiTimelineSubTrack_ID_") + std::to_string(i);
					ImGuiID subTrackId = ImGui::GetID(subTrackStr.c_str());
					ImVec2 subTrackTopLeft = ImVec2(canvasPos.x + legendSize.x, subTrackTopY);
					ImVec2 subTrackBottomRight = ImVec2(canvasPos.x + canvasSize.x, subTrackBottomY);
					ImGui::PushID(subTrackId);
					if (ImGui::IsMouseHoveringRect(subTrackTopLeft, subTrackBottomRight))
					{
						ImGui::SetLastItemData(id, ImGuiItemFlags_None, ImGuiItemStatusFlags_HoveredRect, ImRect(subTrackTopLeft, subTrackBottomRight));
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(TIMELINE_DRAG_DROP_SUB_SEGMENT_PAYLOAD_ID))
							{
								res.flags |= ImGuiTimelineResultFlags_DragDropPayloadHit;
								res.dragDropPayloadData = payload->Data;
								res.dragDropPayloadDataSize = payload->DataSize;
								float normalizedMousePos = (ImGui::GetMousePos().x - canvasPos.x - legendSize.x) / (timelineRulerEnd.x - timelineRulerBegin.x);
								int firstAbsoluteFrame = (int)(amountOfTimeVisibleInTimeline * normalizedMousePos) + *firstFrame;

								// Find the segment this is a part of
								res.trackIndex = i;
								int segmentIndex = findSegmentFromFrame(tracks, res.trackIndex, firstAbsoluteFrame);
								if (segmentIndex != -1)
								{
									res.segmentIndex = segmentIndex;
									res.dragDropPayloadFirstFrame = firstAbsoluteFrame - tracks[i].segments[segmentIndex].frameStart;
								}
								res.activeObjectIsSubSegment = true;
							}
							ImGui::EndDragDropTarget();
						}
					}
					ImGui::PopID();
				}

				currentTrackTop += trackHeight;
			}
		}
		// ---------------------- End Handle Drag Drop Target ------------------------------

		data->Scroll.x = scrollOffsetX;
		data->Scroll.y = scrollOffsetY;

		return res;
	}

	static bool handleLegendSplitter(float* legendWidth, bool mouseHovering)
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

		if (state == DragState::Hover && mouseHovering)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImGuiStyle& style = ImGui::GetStyle();
			constexpr float splitterRenderWidth = 4.0f;
			const ImVec4& splitterBgColor = style.Colors[ImGuiCol_FrameBgHovered];
			drawList->AddRectFilled(splitterBegin, splitterBegin + ImVec2(splitterRenderWidth, canvasSize.y), ImColor(splitterBgColor));
		}

		return state == DragState::Active;
	}

	const char* ImGuiTimeline_DragDropSegmentPayloadId()
	{
		return TIMELINE_DRAG_DROP_SEGMENT_PAYLOAD_ID;
	}

	const char* ImGuiTimeline_DragDropSubSegmentPayloadId()
	{
		return TIMELINE_DRAG_DROP_SUB_SEGMENT_PAYLOAD_ID;
	}

	void ImGuiTimeline_free()
	{
		if (audioCache != nullptr)
		{
			g_memory_free(audioCache);
			audioCache = nullptr;
		}

		maxLengthAudioCache = 0;
		lengthAudioCache = 0;
	}

	static bool handleResizeElement(float* currentValue, DragState* state, const ImVec2& valueBounds, const ImVec2& mouseBounds, const ImVec2& hoverRectStart, const ImVec2& hoverRectEnd, ResizeFlags flags)
	{
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

	static bool handleSegment(const ImVec2& segmentStart, const ImVec2& segmentEnd, ImGuiTimeline_Segment* segment, SegmentChangeType* changeType, ImGuiID segmentID, const ImVec2& timelineSize, float amountOfTimeVisibleInTimeline)
	{
		static DragState dragState = DragState::None;
		static DragState leftResizeState = DragState::None;
		static DragState rightResizeState = DragState::None;
		constexpr float resizeDragWidth = 15.0f;
		constexpr float halfResizeDragWidth = resizeDragWidth / 2.0f;
		static ImVec2 startDragPos = ImVec2();
		static int ogFrameStart = 0;
		static int ogFrameDuration = 0;

		bool changed = false;
		*changeType = SegmentChangeType::None;
		ImVec2 segmentStart_ = segmentStart + ImVec2(halfResizeDragWidth + 0.1f, 0.0f);
		ImVec2 segmentEnd_ = segmentEnd - ImVec2(halfResizeDragWidth - 0.1f, 0.0f);
		ImVec2 leftResizeStart = segmentStart - ImVec2(halfResizeDragWidth, 0.0f);
		ImVec2 leftResizeEnd = ImVec2(segmentStart_.x, segmentEnd_.y);
		ImVec2 rightResizeStart = ImVec2(segmentEnd_.x, segmentStart.y);
		ImVec2 rightResizeEnd = segmentEnd + ImVec2(halfResizeDragWidth, 0.0f);

		// Subtract the height of the text area at the bottom of the segment so
		// that you can click there without moving the segment
		segmentEnd_.y -= segmentTextAreaHeight;

		if (segmentDragID == UINT32_MAX || segmentDragID == segmentID)
		{
			handleDragState(segmentStart_, segmentEnd_, &dragState);
			handleDragState(leftResizeStart, leftResizeEnd, &leftResizeState);
			handleDragState(rightResizeStart, rightResizeEnd, &rightResizeState);
		}

		const ImGuiIO& io = ImGui::GetIO();

		// If nothing is being dragged, then check all segments
		if (segmentDragID == UINT32_MAX)
		{
			if (dragState != DragState::None)
			{
				segmentDragID = segmentID;
			}
			else if (leftResizeState != DragState::None)
			{
				segmentDragID = segmentID;
			}
			else if (rightResizeState != DragState::None)
			{
				segmentDragID = segmentID;
			}
		}

		if (segmentDragID != segmentID)
		{
			return false;
		}

		if (dragState == DragState::Hover)
		{
			startDragPos = io.MousePos;
			ogFrameStart = segment->frameStart;
		}

		if (leftResizeState == DragState::Hover || rightResizeState == DragState::Hover)
		{
			startDragPos = io.MousePos;
			ogFrameStart = segment->frameStart;
			ogFrameDuration = segment->frameDuration;
		}

		// Then handle based on the appropriate drag states
		if (segmentDragID != UINT32_MAX && segmentDragID == segmentID)
		{
			if (dragState == DragState::None && leftResizeState == DragState::None && rightResizeState == DragState::None)
			{
				segmentDragID = UINT32_MAX;
				return false;
			}

			// Get the mouse delta
			float delta = io.MousePos.x - startDragPos.x;
			float normalizedMouseDelta = delta / timelineSize.x;
			int frameChange = (int)(normalizedMouseDelta * amountOfTimeVisibleInTimeline);

			// Handle drag move cursor
			if (dragState != DragState::None)
			{
				if (dragState == DragState::Hover || dragState == DragState::Active)
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
				}

				if (dragState == DragState::Active)
				{
					segment->frameStart = ogFrameStart + frameChange;
					segment->frameStart = glm::max(segment->frameStart, 0);
					changed = true;
					*changeType = SegmentChangeType::Moved;
				}
			}
			// Handle left/right resize cursors
			else if (leftResizeState != DragState::None || rightResizeState != DragState::None)
			{
				if (leftResizeState == DragState::Hover || leftResizeState == DragState::Active ||
					rightResizeState == DragState::Hover || rightResizeState == DragState::Active)
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
				}

				if (leftResizeState == DragState::Active)
				{
					segment->frameStart = ogFrameStart + frameChange;
					segment->frameDuration = ogFrameDuration - frameChange;
					if (segment->frameStart < 0)
					{
						segment->frameDuration += segment->frameStart;
						segment->frameStart = 0;
					}

					if (segment->frameDuration <= 0)
					{
						segment->frameStart += segment->frameDuration;
						segment->frameDuration = 1;
					}

					changed = true;
					*changeType = SegmentChangeType::Resized;
				}

				if (rightResizeState == DragState::Active)
				{
					segment->frameDuration = ogFrameDuration + frameChange;

					if (segment->frameDuration <= 0)
					{
						segment->frameDuration = 1;
					}

					changed = true;
					*changeType = SegmentChangeType::Resized;
				}
			}
		}

		return changed;
	}

	static bool handleSubSegment(const ImVec2& segmentStart, const ImVec2& segmentEnd, ImGuiTimeline_SubSegment* segment, SegmentChangeType* changeType, ImGuiID segmentID, const ImVec2& timelineSize, float amountOfTimeVisibleInTimeline)
	{
		static DragState dragState = DragState::None;
		static DragState leftResizeState = DragState::None;
		static DragState rightResizeState = DragState::None;
		constexpr float resizeDragWidth = 15.0f;
		constexpr float halfResizeDragWidth = resizeDragWidth / 2.0f;
		static ImGuiID dragID = UINT32_MAX;
		static ImVec2 startDragPos = ImVec2();
		static int ogFrameStart = 0;
		static int ogFrameDuration = 0;

		bool changed = false;
		*changeType = SegmentChangeType::None;
		ImVec2 segmentStart_ = segmentStart + ImVec2(halfResizeDragWidth + 0.1f, 0.0f);
		ImVec2 segmentEnd_ = segmentEnd - ImVec2(halfResizeDragWidth - 0.1f, 0.0f);
		ImVec2 leftResizeStart = segmentStart - ImVec2(halfResizeDragWidth, 0.0f);
		ImVec2 leftResizeEnd = ImVec2(segmentStart_.x, segmentEnd_.y);
		ImVec2 rightResizeStart = ImVec2(segmentEnd_.x, segmentStart.y);
		ImVec2 rightResizeEnd = segmentEnd + ImVec2(halfResizeDragWidth, 0.0f);

		// Subtract the height of the text area at the bottom of the segment so
		// that you can click there without moving the segment
		segmentEnd_.y -= segmentTextAreaHeight;

		if (dragID == UINT32_MAX || dragID == segmentID)
		{
			handleDragState(segmentStart_, segmentEnd_, &dragState);
			handleDragState(leftResizeStart, leftResizeEnd, &leftResizeState);
			handleDragState(rightResizeStart, rightResizeEnd, &rightResizeState);
		}

		const ImGuiIO& io = ImGui::GetIO();

		// If nothing is being dragged, then check all segments
		if (dragID == UINT32_MAX)
		{
			if (dragState != DragState::None)
			{
				dragID = segmentID;
			}
			else if (leftResizeState != DragState::None)
			{
				dragID = segmentID;
			}
			else if (rightResizeState != DragState::None)
			{
				dragID = segmentID;
			}
		}

		if (dragID != segmentID)
		{
			return false;
		}

		if (dragState == DragState::Hover)
		{
			startDragPos = io.MousePos;
			ogFrameStart = segment->frameStart;
		}

		if (leftResizeState == DragState::Hover || rightResizeState == DragState::Hover)
		{
			startDragPos = io.MousePos;
			ogFrameStart = segment->frameStart;
			ogFrameDuration = segment->frameDuration;
		}

		// Then handle based on the appropriate drag states
		if (dragID != UINT32_MAX && dragID == segmentID)
		{
			if (dragState == DragState::None && leftResizeState == DragState::None && rightResizeState == DragState::None)
			{
				dragID = UINT32_MAX;
				return false;
			}

			// Get the mouse delta
			float delta = io.MousePos.x - startDragPos.x;
			float normalizedMouseDelta = delta / timelineSize.x;
			int frameChange = (int)(normalizedMouseDelta * amountOfTimeVisibleInTimeline);

			// Handle drag move cursor
			if (dragState != DragState::None)
			{
				if (dragState == DragState::Hover || dragState == DragState::Active)
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
				}

				if (dragState == DragState::Active)
				{
					segment->frameStart = ogFrameStart + frameChange;
					segment->frameStart = glm::max(segment->frameStart, 0);
					changed = true;
					*changeType = SegmentChangeType::Moved;
				}
			}
			// Handle left/right resize cursors
			else if (leftResizeState != DragState::None || rightResizeState != DragState::None)
			{
				if (leftResizeState == DragState::Hover || leftResizeState == DragState::Active ||
					rightResizeState == DragState::Hover || rightResizeState == DragState::Active)
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
				}

				if (leftResizeState == DragState::Active)
				{
					segment->frameStart = ogFrameStart + frameChange;
					segment->frameDuration = ogFrameDuration - frameChange;
					if (segment->frameStart < 0)
					{
						segment->frameDuration += segment->frameStart;
						segment->frameStart = 0;
					}

					if (segment->frameDuration <= 0)
					{
						segment->frameStart += segment->frameDuration;
						segment->frameDuration = 1;
					}

					changed = true;
					*changeType = SegmentChangeType::Resized;
				}

				if (rightResizeState == DragState::Active)
				{
					segment->frameDuration = ogFrameDuration + frameChange;

					if (segment->frameDuration <= 0)
					{
						segment->frameDuration = 1;
					}

					changed = true;
					*changeType = SegmentChangeType::Resized;
				}
			}
		}

		return changed;
	}

	static void framesToTimeStr(char* strBuffer, size_t strBufferLength, int frame)
	{
		int relativeFrame = frame % 60;
		int numSeconds = frame / 60;
		int numRelativeSeconds = numSeconds % 60;
		int numMinutes = numSeconds / 60;
		int numRelativeMinutes = numMinutes % 60;
		int numHours = numMinutes / 60;
		// Max formatted string length is HH:MM:SS:ff
		constexpr char requiredBuffer[] = "HH:MM:SS:ff";
		constexpr size_t requiredBufferSize = sizeof(requiredBuffer) / sizeof(strBuffer[0]);
		g_logger_assert(strBufferLength >= requiredBufferSize, "String buffer length must be at least '%d' characters.", requiredBufferSize);
		if (numHours < 100 && numHours >= 0)
		{
#ifdef sprintf_s
			sprintf_s(strBuffer, strBufferLength, "%02d:%02d:%02d.%02d", numHours, numRelativeMinutes, numRelativeSeconds, relativeFrame);
#else
			snprintf(strBuffer, strBufferLength, "%02d:%02d:%02d.%02d", numHours, numRelativeMinutes, numRelativeSeconds, relativeFrame);
#endif
		}
		else
		{
			g_logger_error("Uh oh.");
		}
	}

	static float getScrollFromFrame(float amountOfTimeVisibleInTimeline, int firstFrame)
	{
		float normalizedScrollDistance = (float)firstFrame / amountOfTimeVisibleInTimeline;
		float res = normalizedScrollDistance * (timelineRulerEnd.x - timelineRulerBegin.x);
		res = glm::max(res, 0.0f);
		return res;
	}

	static bool handleDragState(const ImVec2& start, const ImVec2& end, DragState* state)
	{
		bool changed = false;
		if (ImGui::IsMouseHoveringRect(start, end))
		{
			if (*state == DragState::None && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
			{
				*state = DragState::Hover;
				changed = true;
			}

			if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] && *state == DragState::Hover)
			{
				*state = DragState::Active;
				changed = true;
			}
		}
		else if (*state == DragState::Hover)
		{
			*state = DragState::None;
			changed = true;
		}

		if (*state == DragState::Active && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
		{
			*state = DragState::None;
			changed = true;
		}

		return changed;
	}

	static bool beginPopupContextTimelineItem(const char* str_id, const ImVec2& rectBegin, const ImVec2& rectEnd, ImGuiPopupFlags popup_flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;
		ImGuiID id = str_id ? window->GetID(str_id) : g.LastItemData.ID;    // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
		IM_ASSERT(id != 0);                                             // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
		int mouse_button = (popup_flags & ImGuiPopupFlags_MouseButtonMask_);
		if (ImGui::IsMouseReleased(mouse_button) && ImGui::IsMouseHoveringRect(rectBegin, rectEnd))
			ImGui::OpenPopupEx(id, popup_flags);
		return ImGui::BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
	}

	static int findSegmentFromFrame(const ImGuiTimeline_Track* tracks, int trackIndex, int firstAbsoluteFrame)
	{
		for (int si = 0; si < tracks[trackIndex].numSegments; si++)
		{
			const ImGuiTimeline_Segment& segment = tracks[trackIndex].segments[si];
			if (firstAbsoluteFrame >= segment.frameStart &&
				segment.frameStart + segment.frameDuration > firstAbsoluteFrame)
			{
				return si;
			}
		}

		return -1;
	}

	static void snapToCursor(int* currentFrame, int* firstFrame, int* segmentFrameStart, int* segmentFrameDuration, float* offsetX, float* width, float amountOfTimeVisibleInTimeline, SegmentChangeType changeType)
	{
		*offsetX = calculateSegmentOffset(*segmentFrameStart, *firstFrame, amountOfTimeVisibleInTimeline);
		*width = calculateSegmentWidth(*segmentFrameDuration, amountOfTimeVisibleInTimeline);

		// Handle the beginning of the segment snapping to the cursor
		float cursorOffset = calculateCursorOffset(*currentFrame, *firstFrame, amountOfTimeVisibleInTimeline);
		if (cursorOffset >= *offsetX - magnetThreshold && cursorOffset <= *offsetX + magnetThreshold)
		{
			// If we snap it, then we have to adjust the width to make sure it doesn't change
			int oldFrameStart = *segmentFrameStart;
			*segmentFrameStart = *currentFrame;
			int delta = oldFrameStart - *segmentFrameStart;

			// If the user was resizing, we need to adjust the frame duration to make sure
			// the segment doesn't look like it moved suddenly. Comment this out with the magnet
			// enabled and resize a segment to snap against another segment to see what this does
			// in more detail.
			if (changeType == SegmentChangeType::Resized)
			{
				*segmentFrameDuration += delta;
			}
		}

		// Handle the end of the segment snapping to the cursor
		if (cursorOffset >= *offsetX + *width - magnetThreshold && cursorOffset <= *offsetX + *width + magnetThreshold)
		{
			if (changeType == SegmentChangeType::Moved)
			{
				*segmentFrameStart = *currentFrame - *segmentFrameDuration;
			}
			else
			{
				// If we snap it while resizing, then we have to adjust the width 
				// to make sure it doesn't change
				*segmentFrameDuration = *currentFrame - *segmentFrameStart;
			}
		}
	}

	static void snapToPreviousSegment(int* firstFrame, float* offsetX, int* segmentFrameStart, int* segmentFrameDuration, int lastSegmentFrameStart, int lastSegmentFrameDuration, SegmentChangeType changeType, float amountOfTimeVisibleInTimeline)
	{
		float lastOffset = calculateSegmentOffset(lastSegmentFrameStart, *firstFrame, amountOfTimeVisibleInTimeline);
		float lastWidth = calculateSegmentWidth(lastSegmentFrameDuration, amountOfTimeVisibleInTimeline);
		if (lastOffset + lastWidth >= *offsetX - magnetThreshold && lastOffset + lastWidth <= *offsetX + magnetThreshold)
		{
			// If we snap it, then we have to adjust the width to make sure it doesn't change
			int oldFrameStart = *segmentFrameStart;
			*segmentFrameStart = lastSegmentFrameStart + lastSegmentFrameDuration;
			int delta = oldFrameStart - *segmentFrameStart;

			// If the user was resizing, we need to adjust the frame duration to make sure
			// the segment doesn't look like it moved suddenly. Comment this out with the magnet
			// enabled and resize a segment to snap against another segment to see what this does
			// in more detail.
			if (changeType == SegmentChangeType::Resized)
			{
				*segmentFrameDuration += delta;
			}
		}
	}

	static void snapToNextSegment(int* firstFrame, float* offsetX, float* width, int* segmentFrameStart, int* segmentFrameDuration, int nextSegmentFrameStart, int, SegmentChangeType changeType, float amountOfTimeVisibleInTimeline)
	{
		float nextOffset = calculateSegmentOffset(nextSegmentFrameStart, *firstFrame, amountOfTimeVisibleInTimeline);
		if (nextOffset >= *offsetX + *width - magnetThreshold && nextOffset <= *offsetX + *width + magnetThreshold)
		{
			if (changeType == SegmentChangeType::Moved)
			{
				*segmentFrameStart = nextSegmentFrameStart - *segmentFrameDuration;
			}
			else
			{
				// If we snap it while resizing, then we have to adjust the width 
				// to make sure it doesn't change
				*segmentFrameDuration = nextSegmentFrameStart - *segmentFrameStart;
			}
		}
	}

	static void checkAudioCacheResize(int numElementsToAdd)
	{
		if (numElementsToAdd + lengthAudioCache >= maxLengthAudioCache)
		{
			if (maxLengthAudioCache == 0)
			{
				maxLengthAudioCache = numElementsToAdd;
			}

			maxLengthAudioCache *= 2;
			if (audioCache != nullptr)
			{
				audioCache = (ImVec2*)g_memory_realloc(audioCache, sizeof(ImVec2) * maxLengthAudioCache);
			}
			else
			{
				audioCache = (ImVec2*)g_memory_allocate(sizeof(ImVec2) * maxLengthAudioCache);
			}

			g_logger_assert(audioCache != nullptr, "Ran out of RAM.");
		}
	}
}