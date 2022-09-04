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
		std::unordered_map<uint32, uint32> idMap;
		// Always sorted by startFrame and trackIndex
		std::vector<Animation> animations;

		// NOTE(gabe): So this is due to my whacky architecture, but at the beginning of rendering
		// each frame we actually need to reset the camera position to its start position. I really
		// need to figure out a better architecture for this haha
		Vec2 sceneCamera2DStartPos;
		OrthoCamera* sceneCamera2D;
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
			"Length",
		};

		// -------- Internal Functions --------
		static void deserializeAnimationManagerExV1(AnimationManagerData* am, RawMemory& memory);
		static bool compareAnimObject(const AnimObject& ob1, const AnimObject& ob2);
		static bool compareAnimation(const Animation& a1, const Animation& a2);

		AnimationManagerData* create(OrthoCamera& camera)
		{
			void* animManagerMemory = g_memory_allocate(sizeof(AnimationManagerData));
			// Placement new to ensure the vectors and stuff are appropriately constructed
			// but I can still use my memory tracker
			AnimationManagerData* res = new(animManagerMemory)AnimationManagerData();

			res->sceneCamera2D = &camera;
			res->sceneCamera2DStartPos = res->sceneCamera2D->position;
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

				// Call destructor to properly destruct vector objects
				am->~AnimationManagerData();
				g_memory_free(am);
			}
		}

		void addAnimObject(AnimationManagerData* am, const AnimObject& object)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			bool insertedObject = false;
			for (auto iter = am->objects.begin(); iter != am->objects.end(); iter++)
			{
				// Insert it here. The list will always be sorted
				if (object.frameStart < iter->frameStart)
				{
					am->objects.insert(iter, object);
					insertedObject = true;
					break;
				}
			}

			if (!insertedObject)
			{
				// If we didn't insert the object
				// that means it must start after all the
				// current animObject start times.
				am->objects.push_back(object);
			}
		}

		void addAnimationTo(const Animation& animation, AnimObject& animObject)
		{
			for (auto iter = animObject.animations.begin(); iter != animObject.animations.end(); iter++)
			{
				// Insert it here. The list will always be sorted
				if (animation.frameStart < iter->frameStart)
				{
					animObject.animations.insert(iter, animation);
					return;
				}
			}

			// If we didn't insert the animation
			// that means it must start after all the
			// current animation start times.
			animObject.animations.push_back(animation);
		}

		void addAnimationTo(AnimationManagerData* am, const Animation& animation, int animObjectId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			for (int ai = 0; ai < am->objects.size(); ai++)
			{
				AnimObject& animObject = am->objects[ai];
				if (animObject.id != animObjectId)
				{
					continue;
				}

				for (auto iter = animObject.animations.begin(); iter != animObject.animations.end(); iter++)
				{
					// Insert it here. The list will always be sorted
					if (animation.frameStart < iter->frameStart)
					{
						animObject.animations.insert(iter, animation);
						return;
					}
				}

				// If we didn't insert the animation
				// that means it must start after all the
				// current animation start times.
				animObject.animations.push_back(animation);
				return;
			}
		}

		bool removeAnimObject(AnimationManagerData* am, int animObjectId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the anim object
			for (int obji = 0; obji < am->objects.size(); obji++)
			{
				if (am->objects[obji].id == animObjectId)
				{
					// Free all animations in this object
					for (int animi = 0; animi < am->objects[obji].animations.size(); animi++)
					{
						g_logger_assert(am->objects[obji].animations[animi].objectId == animObjectId, "How did this happen?");
						am->objects[obji].animations[animi].free();
					}

					am->objects[obji].free();
					am->objects.erase(am->objects.begin() + obji);
					return true;
				}
			}

			return false;
		}

		bool removeAnimation(AnimationManagerData* am, int animObjectId, int animationId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the anim object
			for (int obji = 0; obji < am->objects.size(); obji++)
			{
				if (am->objects[obji].id == animObjectId)
				{
					// Free all animations in this object
					for (int animi = 0; animi < am->objects[obji].animations.size(); animi++)
					{
						if (am->objects[obji].animations[animi].id == animationId)
						{
							g_logger_assert(am->objects[obji].animations[animi].objectId == animObjectId, "How did this happen?");
							am->objects[obji].animations[animi].free();
							am->objects[obji].animations.erase(am->objects[obji].animations.begin() + animi);
							return true;
						}
					}
				}
			}

			return false;
		}

		bool setAnimObjectTime(AnimationManagerData* am, int animObjectId, int frameStart, int duration)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the animation then reinsert it. That way we make sure the list
			// stays sorted
			AnimObject animObjectCopy;
			int objectIndex = -1;
			for (int i = 0; i < am->objects.size(); i++)
			{
				const AnimObject& animObject = am->objects[i];
				if (animObject.id == animObjectId)
				{
					if (animObject.frameStart == frameStart && animObject.duration == duration)
					{
						// If nothing changed, just return that the change was successful
						return true;
					}

					animObjectCopy = animObject;
					objectIndex = i;
					break;
				}
			}

			if (objectIndex != -1)
			{
				am->objects.erase(am->objects.begin() + objectIndex);
				animObjectCopy.frameStart = frameStart;
				animObjectCopy.duration = duration;
				addAnimObject(am, animObjectCopy);
				return true;
			}

			return false;
		}

		bool setAnimationTime(AnimationManagerData* am, int animObjectId, int animationId, int frameStart, int duration)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Remove the animation then reinsert it. That way we make sure the list
			// stays sorted
			Animation animationCopy;
			int animationIndex = -1;
			int animObjectIndex = -1;
			for (int i = 0; i < am->objects.size(); i++)
			{
				if (am->objects[i].id == animObjectId)
				{
					// TODO: This is nasty. I should probably use a hash map to store
					// animations and anim objects
					for (int j = 0; j < am->objects[i].animations.size(); j++)
					{
						const Animation& animation = am->objects[i].animations[j];
						if (animation.id == animationId)
						{
							if (animation.frameStart == frameStart && animation.duration == duration)
							{
								// If nothing changed, just return that the change was successful
								return true;
							}

							animationCopy = animation;
							animationIndex = j;
							animObjectIndex = i;
							break;
						}
					}
				}

				if (animationIndex != -1)
				{
					break;
				}
			}

			if (animationIndex != -1)
			{
				am->objects[animObjectIndex].animations.erase(am->objects[animObjectIndex].animations.begin() + animationIndex);
				animationCopy.frameStart = frameStart;
				animationCopy.duration = duration;
				addAnimationTo(animationCopy, am->objects[animObjectIndex]);
				return true;
			}

			return false;

		}

		void setAnimObjectTrack(AnimationManagerData* am, int animObjectId, int track)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			for (int i = 0; i < am->objects.size(); i++)
			{
				if (am->objects[i].id == animObjectId)
				{
					am->objects[i].timelineTrack = track;
					return;
				}
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

			Framebuffer res = FramebufferBuilder(outputWidth, outputHeight)
				.addColorAttachment(compositeTexture)
				.addColorAttachment(accumulationTexture)
				.addColorAttachment(revelageTexture)
				.includeDepthStencil()
				.generate();

			return res;
		}

		void render(AnimationManagerData* am, NVGcontext* vg, int frame, Framebuffer& framebuffer)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

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
				objectIter->isAnimating = false;

				// Update any updateable objects
				if (objectIter->objectType == AnimObjectTypeV1::LaTexObject)
				{
					objectIter->as.laTexObject.update();
				}

				// Reset children state
				for (int i = 0; i < objectIter->children.size(); i++)
				{
					AnimObject* child = &objectIter->children[i];
					if (child->_svgObjectStart != nullptr && child->svgObject != nullptr)
					{
						Svg::copy(child->svgObject, child->_svgObjectStart);
					}

					// TODO: Create some sort of apply animation, then apply animations locally
					// and then transform according to parent position after animation applications
					child->position = child->_positionStart + objectIter->position;
					child->rotation = child->_rotationStart + objectIter->rotation;
					child->scale.x = child->_scaleStart.x * objectIter->scale.x;
					child->scale.y = child->_scaleStart.y * objectIter->scale.y;
					child->scale.z = child->_scaleStart.z * objectIter->scale.z;

					// Update any updateable objects
					if (child->objectType == AnimObjectTypeV1::LaTexObject)
					{
						child->as.laTexObject.update();
					}
				}

				for (auto animIter = objectIter->animations.begin(); animIter != objectIter->animations.end(); animIter++)
				{
					float parentFrameStart = (float)animIter->getParent(am)->frameStart;
					float absoluteFrameStart = animIter->frameStart + parentFrameStart;
					int animDeathTime = (int)absoluteFrameStart + animIter->duration;
					if (absoluteFrameStart <= frame)
					{
						if (frame <= animDeathTime)
						{
							objectIter->isAnimating = true;
							float interpolatedT = ((float)frame - absoluteFrameStart) / (float)animIter->duration;
							animIter->render(am, vg, interpolatedT);
						}
						else
						{
							animIter->applyAnimation(am, vg);
						}
					}
				}

				int objectDeathTime = objectIter->frameStart + objectIter->duration;
				if (!objectIter->isAnimating && objectIter->frameStart <= frame && frame <= objectDeathTime)
				{
					objectIter->render(vg);
				}
			}
		}

		int lastAnimatedFrame(const AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			int lastFrame = -1;
			for (auto objectIter = am->objects.begin(); objectIter != am->objects.end(); objectIter++)
			{
				int objectDeathTime = objectIter->frameStart + objectIter->duration;
				lastFrame = glm::max(lastFrame, objectDeathTime);
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

			for (int i = 0; i < am->objects.size(); i++)
			{
				if (am->objects[i].id == animObjectId)
				{
					return &am->objects[i];
				}
			}

			return nullptr;
		}

		Animation* getMutableAnimation(AnimationManagerData* am, int animationId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			for (int i = 0; i < am->objects.size(); i++)
			{
				for (int ai = 0; ai < am->objects[i].animations.size(); ai++)
				{
					if (am->objects[i].animations[ai].id == animationId)
					{
						return &am->objects[i].animations[ai];
					}
				}
			}

			return nullptr;
		}

		const std::vector<AnimObject>& getAnimObjects(const AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");
			return am->objects;
		}

		const AnimObject* getNextAnimObject(const AnimationManagerData* am, int animObjectId)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			int timelineTrack = -1;
			int frameStart = -1;
			int objIndex = -1;
			for (int i = 0; i < am->objects.size(); i++)
			{
				if (objIndex != -1)
				{
					if (am->objects[i].timelineTrack == timelineTrack && am->objects[i].frameStart > am->objects[objIndex].frameStart)
					{
						g_logger_assert(am->objects[i].frameStart >= am->objects[objIndex].frameStart + am->objects[objIndex].duration, "These two objects are intersecting.");
						return &am->objects[i];
					}
				}

				if (am->objects[i].id == animObjectId)
				{
					g_logger_assert(objIndex == -1, "Multiple objects found with object id %d", animObjectId);
					timelineTrack = am->objects[i].timelineTrack;
					frameStart = am->objects[i].frameStart;
					objIndex = i;
				}
			}

			return nullptr;
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
			// animObjects   -> dynamic
			uint32 numAnimations = (uint32)am->objects.size();
			memory.write<uint32>(&numAnimations);

			// Write out each animation followed by 0xDEADBEEF
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

			// Need to sort animation objects to ensure they get animations 
			// applied in the correct order
			sortAnimObjects(am);
		}

		void sortAnimObjects(AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			std::sort(am->objects.begin(), am->objects.end(), compareAnimObject);
			for (int i = 0; i < am->objects.size(); i++)
			{
				std::sort(am->objects[i].animations.begin(), am->objects[i].animations.end(), compareAnimation);
			}
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
			// animObjects   -> dynamic
			uint32 numAnimations;
			memory.read<uint32>(&numAnimations);

			// Write out each animation followed by 0xDEADBEEF
			for (uint32 i = 0; i < numAnimations; i++)
			{
				AnimObject animObject = AnimObject::deserialize(memory, version);
				am->objects.push_back(animObject);
				uint32 magicNumber;
				memory.read<uint32>(&magicNumber);
				g_logger_assert(magicNumber == MAGIC_NUMBER, "Corrupted animation in file data. Bad magic number '0x%8x'", magicNumber);
			}
		}

		static bool compareAnimObject(const AnimObject& ob1, const AnimObject& ob2)
		{
			return ob1.frameStart < ob2.frameStart;
		}

		static bool compareAnimation(const Animation& a1, const Animation& a2)
		{
			return a1.frameStart < a2.frameStart;
		}
	}
}