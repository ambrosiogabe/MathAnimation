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

struct ImTimelineWindowData
{
	ImGuiID windowID;
	ImVec2 Scroll;
};

static constexpr int maxNumTimelines = 16;
static int numTimelinesActive = 0;
static ImTimelineWindowData windowData[maxNumTimelines];

namespace MathAnim
{
	// ----------- Internal Variables -----------
	// Config Values
	constexpr float timelineHorizontalScrollSensitivity = 12.0f;
	constexpr float timelineVerticalScrollSensitivity = 10.0f;
	constexpr int fps = 60;
	constexpr int autoPlayFrameDelta = 50;

	// Dimensional values
	constexpr int maxHighlightedSquares = 12;
	constexpr float timelineRulerHeight = 65.0f;
	constexpr float timelineRulerBorderHeight = 3.0f;
	constexpr float timelineRulerTickSpacing = 9.0f;
	constexpr float minDistanceBetweenRulerTimecodes = 320.0f;

	constexpr int tickWidth = 2;
	constexpr int smallTickHeight = 7;
	constexpr int mediumTickHeight = 15;
	constexpr int largeTickHeight = 30;
	constexpr int boundaryTickHeight = 45;

	constexpr int trackHeight = 145;
	constexpr int trackNamePadding = 15;

	constexpr ImVec2 segmentTextAreaPadding = ImVec2(10.0f, 10.0f);
	constexpr ImVec2 expandButtonSize = ImVec2(20.0f, 12.0f);
	constexpr int segmentTextAreaHeight = 40;

	// Colors
	constexpr ImU32 boundaryTickColor = IM_COL32(135, 135, 135, 255);
	constexpr ImU32 largeTickColor = IM_COL32(105, 105, 105, 255);
	constexpr ImU32 defaultTickColor = IM_COL32(85, 85, 85, 255);

	constexpr ImU32 canvasColor = IM_COL32(20, 20, 20, 255);
	constexpr ImU32 legendBackground = IM_COL32(35, 35, 35, 255);
	constexpr ImU32 timelineTrackDark = IM_COL32(10, 10, 10, 255);
	constexpr ImU32 cursorColor = IM_COL32(214, 118, 111, 255);

	constexpr ImU32 segmentColor = IM_COL32(133, 116, 184, 255);
	constexpr ImU32 segmentDarkColor = IM_COL32(101, 88, 138, 255);
	constexpr ImU32 segmentBoldColor = IM_COL32(69, 61, 92, 255);
	constexpr ImU32 subSegmentColor = IM_COL32(119, 186, 122, 255);
	constexpr ImU32 subSegmentDarkColor = IM_COL32(87, 135, 85, 255);

	// ----------- Internal Functions -----------
	static bool handleLegendSplitter(const ImVec2& canvasPos, const ImVec2& canvasSize, const ImVec2& legendSize, float* legendWidth);
	static bool handleResizeElement(float* currentValue, DragState* state, const ImVec2& valueBounds, const ImVec2& mouseBounds, const ImVec2& hoverRectStart, const ImVec2& hoverRectEnd, ResizeFlags flags);
	static bool handleSegment(const ImVec2& segmentStart, const ImVec2& segmentEnd, ImGuiTimeline_Segment* segment, ImGuiID segmentID, const ImVec2& timelineSize, float amountOfTimeVisibleInTimeline);
	static bool handleSubSegment(const ImVec2& segmentStart, const ImVec2& segmentEnd, ImGuiTimeline_SubSegment* segment, ImGuiID segmentID, const ImVec2& timelineSize, float amountOfTimeVisibleInTimeline);
	static void framesToTimeStr(char* strBuffer, size_t strBufferLength, int frame);
	static float getScrollFromFrame(float amountOfTimeVisibleInTimeline, int firstFrame, const ImVec2& timelineRulerEnd, const ImVec2& timelineRulerBegin);
	static bool handleDragState(const ImVec2& start, const ImVec2& end, DragState* state);
	static bool beginPopupContextTimelineItem(const char* str_id, const ImVec2& rectBegin, const ImVec2& rectEnd, ImGuiPopupFlags popup_flags = 1);

