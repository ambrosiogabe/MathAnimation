#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "animation/Svg.h"
#include "renderer/Renderer.h"
#include "renderer/Texture.h"
#include "renderer/Framebuffer.h"
#include "renderer/OrthoCamera.h"

namespace MathAnim
{
	struct AnimationManagerData
	{
		std::vector<AnimObject> objects;
		// Maps from AnimObjectId -> Index in objects vector
		std::unordered_map<AnimObjId, int32> objectIdMap;

		// Always sorted by startFrame and trackIndex
		std::vector<Animation> animations;
		// Maps From AnimationId -> Index in animations vector
		std::unordered_map<AnimId, int32> animationIdMap;

		// This list gets updated whenever the timeline cursor moves
		// It should hold the list of animations that are currently
		// being rendered at the timeline cursor
		std::vector<Animation> activeAnimations;

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
				for (int i = 0; i < am->objects.size(); i++)
				{
					am->objects[i].free();
				}

				for (int i = 0; i < am->animations.size(); i++)
				{
					am->animations[i].free();
				}

				// Call destructor to properly destruct vector objects
				am->~AnimationManagerData();
				g_memory_free(am);
			}
		}

		void addAnimObject(AnimationManagerData* am, const AnimObject& object)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			am->objects.push_back(object);
			am->objectIdMap[object.id] = am->objects.size() - 1;
		}

		void addAnimation(AnimationManagerData* am, const Animation& animation)
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

		static bool removeSingleAnimObject(AnimationManagerData* am, AnimObjId animObj);
		bool removeAnimObject(AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// First remove all children objects
			bool success = true;
			for (int i = 0; i < am->objects.size(); i++)
			{
				AnimObject& objIter = am->objects[i];
				if (objIter.parentId == animObj)
				{
					bool singleSuccess = removeSingleAnimObject(am, objIter.id);
					success = success && singleSuccess;
					i--;
				}
			}

			// Then remove the parent
			bool singleSuccess = removeSingleAnimObject(am, animObj);
			success = success && singleSuccess;

			return success;
		}

		static bool removeSingleAnimObject(AnimationManagerData* am, AnimObjId animObj)
		{
			// Remove the anim object
			auto iter = am->objectIdMap.find(animObj);
			if (iter != am->objectIdMap.end())
			{
				int animObjectIndex = iter->second;
				if (animObjectIndex >= 0 && animObjectIndex < am->objects.size())
				{
					am->objects[animObjectIndex].free();

					auto updateIter = am->objects.erase(am->objects.begin() + animObjectIndex);
					am->objectIdMap.erase(animObj);

					// Update indices
					for (; updateIter != am->objects.end(); updateIter++)
					{
						am->objectIdMap[updateIter->id] = updateIter - am->objects.begin();
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

					return true;
				}
			}

			return false;
		}

		bool removeAnimation(AnimationManagerData* am, AnimId anim)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the animation
			auto iter = am->animationIdMap.find(anim);
			if (iter != am->animationIdMap.end())
			{
				int animationIndex = iter->second;
				if (animationIndex >= 0 && animationIndex < am->animations.size())
				{
					auto updateIter = am->animations.erase(am->animations.begin() + animationIndex);
					am->animationIdMap.erase(anim);

					for (; updateIter != am->animations.end(); updateIter++)
					{
						am->animationIdMap[updateIter->id] = (updateIter - am->animations.begin());
					}

					// TODO: Also remove any references of this animation from all other animations

					return true;
				}
			}

			return false;
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
				int animationIndex = animationIndexIter->second;
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
				.setFormat(ByteFormat::R32_UI)
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

			// If the current frame has changed, then apply the delta changes
			// TODO: Optimization, optimize and check this is the problem and if so 
			// if (deltaFrame != 0)
			{
				// TODO: Optimization, optimize and check this is the problem and if so 
				// am->activeAnimations.clear();
				am->currentFrame += deltaFrame;

				// TODO: Add unApply/apply to all animations to go backwards and forwards
				// deltas.
				// For now I'll just reapply all changes from the start of the scene

				// Reset everything to state at frame 0
				am->sceneCamera2D->position = am->sceneCamera2DStartPos;

				// Reset all object positions
				for (auto objectIter = am->objects.begin(); objectIter != am->objects.end(); objectIter++)
				{
					// Reset to original state and apply animations in order
					if (objectIter->_svgObjectStart != nullptr && objectIter->svgObject != nullptr)
					{
						Svg::copy(objectIter->svgObject, objectIter->_svgObjectStart);
					}
					objectIter->position = objectIter->_positionStart;
					objectIter->rotation = objectIter->_rotationStart;
					objectIter->scale = objectIter->_scaleStart;
					objectIter->fillColor = objectIter->_fillColorStart;
					objectIter->strokeColor = objectIter->_strokeColorStart;
					objectIter->strokeWidth = objectIter->_strokeWidthStart;
					objectIter->percentCreated = 0.0f;
					objectIter->status = AnimObjectStatus::Inactive;

					// Update any updateable objects
					if (objectIter->objectType == AnimObjectTypeV1::LaTexObject)
					{
						objectIter->as.laTexObject.update();
					}
				}

				// Apply any changes from animations in order
				for (auto animIter = am->animations.begin(); animIter != am->animations.end(); animIter++)
				{
					float frameStart = (float)animIter->frameStart;
					int animDeathTime = (int)frameStart + animIter->duration;
					if (frameStart <= am->currentFrame)
					{
						bool animationComplete = am->currentFrame >= animDeathTime;

						// TODO: Optimization, optimize and check this is the problem and if so 
						// If it's the middle of an animation then make sure 
						// to keep track of it
						// am->activeAnimations.push_back(*animIter);
						// Then apply the animation
						float interpolatedT = ((float)am->currentFrame - frameStart) / (float)animIter->duration;
						animIter->applyAnimation(am, vg, interpolatedT);
					}
				}

				// ----- Apply the parent->child transformations -----
				// First find all the root objects
				std::queue<int32> rootObjects = {};
				for (auto objIter = am->objects.begin(); objIter != am->objects.end(); objIter++)
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
					int32 nextObjId = rootObjects.front();
					rootObjects.pop();

					// Update child transform
					AnimObject* nextObj = getMutableObject(am, nextObjId);
					if (nextObj)
					{
						updateGlobalTransform(*nextObj);

						// Then apply parent transformation if the parent exists
						// At this point the parent should have been updated already
						// since the queue is FIFO
						if (!isNull(nextObj->parentId))
						{
							const AnimObject* parent = getObject(am, nextObj->parentId);
							if (parent)
							{
								nextObj->_globalPositionStart += parent->_globalPositionStart;
								nextObj->globalPosition += parent->globalPosition;
								nextObj->globalTransform *= parent->globalTransform;
							}
						}

						// Then append all direct children to the queue so they are
						// recursively updated
						for (auto childIter = am->objects.begin(); childIter != am->objects.end(); childIter++)
						{
							if (childIter->parentId == nextObj->id)
							{
								rootObjects.push(childIter->id);
							}
						}
					}
				}

				// TODO: Optimization, optimize and check this is the problem and if so 
				// Sort the active animations list so they're always applied in order
				// std::sort(am->activeAnimations.begin(), am->activeAnimations.end(), compareAnimation);
			}

			// TODO: Optimization, optimize and check this is the problem and if so 
			// get this working if large scenes start lagging
			// 
			// If the frame didn't change re-apply active animations
			// so that any changes to object properties take effect
			//if (deltaFrame == 0)
			//{
			//	// Apply any active animations
			//	for (auto animIter = am->activeAnimations.begin(); animIter != am->activeAnimations.end(); animIter++)
			//	{
			//		float frameStart = (float)animIter->frameStart;
			//		float interpolatedT = ((float)am->currentFrame - frameStart) / (float)animIter->duration;
			//		//animIter->render(am, vg, interpolatedT);
			//		animIter->applyAnimation(am, vg, interpolatedT);
			//	}
			//}

			// Render any active/animating objects
			// Make sure to initialize the NanoVG cache and then flush it after all the 
			// draw calls are complete
			Svg::beginFrame(vg);

			for (auto objectIter = am->objects.begin(); objectIter != am->objects.end(); objectIter++)
			{
				if (objectIter->status != AnimObjectStatus::Inactive)
				{
					objectIter->render(vg);
				}
			}

			Svg::endFrame(vg);
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

		const AnimObject* getObject(const AnimationManagerData* am, AnimObjId animObj)
		{
			return getMutableObject((AnimationManagerData*)am, animObj);
		}

		AnimObject* getMutableObject(AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			if (isNull(animObj))
			{
				return nullptr;
			}

			auto iter = am->objectIdMap.find(animObj);
			if (iter != am->objectIdMap.end())
			{
				int objectIndex = iter->second;
				if (objectIndex >= 0 && objectIndex < am->objects.size())
				{
					return &am->objects[objectIndex];
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
				int animationIndex = iter->second;
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
			return am->objects;
		}

		const std::vector<Animation>& getAnimations(const AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");
			return am->animations;
		}

		std::vector<AnimId> getAssociatedAnimations(const AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			std::vector<int32> res;
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
			std::vector<AnimObjId> res;
			for (int i = 0; i < am->objects.size(); i++)
			{
				if (am->objects[i].parentId == animObj)
				{
					res.push_back(am->objects[i].id);
				}
			}

			return res;
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
			uint32 numAnimObjects = (uint32)am->objects.size();
			memory.write<uint32>(&numAnimObjects);

			// Write out each anim object followed by 0xDEADBEEF
			for (int i = 0; i < am->objects.size(); i++)
			{
				if (!am->objects[i].isGenerated)
				{
					am->objects[i].serialize(memory);
				}
				memory.write<uint32>(&MAGIC_NUMBER);
			}

			memory.shrinkToFit();

			return memory;
		}

		void deserialize(AnimationManagerData* am, RawMemory& memory)
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
			}
			else
			{
				g_logger_error("AnimationManagerEx serialized with unknown version '%d'.", serializerVersion);
			}

			// Need to sort animations they get applied in the correct order
			sortAnimations(am);
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
				am->objects.push_back(animObject);
				uint32 magicNumber;
				memory.read<uint32>(&magicNumber);
				g_logger_assert(magicNumber == MAGIC_NUMBER, "Corrupted animation in file data. Bad magic number '0x%8x'", magicNumber);

				am->objectIdMap[animObject.id] = i;
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
	}
}