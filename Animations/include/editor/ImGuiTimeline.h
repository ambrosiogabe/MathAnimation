#ifndef MATH_ANIM_IMGUI_TIMELINE_H
#define MATH_ANIM_IMGUI_TIMELINE_H
#include "core.h"

namespace MathAnim
{
	enum class ImGuiTimelineFlags : uint8
	{
		None,
	};

	struct Segment
	{
		int frameStart;
		int frameDuration;
	};

	struct Track
	{
		int numSegments;
		Segment* segments;
		char* trackName;
	};

	bool ImGuiTimeline(Track* tracks, int numTracks, int* currentFrame, int* firstFrame, int numFrames, float* zoom = nullptr, ImGuiTimelineFlags flags = ImGuiTimelineFlags::None);
}

#endif 