#include "editor/timeline/Timeline.h"
#include "editor/timeline/ImGuiTimeline.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/UndoSystem.h"
#include "core.h"
#include "core/Application.h"
#include "core/Serialization.hpp"
#include "animation/AnimationManager.h"
#include "audio/Audio.h"
#include "audio/WavLoader.h"

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <nlohmann/json.hpp>
#include <nfd.h>

namespace MathAnim
{
	namespace Timeline
	{
		// ------- Private variables --------
		static ImGuiTimeline_Track* tracks;
		static int numTracks;
		static AudioSource audioSource;
		static WavData audioData;
		static ImGuiTimeline_AudioData imguiAudioData;

		// ------- Internal Functions --------
		static void loadAudioSource(const char* filepath);

		static ImGuiTimeline_Track createDefaultTrack(char* trackName = nullptr);
		static void freeTrack(ImGuiTimeline_Track& track, AnimationManagerData* am);
		static void addNewDefaultTrack(AnimationManagerData* am, int insertIndex = INT32_MAX);
		static void deleteTrack(AnimationManagerData* am, int index);

		static void setupImGuiTimelineDataFromAnimations(AnimationManagerData* am, int numTracksToCreate = INT32_MAX);
		static void deleteSegment(ImGuiTimeline_Track& track, int segmentIndex, AnimationManagerData* am);

		TimelineData initInstance()
		{
			TimelineData res = {};
			res.audioSourceFile = nullptr;
			res.audioSourceFileLength = 0;
			res.currentFrame = 0;
			res.firstFrame = 0;
			res.zoomLevel = 5.0f;
			return res;
		}

		void init(AnimationManagerData* am)
		{
			audioSource = Audio::defaultAudioSource();

			// I have to allocate a dummy allocation here because my memory utilities library
			// doesn't let realloc work with nullptr *yet*
			tracks = (ImGuiTimeline_Track*)g_memory_allocate(sizeof(ImGuiTimeline_Track));
			numTracks = 0;

			// Initialize some dummy data for now
			setupImGuiTimelineDataFromAnimations(am);
		}

