#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "renderer/Renderer.h"
#include "renderer/Texture.h"
#include "renderer/Framebuffer.h"
#include "renderer/OrthoCamera.h"
#include "svg/Svg.h"
#include "core/Application.h"

namespace MathAnim
{
	struct AnimationManagerData
	{
		SceneSnapshot startingSnapshot;
		SceneSnapshot currentSnapshot;
		std::vector<SceneSnapshot> snapshots;

		// Always sorted by startFrame and trackIndex
		std::vector<Animation> animations;
		// Maps From AnimationId -> Index in animations vector
		std::unordered_map<AnimId, size_t> animationIdMap;

		// These queues are so that we can add/remove elements
		// in the middle of the frame without any re-allocations
		// which would invalidate pointers
		std::vector<AnimObject> queuedAddObjects;
		std::vector<AnimObjId> queuedRemoveObjects;
		std::vector<Animation> queuedAddAnimations;
		std::vector<AnimId> queuedRemoveAnimations;

		// NOTE(gabe): So this is due to my whacky architecture, but at the beginning of rendering
		// each frame we actually need to reset the camera position to its start position. I really
		// need to figure out a better architecture for this haha
		Vec2 sceneCamera2DStartPos;
		OrthoCamera* sceneCamera2D;
		int currentFrame;
	};

	namespace AnimationManager
	{
		// ------- Private variables --------
		static const char* animationObjectTypeNames[] = {
			"None",
			"Text Object",
			"LaTex Object",
			"Square",
			"Circle",
			"Cube",
			"Axis",
			"Length"
		};

		static const char* animationTypeNames[] = {
			"None",
			"Write In Text",
			"Move To",
			"Create",
			"Replacement Transform",
			"Un-Create",
			"Fade In",
			"Fade Out",
			"Rotate To",
			"Animate Stroke Color",
			"Animate Fill Color",
			"Animate Stroke Width",
			"Move Camera To",
			"Shift",
			"Length",
		};

		// -------- Internal Functions --------
		static void deserializeAnimationManagerExV1(AnimationManagerData* am, RawMemory& memory);
		static bool compareAnimation(const Animation& a1, const Animation& a2);
		static void updateGlobalTransform(AnimObject& obj);
		static void addQueuedAnimObject(AnimationManagerData* am, const AnimObject& obj);
		static void addQueuedAnimation(AnimationManagerData* am, const Animation& anim);
		static void removeQueuedAnimObject(AnimationManagerData* am, AnimObjId animObj);
		static void removeQueuedAnimation(AnimationManagerData* am, AnimId animation);
		static void removeSingleAnimObject(AnimationManagerData* am, AnimObjId animObj);
		static void removeSingleAnimObjectSnapshot(SceneSnapshot& snapshot, AnimObjId animObj);
		static void freeSnapshot(SceneSnapshot& snapshot);
		static void applyDelta(AnimationManagerData* am, NVGcontext* vg, int deltaFrame);
		static void applyGlobalTransforms(SceneSnapshot& snapshot);
		static void applyAnimationsFrom(AnimationManagerData* am, NVGcontext* vg, SceneSnapshot& snapshot, int startIndex, int frame);

		AnimationManagerData* create(OrthoCamera& camera)
		{
			void* animManagerMemory = g_memory_allocate(sizeof(AnimationManagerData));
			// Placement new to ensure the vectors and stuff are appropriately constructed
			// but I can still use my memory tracker
			AnimationManagerData* res = new(animManagerMemory)AnimationManagerData();

			res->sceneCamera2D = &camera;
			res->sceneCamera2DStartPos = res->sceneCamera2D->position;
			res->currentFrame = 0;
			return res;
		}

		void free(AnimationManagerData* am)
		{
			if (am)
			{
				// Free all snapshots
				for (auto& snapshot : am->snapshots)
				{
					freeSnapshot(snapshot);
				}

				// Free starting and current snapshot
				freeSnapshot(am->startingSnapshot);
				freeSnapshot(am->currentSnapshot);

				// Free animations and stuff
				for (int i = 0; i < am->animations.size(); i++)
				{
					am->animations[i].free();
				}

				// Call destructor to properly destruct vector objects
				am->~AnimationManagerData();
				g_memory_free(am);
			}
		}