	ImGuiTimelineResult ImGuiTimeline(ImGuiTimeline_Track* tracks, int numTracks, int* currentFrame, int* firstFrame, float* inZoom, ImGuiTimelineFlags flags)
	{
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
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();        // Resize canvas to what's available

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

		float scrollOffsetX = data->Scroll.x;
		float scrollOffsetY = data->Scroll.y;

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

		float firstTimecodePosition = glm::floor((scrollOffsetX / minDistanceBetweenRulerTimecodes)) * minDistanceBetweenRulerTimecodes - scrollOffsetX;
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
				int firstTickPos = (int)glm::floor((scrollOffsetX / minDistanceBetweenRulerTimecodes));
				int numFramesToThisPos = (firstTickPos + i) * numTicksBetweenBoundaries;

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

		// ---------------------- Handle horizontal/vertical scrolling ----------------------
		// Handle horizontal scrolling while over the timeline or timeline ruler
		if (ImGui::IsMouseHoveringRect(timelineRulerBegin, ImVec2(timelineRulerEnd.x, canvasPos.y + canvasSize.y)))
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
		if (ImGui::IsMouseHoveringRect(canvasPos + ImVec2(0, timelineRulerHeight), ImVec2(timelineRulerEnd.x, canvasPos.y + canvasSize.y)))
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

				// Figure out if the track is expanded
				tracks[i].isExpanded = false;
				for (int si = 0; si < tracks[i].numSegments; si++)
				{
					if (tracks[i].segments[si].isExpanded)
					{
						tracks[i].isExpanded = true;
						break;
					}
				}

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
			float absTracksTop = canvasPos.y + timelineRulerHeight;
			float currentTrackTop = canvasPos.y + timelineRulerHeight - scrollOffsetY;
			for (int i = 0; i < numTracks; i++)
			{
				float trackTopY = currentTrackTop;
				float trackBottomY = canvasPos.y + timelineRulerHeight + ((float)(i + 1) * trackHeight) - scrollOffsetY;

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

					std::string strId = "Segment_" + std::to_string(i) + "_" + std::to_string(si);
					ImGuiID segmentID = ImHashStr(strId.c_str(), strId.size());
					static ImGuiID activeSegmentID = UINT32_MAX;

					float offsetX = (segment.frameStart - (float)*firstFrame) / amountOfTimeVisibleInTimeline * (canvasSize.x - legendSize.x);
					float width = (segment.frameDuration / amountOfTimeVisibleInTimeline) * (canvasSize.x - legendSize.x);
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

						if (handleSegment(segmentStart, segmentEnd, &segment, segmentID, timelineSize, amountOfTimeVisibleInTimeline))
						{
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
						}
						else if (activeSegmentID == segmentID && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
						{
							activeSegmentID = UINT32_MAX;
						}

						// Draw the segment
						drawList->AddRectFilled(segmentStart, segmentEnd, segmentColor, 10.0f);
						if (activeSegmentID == segmentID)
						{
							drawList->AddRect(segmentStart, segmentEnd, cursorColor, 10.0f, 0, 4.0f);
						}
						ImVec2 borderStart = segmentStart + ImVec2(0.0f, realTrackHeight - segmentTextAreaHeight);
						ImVec2 borderEnd = ImVec2(segmentEnd.x, borderStart.y + 3.0f);
						drawList->AddRectFilled(borderStart, borderEnd, segmentDarkColor);
						ImVec2 textPosition = segmentStart + ImVec2(0.0f, realTrackHeight - segmentTextAreaHeight) + segmentTextAreaPadding;
						// Vertically center the text
						ImVec2 segmentTextSize = ImGui::CalcTextSize(segment.segmentName);
						textPosition.y += (segmentTextAreaHeight - segmentTextSize.y - segmentTextAreaPadding.y * 2.0f) / 2.0f;
						drawList->AddText(textPosition, ImColor(fontColor), segment.segmentName);

						// Draw expansion triangle
						constexpr ImVec2 expandArrowRectPadding = ImVec2(6.0f, 6.0f);
						ImVec2 expandButtonStart = (segmentEnd - segmentTextAreaPadding - expandButtonSize);
						ImVec2 expandArrowRectStart = expandButtonStart - expandArrowRectPadding;
						ImVec2 expandArrowRectEnd = expandButtonStart + expandButtonSize + expandArrowRectPadding;
						if (segment.isExpanded)
						{
							drawList->AddRectFilled(
								expandArrowRectStart,
								expandArrowRectEnd,
								ImColor(fontColor),
								5.0f
							);
							drawList->AddTriangle(
								expandButtonStart,
								expandButtonStart + ImVec2(expandButtonSize.x, 0.0f),
								expandButtonStart + ImVec2(expandButtonSize.x / 2.0f, expandButtonSize.y),
								segmentColor,
								3.0f
							);
						}
						else
						{
							drawList->AddTriangle(
								expandButtonStart,
								expandButtonStart + ImVec2(expandButtonSize.x, 0.0f),
								expandButtonStart + ImVec2(expandButtonSize.x / 2.0f, expandButtonSize.y),
								segmentBoldColor,
								3.0f
							);
						}

						// Check if the user clicked the segment arrow to expand or un-expand it
						if (ImGui::IsMouseHoveringRect(expandArrowRectStart, expandArrowRectEnd) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
						{
							segment.isExpanded = !segment.isExpanded;
						}
					} // End segment intersects with timeline check

					if (segment.isExpanded)
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
							float offsetX = (subSegmentAbsFrameStart - (float)*firstFrame) / amountOfTimeVisibleInTimeline * (canvasSize.x - legendSize.x);
							float width = (subSegment.frameDuration / amountOfTimeVisibleInTimeline) * (canvasSize.x - legendSize.x);
							// Check if segment intersects with the timeline
							// We can check if:
							//    segmentEnd >= timelineBegin && segmentBegin <= timelineEnd
							if (shouldDrawSubtrack &&
								subSegmentAbsFrameStart <= ((int)amountOfTimeVisibleInTimeline + *firstFrame) &&
								subSegmentAbsFrameStart + subSegment.frameDuration >= *firstFrame)
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

								ImVec2 subSegmentStart = ImVec2(canvasPos.x + legendSize.x + offsetX, subTrackTopY);
								ImVec2 subSegmentEnd = ImVec2(subSegmentStart.x + width, subTrackBottomY);

								std::string strId = "SubSegment_" + std::to_string(i) + "_" + std::to_string(si) + "_" + std::to_string(subSegmenti);
								ImGuiID segmentID = ImHashStr(strId.c_str(), strId.size());
								if (handleSubSegment(subSegmentStart, subSegmentEnd, &subSegment, segmentID, timelineSize, amountOfTimeVisibleInTimeline))
								{
									g_logger_assert(res.segmentIndex == -1, "Invalid result. User somehow modified two segments at once.");
									res.flags |= ImGuiTimelineResultFlags_SubSegmentTimeChanged;
									res.segmentIndex = si;
									res.subSegmentIndex = subSegmenti;
									res.trackIndex = i;
									if (activeSegmentID != segmentID)
									{
										res.flags |= ImGuiTimelineResultFlags_ActiveObjectChanged;
										res.activeObjectIsSubSegment = true;
										activeSegmentID = segmentID;
									}
								}
								else if (activeSegmentID == segmentID && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
								{
									activeSegmentID = UINT32_MAX;
								}

								if (shouldDrawSubtrack)
								{
									// Draw the segment
									drawList->AddRectFilled(subSegmentStart, subSegmentEnd, subSegmentColor, 10.0f);
									if (activeSegmentID == segmentID)
									{
										drawList->AddRect(subSegmentStart, subSegmentEnd, cursorColor, 10.0f, 0, 4.0f);
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

		// ---------------------- Handle Timeline Cursor ------------------------------
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
					scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame, timelineRulerEnd, timelineRulerBegin);
					res.flags |= ImGuiTimelineResultFlags_FirstFrameChanged;
				}
				else if (*currentFrame > *firstFrame + (int)amountOfTimeVisibleInTimeline)
				{
					*firstFrame += 3;
					scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame, timelineRulerEnd, timelineRulerBegin);
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
				*firstFrame -= autoPlayFrameDelta;
				changed = true;
			}
			else if (*currentFrame > (*firstFrame + (int)amountOfTimeVisibleInTimeline))
			{
				*firstFrame += autoPlayFrameDelta;
				changed = true;
			}

			if (changed)
			{
				scrollOffsetX = getScrollFromFrame(amountOfTimeVisibleInTimeline, *firstFrame, timelineRulerEnd, timelineRulerBegin);
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
			static char strBuffer[] = "HH:MM:SS:ff";
			size_t strBufferSize = sizeof(strBuffer) / sizeof(strBuffer[0]);
			framesToTimeStr(strBuffer, strBufferSize, *currentFrame);

			ImGui::PushFont(ImGuiLayer::getLargeFont());
			ImVec2 textSize = ImGui::CalcTextSize(strBuffer, strBuffer + strBufferSize - 1);
			ImVec2 textPos = canvasPos + ((timecodeRectSize - textSize) / 2.0f);
			ImVec4& fontColor = style.Colors[ImGuiCol_Text];
			drawList->AddText(textPos, ImColor(fontColor), strBuffer, strBuffer + strBufferSize - 1);
			ImGui::PopFont();

			// Draw all the track labels
			float currentTrackTop = canvasPos.y + timelineRulerHeight - scrollOffsetY;
			ImGui::PushClipRect(canvasPos + ImVec2(0.0f, timelineRulerHeight), canvasPos + legendSize, true);
			for (int i = 0; i < numTracks; i++)
			{
				ImVec2 textSize = ImGui::CalcTextSize(tracks[i].trackName);
				float offsetY = currentTrackTop + (trackHeight - textSize.y) / 2.0f;
				drawList->AddText(ImVec2(canvasPos.x + trackNamePadding, offsetY), ImColor(fontColor), tracks[i].trackName);

				// Draw border top and bottom
				float thisTrackHeight = tracks[i].isExpanded ? trackHeight * 2.0f : trackHeight;
				ImVec2 legendTrackNameTop = ImVec2(canvasPos.x, currentTrackTop);
				ImVec2 legendTrackNameBottom = ImVec2(canvasPos.x + legendSize.x, currentTrackTop + thisTrackHeight + 1.5f);
				drawList->AddRect(
					legendTrackNameTop,
					ImVec2(canvasPos.x + legendSize.x, currentTrackTop + 1.5f),
					IM_COL32(0, 0, 0, 255)
				);

				drawList->AddRect(
					ImVec2(canvasPos.x, currentTrackTop + thisTrackHeight),
					legendTrackNameBottom,
					IM_COL32(0, 0, 0, 255)
				);

				if (tracks[i].isExpanded)
				{
					currentTrackTop += trackHeight;
				}

				currentTrackTop += trackHeight;

				// Handle right clicking on the legend by popping up context menu
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
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
				ImGui::PopStyleVar();
			}
			ImGui::PopClipRect();
		}
		// ---------------------- End Legend ------------------------------

		data->Scroll.x = scrollOffsetX;
		data->Scroll.y = scrollOffsetY;

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

	static bool handleSegment(const ImVec2& segmentStart, const ImVec2& segmentEnd, ImGuiTimeline_Segment* segment, ImGuiID segmentID, const ImVec2& timelineSize, float amountOfTimeVisibleInTimeline)
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
		ImVec2 segmentStart_ = segmentStart + ImVec2(halfResizeDragWidth + 0.1f, 0.0f);
		ImVec2 segmentEnd_ = segmentEnd - ImVec2(halfResizeDragWidth - 0.1f, 0.0f);
		ImVec2 leftResizeStart = segmentStart - ImVec2(halfResizeDragWidth, 0.0f);
		ImVec2 leftResizeEnd = ImVec2(segmentStart_.x, segmentEnd_.y);
		ImVec2 rightResizeStart = ImVec2(segmentEnd_.x, segmentStart.y);
		ImVec2 rightResizeEnd = segmentEnd + ImVec2(halfResizeDragWidth, 0.0f);

		// Subtract the height of the text area at the bottom of the segment so
		// that you can click there without moving the segment
		segmentEnd_.y -= segmentTextAreaHeight;

		handleDragState(segmentStart_, segmentEnd_, &dragState);
		handleDragState(leftResizeStart, leftResizeEnd, &leftResizeState);
		handleDragState(rightResizeStart, rightResizeEnd, &rightResizeState);

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
				}

				if (rightResizeState == DragState::Active)
				{
					segment->frameDuration = ogFrameDuration + frameChange;

					if (segment->frameDuration <= 0)
					{
						segment->frameDuration = 1;
					}

					changed = true;
				}
			}
		}