		void update(TimelineData& timelineData, AnimationManagerData* am)
		{
			// NOTE: For best results, it's usually a good idea to specify 0 padding for the window
			// so that the timeline can expand to the full width/height of the window
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			if (ImGui::Begin("Timeline"))
			{

				timelineData.currentFrame = Application::getFrameIndex();

				ImGuiTimelineFlags flags = ImGuiTimelineFlags_None;
				flags |= ImGuiTimelineFlags_EnableZoomControl;
				flags |= ImGuiTimelineFlags_EnableMagnetControl;

				if (Application::getEditorPlayState() == AnimState::PlayForward ||
					Application::getEditorPlayState() == AnimState::PlayReverse)
				{
					flags |= ImGuiTimelineFlags_FollowTimelineCursor;

					if (!Audio::isNull(audioSource) && !audioSource.isPlaying)
					{
						float offset = 0.0f;
						if (timelineData.currentFrame > 0)
						{
							offset = timelineData.currentFrame / 60.0f;
						}
						Audio::play(audioSource, offset);
					}
				}
				else
				{
					if (!Audio::isNull(audioSource) && audioSource.isPlaying)
					{
						Audio::stop(audioSource);
					}
				}

				ImGuiTimeline_AudioData* imguiAudioDataPtr = Audio::isNull(audioSource)
					? nullptr
					: &imguiAudioData;
				ImGuiTimelineResult res = ImGuiTimeline(
					tracks,
					numTracks,
					&timelineData.currentFrame,
					&timelineData.firstFrame,
					&timelineData.zoomLevel,
					imguiAudioDataPtr,
					flags
				);

				if (res.flags & ImGuiTimelineResultFlags_CurrentFrameChanged)
				{
					Application::setFrameIndex(timelineData.currentFrame);
				}

				if (res.flags & ImGuiTimelineResultFlags_AddTrackClicked)
				{
					addNewDefaultTrack(am, res.trackIndex);
				}

				if (res.flags & ImGuiTimelineResultFlags_DeleteTrackClicked)
				{
					deleteTrack(am, res.trackIndex);
				}

				// If we have a filepath for an audio source and the current audio source is null
				// try to load it
				if (timelineData.audioSourceFileLength > 0 && Audio::isNull(audioSource))
				{
					loadAudioSource((const char*)timelineData.audioSourceFile);

					if (Audio::isNull(audioSource))
					{
						// Failed to load the file, there must be something wrong with it.
						g_logger_error("Failed to load audio source file '{}'", timelineData.audioSourceFile);
						g_memory_free(timelineData.audioSourceFile);
						timelineData.audioSourceFile = nullptr;
						timelineData.audioSourceFileLength = 0;
					}
				}

				if (res.flags & ImGuiTimelineResultFlags_DeleteAudioSource)
				{
					Audio::free(audioSource);
					WavLoader::free(audioData);
					timelineData.audioSourceFile = (uint8*)g_memory_realloc(timelineData.audioSourceFile, sizeof(uint8));
					timelineData.audioSourceFile[0] = '\0';
					timelineData.audioSourceFileLength = 0;
				}

				if (res.flags & ImGuiTimelineResultFlags_AddAudioSource)
				{
					nfdchar_t* outPath = NULL;
					nfdresult_t result = NFD_OpenDialog("wav", NULL, &outPath);

					if (result == NFD_OKAY)
					{
						loadAudioSource(outPath);

						size_t pathLength = std::strlen(outPath);
						timelineData.audioSourceFile = (uint8*)g_memory_realloc(timelineData.audioSourceFile, sizeof(uint8) * (pathLength + 1));
						g_memory_copyMem(timelineData.audioSourceFile, outPath, sizeof(uint8) * (pathLength + 1));
						timelineData.audioSourceFileLength = pathLength;

						std::free(outPath);
					}
					else if (result == NFD_CANCEL)
					{
						g_logger_info("User cancelled adding audio source.");
					}
					else
					{
						g_logger_error("Error opening audio source:\n\t'{}'", NFD_GetError());
					}
				}

				if (res.flags & ImGuiTimelineResultFlags_DeleteActiveObject)
				{
					ImGuiTimeline_Track& track = tracks[res.trackIndex];
					deleteSegment(track, res.segmentIndex, am);

					//UndoSystem::removeAnimationFromTimeline(
					//	Application::getUndoSystem(),
					//	track.segments[res.segmentIndex].userData.as.idData
					//);
				}

				if (res.flags & ImGuiTimelineResultFlags_DragDropPayloadHit)
				{
					g_logger_assert(res.dragDropPayloadDataSize == sizeof(TimelinePayload), "Invalid payload.");
					TimelinePayload* payloadData = (TimelinePayload*)res.dragDropPayloadData;
					//Animation animation = Animation::createDefault(payloadData->animType, res.dragDropPayloadFirstFrame, 60);
					//animation.timelineTrack = res.trackIndex;
					//AnimationManager::addAnimation(am, animation);
					//addAnimation(animation);

					UndoSystem::addNewAnimationToTimeline(
						Application::getUndoSystem(),
						(int)payloadData->animType,
						res.dragDropPayloadFirstFrame,
						60 * 1,
						res.trackIndex
					);
				}

				static bool resetSegmentTime = true;
				static int ogFrameStart = 0;
				static int ogFrameDuration = 0;
				if (res.flags & ImGuiTimelineResultFlags_SegmentTimeChanged)
				{
					const ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
					AnimId animationId = segment.userData.as.idData;

					if (resetSegmentTime)
					{
						ogFrameStart = segment.frameStart;
						ogFrameDuration = segment.frameDuration;
						resetSegmentTime = false;
					}

					AnimationManager::setAnimationTime(
						am,
						animationId,
						segment.frameStart,
						segment.frameDuration
					);
				}

				if (res.flags & ImGuiTimelineResultFlags_SegmentTimeDragEnded)
				{
					resetSegmentTime = true;

					const ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
					AnimId animationId = segment.userData.as.idData;
					UndoSystem::setAnimationTime(
						Application::getUndoSystem(),
						animationId,
						ogFrameStart,
						ogFrameDuration,
						segment.frameStart,
						segment.frameDuration
					);
				}

				if (res.flags & ImGuiTimelineResultFlags_ActiveObjectDeselected)
				{
					InspectorPanel::setActiveAnimation(NULL_ANIM);
				}

				if (res.flags & ImGuiTimelineResultFlags_ActiveObjectChanged)
				{
					const ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
					InspectorPanel::setActiveAnimation(segment.userData.as.idData);
				}
			}

			ImGui::End();
			ImGui::PopStyleVar();
		}