		void endFrame(AnimationManagerData* am)
		{
			// Remove all queued delete objects
			for (auto animObjId : am->queuedRemoveObjects)
			{
				removeQueuedAnimObject(am, animObjId);
			}
			// Clear queue
			am->queuedRemoveObjects.clear();

			// Remove all queued delete animations
			for (auto animId : am->queuedRemoveAnimations)
			{
				removeQueuedAnimation(am, animId);
			}
			// Clear queue
			am->queuedRemoveAnimations.clear();

			// Add all queued objects
			for (const auto& obj : am->queuedAddObjects)
			{
				addQueuedAnimObject(am, obj);
			}
			// Clear queue
			am->queuedAddObjects.clear();

			// Add all queued animations
			for (const auto& animation : am->queuedAddAnimations)
			{
				addQueuedAnimation(am, animation);
			}
			// Clear queue
			am->queuedAddAnimations.clear();
		}

		SceneSnapshot buildSnapshot(AnimationManagerData* am, NVGcontext* vg, uint32 absoluteFrame)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Reset everything to state at frame 0
			SceneSnapshot newSnapshot;

			newSnapshot.camera2DPos = am->startingSnapshot.camera2DPos;

			// Copy all the starting anim objects to the new snapshot
			for (auto objectIter = am->startingSnapshot.objects.begin(); objectIter != am->startingSnapshot.objects.end(); objectIter++)
			{
				// Reset to original state and apply animations in order
				AnimObject obj = AnimObject::createCopy(*objectIter);
				newSnapshot.objectIdMap[obj.id] = newSnapshot.objects.size();
				newSnapshot.objects.emplace_back(obj);
			}

			// Then apply each animation up to the current frame
			for (auto objectIter = newSnapshot.objects.begin(); objectIter != newSnapshot.objects.end(); objectIter++)
			{
				// Update any updateable objects
				// TODO: Is this important?
				if (objectIter->objectType == AnimObjectTypeV1::LaTexObject)
				{
					objectIter->as.laTexObject.update();
				}
			}

			applyAnimationsFrom(am, vg, newSnapshot, 0, absoluteFrame);
			applyGlobalTransforms(newSnapshot);

			return newSnapshot;
		}

