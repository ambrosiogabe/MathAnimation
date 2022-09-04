#ifndef MATH_ANIM_IMGUI_TIMELINE_H
#define MATH_ANIM_IMGUI_TIMELINE_H
#include "core.h"

namespace MathAnim
{
	typedef int ImGuiTimelineFlags;
	enum _ImGuiTimelineFlags
	{
		ImGuiTimelineFlags_None                   = 0x0,
		ImGuiTimelineFlags_FollowTimelineCursor   = 0x1,
		ImGuiTimelineFlags_EnableMagnetControl    = 0x2,
		ImGuiTimelineFlags_EnableZoomControl      = 0x4,
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
		ImGuiTimelineResultFlags_ActiveObjectChanged = 0x100,
		ImGuiTimelineResultFlags_DragDropPayloadHit  = 0x200,
		ImGuiTimelineResultFlags_DeleteActiveObject  = 0x400,
		ImGuiTimelineResultFlags_AddAudioSource      = 0x800,
		ImGuiTimelineResultFlags_DeleteAudioSource   = 0x1000,
		ImGuiTimelineResultFlags_ActiveObjectDeselected = 0x2000
	};

	struct ImGuiTimelineResult
	{
		void* dragDropPayloadData;
		size_t dragDropPayloadDataSize;
		int dragDropPayloadFirstFrame;
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
		struct
		{
			union
			{
				void* ptrData;
				int intData;
			} as;
		} userData;

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

	struct ImGuiTimeline_AudioData
	{
		uint32 sampleRate;
		uint32 bytesPerSec;
		uint16 blockAlignment;
		uint16 bitsPerSample;
		int32 numAudioChannels;
		uint32 dataSize;
		uint8* data;
	};

	ImGuiTimelineResult ImGuiTimeline(ImGuiTimeline_Track* tracks, int numTracks, int* currentFrame, int* firstFrame, float* zoom = nullptr, const ImGuiTimeline_AudioData* audioData = nullptr, ImGuiTimelineFlags flags = ImGuiTimelineFlags_None);
	const char* ImGuiTimeline_DragDropSegmentPayloadId();
	const char* ImGuiTimeline_DragDropSubSegmentPayloadId();

	void ImGuiTimeline_free();
}

#endif 