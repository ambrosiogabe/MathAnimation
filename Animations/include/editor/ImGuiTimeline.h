#ifndef MATH_ANIM_IMGUI_TIMELINE_H
#define MATH_ANIM_IMGUI_TIMELINE_H
#include "core.h"

namespace MathAnim
{
	typedef int ImGuiTimelineFlags;
	enum _ImGuiTimelineFlags
	{
		ImGuiTimelineFlags_None                   = 0x0,
		ImGuiTimelineFlags_FollowTimelineCursor   = 0x1
	};

	typedef int ImGuiTimelineResultFlags;
	enum _ImGuiTimelineResultFlags
	{
		ImGuiTimelineResultFlags_None                = 0x0,
		ImGuiTimelineResultFlags_FirstFrameChanged   = 0x1,
		ImGuiTimelineResultFlags_CurrentFrameChanged = 0x2,
		ImGuiTimelineResultFlags_AddTrackClicked     = 0x4,
		ImGuiTimelineResultFlags_DeleteTrackClicked  = 0x8,
	};

	struct ImGuiTimelineResult
	{
		int trackIndex;
		ImGuiTimelineResultFlags flags;
	};

	struct ImGuiTimeline_Segment
	{
		int frameStart;
		int frameDuration;
	};

	struct ImGuiTimeline_Track
	{
		int numSegments;
		ImGuiTimeline_Segment* segments;
		char* trackName;
	};

	ImGuiTimelineResult ImGuiTimeline(ImGuiTimeline_Track* tracks, int numTracks, int* currentFrame, int* firstFrame, float* zoom = nullptr, float* scrollOffsetX = nullptr, ImGuiTimelineFlags flags = ImGuiTimelineFlags_None);
}

#endif 