		void addAnimObject(AnimationManagerData* am, const AnimObject& object)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// These adds get queued until the end of the frame so that
			// pointers are stable for at least the duration of one frame
			am->queuedAddObjects.push_back(object);
		}

		void addAnimation(AnimationManagerData* am, const Animation& animation)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// These adds get queued until the end of the frame so that
			// pointers are stable for at least the duration of one frame
			am->queuedAddAnimations.push_back(animation);
		}

		void updateAnimObjectAllSnapshots(AnimationManagerData* am, const AnimObject& objRef)
		{
			AnimObject reference = AnimObject::createCopy(objRef);

			for (auto& snapshot : am->snapshots)
			{
				AnimObject* obj = getMutableObjectFromSnapshot(snapshot, objRef.id);
				obj->free();
				*obj = AnimObject::createCopy(reference);
			}

			AnimObject* obj1 = getMutableObjectFromSnapshot(am->startingSnapshot, objRef.id);
			obj1->free();
			*obj1 = AnimObject::createCopy(reference);

			AnimObject* obj2 = getMutableObjectFromSnapshot(am->currentSnapshot, objRef.id);
			obj2->free();
			*obj2 = AnimObject::createCopy(reference);

			reference.free();
		}

		void removeAnimObject(AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// These removes get queued until the end of the frame so that
			// pointers are stable for at least the duration of one frame
			am->queuedRemoveObjects.push_back(animObj);
		}

		void removeAnimation(AnimationManagerData* am, AnimId anim)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// These removes get queued until the end of the frame so that
			// pointers are stable for at least the duration of one frame
			am->queuedRemoveAnimations.push_back(anim);
		}

		bool setAnimationTime(AnimationManagerData* am, AnimId anim, int frameStart, int duration)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the animation then reinsert it. That way we make sure the list
			// stays sorted
			Animation animationCopy;
			auto animationIndexIter = am->animationIdMap.find(anim);

			if (animationIndexIter != am->animationIdMap.end())
			{
				size_t animationIndex = animationIndexIter->second;
				if (animationIndex >= 0 && animationIndex < am->animations.size())
				{
					animationCopy = am->animations[animationIndex];
					removeAnimation(am, anim);

					animationCopy.frameStart = frameStart;
					animationCopy.duration = duration;

					// This should update it's index and any other effected indices
					addAnimation(am, animationCopy);
					return true;
				}
			}

			return false;

		}

		void setAnimationTrack(AnimationManagerData* am, AnimId anim, int track)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			Animation* animation = getMutableAnimation(am, anim);
			if (animation)
			{
				animation->timelineTrack = track;
			}
		}

		Framebuffer prepareFramebuffer(int outputWidth, int outputHeight)
		{
			Texture compositeTexture = TextureBuilder()
				.setFormat(ByteFormat::RGBA8_UI)
				.setMinFilter(FilterMode::Linear)
				.setMagFilter(FilterMode::Linear)
				.setWidth(outputWidth)
				.setHeight(outputHeight)
				.build();

			Texture accumulationTexture = TextureBuilder()
				.setFormat(ByteFormat::RGBA16_F)
				.setMinFilter(FilterMode::Linear)
				.setMagFilter(FilterMode::Linear)
				.setWidth(outputWidth)
				.setHeight(outputHeight)
				.build();

			Texture revelageTexture = TextureBuilder()
				.setFormat(ByteFormat::R8_F)
				.setMinFilter(FilterMode::Linear)
				.setMagFilter(FilterMode::Linear)
				.setWidth(outputWidth)
				.setHeight(outputHeight)
				.build();

			Texture objIdTexture = TextureBuilder()
				.setFormat(ByteFormat::RG32_UI)
				.setMinFilter(FilterMode::Nearest)
				.setMagFilter(FilterMode::Nearest)
				.setWidth(outputWidth)
				.setHeight(outputHeight)
				.build();

			Framebuffer res = FramebufferBuilder(outputWidth, outputHeight)
				.addColorAttachment(compositeTexture)
				.addColorAttachment(accumulationTexture)
				.addColorAttachment(revelageTexture)
				.addColorAttachment(objIdTexture)
				.includeDepthStencil()
				.generate();

			return res;
		}

		void render(AnimationManagerData* am, NVGcontext* vg, int deltaFrame)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			if (deltaFrame != 0)
			{
				applyDelta(am, vg, deltaFrame);
				//am->currentFrame += deltaFrame;
				//am->currentSnapshot = findSnapshot(am->currentFrame);
				//freeSnapshot(am->currentSnapshot);
				//am->currentSnapshot = buildSnapshot(am, vg, am->currentFrame);
			}

			// Render any active/animating objects
			// Make sure to initialize the NanoVG cache and then flush it after all the 
			// draw calls are complete
			for (auto objectIter = am->currentSnapshot.objects.begin(); objectIter != am->currentSnapshot.objects.end(); objectIter++)
			{
				if (objectIter->status != AnimObjectStatus::Inactive)
				{
					objectIter->render(vg, am);
				}
			}
		}

		int lastAnimatedFrame(const AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			int lastFrame = -1;
			for (auto animIter = am->animations.begin(); animIter != am->animations.end(); animIter++)
			{
				int animDeathTime = animIter->frameStart + animIter->duration;
				lastFrame = glm::max(lastFrame, animDeathTime);
			}

			return lastFrame;
		}

		const AnimObject* getPendingObject(const AnimationManagerData* am, AnimObjId animObj)
		{
			for (int i = 0; i < am->queuedAddObjects.size(); i++)
			{
				if (am->queuedAddObjects[i].id == animObj)
				{
					return &am->queuedAddObjects[i];
				}
			}

			return nullptr;
		}

		const AnimObject* getObject(const AnimationManagerData* am, AnimObjId animObj)
		{
			return getMutableObject((AnimationManagerData*)am, animObj);
		}

		const AnimObject* getObjectFromSnapshot(const SceneSnapshot& snapshot, AnimObjId animObj)
		{
			return getMutableObjectFromSnapshot((SceneSnapshot&)snapshot, animObj);
		}

		AnimObject* getMutableObject(AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			return getMutableObjectFromSnapshot(am->currentSnapshot, animObj);
		}

		AnimObject* getMutableObjectFromSnapshot(SceneSnapshot& snapshot, AnimObjId animObj)
		{
			if (isNull(animObj))
			{
				return nullptr;
			}

			auto iter = snapshot.objectIdMap.find(animObj);
			if (iter != snapshot.objectIdMap.end())
			{
				size_t objectIndex = iter->second;
				if (objectIndex >= 0 && objectIndex < snapshot.objects.size())
				{
					return &snapshot.objects[objectIndex];
				}
			}

			return nullptr;
		}

		const Animation* getAnimation(const AnimationManagerData* am, AnimId anim)
		{
			return getMutableAnimation((AnimationManagerData*)am, anim);
		}

		Animation* getMutableAnimation(AnimationManagerData* am, AnimId anim)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			if (isNull(anim))
			{
				return nullptr;
			}

			auto iter = am->animationIdMap.find(anim);
			if (iter != am->animationIdMap.end())
			{
				size_t animationIndex = iter->second;
				if (animationIndex >= 0 && animationIndex < am->animations.size())
				{
					return &am->animations[animationIndex];
				}
			}

			return nullptr;
		}

		const std::vector<AnimObject>& getAnimObjects(const AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");
			return am->currentSnapshot.objects;
		}

		const std::vector<Animation>& getAnimations(const AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");
			return am->animations;
		}

		std::vector<AnimId> getAssociatedAnimations(const AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			std::vector<AnimId> res;
			for (auto anim : am->animations)
			{
				auto iter = std::find(anim.animObjectIds.begin(), anim.animObjectIds.end(), animObj);
				if (iter != anim.animObjectIds.end())
				{
					res.push_back(anim.id);
				}
			}

			return res;
		}

		std::vector<AnimObjId> getChildren(const AnimationManagerData* am, AnimObjId animObj)
		{
			return getChildrenFromSnapshot(am->currentSnapshot, animObj);
		}

		std::vector<AnimObjId> getChildrenFromSnapshot(const SceneSnapshot& snapshot, AnimObjId animObj)
		{
			std::vector<AnimObjId> res;
			for (int i = 0; i < snapshot.objects.size(); i++)
			{
				if (snapshot.objects[i].parentId == animObj)
				{
					res.push_back(snapshot.objects[i].id);
				}
			}

			return res;
		}

		const SceneSnapshot& getCurrentSnapshot(const AnimationManagerData* am)
		{
			return getCurrentSnapshot((AnimationManagerData*)am);
		}

		SceneSnapshot& getMutableCurrentSnapshot(AnimationManagerData* am)
		{
			return am->currentSnapshot;
		}

		RawMemory serialize(const AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// This data should always be present regardless of file version
			// Container data layout
			// magicNumber   -> uint32
			// version       -> uint32
			RawMemory memory;
			memory.init(sizeof(uint32) + sizeof(uint32));
			memory.write<uint32>(&MAGIC_NUMBER);
			memory.write<uint32>(&SERIALIZER_VERSION);

			// Custom data starts here. Subject to change from version to version
			// numAnimations -> uint32
			// animations    -> dynamic
			uint32 numAnimations = (uint32)am->animations.size();
			memory.write<uint32>(&numAnimations);

			// Write out each animation followed by 0xDEADBEEF
			for (int i = 0; i < am->animations.size(); i++)
			{
				am->animations[i].serialize(memory);
				memory.write<uint32>(&MAGIC_NUMBER);
			}

			// numAnimObjects -> uint32
			// animObjects    -> dynamic
			uint32 numAnimObjects = (uint32)am->startingSnapshot.objects.size();
			memory.write<uint32>(&numAnimObjects);

			// Write out each anim object followed by 0xDEADBEEF
			for (int i = 0; i < am->startingSnapshot.objects.size(); i++)
			{
				am->startingSnapshot.objects[i].serialize(memory);
				memory.write<uint32>(&MAGIC_NUMBER);
			}

			memory.shrinkToFit();

			return memory;
		}

		void deserialize(AnimationManagerData* am, NVGcontext* vg, RawMemory& memory, int currentFrame)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Read magic number and version then dispatch to appropraite
			// deserializer
			// magicNumber   -> uint32
			// version       -> uint32
			uint32 magicNumber;
			memory.read<uint32>(&magicNumber);
			uint32 serializerVersion;
			memory.read<uint32>(&serializerVersion);

			g_logger_assert(magicNumber == MAGIC_NUMBER, "Project file had invalid magic number '0x%8x'. File must have been corrupted.", magicNumber);
			g_logger_assert((serializerVersion != 0 && serializerVersion <= SERIALIZER_VERSION), "Project file saved with invalid version '%d'. Looks like corrupted data.", serializerVersion);

			if (serializerVersion == 1)
			{
				deserializeAnimationManagerExV1(am, memory);
				am->currentFrame = currentFrame;
				Application::resetToFrame(currentFrame);
				am->currentSnapshot = buildSnapshot(am, vg, currentFrame);
			}
			else
			{
				g_logger_error("AnimationManagerEx serialized with unknown version '%d'.", serializerVersion);
			}

			// Need to sort animations they get applied in the correct order
			sortAnimations(am);

			// Build snapshots at key moments
			for (size_t i = 0; i < am->animations.size(); i++)
			{
				const Animation& anim = am->animations[i];
				int animStart = anim.frameStart;
				int animEnd = anim.frameStart + anim.duration;
				am->snapshots.emplace_back(buildSnapshot(am, vg, animStart));
				am->snapshots.emplace_back(buildSnapshot(am, vg, animEnd));
			}

			g_logger_info("Number of snapshots in deserialized scene: %d", am->snapshots.size());
		}

		void sortAnimations(AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			std::sort(am->animations.begin(), am->animations.end(), compareAnimation);
		}

		const char* getAnimObjectName(AnimObjectTypeV1 type)
		{
			g_logger_assert((int)type < (int)AnimObjectTypeV1::Length && (int)type >= 0, "Invalid type '%d'.", (int)type);
			return animationObjectTypeNames[(int)type];
		}

		const char* getAnimationName(AnimTypeV1 type)
		{
			g_logger_assert((int)type < (int)AnimTypeV1::Length && (int)type >= 0, "Invalid type '%d'.", (int)type);
			return animationTypeNames[(int)type];
		}

		// -------- Internal Functions --------
		static void deserializeAnimationManagerExV1(AnimationManagerData* am, RawMemory& memory)
		{
			// We're in function V1 so this is a version 1 for sure
			constexpr uint32 version = 1;

			// numAnimations -> uint32
			// animations    -> dynamic
			uint32 numAnimations;
			memory.read<uint32>(&numAnimations);

			// Read each animation followed by 0xDEADBEEF
			for (uint32 i = 0; i < numAnimations; i++)
			{
				Animation animation = Animation::deserialize(memory, version);
				am->animations.push_back(animation);
				uint32 magicNumber;
				memory.read<uint32>(&magicNumber);
				g_logger_assert(magicNumber == MAGIC_NUMBER, "Corrupted animation in file data. Bad magic number '0x%8x'", magicNumber);

				am->animationIdMap[animation.id] = i;
			}

			// numAnimObjects -> uint32
			// animObjects    -> dynamic
			uint32 numAnimObjects;
			memory.read<uint32>(&numAnimObjects);

			// Read each anim object followed by 0xDEADBEEF
			for (uint32 i = 0; i < numAnimObjects; i++)
			{
				AnimObject animObject = AnimObject::deserialize(am, memory, version);
				am->startingSnapshot.objects.push_back(animObject);
				uint32 magicNumber;
				memory.read<uint32>(&magicNumber);
				g_logger_assert(magicNumber == MAGIC_NUMBER, "Corrupted animation in file data. Bad magic number '0x%8x'", magicNumber);

				am->startingSnapshot.objectIdMap[animObject.id] = i;
			}
		}

		static bool compareAnimation(const Animation& a1, const Animation& a2)
		{
			// Sort by timeline track secondarily
			if (a1.frameStart == a2.frameStart)
			{
				return a1.timelineTrack < a2.timelineTrack;
			}

			return a1.frameStart < a2.frameStart;
		}

		static void updateGlobalTransform(AnimObject& obj)
		{
			obj._globalPositionStart = obj._positionStart;
			obj.globalPosition = obj.position;
			obj.globalTransform = glm::identity<glm::mat4>();
			obj.globalTransform = glm::translate(obj.globalTransform, CMath::convert(obj.globalPosition));
			glm::quat xRotation = glm::angleAxis(glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::quat yRotation = glm::angleAxis(glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::quat zRotation = glm::angleAxis(glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			obj.globalTransform *= glm::toMat4(xRotation * yRotation * zRotation);
			obj.globalTransform = glm::scale(obj.globalTransform, CMath::convert(obj.scale));
		}

		static void addQueuedAnimObject(AnimationManagerData* am, const AnimObject& obj)
		{
			am->startingSnapshot.objects.push_back(obj);
			am->startingSnapshot.objectIdMap[obj.id] = am->startingSnapshot.objects.size() - 1;

			AnimObject copy1 = AnimObject::createCopy(obj);
			am->currentSnapshot.objects.push_back(copy1);
			am->currentSnapshot.objectIdMap[obj.id] = am->currentSnapshot.objects.size() - 1;

			for (auto& snapshot : am->snapshots)
			{
				AnimObject copy2 = AnimObject::createCopy(obj);
				snapshot.objects.push_back(copy2);
				snapshot.objectIdMap[obj.id] = snapshot.objects.size() - 1;
			}
		}

		static void addQueuedAnimation(AnimationManagerData* am, const Animation& animation)
		{
			for (auto iter = am->animations.begin(); iter != am->animations.end(); iter++)
			{
				// Insert it here. The list will always be sorted
				if (animation.frameStart < iter->frameStart)
				{
					auto updateIter = am->animations.insert(iter, animation);

					// Update indices
					for (; updateIter != am->animations.end(); updateIter++)
					{
						am->animationIdMap[updateIter->id] = (updateIter - am->animations.begin());
					}

					return;
				}
			}

			// If we didn't insert the animation that means it must start after all the
			// current animation start times.
			am->animations.push_back(animation);
			am->animationIdMap[animation.id] = am->animations.size() - 1;
		}

		static void removeQueuedAnimObject(AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// First remove all children objects
			for (int i = 0; i < am->currentSnapshot.objects.size(); i++)
			{
				AnimObject& objIter = am->currentSnapshot.objects[i];
				if (objIter.parentId == animObj)
				{
					removeSingleAnimObject(am, objIter.id);
					i--;
				}
			}

			// Then remove the parent
			removeSingleAnimObject(am, animObj);
		}

		static void removeQueuedAnimation(AnimationManagerData* am, AnimId anim)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the animation
			auto iter = am->animationIdMap.find(anim);
			if (iter != am->animationIdMap.end())
			{
				size_t animationIndex = iter->second;
				if (animationIndex >= 0 && animationIndex < am->animations.size())
				{
					auto updateIter = am->animations.erase(am->animations.begin() + animationIndex);
					am->animationIdMap.erase(anim);

					for (; updateIter != am->animations.end(); updateIter++)
					{
						am->animationIdMap[updateIter->id] = (updateIter - am->animations.begin());
					}

					// TODO: Also remove any references of this animation from all other animations
				}
			}
		}

		static void removeSingleAnimObject(AnimationManagerData* am, AnimObjId animObj)
		{
			removeSingleAnimObjectSnapshot(am->currentSnapshot, animObj);
			removeSingleAnimObjectSnapshot(am->startingSnapshot, animObj);
			for (auto& snapshot : am->snapshots)
			{
				removeSingleAnimObjectSnapshot(snapshot, animObj);
			}

			// Remove any references from old animations
			for (auto animIter = am->animations.begin(); animIter != am->animations.end(); animIter++)
			{
				// TODO: The animIterIds should be a set not a vector
				auto deleteIterAnimRef = std::find(animIter->animObjectIds.begin(), animIter->animObjectIds.end(), animObj);
				if (deleteIterAnimRef != animIter->animObjectIds.end())
				{
					animIter->animObjectIds.erase(deleteIterAnimRef);
				}
				// TODO: This is gross, find some way to automatically update references
				else if (animIter->type == AnimTypeV1::Transform)
				{
					if (animIter->as.replacementTransform.dstAnimObjectId == animObj)
					{
						animIter->as.replacementTransform.dstAnimObjectId = NULL_ANIM_OBJECT;
					}
					else if (animIter->as.replacementTransform.srcAnimObjectId == animObj)
					{
						animIter->as.replacementTransform.srcAnimObjectId = NULL_ANIM_OBJECT;
					}
				}
			}
		}

		static void removeSingleAnimObjectSnapshot(SceneSnapshot& snapshot, AnimObjId animObj)
		{
			// Remove the anim object
			auto iter = snapshot.objectIdMap.find(animObj);
			if (iter != snapshot.objectIdMap.end())
			{
				size_t animObjectIndex = iter->second;
				if (animObjectIndex >= 0 && animObjectIndex < snapshot.objects.size())
				{
					snapshot.objects[animObjectIndex].free();

					auto updateIter = snapshot.objects.erase(snapshot.objects.begin() + animObjectIndex);
					snapshot.objectIdMap.erase(animObj);

					// Update indices
					for (; updateIter != snapshot.objects.end(); updateIter++)
					{
						snapshot.objectIdMap[updateIter->id] = updateIter - snapshot.objects.begin();
					}
				}
			}
		}

		static void freeSnapshot(SceneSnapshot& snapshot)
		{
			for (int i = 0; i < snapshot.objectIdMap.size(); i++)
			{
				snapshot.objects[i].free();
			}
		}

		static void applyDelta(AnimationManagerData* am, NVGcontext* vg, int deltaFrame)
		{
			int previousFrame = am->currentFrame;
			am->currentFrame += deltaFrame;
			int newFrame = am->currentFrame;

			if (am->animations.size() == 0)
			{
				return;
			}

			// Assume newFrame < previousFrame
			int animIndexToStartFrom = (int)am->animations.size() - 1;
			for (size_t i = 0; i < am->animations.size(); i++)
			{
				const Animation& anim = am->animations[i];
				if (anim.frameStart > previousFrame)
				{
					g_logger_assert(i > 0, "Somehow the timeline cursor is at -1.");
					animIndexToStartFrom = (int)i - 1;
					break;
				}
			}

			int reapplyAnimationsFrom = 0;
			for (int i = animIndexToStartFrom; i >= 0; i--)
			{
				const Animation& anim = am->animations[i];
				int animStart = anim.frameStart;
				int animEnd = anim.frameStart + anim.duration;
				bool intersecting = newFrame <= animStart && animEnd <= previousFrame;
				if (intersecting)
				{
					anim.applyAnimationToSnapshot(am->currentSnapshot, vg, 0.0f);
				}
				else
				{
					g_logger_assert(i < am->animations.size(), "Somehow we didn't reapply anything.");
					reapplyAnimationsFrom = i + 1;
					break;
				}
			}

			applyAnimationsFrom(am, vg, am->currentSnapshot, animIndexToStartFrom, newFrame);
			applyGlobalTransforms(am->currentSnapshot);
		}

		static void applyGlobalTransforms(SceneSnapshot& snapshot)
		{
			// ----- Apply the parent->child transformations -----
			// First find all the root objects
			std::queue<AnimObjId> rootObjects = {};
			for (auto objIter = snapshot.objects.begin(); objIter != snapshot.objects.end(); objIter++)
			{
				// If the object has no parent, it's a root object.
				// Update the transform then update children recursively
				// and in order from parent->child
				if (isNull(objIter->parentId))
				{
					rootObjects.push(objIter->id);
				}
			}

			// Then update all children
			// Loop through each root object and recursively update the transforms by appending children to the queue
			while (rootObjects.size() > 0)
			{
				AnimObjId nextObjId = rootObjects.front();
				rootObjects.pop();

				// Update child transform
				AnimObject* nextObj = getMutableObjectFromSnapshot(snapshot, nextObjId);
				if (nextObj)
				{
					updateGlobalTransform(*nextObj);

					// Then apply parent transformation if the parent exists
					// At this point the parent should have been updated already
					// since the queue is FIFO
					if (!isNull(nextObj->parentId))
					{
						const AnimObject* parent = getObjectFromSnapshot(snapshot, nextObj->parentId);
						if (parent)
						{
							nextObj->_globalPositionStart += parent->_globalPositionStart;
							nextObj->globalPosition += parent->globalPosition;
							nextObj->globalTransform *= parent->globalTransform;
						}
					}

					// Then append all direct children to the queue so they are
					// recursively updated
					for (auto childIter = snapshot.objects.begin(); childIter != snapshot.objects.end(); childIter++)
					{
						if (childIter->parentId == nextObj->id)
						{
							rootObjects.push(childIter->id);
						}
					}
				}
			}
		}

		static void applyAnimationsFrom(AnimationManagerData* am, NVGcontext* vg, SceneSnapshot& snapshot, int startIndex, int currentFrame)
		{
			// Apply any changes from animations in order
			for (auto animIter = am->animations.begin(); animIter != am->animations.end(); animIter++)
			{
				float frameStart = (float)animIter->frameStart;
				int animDeathTime = (int)frameStart + animIter->duration;
				if (frameStart <= currentFrame)
				{
					// Then apply the animation
					float interpolatedT = ((float)currentFrame - frameStart) / (float)animIter->duration;
					animIter->applyAnimationToSnapshot(snapshot, vg, interpolatedT);
				}
			}
		}
	}
}