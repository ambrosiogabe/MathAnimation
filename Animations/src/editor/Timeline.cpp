#include "core.h"
#include "core/Application.h"
#include "core/Platform.h"
#include "editor/Timeline.h"
#include "editor/ImGuiTimeline.h"
#include "editor/SceneHierarchyPanel.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "animation/Svg.h"
#include "renderer/Fonts.h"
#include "audio/Audio.h"
#include "audio/WavLoader.h"

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <nfd.h>

namespace MathAnim
{
	namespace Timeline
	{
		// ------- Private variables --------
		static ImGuiTimeline_Track* tracks;
		static int numTracks;
		static int activeAnimObjectId = INT32_MAX;
		static int activeAnimationId = INT32_MAX;
		static AudioSource audioSource;
		static WavData audioData;
		static ImGuiTimeline_AudioData imguiAudioData;

		static constexpr float slowDragSpeed = 0.02f;

		// ------- Internal Functions --------
		static void loadAudioSource(const char* filepath);

		static ImGuiTimeline_Track createDefaultTrack(char* trackName = nullptr);
		static void freeTrack(ImGuiTimeline_Track& track);
		static void addNewDefaultTrack(int insertIndex = INT32_MAX);
		static void deleteTrack(int index);
		static void handleAnimObjectInspector(int animObjectId);
		static void handleAnimationInspector(int animationId);
		static void handleTextObjectInspector(AnimObject* object);
		static void handleLaTexObjectInspector(AnimObject* object);
		static void handleMoveToAnimationInspector(Animation* animation);
		static void handleRotateToAnimationInspector(Animation* animation);
		static void handleAnimateStrokeColorAnimationInspector(Animation* animation);
		static void handleAnimateFillColorAnimationInspector(Animation* animation);
		static void handleAnimateCameraMoveToAnimationInspector(Animation* animation);
		static void handleSquareInspector(AnimObject* object);
		static void handleCircleInspector(AnimObject* object);
		static void handleCubeInspector(AnimObject* object);
		static void handleAxisInspector(AnimObject* object);

		static void setupImGuiTimelineDataFromAnimations(int numTracksToCreate = INT32_MAX);
		static void addAnimObject(const AnimObject& object);
		static void addAnimation(const Animation& animation, int animObjectId);
		static void deleteSegment(ImGuiTimeline_Track& track, int segmentIndex);
		static void deleteSubSegment(ImGuiTimeline_Segment& segment, int subSegmentIndex);
		static void resetImGuiData();

		TimelineData initInstance()
		{
			TimelineData res;
			res.audioSourceFile = nullptr;
			res.audioSourceFileLength = 0;
			res.currentFrame = 0;
			res.firstFrame = 0;
			res.zoomLevel = 5.0f;
			return res;
		}

		void init()
		{
			audioSource = Audio::defaultAudioSource();

			// I have to allocate a dummy allocation here because my memory utilities library
			// doesn't let realloc work with nullptr *yet*
			tracks = (ImGuiTimeline_Track*)g_memory_allocate(sizeof(ImGuiTimeline_Track));
			numTracks = 0;

			// Initialize some dummy data for now
			setupImGuiTimelineDataFromAnimations();
		}

		void update(TimelineData& timelineData)
		{
			// NOTE: For best results, it's usually a good idea to specify 0 padding for the window
			// so that the timeline can expand to the full width/height of the window
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::Begin("Timeline");

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
				addNewDefaultTrack(res.trackIndex);
			}

			if (res.flags & ImGuiTimelineResultFlags_DeleteTrackClicked)
			{
				deleteTrack(res.trackIndex);
			}

			// If we have a filepath for an audio source and the current audio source is null
			// try to load it
			if (timelineData.audioSourceFileLength > 0 && Audio::isNull(audioSource))
			{
				loadAudioSource((const char*)timelineData.audioSourceFile);

				if (Audio::isNull(audioSource))
				{
					// Failed to load the file, there must be something wrong with it.
					g_logger_error("Failed to load audio source file '%s'", timelineData.audioSourceFile);
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
					g_logger_error("Error opening audio source:\n\t%s", NFD_GetError());
				}
			}

