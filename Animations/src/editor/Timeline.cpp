#include "core.h"

#include "editor/Timeline.h"
#include "editor/ImGuiTimeline.h"
#include "core/Application.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "animation/Svg.h"

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
		static ImGuiTimeline_Track* tracks;
		static int numTracks;

		// ------- Internal Functions --------
		static ImGuiTimeline_Track createDefaultTrack(char* trackName = nullptr);
		static void freeTrack(ImGuiTimeline_Track& track);
		static void addNewDefaultTrack(int insertIndex = INT32_MAX);
		static void deleteTrack(int index);
		static void handleAnimObjectInspector(int animObjectId);
		static void handleAnimationInspector(int animationId);
		static void handleTextObjectInspector(AnimObject* object);
		static void handleMoveToAnimationInspector(Animation* animation);
		static void handleSquareInspector(AnimObject* object);
		static void handleCircleInspector(AnimObject* object);

		static void setupImGuiTimelineDataFromAnimations(int numTracksToCreate = INT32_MAX);
		static void resetImGuiData();


		void init()
		{
			// I have to allocate a dummy allocation here because my memory utilities library
			// doesn't let realloc work with nullptr *yet*
			tracks = (ImGuiTimeline_Track*)g_memory_allocate(sizeof(ImGuiTimeline_Track));
			numTracks = 0;

			// Initialize some dummy data for now
			setupImGuiTimelineDataFromAnimations();
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


			ImGuiTimelineResult res = ImGuiTimeline(tracks, numTracks, &currentFrame, &firstFrame, nullptr, flags);
			if (res.flags & ImGuiTimelineResultFlags_CurrentFrameChanged)
			{
				Application::setFrameIndex(currentFrame);
			}

			if (res.flags & ImGuiTimelineResultFlags_AddTrackClicked)
			{
				addNewDefaultTrack(res.trackIndex);
			}

			if (res.flags & ImGuiTimelineResultFlags_DeleteTrackClicked)
			{
				deleteTrack(res.trackIndex);
			}

			if (res.flags & ImGuiTimelineResultFlags_DragDropPayloadHit)
			{
				g_logger_assert(res.dragDropPayloadDataSize == sizeof(TimelinePayload), "Invalid payload.");
				res.trackIndex;
				res.dragDropPayloadFirstFrame;
				res.dragDropPayloadData;
				TimelinePayload* payloadData = (TimelinePayload*)res.dragDropPayloadData;
				if (payloadData->isAnimObject)
				{
					if (!res.activeObjectIsSubSegment)
					{
						AnimObject object = AnimObject::createDefault(payloadData->objectType, res.dragDropPayloadFirstFrame, 120);
						object.timelineTrack = res.trackIndex;
						AnimationManager::addAnimObject(object);
					}
				}
				else
				{
					if (res.activeObjectIsSubSegment)
					{
						const ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
						int animObjectId = segment.userData.as.intData;
						Animation animation = Animation::createDefault(payloadData->animType, res.dragDropPayloadFirstFrame, 30, animObjectId);
						AnimationManager::addAnimationTo(animation, animObjectId);
					}
				}

				resetImGuiData();
			}

			if (res.flags & ImGuiTimelineResultFlags_SegmentTimeChanged)
			{
				const ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
				int animObjectIndex = segment.userData.as.intData;
				AnimationManager::setAnimObjectTime(animObjectIndex, segment.frameStart, segment.frameDuration);
			}

			if (res.flags & ImGuiTimelineResultFlags_SubSegmentTimeChanged)
			{
				const ImGuiTimeline_SubSegment& subSegment = tracks[res.trackIndex].segments[res.segmentIndex].subSegments[res.subSegmentIndex];
				int animationId = (int)(uintptr_t)subSegment.userData;
				int animObjectId = tracks[res.trackIndex].segments[res.segmentIndex].userData.as.intData;
				AnimationManager::setAnimationTime(animObjectId, animationId, subSegment.frameStart, subSegment.frameDuration);
			}

			static int activeAnimObjectId = INT32_MAX;
			static int activeAnimationId = INT32_MAX;
			if (res.flags & ImGuiTimelineResultFlags_ActiveObjectChanged)
			{
				if (res.activeObjectIsSubSegment)
				{
					const ImGuiTimeline_SubSegment& subSegment = tracks[res.trackIndex].segments[res.segmentIndex].subSegments[res.subSegmentIndex];
					activeAnimationId = (int)(uintptr_t)subSegment.userData;
					activeAnimObjectId = INT32_MAX;
				}
				else
				{
					const ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
					activeAnimObjectId = segment.userData.as.intData;
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
			if (tracks)
			{
				for (int i = 0; i < numTracks; i++)
				{
					freeTrack(tracks[i]);
				}
				g_memory_free(tracks);
			}
		}

		// ------- Internal Functions --------
		static ImGuiTimeline_Track createDefaultTrack(char* inTrackName)
		{
			ImGuiTimeline_Track defaultTrack;
			defaultTrack.numSegments = 0;
			defaultTrack.segments = nullptr;

			static int counter = 0;
			constexpr char defaultTrackName[] = "New Track XX";
			if (counter >= 100)
			{
				counter = 0;
			}
			char counterString[2];
			_itoa_s(counter, counterString, 10);

			if (inTrackName)
			{
				defaultTrack.trackName = inTrackName;
			}
			else
			{
				constexpr size_t defaultTrackNameLength = sizeof(defaultTrackName) / sizeof(defaultTrackName[0]);
				char* trackName = (char*)g_memory_allocate(sizeof(char) * defaultTrackNameLength);
				g_memory_copyMem(trackName, (void*)defaultTrackName, sizeof(char) * defaultTrackNameLength);
				trackName[defaultTrackNameLength - 3] = counterString[0];
				trackName[defaultTrackNameLength - 2] = counterString[1];
				defaultTrack.trackName = trackName;
			}
			defaultTrack.isExpanded = false;
			counter++;

			return defaultTrack;
		}

		static void freeTrack(ImGuiTimeline_Track& track)
		{
			track.isExpanded = false;
			if (track.segments)
			{
				for (int i = 0; i < track.numSegments; i++)
				{
					// Free all the animations and anim objects associated with this track
					int animObjectId = track.segments[i].userData.as.intData;
					AnimationManager::removeAnimObject(animObjectId);

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

		static void addNewDefaultTrack(int insertIndex)
		{
			numTracks++;
			tracks = (ImGuiTimeline_Track*)g_memory_realloc(tracks, sizeof(ImGuiTimeline_Track) * numTracks);
			g_logger_assert(tracks != nullptr, "Failed to initialize memory for tracks.");

			if (insertIndex == INT32_MAX || insertIndex == numTracks - 1)
			{
				tracks[numTracks - 1] = createDefaultTrack();
			}
			else
			{
				g_logger_assert(insertIndex >= 0 && insertIndex < numTracks, "Invalid insert index '%d' for track.", insertIndex);
				// Move all tracks after insert index to the end
				std::memmove(tracks + insertIndex + 1, tracks + insertIndex, (numTracks - insertIndex - 1) * sizeof(ImGuiTimeline_Track));
				tracks[insertIndex] = createDefaultTrack();
			}

			for (int track = 0; track < numTracks; track++)
			{
				for (int segment = 0; segment < tracks[track].numSegments; segment++)
				{
					int animObjectId = tracks[track].segments[segment].userData.as.intData;
					AnimationManager::setAnimObjectTrack(animObjectId, track);
				}
			}
		}

		static void deleteTrack(int index)
		{
			g_logger_assert(index >= 0 && index < numTracks, "Invalid track index '%d' for deleting track.", index);
			freeTrack(tracks[index]);

			// If the track wasn't the last track, move everything ahead of it back one
			if (index < numTracks - 1)
			{
				std::memmove(tracks + index, tracks + index + 1, (numTracks - index - 1) * sizeof(ImGuiTimeline_Track));
			}
			else if (index == 0 && numTracks == 1)
			{
				g_memory_free(tracks);
				// Allocate dummy memory
				tracks = (ImGuiTimeline_Track*)g_memory_allocate(sizeof(ImGuiTimeline_Track));
			}
			numTracks--;

			for (int track = 0; track < numTracks; track++)
			{
				for (int segment = 0; segment < tracks[track].numSegments; segment++)
				{
					int animObjectId = tracks[track].segments[segment].userData.as.intData;
					AnimationManager::setAnimObjectTrack(animObjectId, track);
				}
			}

			// After we delete the track, we have to reset our ImGui data
			resetImGuiData();
		}

		static void handleAnimObjectInspector(int animObjectId)
		{
			AnimObject* animObject = AnimationManager::getMutableObject(animObjectId);
			if (!animObject)
			{
				g_logger_error("No anim object with id '%d' exists", animObject);
				return;
			}

			ImGui::DragFloat2(": Position", (float*)&animObject->_positionStart.x);

			ImGui::DragFloat(": Stroke Width", (float*)&animObject->_strokeWidthStart);
			float strokeColor[4] = {
				(float)animObject->_strokeColorStart.r / 255.0f,
				(float)animObject->_strokeColorStart.g / 255.0f,
				(float)animObject->_strokeColorStart.b / 255.0f,
				(float)animObject->_strokeColorStart.a / 255.0f,
			};
			if (ImGui::ColorEdit4(": Stroke Color", strokeColor))
			{
				animObject->_strokeColorStart.r = (uint8)(strokeColor[0] * 255.0f);
				animObject->_strokeColorStart.g = (uint8)(strokeColor[1] * 255.0f);
				animObject->_strokeColorStart.b = (uint8)(strokeColor[2] * 255.0f);
				animObject->_strokeColorStart.a = (uint8)(strokeColor[3] * 255.0f);
			}

			float fillColor[4] = {
				(float)animObject->_fillColorStart.r / 255.0f,
				(float)animObject->_fillColorStart.g / 255.0f,
				(float)animObject->_fillColorStart.b / 255.0f,
				(float)animObject->_fillColorStart.a / 255.0f,
			};
			if (ImGui::ColorEdit4(": Fill Color", fillColor))
			{
				animObject->_fillColorStart.r = (uint8)(fillColor[0] * 255.0f);
				animObject->_fillColorStart.g = (uint8)(fillColor[1] * 255.0f);
				animObject->_fillColorStart.b = (uint8)(fillColor[2] * 255.0f);
				animObject->_fillColorStart.a = (uint8)(fillColor[3] * 255.0f);
			}

			switch (animObject->objectType)
			{
			case AnimObjectTypeV1::TextObject:
				handleTextObjectInspector(animObject);
				break;
			case AnimObjectTypeV1::LaTexObject:
				g_logger_assert(false, "TODO: Implement me.");
				break;
			case AnimObjectTypeV1::Square:
				handleSquareInspector(animObject);
				break;
			case AnimObjectTypeV1::Circle:
				handleCircleInspector(animObject);
				break;
			default:
				g_logger_error("Unknown anim object type: %d", (int)animObject->objectType);
				break;
			}
		}

		static void handleAnimationInspector(int animationId)
		{
			Animation* animation = AnimationManager::getMutableAnimation(animationId);
			if (!animation)
			{
				g_logger_error("No animation with id '%d' exists", animationId);
				return;
			}

			switch (animation->type)
			{
			case AnimTypeV1::WriteInText:
			case AnimTypeV1::Create:
			case AnimTypeV1::Transform:
			case AnimTypeV1::UnCreate:
			case AnimTypeV1::FadeIn:
			case AnimTypeV1::FadeOut:
				// NOP
				break;
			case AnimTypeV1::MoveTo:
				handleMoveToAnimationInspector(animation);
				break;
			default:
				g_logger_error("Unknown animation type: %d", (int)animation->type);
				break;
			}
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
				object->as.textObject.text = (char*)g_memory_realloc(object->as.textObject.text, sizeof(char) * (newLength + 1));
				object->as.textObject.textLength = (int32_t)newLength;
				g_memory_copyMem(object->as.textObject.text, scratch, newLength * sizeof(char));
				object->as.textObject.text[newLength] = '\0';
			}
		}

		static void handleMoveToAnimationInspector(Animation* animation)
		{
			ImGui::DragFloat2(": Target", &animation->as.moveTo.target.x);
		}

		static void handleSquareInspector(AnimObject* object)
		{
			if (ImGui::DragFloat(": Side Length", &object->as.square.sideLength))
			{
				// TODO: Do something better than this
				object->svgObject->free();
				g_memory_free(object->svgObject);
				object->svgObject = nullptr;

				object->_svgObjectStart->free();
				g_memory_free(object->_svgObjectStart);
				object->_svgObjectStart = nullptr;

				object->as.square.init(object);
			}
		}

		static void handleCircleInspector(AnimObject* object)
		{
			if (ImGui::DragFloat(": Radius", &object->as.circle.radius))
			{
				object->svgObject->free();
				g_memory_free(object->svgObject);
				object->svgObject = nullptr;

				object->_svgObjectStart->free();
				g_memory_free(object->_svgObjectStart);
				object->_svgObjectStart = nullptr;

				object->as.circle.init(object);
			}
		}

		static void setupImGuiTimelineDataFromAnimations(int numTracksToCreate)
		{
			const std::vector<AnimObject> animObjects = AnimationManager::getAnimObjects();

			// Find the max timeline track and add that many default tracks
			int maxTimelineTrack = -1;
			for (int i = 0; i < animObjects.size(); i++)
			{
				maxTimelineTrack = glm::max(animObjects[i].timelineTrack, maxTimelineTrack);
			}

			if (numTracksToCreate != INT32_MAX)
			{
				maxTimelineTrack = glm::max(maxTimelineTrack, numTracksToCreate - 1);
			}

			// If we don't have enough memory for some reason,
			// allocate enough new tracks to make sure we do
			if (maxTimelineTrack + 1 > numTracks)
			{
				for (int i = numTracks; i < maxTimelineTrack + 1; i++)
				{
					addNewDefaultTrack();
				}
			}
			g_logger_assert(maxTimelineTrack + 1 <= numTracks, "Invalid num tracks to create.");

			// Initialize all the tracks memory
			for (int track = 0; track < maxTimelineTrack + 1; track++)
			{
				// Count how many segment there are
				int numSegments = 0;
				for (int i = 0; i < animObjects.size(); i++)
				{
					if (animObjects[i].timelineTrack == track)
					{
						numSegments++;
					}
				}

				// Initialize segment memory
				tracks[track].numSegments = numSegments;
				if (numSegments > 0)
				{
					tracks[track].segments = (ImGuiTimeline_Segment*)g_memory_allocate(sizeof(ImGuiTimeline_Segment) * numSegments);
				}
				else
				{
					tracks[track].segments = nullptr;
				}

				// Count how many subsegments there are for each segment
				int segment = 0;
				for (int i = 0; i < animObjects.size(); i++)
				{
					if (animObjects[i].timelineTrack == track)
					{
						// Initialize the subsegment memory
						int numAnimations = (int)animObjects[i].animations.size();
						g_logger_assert(tracks[track].numSegments > segment, "Somehow we didn't allocate memory for this track.");
						tracks[track].segments[segment].numSubSegments = numAnimations;
						if (numAnimations > 0)
						{
							tracks[track].segments[segment].subSegments = (ImGuiTimeline_SubSegment*)g_memory_allocate(sizeof(ImGuiTimeline_SubSegment) * numAnimations);
						}
						else
						{
							tracks[track].segments[segment].subSegments = nullptr;
						}
						segment++;
					}
				}
			}

			// Initialize tracks data
			for (int track = 0; track < maxTimelineTrack + 1; track++)
			{
				int segment = 0;
				for (int i = 0; i < animObjects.size(); i++)
				{
					if (animObjects[i].timelineTrack == track)
					{
						tracks[track].segments[segment].frameStart = animObjects[i].frameStart;
						tracks[track].segments[segment].frameDuration = animObjects[i].duration;
						tracks[track].segments[segment].userData.as.intData = animObjects[i].id;
						tracks[track].segments[segment].segmentName = AnimationManager::getAnimObjectName(animObjects[i].objectType);
						tracks[track].segments[segment].isExpanded = false;

						for (int j = 0; j < animObjects[i].animations.size(); j++)
						{
							g_logger_assert(animObjects[i].animations[j].objectId == animObjects[i].id, "How did this happen?");
							g_logger_assert(j < tracks[track].segments[segment].numSubSegments, "Invalid index '%d'/'%d'", j, tracks[track].segments[segment].numSubSegments);
							ImGuiTimeline_SubSegment& subSegment = tracks[track].segments[segment].subSegments[j];
							subSegment.frameStart = animObjects[i].animations[j].frameStart;
							subSegment.frameDuration = animObjects[i].animations[j].duration;
							subSegment.segmentName = AnimationManager::getAnimationName(animObjects[i].animations[j].type);
							subSegment.userData = (void*)(uintptr_t)animObjects[i].animations[j].id;
						}

						segment++;
					}
				}
			}
		}

		static void resetImGuiData()
		{
			for (int track = 0; track < numTracks; track++)
			{
				if (tracks[track].segments)
				{
					for (int i = 0; i < tracks[track].numSegments; i++)
					{
						if (tracks[track].segments[i].subSegments)
						{
							g_memory_free(tracks[track].segments[i].subSegments);
							tracks[track].segments[i].subSegments = nullptr;
							tracks[track].segments[i].numSubSegments = 0;
						}
					}

					g_memory_free(tracks[track].segments);
					tracks[track].segments = nullptr;
					tracks[track].numSegments = 0;
				}
			}

			setupImGuiTimelineDataFromAnimations(numTracks);
		}
	}
}