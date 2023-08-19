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
		ImGuiTimelineResultFlags_None                   = 0x0,
		ImGuiTimelineResultFlags_FirstFrameChanged      = 1,
		ImGuiTimelineResultFlags_CurrentFrameChanged    = ImGuiTimelineResultFlags_FirstFrameChanged << 1,
		ImGuiTimelineResultFlags_AddTrackClicked        = ImGuiTimelineResultFlags_CurrentFrameChanged << 1,
		ImGuiTimelineResultFlags_DeleteTrackClicked     = ImGuiTimelineResultFlags_AddTrackClicked << 1,
		ImGuiTimelineResultFlags_SegmentTimeChanged     = ImGuiTimelineResultFlags_DeleteTrackClicked << 1,
		ImGuiTimelineResultFlags_SegmentTimeDragEnded   = ImGuiTimelineResultFlags_SegmentTimeChanged << 1,
		ImGuiTimelineResultFlags_SegmentTrackChanged    = ImGuiTimelineResultFlags_SegmentTimeDragEnded << 1,
		ImGuiTimelineResultFlags_ActiveObjectChanged    = ImGuiTimelineResultFlags_SegmentTrackChanged << 1,
		ImGuiTimelineResultFlags_DragDropPayloadHit     = ImGuiTimelineResultFlags_ActiveObjectChanged << 1,
		ImGuiTimelineResultFlags_DeleteActiveObject     = ImGuiTimelineResultFlags_DragDropPayloadHit << 1,
		ImGuiTimelineResultFlags_AddAudioSource         = ImGuiTimelineResultFlags_DeleteActiveObject << 1,
		ImGuiTimelineResultFlags_DeleteAudioSource      = ImGuiTimelineResultFlags_AddAudioSource << 1,
		ImGuiTimelineResultFlags_ActiveObjectDeselected = ImGuiTimelineResultFlags_DeleteAudioSource << 1
	};

	struct ImGuiTimelineResult
	{
		void* dragDropPayloadData;
		size_t dragDropPayloadDataSize;
		int dragDropPayloadFirstFrame;
		int trackIndex;
		int segmentIndex;
		ImGuiTimelineResultFlags flags;
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
				AnimId idData;
			} as;
		} userData;
	};

	struct ImGuiTimeline_Track
	{
		int numSegments;
		ImGuiTimeline_Segment* segments;
		const char* trackName;
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

	void ImGuiTimeline_SetActiveSegment(int trackIndex, int segmentIndex);

	ImGuiTimelineResult ImGuiTimeline(ImGuiTimeline_Track* tracks, int numTracks, int* currentFrame, int* firstFrame, float* zoom = nullptr, const ImGuiTimeline_AudioData* audioData = nullptr, ImGuiTimelineFlags flags = ImGuiTimelineFlags_None);
	const char* ImGuiTimeline_DragDropSegmentPayloadId();
	const char* ImGuiTimeline_DragDropSubSegmentPayloadId();

	void ImGuiTimeline_free();
}

#endif 