			if (res.flags & ImGuiTimelineResultFlags_DeleteActiveObject)
			{
				if (res.activeObjectIsSubSegment)
				{
					ImGuiTimeline_Segment& segment = tracks[res.trackIndex].segments[res.segmentIndex];
					deleteSubSegment(segment, res.subSegmentIndex);
				}
				else
				{
					ImGuiTimeline_Track& track = tracks[res.trackIndex];
					deleteSegment(track, res.segmentIndex);
				}
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
						SceneHierarchyPanel::addNewAnimObject(object);
						addAnimObject(object);
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
						addAnimation(animation, animObjectId);
					}
				}
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

			if (res.flags & ImGuiTimelineResultFlags_ActiveObjectDeselected)
			{
				activeAnimationId = INT32_MAX;
				activeAnimObjectId = INT32_MAX;
			}

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

		void free()
		{
			// TODO: Synchronize this with freeInstance
			if (tracks)
			{
				for (int i = 0; i < numTracks; i++)
				{
					freeTrack(tracks[i]);
				}
				g_memory_free(tracks);
			}

			Audio::free(audioSource);
			WavLoader::free(audioData);
			ImGuiTimeline_free();
		}

		void setActiveAnimObject(int animObjectId)
		{
			activeAnimObjectId = animObjectId;
			activeAnimationId = INT32_MAX;
		}

		int getActiveAnimObject()
		{
			return activeAnimObjectId;
		}

		RawMemory serialize(const TimelineData& timelineData)
		{
			// audioSourceFileLength     -> uint32
			// audioSourceFile           -> uint8[audioSourceFileLength + 1]
			// firstFrame                -> int32
			// currentFrame              -> int32
			// zoomLevel                 -> float
			RawMemory res;
			res.init(sizeof(TimelineData));

			g_logger_assert(timelineData.audioSourceFileLength < UINT32_MAX, "Corrupted timeline data. Somehow audio source file length > UINT32_MAX '%d'", timelineData.audioSourceFileLength);
			uint32 fileLengthU32 = (uint32)timelineData.audioSourceFileLength;
			res.write<uint32>(&fileLengthU32);
			res.writeDangerous(timelineData.audioSourceFile, fileLengthU32);
			uint8 nullByte = '\0';
			res.write<uint8>(&nullByte);
			res.write<int32>(&timelineData.firstFrame);
			res.write<int32>(&timelineData.currentFrame);
			res.write<float>(&timelineData.zoomLevel);

			res.shrinkToFit();

			return res;
		}

		TimelineData deserialize(RawMemory& memory)
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

					if (animObjectId == activeAnimObjectId)
					{
						activeAnimObjectId = INT32_MAX;
					}

					AnimationManager::removeAnimObject(animObjectId);

					if (track.segments[i].subSegments)
					{
						for (int subi = 0; subi < track.segments[i].numSubSegments; subi++)
						{
							if ((uintptr_t)track.segments[i].subSegments[subi].userData == activeAnimationId)
							{
								activeAnimationId = INT32_MAX;
							}
						}

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

			// TODO: It seems to work fine without this... 
			// debug this a bit more and make sure this isn't actually needed
			// DEPRECATED:
			// After we delete the track, we have to reset our ImGui data
			// resetImGuiData();
		}