		void updateAnimation(AnimId anim, int frameStart, int frameDuration) 
		{
			for (size_t i = 0; i < numTracks; i++)
			{
				ImGuiTimeline_Track& track = tracks[i];
				for (size_t segI = 0; segI < track.numSegments; segI++) 
				{
					ImGuiTimeline_Segment& segment = track.segments[segI];
					if (segment.userData.as.idData == anim) 
					{
						segment.frameStart = frameStart;
						segment.frameDuration = frameDuration;
						return;
					}
				}
			}
		}

		void addAnimation(const Animation& animation)
		{
			g_logger_assert(animation.timelineTrack < numTracks, "This shouldn't be happening");

			ImGuiTimeline_Track& track = tracks[animation.timelineTrack];

			track.numSegments++;
			if (track.segments != nullptr)
			{
				track.segments = (ImGuiTimeline_Segment*)g_memory_realloc(track.segments, sizeof(ImGuiTimeline_Segment) * track.numSegments);
			}
			else
			{
				g_logger_assert(track.numSegments == 1, "Should not have had null segments if there was a non-zero number of segments.");
				track.segments = (ImGuiTimeline_Segment*)g_memory_allocate(sizeof(ImGuiTimeline_Segment) * track.numSegments);
			}
			g_logger_assert(track.segments != nullptr, "Ran out of RAM.");

			track.segments[track.numSegments - 1].frameDuration = animation.duration;
			track.segments[track.numSegments - 1].frameStart = animation.frameStart;
			track.segments[track.numSegments - 1].segmentName = Animation::getAnimationName(animation.type);
			track.segments[track.numSegments - 1].userData.as.idData = animation.id;
		}

		void removeAnimation(AnimationManagerData* am, AnimId anim)
		{
			for (int i = 0; i < numTracks; i++)
			{
				ImGuiTimeline_Track& track = tracks[i];
				for (int segI = 0; segI < track.numSegments; segI++)
				{
					ImGuiTimeline_Segment& segment = track.segments[segI];
					if (segment.userData.as.idData == anim)
					{
						deleteSegment(track, segI, am);
						return;
					}
				}
			}
		}

		void freeInstance(TimelineData& timelineData)
		{
			// Free timeline data
			if (timelineData.audioSourceFile)
			{
				g_memory_free(timelineData.audioSourceFile);
			}
			timelineData.audioSourceFile = nullptr;
			timelineData.audioSourceFileLength = 0;
			timelineData.currentFrame = 0;
			timelineData.firstFrame = 0;
		}

		void free(AnimationManagerData* am)
		{
			// TODO: Synchronize this with freeInstance
			if (tracks)
			{
				for (int i = 0; i < numTracks; i++)
				{
					freeTrack(tracks[i], am);
				}
				g_memory_free(tracks);
			}

			Audio::free(audioSource);
			WavLoader::free(audioData);
			ImGuiTimeline_free();
		}

		void serialize(const TimelineData& timelineData, nlohmann::json& j)
		{
			g_logger_assert(timelineData.audioSourceFileLength < UINT32_MAX, "Corrupted timeline data. Somehow audio source file length > UINT32_MAX. Length: '{}'", timelineData.audioSourceFileLength);
			SERIALIZE_NULLABLE_U8_CSTRING(j, &timelineData, audioSourceFile, "Undefined");
			SERIALIZE_NON_NULL_PROP(j, &timelineData, firstFrame);
			SERIALIZE_NON_NULL_PROP(j, &timelineData, currentFrame);
			SERIALIZE_NON_NULL_PROP(j, &timelineData, zoomLevel);
		}

