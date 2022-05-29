#include "core.h"

#include "editor/Timeline.h"
#include "editor/ImGuiTimeline.h"
#include "core/Application.h"
#include "animation/Animation.h"

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "ImGuizmo.h"
#include "ImSequencer.h"
#include "ImCurveEdit.h"

namespace MathAnim
{
	namespace Timeline
	{
		// ------- Private variables --------
		ImGuiTimeline_Track* tracks;
		int numTracks;

		// ------- Internal Functions --------
		static ImGuiTimeline_Track createDefaultTrack();
		static void freeTrack(ImGuiTimeline_Track& track);
		static void addNewDefaultTrack();

		void init()
		{
			// I have to allocate a dummy allocation here because my memory utilities library
			// doesn't let realloc work with nullptr *yet*
			tracks = (ImGuiTimeline_Track*)g_memory_allocate(sizeof(ImGuiTimeline_Track));
			numTracks = 0;

			const std::vector<AnimObject> animObjects = AnimationManagerEx::getAnimObjects();

			// Initialize some dummy data for now
			addNewDefaultTrack();
			tracks[0].numSegments = animObjects.size();
			tracks[0].segments = (ImGuiTimeline_Segment*)g_memory_allocate(sizeof(ImGuiTimeline_Segment) * animObjects.size());
			for (int i = 0; i < animObjects.size(); i++)
			{
				tracks[0].segments[i].frameStart = animObjects[i].frameStart;
				tracks[0].segments[i].frameDuration = animObjects[i].duration;
				tracks[0].segments[i].userData = (void*)animObjects[i].id;
			}
		}

		void update()
		{
			ImGui::Begin("Timeline");

			static int selectedEntry = -1;
			static int firstFrame = 0;
			static bool expanded = true;
			int currentFrame = Application::getFrameIndex();

			ImGui::PushItemWidth(180);
			if (ImGui::InputInt("Frame ", &currentFrame))
			{
				Application::setFrameIndex(currentFrame);
			}
			ImGui::PopItemWidth();

			ImGuiTimelineFlags flags = ImGuiTimelineFlags_None;
			if (Application::getEditorPlayState() == AnimState::PlayForward ||
				Application::getEditorPlayState() == AnimState::PlayReverse)
			{
				flags |= ImGuiTimelineFlags_FollowTimelineCursor;
			}


			ImGuiTimelineResult res = ImGuiTimeline(tracks, numTracks, &currentFrame, &firstFrame, nullptr, nullptr, flags);
			if (res.flags & ImGuiTimelineResultFlags_CurrentFrameChanged)
			{
				Application::setFrameIndex(currentFrame);
			}

			if (res.flags & ImGuiTimelineResultFlags_AddTrackClicked)
			{
				addNewDefaultTrack();
			}

			if (res.flags & ImGuiTimelineResultFlags_SegmentTimeChanged)
			{
				const ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
				int animationIndex = (int)segment.userData;
				AnimationManagerEx::setAnimObjectTime(animationIndex, segment.frameStart, segment.frameDuration);
			}

			ImGui::End();
		}

		void free()
		{
			if (tracks != nullptr && numTracks > 0)
			{
				for (int i = 0; i < numTracks; i++)
				{
					freeTrack(tracks[i]);
				}
				g_memory_free(tracks);
			}
		}

		// ------- Internal Functions --------
		static ImGuiTimeline_Track createDefaultTrack()
		{
			ImGuiTimeline_Track defaultTrack;
			defaultTrack.numSegments = 0;
			defaultTrack.segments = nullptr;

			constexpr char defaultTrackName[] = "New Track";
			constexpr size_t defaultTrackNameLength = sizeof(defaultTrackName) / sizeof(defaultTrackName[0]);
			char* trackName = (char*)g_memory_allocate(sizeof(char) * defaultTrackNameLength);
			g_memory_copyMem(trackName, (void*)defaultTrackName, sizeof(char) * defaultTrackNameLength);
			defaultTrack.trackName = trackName;

			return defaultTrack;
		}

		static void freeTrack(ImGuiTimeline_Track& track)
		{
			track.numSegments = 0;
			if (track.segments)
			{
				g_memory_free(track.segments);
				track.segments = nullptr;
			}

			if (track.trackName)
			{
				g_memory_free(track.trackName);
				track.trackName = nullptr;
			}
		}

		static void addNewDefaultTrack()
		{
			numTracks++;
			tracks = (ImGuiTimeline_Track*)g_memory_realloc(tracks, sizeof(ImGuiTimeline_Track) * numTracks);
			tracks[numTracks - 1] = createDefaultTrack();
			g_logger_assert(tracks != nullptr, "Failed to initialize memory for tracks.");
		}
	}
}