		static void handleAnimObjectInspector(int animObjectId)
		{
			AnimObject* animObject = AnimationManager::getMutableObject(animObjectId);
			if (!animObject)
			{
				g_logger_error("No anim object with id '%d' exists", animObject);
				activeAnimObjectId = INT32_MAX;
				return;
			}

			bool objChanged = false;

			constexpr int scratchLength = 256;
			char scratch[scratchLength] = {};
			if (animObject->nameLength < scratchLength - 1)
			{
				g_memory_copyMem(scratch, animObject->name, animObject->nameLength * sizeof(char));
				scratch[animObject->nameLength] = '\0';
				if (ImGui::InputText(": Name", scratch, scratchLength * sizeof(char)))
				{
					size_t newLength = std::strlen(scratch);
					animObject->name = (uint8*)g_memory_realloc(animObject->name, sizeof(char) * (newLength + 1));
					animObject->nameLength = (int32_t)newLength;
					g_memory_copyMem(animObject->name, scratch, newLength * sizeof(char));
					animObject->name[newLength] = '\0';
				}
			}
			else
			{
				g_logger_error("Anim Object name has more 256 characters. Tell Gabe to increase scratch length for Anim Object names.");
			}

			objChanged = ImGui::DragFloat3(": Position", (float*)&animObject->_positionStart.x, slowDragSpeed) || objChanged;
			objChanged = ImGui::DragFloat3(": Rotation", (float*)&animObject->_rotationStart.x) || objChanged;
			objChanged = ImGui::DragFloat3(": Scale", (float*)&animObject->_scaleStart.x, slowDragSpeed) || objChanged;
			objChanged = ImGui::DragFloat(": SVG Scale", &animObject->svgScale, slowDragSpeed) || objChanged;

			// NanoVG only allows stroke width between [0-200] so we reflect that here
			objChanged = ImGui::DragFloat(": Stroke Width", (float*)&animObject->_strokeWidthStart, slowDragSpeed, 0.0f, 10.0f) || objChanged;
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
				objChanged = true;
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
				objChanged = true;
			}

			objChanged = ImGui::Checkbox(": Is Transparent", &animObject->isTransparent) || objChanged;
			objChanged = ImGui::Checkbox(": Is 3D", &animObject->is3D) || objChanged;
			objChanged = ImGui::Checkbox(": Draw Debug Boxes", &animObject->drawDebugBoxes) || objChanged;
			if (animObject->drawDebugBoxes)
			{
				objChanged = ImGui::Checkbox(": Draw Curve Debug Boxes", &animObject->drawCurveDebugBoxes) || objChanged;
			}

			switch (animObject->objectType)
			{
			case AnimObjectTypeV1::TextObject:
				handleTextObjectInspector(animObject);
				break;
			case AnimObjectTypeV1::LaTexObject:
				handleLaTexObjectInspector(animObject);
				break;
			case AnimObjectTypeV1::Square:
				handleSquareInspector(animObject);
				break;
			case AnimObjectTypeV1::Circle:
				handleCircleInspector(animObject);
				break;
			case AnimObjectTypeV1::Cube:
				handleCubeInspector(animObject);
				break;
			case AnimObjectTypeV1::Axis:
				handleAxisInspector(animObject);
				break;
			default:
				g_logger_error("Unknown anim object type: %d", (int)animObject->objectType);
				break;
			}