		TimelineData deserialize(const nlohmann::json& j)
		{
			TimelineData res = {};

			DESERIALIZE_NULLABLE_U8_CSTRING(&res, audioSourceFile, j);
			if (res.audioSourceFile && std::strcmp((char*)res.audioSourceFile, "Undefined") == 0) 
			{
				g_memory_free(res.audioSourceFile);
				res.audioSourceFile = nullptr;
				res.audioSourceFileLength = 0;
			} 

			DESERIALIZE_PROP(&res, firstFrame, j, 0);
			DESERIALIZE_PROP(&res, currentFrame, j, 0);
			DESERIALIZE_PROP(&res, zoomLevel, j, 1.0f);
			Application::setFrameIndex(res.currentFrame);

			return res;
		}

		TimelineData legacy_deserialize(RawMemory& memory)
		{
			TimelineData res = {};

			// audioSourceFileLength     -> uint32
			// audioSourceFile           -> uint8[audioSourceFileLength + 1]
			// firstFrame                -> int32
			// currentFrame              -> int32
			// zoomLevel                 -> float

			uint32 fileLengthU32;
			if (!memory.read<uint32>(&fileLengthU32))
			{
				g_logger_error("Corrupted timeline data in project file.");
				return res;
			}

			res.audioSourceFileLength = (size_t)fileLengthU32;
			res.audioSourceFile = (uint8*)g_memory_allocate(sizeof(uint8) * (res.audioSourceFileLength + 1));
			memory.readDangerous(res.audioSourceFile, res.audioSourceFileLength + 1);
			memory.read<int32>(&res.firstFrame);
			memory.read<int32>(&res.currentFrame);
			memory.read<float>(&res.zoomLevel);

			Application::setFrameIndex(res.currentFrame);

			return res;
		}

		// ------- Internal Functions --------
		static void loadAudioSource(const char* filepath)
		{
			Audio::free(audioSource);
			WavLoader::free(audioData);
			audioData = WavLoader::loadWavFile(filepath);
			audioSource = Audio::loadWavFile(audioData);

			// TODO: Popup an error if loading fails to let the user know
			if (!Audio::isNull(audioSource))
			{
				// Copy data to imgui struct if success
				imguiAudioData.bitsPerSample = audioData.bitsPerSample;
				imguiAudioData.blockAlignment = audioData.blockAlignment;
				imguiAudioData.bytesPerSec = audioData.bytesPerSec;
				imguiAudioData.data = audioData.audioData;
				imguiAudioData.dataSize = audioData.dataSize;
				imguiAudioData.numAudioChannels = audioData.audioChannelType == AudioChannelType::Dual ? 2 : 1;
				imguiAudioData.sampleRate = audioData.sampleRate;
			}
			else
			{
				g_memory_zeroMem(&imguiAudioData, sizeof(ImGuiTimeline_AudioData));
			}
		}

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
			char counterString[3];
#ifdef _itoa_s
			_itoa_s(counter, counterString, 10);
#else
			snprintf(counterString, sizeof(counterString), "%d", counter);
#endif

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
			counter++;

			return defaultTrack;
		}

