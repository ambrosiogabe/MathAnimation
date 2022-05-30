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
		ImGuiTimelineResultFlags_SegmentTimeChanged  = 0x10,
		ImGuiTimelineResultFlags_SegmentTrackChanged = 0x20,
		ImGuiTimelineResultFlags_SubSegmentTimeChanged  = 0x40,
		ImGuiTimelineResultFlags_SubSegmentTrackChanged = 0x80,
		ImGuiTimelineResultFlags_ActiveObjectChanged = 0x100
	};

	struct ImGuiTimelineResult
	{
		int trackIndex;
		int segmentIndex;
		int subSegmentIndex;
		bool activeObjectIsSubSegment;
		ImGuiTimelineResultFlags flags;
	};

	struct ImGuiTimeline_SubSegment
	{
		int frameStart;
		int frameDuration;
		const char* segmentName;
		void* userData;
	};

	struct ImGuiTimeline_Segment
	{
		int frameStart;
		int frameDuration;
		const char* segmentName;
		void* userData;
		bool isExpanded;

		// TODO: Should I allow this to also be a track and recurse through all subtracks?
		// If this is nullptr, then the expanded track is empty
		ImGuiTimeline_SubSegment* subSegments;
		int numSubSegments;
	};

	struct ImGuiTimeline_Track
	{
		int numSegments;
		ImGuiTimeline_Segment* segments;
		const char* trackName;
		bool isExpanded;
	};

	ImGuiTimelineResult ImGuiTimeline(ImGuiTimeline_Track* tracks, int numTracks, int* currentFrame, int* firstFrame, float* zoom = nullptr, float* scrollOffsetX = nullptr, ImGuiTimelineFlags flags = ImGuiTimelineFlags_None);
}

#endif 