			if (objChanged)
			{
				for (int i = 0; i < animObject->children.size(); i++)
				{
					animObject->children[i].takeParentAttributes(animObject);
				}
			}
		}

		static void handleAnimationInspector(int animationId)
		{
			Animation* animation = AnimationManager::getMutableAnimation(animationId);
			if (!animation)
			{
				g_logger_error("No animation with id '%d' exists", animationId);
				activeAnimationId = INT32_MAX;
				return;
			}

			int currentType = (int)(animation->easeType) - 1;
			if (ImGui::Combo(": Ease Type", &currentType, &easeTypeNames[1], (int)EaseType::Length - 1))
			{
				g_logger_assert(currentType >= 0 && currentType + 1 < (int)EaseType::Length, "How did this happen?");
				animation->easeType = (EaseType)(currentType + 1);
			}

			int currentDirection = (int)(animation->easeDirection) - 1;
			if (ImGui::Combo(": Ease Direction", &currentDirection, &easeDirectionNames[1], (int)EaseDirection::Length - 1))
			{
				g_logger_assert(currentDirection >= 0 && currentDirection + 1 < (int)EaseDirection::Length, "How did this happen?");
				animation->easeDirection = (EaseDirection)(currentDirection + 1);
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
			case AnimTypeV1::RotateTo:
				handleRotateToAnimationInspector(animation);
				break;
			case AnimTypeV1::AnimateFillColor:
				handleAnimateFillColorAnimationInspector(animation);
				break;
			case AnimTypeV1::AnimateStrokeColor:
				handleAnimateStrokeColorAnimationInspector(animation);
				break;
			case AnimTypeV1::CameraMoveTo:
				handleAnimateCameraMoveToAnimationInspector(animation);
				break;
			default:
				g_logger_error("Unknown animation type: %d", (int)animation->type);
				break;
			}
		}

		static void handleTextObjectInspector(AnimObject* object)
		{
			const std::vector<std::string>& fonts = Platform::getAvailableFonts();
			int fontIndex = -1;
			if (object->as.textObject.font != nullptr)
			{
				std::filesystem::path fontFilepath = object->as.textObject.font->fontFilepath;
				fontFilepath.make_preferred();
				int index = 0;
				for (const auto& font : fonts)
				{
					std::filesystem::path f1 = std::filesystem::path(font);
					f1.make_preferred();
					if (fontFilepath == f1)
					{
						fontIndex = index;
						break;
					}
					index++;
				}
			}
			const char* previewValue = "No Font Selected";
			if (fontIndex != -1)
			{
				previewValue = fonts[fontIndex].c_str();
			}
			else
			{
				if (object->as.textObject.font != nullptr)
				{
					g_logger_warning("Could not find font %s", object->as.textObject.font->fontFilepath.c_str());
				}
			}

			if (ImGui::BeginCombo(": Font", previewValue))
			{
				for (int n = 0; n < fonts.size(); n++)
				{
					const bool is_selected = (fontIndex == n);
					if (ImGui::Selectable(fonts[n].c_str(), is_selected))
					{
						if (object->as.textObject.font)
						{
							Fonts::unloadFont(object->as.textObject.font);
						}
						object->as.textObject.font = Fonts::loadFont(fonts[n].c_str(), Application::getNvgContext());
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

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

		static void handleLaTexObjectInspector(AnimObject* object)
		{
			constexpr int scratchLength = 2048;
			char scratch[scratchLength] = {};
			if (object->as.laTexObject.textLength >= scratchLength)
			{
				g_logger_error("Text object has more than %d characters. Tell Gabe to increase scratch length for LaTex objects.", scratchLength);
				return;
			}
			g_memory_copyMem(scratch, object->as.laTexObject.text, object->as.laTexObject.textLength * sizeof(char));
			scratch[object->as.laTexObject.textLength] = '\0';
			if (ImGui::InputTextMultiline(": LaTeX", scratch, scratchLength * sizeof(char)))
			{
				size_t newLength = std::strlen(scratch);
				object->as.laTexObject.text = (char*)g_memory_realloc(object->as.laTexObject.text, sizeof(char) * (newLength + 1));
				object->as.laTexObject.textLength = (int32_t)newLength;
				g_memory_copyMem(object->as.laTexObject.text, scratch, newLength * sizeof(char));
				object->as.laTexObject.text[newLength] = '\0';
			}

			ImGui::Checkbox(": Is Equation", &object->as.laTexObject.isEquation);

			// Disable the generate button if it's currently parsing some SVG
			bool disableButton = object->as.laTexObject.isParsingLaTex;
			if (disableButton)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
			if (ImGui::Button("Generate LaTeX"))
			{
				object->as.laTexObject.parseLaTex();
			}
			if (disableButton)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
		}

		static void handleMoveToAnimationInspector(Animation* animation)
		{
			ImGui::DragFloat3(": Target Position", &animation->as.modifyVec3.target.x, slowDragSpeed);
		}

		static void handleRotateToAnimationInspector(Animation* animation)
		{
			ImGui::DragFloat3(": Target Rotation", &animation->as.modifyVec3.target.x);
		}

		static void handleAnimateStrokeColorAnimationInspector(Animation* animation)
		{
			float strokeColor[4] = {
				(float)animation->as.modifyU8Vec4.target.r / 255.0f,
				(float)animation->as.modifyU8Vec4.target.g / 255.0f,
				(float)animation->as.modifyU8Vec4.target.b / 255.0f,
				(float)animation->as.modifyU8Vec4.target.a / 255.0f,
			};
			if (ImGui::ColorEdit4(": Stroke Color", strokeColor))
			{
				animation->as.modifyU8Vec4.target.r = (uint8)(strokeColor[0] * 255.0f);
				animation->as.modifyU8Vec4.target.g = (uint8)(strokeColor[1] * 255.0f);
				animation->as.modifyU8Vec4.target.b = (uint8)(strokeColor[2] * 255.0f);
				animation->as.modifyU8Vec4.target.a = (uint8)(strokeColor[3] * 255.0f);
			}
		}

		static void handleAnimateFillColorAnimationInspector(Animation* animation)
		{
			float strokeColor[4] = {
				(float)animation->as.modifyU8Vec4.target.r / 255.0f,
				(float)animation->as.modifyU8Vec4.target.g / 255.0f,
				(float)animation->as.modifyU8Vec4.target.b / 255.0f,
				(float)animation->as.modifyU8Vec4.target.a / 255.0f,
			};
			if (ImGui::ColorEdit4(": Fill Color", strokeColor))
			{
				animation->as.modifyU8Vec4.target.r = (uint8)(strokeColor[0] * 255.0f);
				animation->as.modifyU8Vec4.target.g = (uint8)(strokeColor[1] * 255.0f);
				animation->as.modifyU8Vec4.target.b = (uint8)(strokeColor[2] * 255.0f);
				animation->as.modifyU8Vec4.target.a = (uint8)(strokeColor[3] * 255.0f);
			}
		}

		static void handleAnimateCameraMoveToAnimationInspector(Animation* animation)
		{
			ImGui::DragFloat2(": Camera Target Position", &animation->as.modifyVec2.target.x, slowDragSpeed);
		}

		static void handleSquareInspector(AnimObject* object)
		{
			if (ImGui::DragFloat(": Side Length", &object->as.square.sideLength, slowDragSpeed))
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
			if (ImGui::DragFloat(": Radius", &object->as.circle.radius, slowDragSpeed))
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

		static void handleCubeInspector(AnimObject* object)
		{
			if (ImGui::DragFloat(": Side Length", &object->as.cube.sideLength, slowDragSpeed))
			{
				for (int i = 0; i < object->children.size(); i++)
				{
					object->children[i].free();
				}
				object->children.clear();

				object->as.cube.init(object);
			}
		}

		static void handleAxisInspector(AnimObject* object)
		{
			bool reInitObject = false;

			reInitObject = ImGui::DragFloat3(": Axes Length", object->as.axis.axesLength.values, slowDragSpeed) || reInitObject;

			int xVals[2] = { object->as.axis.xRange.min, object->as.axis.xRange.max };
			if (ImGui::DragInt2(": X-Range", xVals, slowDragSpeed))
			{
				// Make sure it's in strictly increasing order
				if (xVals[0] < xVals[1])
				{
					object->as.axis.xRange.min = xVals[0];
					object->as.axis.xRange.max = xVals[1];
					reInitObject = true;
				}
			}

			int yVals[2] = { object->as.axis.yRange.min, object->as.axis.yRange.max };
			if (ImGui::DragInt2(": Y-Range", yVals, slowDragSpeed))
			{
				// Make sure it's in strictly increasing order
				if (yVals[0] < yVals[1])
				{
					object->as.axis.yRange.min = yVals[0];
					object->as.axis.yRange.max = yVals[1];
					reInitObject = true;
				}
			}

			int zVals[2] = { object->as.axis.zRange.min, object->as.axis.zRange.max };
			if (ImGui::DragInt2(": Z-Range", zVals, slowDragSpeed))
			{
				// Make sure it's in strictly increasing order
				if (zVals[0] < zVals[1])
				{
					object->as.axis.zRange.min = zVals[0];
					object->as.axis.zRange.max = zVals[1];
					reInitObject = true;
				}
			}

			reInitObject = ImGui::DragFloat(": X-Increment", &object->as.axis.xStep, slowDragSpeed) || reInitObject;
			reInitObject = ImGui::DragFloat(": Y-Increment", &object->as.axis.yStep, slowDragSpeed) || reInitObject;
			reInitObject = ImGui::DragFloat(": Z-Increment", &object->as.axis.zStep, slowDragSpeed) || reInitObject;
			reInitObject = ImGui::DragFloat(": Tick Width", &object->as.axis.tickWidth, slowDragSpeed) || reInitObject;
			reInitObject = ImGui::Checkbox(": Draw Labels", &object->as.axis.drawNumbers) || reInitObject;
			if (object->as.axis.drawNumbers)
			{
				reInitObject = ImGui::DragFloat(": Font Size Pixels", &object->as.axis.fontSizePixels, slowDragSpeed, 0.0f, 300.0f) || reInitObject;
				reInitObject = ImGui::DragFloat(": Label Padding", &object->as.axis.labelPadding, slowDragSpeed, 0.0f, 500.0f) || reInitObject;
				reInitObject = ImGui::DragFloat(": Label Stroke Width", &object->as.axis.labelStrokeWidth, slowDragSpeed, 0.0f, 200.0f) || reInitObject;
			}

			//if (ImGui::Checkbox(": Is 3D", &object->as.axis.is3D))
			//{
			//	// Reset to default values if we toggle 3D on or off
			//	if (object->as.axis.is3D)
			//	{
			//		object->_positionStart = Vec3{ 0.0f, 0.0f, 0.0f };
			//		object->as.axis.axesLength = Vec3{ 8.0f, 5.0f, 8.0f };
			//		object->as.axis.xRange = { 0, 8 };
			//		object->as.axis.yRange = { 0, 5 };
			//		object->as.axis.zRange = { 0, 8 };
			//		object->as.axis.tickWidth = 0.2f;
			//		object->_strokeWidthStart = 0.05f;
			//	}
			//	else
			//	{
			//		glm::vec2 outputSize = Application::getOutputSize();
			//		object->_positionStart = Vec3{ outputSize.x / 2.0f, outputSize.y / 2.0f, 0.0f };
			//		object->as.axis.axesLength = Vec3{ 3'000.0f, 1'700.0f, 1.0f };
			//		object->as.axis.xRange = { 0, 18 };
			//		object->as.axis.yRange = { 0, 10 };
			//		object->as.axis.zRange = { 0, 10 };
			//		object->as.axis.tickWidth = 75.0f;
			//		object->_strokeWidthStart = 7.5f;
			//	}
			//	reInitObject = true;
			//}

			if (reInitObject)
			{
				for (int i = 0; i < object->children.size(); i++)
				{
					object->children[i].free();
				}
				object->children.clear();

				object->as.axis.init(object);
			}
		}

		static void setupImGuiTimelineDataFromAnimations(int numTracksToCreate)
		{
			const std::vector<AnimObject>& animObjects = AnimationManager::getAnimObjects();

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

		static void addAnimObject(const AnimObject& object)
		{
			g_logger_assert(object.timelineTrack < numTracks, "This shouldn't be happening");

			ImGuiTimeline_Track& track = tracks[object.timelineTrack];

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

			track.segments[track.numSegments - 1].frameDuration = object.duration;
			track.segments[track.numSegments - 1].frameStart = object.frameStart;
			track.segments[track.numSegments - 1].segmentName = AnimationManager::getAnimObjectName(object.objectType);
			track.segments[track.numSegments - 1].userData.as.intData = object.id;

			if (object.animations.size() == 0)
			{
				track.segments[track.numSegments - 1].numSubSegments = 0;
				track.segments[track.numSegments - 1].subSegments = nullptr;
			}
			else
			{
				ImGuiTimeline_SubSegment* subSegments = (ImGuiTimeline_SubSegment*)g_memory_allocate(sizeof(ImGuiTimeline_SubSegment) * object.animations.size());
				int numSubSegments = object.animations.size();
				for (int i = 0; i < numSubSegments; i++)
				{
					subSegments[i].frameStart = object.animations[i].frameStart;
					subSegments[i].frameDuration = object.animations[i].duration;
					subSegments[i].segmentName = AnimationManager::getAnimationName(object.animations[i].type);
					subSegments[i].userData = (void*)(uintptr_t)object.animations[i].id;
				}

				track.segments[track.numSegments - 1].subSegments = subSegments;
				track.segments[track.numSegments - 1].numSubSegments = numSubSegments;
			}
		}

		static void addAnimation(const Animation& animation, int animObjectId)
		{
			for (int tracki = 0; tracki < numTracks; tracki++)
			{
				const ImGuiTimeline_Track& track = tracks[tracki];
				for (int segmenti = 0; segmenti < track.numSegments; segmenti++)
				{
					ImGuiTimeline_Segment& segment = track.segments[segmenti];
					if (segment.userData.as.intData == animObjectId)
					{
						// Add subsegment here
						segment.numSubSegments++;
						if (segment.subSegments == nullptr)
						{
							g_logger_assert(segment.numSubSegments == 1, "Should not have had null sub segment data for non-zero subsegemnt size.");
							segment.subSegments = (ImGuiTimeline_SubSegment*)g_memory_allocate(sizeof(ImGuiTimeline_SubSegment) * segment.numSubSegments);
						}
						else
						{
							segment.subSegments = (ImGuiTimeline_SubSegment*)g_memory_realloc(segment.subSegments, sizeof(ImGuiTimeline_SubSegment) * segment.numSubSegments);
						}
						g_logger_assert(segment.subSegments != nullptr, "Ran out of RAM.");

						segment.subSegments[segment.numSubSegments - 1].frameStart = animation.frameStart;
						segment.subSegments[segment.numSubSegments - 1].frameDuration = animation.duration;
						segment.subSegments[segment.numSubSegments - 1].segmentName = AnimationManager::getAnimationName(animation.type);
						segment.subSegments[segment.numSubSegments - 1].userData = (void*)(uintptr_t)animation.id;
						return;
					}
				}
			}

			g_logger_assert(false, "Failed to find animation object with id %d.\nCould not add animation.", animObjectId);
		}

		static void deleteSegment(ImGuiTimeline_Track& track, int segmentIndex)
		{
			// First delete it from our animations
			ImGuiTimeline_Segment& segment = track.segments[segmentIndex];
			AnimationManager::removeAnimObject(segment.userData.as.intData);

			// Unset active object if needed
			if (segment.userData.as.intData == activeAnimObjectId)
			{
				activeAnimObjectId = INT32_MAX;
			}

			// Then free the memory
			if (segment.subSegments)
			{
				g_memory_free(segment.subSegments);
				segment.subSegments = nullptr;
			}
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

		static void deleteSubSegment(ImGuiTimeline_Segment& segment, int subSegmentIndex)
		{
			// First delete it from our animations
			ImGuiTimeline_SubSegment& subSegment = segment.subSegments[subSegmentIndex];
			AnimationManager::removeAnimation(segment.userData.as.intData, (uintptr_t)subSegment.userData);

			// Unset active object if needed
			if ((uintptr_t)subSegment.userData == activeAnimationId)
			{
				activeAnimationId = INT32_MAX;
			}

			// Then zero the memory
			g_memory_zeroMem(&subSegment, sizeof(ImGuiTimeline_SubSegment));

			// Then move the animations over it to "delete" it
			// If it's the last entry, this will effectively move 0 elements
			std::memmove(&segment.subSegments[subSegmentIndex], &segment.subSegments[subSegmentIndex + 1], sizeof(ImGuiTimeline_SubSegment) * (segment.numSubSegments - subSegmentIndex - 1));

			segment.numSubSegments--;
			if (segment.numSubSegments > 0)
			{
				segment.subSegments = (ImGuiTimeline_SubSegment*)g_memory_realloc(segment.subSegments, sizeof(ImGuiTimeline_SubSegment) * segment.numSubSegments);
			}
			else
			{
				g_logger_assert(segment.numSubSegments == 0, "That shouldn't happen.");
				g_memory_free(segment.subSegments);
				segment.subSegments = nullptr;
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