		return changed;
	}

	static bool handleSubSegment(const ImVec2& segmentStart, const ImVec2& segmentEnd, ImGuiTimeline_SubSegment* segment, ImGuiID segmentID, const ImVec2& timelineSize, float amountOfTimeVisibleInTimeline)
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
		ImVec2 segmentStart_ = segmentStart + ImVec2(halfResizeDragWidth + 0.1f, 0.0f);
		ImVec2 segmentEnd_ = segmentEnd - ImVec2(halfResizeDragWidth - 0.1f, 0.0f);
		ImVec2 leftResizeStart = segmentStart - ImVec2(halfResizeDragWidth, 0.0f);
		ImVec2 leftResizeEnd = ImVec2(segmentStart_.x, segmentEnd_.y);
		ImVec2 rightResizeStart = ImVec2(segmentEnd_.x, segmentStart.y);
		ImVec2 rightResizeEnd = segmentEnd + ImVec2(halfResizeDragWidth, 0.0f);

		// Subtract the height of the text area at the bottom of the segment so
		// that you can click there without moving the segment
		segmentEnd_.y -= segmentTextAreaHeight;

		handleDragState(segmentStart_, segmentEnd_, &dragState);
		handleDragState(leftResizeStart, leftResizeEnd, &leftResizeState);
		handleDragState(rightResizeStart, rightResizeEnd, &rightResizeState);

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
				}

				if (rightResizeState == DragState::Active)
				{
					segment->frameDuration = ogFrameDuration + frameChange;

					if (segment->frameDuration <= 0)
					{
						segment->frameDuration = 1;
					}

					changed = true;
				}
			}
		}

		return changed;
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

		bool currentValueChanged = false;
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
}