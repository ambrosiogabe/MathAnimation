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
		std::unordered_map<int32, int32> objectIdMap;

		// Always sorted by startFrame and trackIndex
		std::vector<Animation> animations;
		// Maps From AnimationId -> Index in animations vector
		std::unordered_map<int32, int32> animationIdMap;

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

		bool removeAnimObject(AnimationManagerData* am, int animObjectId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the anim object
			auto iter = am->objectIdMap.find(animObjectId);
			if (iter != am->objectIdMap.end())
			{
				int animObjectIndex = iter->second;
				if (animObjectIndex >= 0 && animObjectIndex < am->objects.size())
				{
					auto updateIter = am->objects.erase(am->objects.begin() + animObjectIndex);
					am->objectIdMap.erase(animObjectId);

					// Update indices
					for (; updateIter != am->objects.end(); updateIter++)
					{
						am->objectIdMap[updateIter->id] = updateIter - am->objects.begin();
					}

					// TODO: Also remove any references of this object from all animations

					return true;
				}
			}

			return false;
		}

		bool removeAnimation(AnimationManagerData* am, int animationId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the animation
			auto iter = am->animationIdMap.find(animationId);
			if (iter != am->animationIdMap.end())
			{
				int animationIndex = iter->second;
				if (animationIndex >= 0 && animationIndex < am->animations.size())
				{
					auto updateIter = am->animations.erase(am->animations.begin() + animationIndex);
					am->animationIdMap.erase(animationId);

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

		bool setAnimationTime(AnimationManagerData* am, int animationId, int frameStart, int duration)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the animation then reinsert it. That way we make sure the list
			// stays sorted
			Animation animationCopy;
			auto animationIndexIter = am->animationIdMap.find(animationId);

			if (animationIndexIter != am->animationIdMap.end())
			{
				int animationIndex = animationIndexIter->second;
				if (animationIndex >= 0 && animationIndex < am->animations.size())
				{
					animationCopy = am->animations[animationIndex];
					removeAnimation(am, animationId);

					animationCopy.frameStart = frameStart;
					animationCopy.duration = duration;

					// This should update it's index and any other effected indices
					addAnimation(am, animationCopy);
					return true;
				}
			}

			return false;

		}

		void setAnimationTrack(AnimationManagerData* am, int animationId, int track)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			Animation* animation = getMutableAnimation(am, animationId);
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
					objectIter->status = AnimObjectStatus::Inactive;

					// Update any updateable objects
					if (objectIter->objectType == AnimObjectTypeV1::LaTexObject)
					{
						objectIter->as.laTexObject.update();
					}

					// Reset children state
					//for (int i = 0; i < objectIter->children.size(); i++)
					//{
					//	AnimObject* child = &objectIter->children[i];
					//	if (child->_svgObjectStart != nullptr && child->svgObject != nullptr)
					//	{
					//		Svg::copy(child->svgObject, child->_svgObjectStart);
					//	}

					//	// TODO: Create some sort of apply animation, then apply animations locally
					//	// and then transform according to parent position after animation applications
					//	child->position = child->_positionStart + objectIter->position;
					//	child->rotation = child->_rotationStart + objectIter->rotation;
					//	child->scale.x = child->_scaleStart.x * objectIter->scale.x;
					//	child->scale.y = child->_scaleStart.y * objectIter->scale.y;
					//	child->scale.z = child->_scaleStart.z * objectIter->scale.z;

					//	// Update any updateable objects
					//	if (child->objectType == AnimObjectTypeV1::LaTexObject)
					//	{
					//		child->as.laTexObject.update();
					//	}
					//}
				}

				// Apply any changes from animations in order
				for (auto animIter = am->animations.begin(); animIter != am->animations.end(); animIter++)
				{
					float frameStart = (float)animIter->frameStart;
					int animDeathTime = (int)frameStart + animIter->duration;
					if (frameStart <= am->currentFrame)
					{
						bool animationComplete = am->currentFrame >= animDeathTime;

						// Set all objects this animation is acting on to animating/active status
						for (auto objIter = animIter->animObjectIds.begin(); objIter != animIter->animObjectIds.end(); objIter++)
						{
							AnimObject* object = getMutableObject(am, *objIter);
							// Omit the check in distribution builds								
#ifndef _DIST
							g_logger_assert(object != nullptr, "Somehow an animation had a reference to a non-existent object.");
#endif
							object->status = animationComplete
								? AnimObjectStatus::Active
								: AnimObjectStatus::Animating;
						}

						// TODO: Optimization, optimize and check this is the problem and if so 
						// If it's the middle of an animation then make sure 
						// to keep track of it
						// am->activeAnimations.push_back(*animIter);
						// Then apply the animation
						float interpolatedT = ((float)am->currentFrame - frameStart) / (float)animIter->duration;
						animIter->applyAnimation(am, vg, interpolatedT);
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
			for (auto objectIter = am->objects.begin(); objectIter != am->objects.end(); objectIter++)
			{
				if (objectIter->status != AnimObjectStatus::Inactive)
				{
					objectIter->render(vg);
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

		bool isObjectNull(int animObjectId)
		{
			return animObjectId == INT32_MAX;
		}

		const AnimObject* getObject(const AnimationManagerData* am, int animObjectId)
		{
			return getMutableObject((AnimationManagerData*)am, animObjectId);
		}

		AnimObject* getMutableObject(AnimationManagerData* am, int animObjectId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			if (animObjectId == INT32_MAX)
			{
				return nullptr;
			}

			auto iter = am->objectIdMap.find(animObjectId);
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

		const Animation* getAnimation(const AnimationManagerData* am, int animationId)
		{
			return getMutableAnimation((AnimationManagerData*)am, animationId);
		}

		Animation* getMutableAnimation(AnimationManagerData* am, int animationId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			if (animationId == INT32_MAX)
			{
				return nullptr;
			}

			auto iter = am->animationIdMap.find(animationId);
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

		std::vector<int32> getAssociatedAnimations(const AnimationManagerData* am, const AnimObject* obj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			std::vector<int32> res;
			for (auto anim : am->animations)
			{
				auto iter = std::find(anim.animObjectIds.begin(), anim.animObjectIds.end(), obj->id);
				if (iter != anim.animObjectIds.end())
				{
					res.push_back(anim.id);
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
				am->objects[i].serialize(memory);
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
				AnimObject animObject = AnimObject::deserialize(memory, version);
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
	}
}