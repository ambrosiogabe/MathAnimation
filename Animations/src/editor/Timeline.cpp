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
		static const char* animationObjectTypeNames[] = {
			"Text Object",
			"LaTex Object"
		};

		static const char* animationTypeNames[] = {
			"Write In Text",
		};

		static ImGuiTimeline_Track* tracks;
		static int numTracks;

		// ------- Internal Functions --------
		static ImGuiTimeline_Track createDefaultTrack();
		static void freeTrack(ImGuiTimeline_Track& track);
		static void addNewDefaultTrack();
		static void handleAnimObjectInspector(int animObjectId);
		static void handleAnimationInspector(int animationId);

		static void handleTextObjectInspector(AnimObject* object);

		void init()
		{
			// I have to allocate a dummy allocation here because my memory utilities library
			// doesn't let realloc work with nullptr *yet*
			tracks = (ImGuiTimeline_Track*)g_memory_allocate(sizeof(ImGuiTimeline_Track));
			numTracks = 0;

			const std::vector<AnimObject> animObjects = AnimationManagerEx::getAnimObjects();
			const std::vector<AnimationEx> anims = AnimationManagerEx::getAnimations();

			// Initialize some dummy data for now
			addNewDefaultTrack();
			tracks[0].numSegments = animObjects.size();
			tracks[0].segments = (ImGuiTimeline_Segment*)g_memory_allocate(sizeof(ImGuiTimeline_Segment) * animObjects.size());
			for (int i = 0; i < animObjects.size(); i++)
			{
				tracks[0].segments[i].frameStart = animObjects[i].frameStart;
				tracks[0].segments[i].frameDuration = animObjects[i].duration;
				tracks[0].segments[i].userData = (void*)animObjects[i].id;
				tracks[0].segments[i].segmentName = animationObjectTypeNames[(int)animObjects[i].objectType];
				tracks[0].segments[i].isExpanded = false;

				int numAnimations = 0;
				for (int j = 0; j < anims.size(); j++)
				{
					if (anims[j].objectId == animObjects[i].id)
					{
						numAnimations++;
					}
				}

				tracks[0].segments[i].numSubSegments = numAnimations;
				tracks[0].segments[i].subSegments = (ImGuiTimeline_SubSegment*)g_memory_allocate(sizeof(ImGuiTimeline_SubSegment) * numAnimations);
				int animationIndexCounter = 0;
				for (int j = 0; j < anims.size(); j++)
				{
					if (anims[j].objectId == animObjects[i].id)
					{
						ImGuiTimeline_SubSegment& subSegment = tracks[0].segments[i].subSegments[animationIndexCounter];
						subSegment.frameStart = anims[j].frameStart;
						subSegment.frameDuration = anims[j].duration;
						subSegment.segmentName = animationTypeNames[(int)anims[j].type];
						subSegment.userData = (void*)anims[j].id;
						
					}
				}
			}
		}

		void update()
		{
			// NOTE: For best results, it's usually a good idea to specify 0 padding for the window
			// so that the timeline can expand to the full width/height of the window
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
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
				int animObjectIndex = (int)segment.userData;
				AnimationManagerEx::setAnimObjectTime(animObjectIndex, segment.frameStart, segment.frameDuration);
			}

			if (res.flags & ImGuiTimelineResultFlags_SubSegmentTimeChanged)
			{
				const ImGuiTimeline_SubSegment& subSegment = tracks[res.trackIndex].segments[res.segmentIndex].subSegments[res.subSegmentIndex];
				int animationIndex = (int)subSegment.userData;
				AnimationManagerEx::setAnimationTime(animationIndex, subSegment.frameStart, subSegment.frameDuration);
			}

			static int activeAnimObjectId = INT32_MAX;
			static int activeAnimationId = INT32_MAX;
			if (res.flags & ImGuiTimelineResultFlags_ActiveObjectChanged)
			{
				if (res.activeObjectIsSubSegment)
				{
					const ImGuiTimeline_SubSegment& subSegment = tracks[res.trackIndex].segments[res.segmentIndex].subSegments[res.subSegmentIndex];
					activeAnimationId = (int)subSegment.userData;
					activeAnimObjectId = INT32_MAX;
				}
				else
				{
					const ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
					activeAnimObjectId = (int)segment.userData;
					activeAnimationId = INT32_MAX;
				}
			}

			ImGui::End();
			ImGui::PopStyleVar();

			ImGui::Begin("Inspector");

			if (activeAnimObjectId != INT32_MAX)
			{
				handleAnimObjectInspector(activeAnimObjectId);
			}
			else if (activeAnimationId != INT32_MAX)
			{
				handleAnimationInspector(activeAnimationId);
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
			defaultTrack.isExpanded = false;

			return defaultTrack;
		}

		static void freeTrack(ImGuiTimeline_Track& track)
		{
			track.isExpanded = false;
			if (track.segments)
			{
				for (int i = 0; i < track.numSegments; i++)
				{
					if (track.segments[i].subSegments)
					{
						g_memory_free(track.segments[i].subSegments);
						track.segments[i].subSegments = nullptr;
						track.segments[i].numSubSegments = 0;
					}
				}

				g_memory_free(track.segments);
				track.segments = nullptr;
			}
			track.numSegments = 0;

			if (track.trackName)
			{
				g_memory_free((void*)track.trackName);
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

		static void handleAnimObjectInspector(int animObjectId)
		{
			AnimObject* animObject = AnimationManagerEx::getMutableObject(animObjectId);
			if (!animObject)
			{
				g_logger_error("No anim object with id '%d' exists", animObject);
				return;
			}

			ImGui::DragFloat2(": Position", (float*)&animObject->position.x);

			switch (animObject->objectType)
			{
			case AnimObjectType::TextObject:
				handleTextObjectInspector(animObject);
				break;
			case AnimObjectType::LaTexObject:
				g_logger_assert(false, "TODO: Implement me.");
				break;
			default:
				g_logger_error("Unknown anim object type: %d", (int)animObject->objectType);
				break;
			}
		}

		static void handleAnimationInspector(int animationId)
		{

		}

		static void handleTextObjectInspector(AnimObject* object)
		{
			ImGui::DragFloat(": Font Size (Px)", &object->as.textObject.fontSizePixels);

			constexpr int scratchLength = 256;
			char scratch[scratchLength] = {};
			if (object->as.textObject.textLength >= scratchLength)
			{
				g_logger_error("Text object has more than 256 characters. Tell Gabe to increase scratch length for text objects.");
				return;
			}
			g_memory_copyMem(scratch, object->as.textObject.text, object->as.textObject.textLength * sizeof(char));
			scratch[object->as.textObject.textLength] = '\0';
			if (ImGui::InputTextMultiline(": Text", scratch, scratchLength * sizeof(char)))
			{
				size_t newLength = std::strlen(scratch);
				object->as.textObject.text = (char*)g_memory_realloc(object->as.textObject.text, sizeof(char) * newLength);
				object->as.textObject.textLength = newLength;
				g_memory_copyMem(object->as.textObject.text, scratch, newLength * sizeof(char));
				object->as.textObject.text[newLength] = '\0';
			}
		}
	}
}