		static void freeTrack(ImGuiTimeline_Track& track, AnimationManagerData* am)
		{
			AnimId currentActiveAnimationId = InspectorPanel::getActiveAnimation();

			if (track.segments)
			{
				for (int i = 0; i < track.numSegments; i++)
				{
					// Free all the animations and anim objects associated with this track
					AnimId animId = track.segments[i].userData.as.idData;

					if (animId == currentActiveAnimationId)
					{
						InspectorPanel::setActiveAnimation(NULL_ANIM);
						currentActiveAnimationId = NULL_ANIM;
					}

					AnimationManager::removeAnimation(am, animId);
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

		static void addNewDefaultTrack(AnimationManagerData* am, int insertIndex)
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
				g_logger_assert(insertIndex >= 0 && insertIndex < numTracks, "Invalid insert index '{}' for track.", insertIndex);
				// Move all tracks after insert index to the end
				std::memmove(tracks + insertIndex + 1, tracks + insertIndex, (numTracks - insertIndex - 1) * sizeof(ImGuiTimeline_Track));
				tracks[insertIndex] = createDefaultTrack();
			}

			for (int track = 0; track < numTracks; track++)
			{
				for (int segment = 0; segment < tracks[track].numSegments; segment++)
				{
					AnimId animationId = tracks[track].segments[segment].userData.as.idData;
					AnimationManager::setAnimationTrack(am, animationId, track);
				}
			}
		}

		static void deleteTrack(AnimationManagerData* am, int index)
		{
			g_logger_assert(index >= 0 && index < numTracks, "Invalid track index '{}' for deleting track.", index);
			freeTrack(tracks[index], am);

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
					AnimId animationId = tracks[track].segments[segment].userData.as.idData;
					AnimationManager::setAnimationTrack(am, animationId, track);
				}
			}

			// TODO: It seems to work fine without this... 
			// debug this a bit more and make sure this isn't actually needed
			// DEPRECATED:
			// After we delete the track, we have to reset our ImGui data
			// resetImGuiData();
		}

		static void setupImGuiTimelineDataFromAnimations(AnimationManagerData* am, int numTracksToCreate)
		{
			const std::vector<Animation>& animations = AnimationManager::getAnimations(am);

			// Find the max timeline track and add that many default tracks
			int maxTimelineTrack = -1;
			for (int i = 0; i < animations.size(); i++)
			{
				maxTimelineTrack = glm::max(animations[i].timelineTrack, maxTimelineTrack);
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
					addNewDefaultTrack(am);
				}
			}
			g_logger_assert(maxTimelineTrack + 1 <= numTracks, "Invalid num tracks to create.");

			// Initialize all the tracks memory
			for (int track = 0; track < maxTimelineTrack + 1; track++)
			{
				// Count how many segment there are
				int numSegments = 0;
				for (int i = 0; i < animations.size(); i++)
				{
					if (animations[i].timelineTrack == track)
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
			}

			// Initialize tracks data
			for (int track = 0; track < maxTimelineTrack + 1; track++)
			{
				int segment = 0;
				for (int i = 0; i < animations.size(); i++)
				{
					if (animations[i].timelineTrack == track)
					{
						tracks[track].segments[segment].frameStart = animations[i].frameStart;
						tracks[track].segments[segment].frameDuration = animations[i].duration;
						tracks[track].segments[segment].userData.as.idData = animations[i].id;
						tracks[track].segments[segment].segmentName = Animation::getAnimationName(animations[i].type);

						segment++;
					}
				}
			}
		}

		void deleteSegment(ImGuiTimeline_Track& track, int segmentIndex, AnimationManagerData* am)
		{
			// First delete it from our animations
			ImGuiTimeline_Segment& segment = track.segments[segmentIndex];
			AnimationManager::removeAnimation(am, segment.userData.as.idData);

			// Unset active object if needed
			if (segment.userData.as.idData == InspectorPanel::getActiveAnimObject())
			{
				InspectorPanel::setActiveAnimObject(am, NULL_ANIM_OBJECT);
			}

			// Then free the memory
			g_memory_zeroMem(&segment, sizeof(ImGuiTimeline_Segment));

			// Then move the animation objects over it to "delete" it
			// If it's the last entry, this will effectively move 0 elements
			std::memmove(&track.segments[segmentIndex], &track.segments[segmentIndex + 1], sizeof(ImGuiTimeline_Segment) * (track.numSegments - segmentIndex - 1));

			track.numSegments--;
			if (track.numSegments > 0)
			{
				track.segments = (ImGuiTimeline_Segment*)g_memory_realloc(track.segments, sizeof(ImGuiTimeline_Segment) * track.numSegments);
			}
			else
			{
				g_logger_assert(track.numSegments == 0, "That shouldn't happen.");
				g_memory_free(track.segments);
				track.segments = nullptr;
			}
		}
	}
}