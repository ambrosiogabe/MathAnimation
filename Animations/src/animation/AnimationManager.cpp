#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "animation/Svg.h"

namespace MathAnim
{
	namespace AnimationManager
	{
		// ------- Private variables --------
		// List of animatable objects, sorted by start time
		static std::vector<AnimObject> mObjects;

		static const char* animationObjectTypeNames[] = {
			"None",
			"Text Object",
			"LaTex Object",
			"Square",
			"Circle",
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
			"Length",
		};

		// Internal Functions
		static void deserializeAnimationManagerExV1(RawMemory& memory);

		void addAnimObject(const AnimObject& object)
		{
			bool insertedObject = false;
			for (auto iter = mObjects.begin(); iter != mObjects.end(); iter++)
			{
				// Insert it here. The list will always be sorted
				if (object.frameStart < iter->frameStart)
				{
					mObjects.insert(iter, object);
					insertedObject = true;
					break;
				}
			}

			if (!insertedObject)
			{
				// If we didn't insert the object
				// that means it must start after all the
				// current animObject start times.
				mObjects.push_back(object);
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

		void addAnimationTo(const Animation& animation, int animObjectId)
		{
			for (int ai = 0; ai < mObjects.size(); ai++)
			{
				AnimObject& animObject = mObjects[ai];
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

		bool removeAnimObject(int animObjectId)
		{
			// Remove the anim object
			for (int i = 0; i < mObjects.size(); i++)
			{
				if (mObjects[i].id == animObjectId)
				{
					// Free all animations in this object
					for (int j = 0; j < mObjects[i].animations.size(); j++)
					{
						g_logger_assert(mObjects[i].animations[j].objectId == animObjectId, "How did this happen?");
						mObjects[i].animations[j].free();
					}

					mObjects[i].free();
					mObjects.erase(mObjects.begin() + i);
					return true;
				}
			}

			return false;
		}

		bool removeAnimation(int animObjectId, int animationId)
		{
			// Remove the anim object
			for (int i = 0; i < mObjects.size(); i++)
			{
				if (mObjects[i].id == animObjectId)
				{
					// Free all animations in this object
					for (int j = 0; j < mObjects[i].animations.size(); j++)
					{
						if (mObjects[i].animations[j].id == animationId)
						{
							g_logger_assert(mObjects[i].animations[i].objectId == animObjectId, "How did this happen?");
							mObjects[i].animations[j].free();
							mObjects[i].animations.erase(mObjects[i].animations.begin() + j);
							return true;
						}
					}
				}
			}

			return false;
		}

		bool setAnimObjectTime(int animObjectId, int frameStart, int duration)
		{
			// Remove the animation then reinsert it. That way we make sure the list
			// stays sorted
			AnimObject animObjectCopy;
			int objectIndex = -1;
			for (int i = 0; i < mObjects.size(); i++)
			{
				const AnimObject& animObject = mObjects[i];
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
				mObjects.erase(mObjects.begin() + objectIndex);
				animObjectCopy.frameStart = frameStart;
				animObjectCopy.duration = duration;
				addAnimObject(animObjectCopy);
				return true;
			}

			return false;
		}

		bool setAnimationTime(int animObjectId, int animationId, int frameStart, int duration)
		{
			// Remove the animation then reinsert it. That way we make sure the list
			// stays sorted
			Animation animationCopy;
			int animationIndex = -1;
			int animObjectIndex = -1;
			for (int i = 0; i < mObjects.size(); i++)
			{
				if (mObjects[i].id == animObjectId)
				{
					// TODO: This is nasty. I should probably use a hash map to store
					// animations and anim objects
					for (int j = 0; j < mObjects[i].animations.size(); j++)
					{
						const Animation& animation = mObjects[i].animations[j];
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
				mObjects[animObjectIndex].animations.erase(mObjects[animObjectIndex].animations.begin() + animationIndex);
				animationCopy.frameStart = frameStart;
				animationCopy.duration = duration;
				addAnimationTo(animationCopy, mObjects[animObjectIndex]);
				return true;
			}

			return false;

		}

		void setAnimObjectTrack(int animObjectId, int track)
		{
			for (int i = 0; i < mObjects.size(); i++)
			{
				if (mObjects[i].id == animObjectId)
				{
					mObjects[i].timelineTrack = track;
					return;
				}
			}
		}

		void render(NVGcontext* vg, int frame)
		{
			for (auto objectIter = mObjects.begin(); objectIter != mObjects.end(); objectIter++)
			{
				// Reset to original state and apply animations in order
				if (objectIter->_svgObjectStart != nullptr && objectIter->svgObject != nullptr)
				{
					Svg::copy(objectIter->svgObject, objectIter->_svgObjectStart);
				}
				objectIter->position = objectIter->_positionStart;
				objectIter->rotation = objectIter->_rotationStart;
				objectIter->fillColor = objectIter->_fillColorStart;
				objectIter->strokeColor = objectIter->_strokeColorStart;
				objectIter->strokeWidth = objectIter->_strokeWidthStart;
				objectIter->isAnimating = false;

				for (auto animIter = objectIter->animations.begin(); animIter != objectIter->animations.end(); animIter++)
				{
					float parentFrameStart = (float)animIter->getParent()->frameStart;
					float absoluteFrameStart = animIter->frameStart + parentFrameStart;
					int animDeathTime = (int)absoluteFrameStart + animIter->duration;
					if (absoluteFrameStart <= frame)
					{
						if (frame <= animDeathTime)
						{
							float interpolatedT = ((float)frame - absoluteFrameStart) / (float)animIter->duration;
							animIter->render(vg, interpolatedT);

							objectIter->isAnimating = true;
						}
						else
						{
							animIter->applyAnimation(vg);
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

		const AnimObject* getObject(int animObjectId)
		{
			return getMutableObject(animObjectId);
		}

		AnimObject* getMutableObject(int animObjectId)
		{
			for (int i = 0; i < mObjects.size(); i++)
			{
				if (mObjects[i].id == animObjectId)
				{
					return &mObjects[i];
				}
			}

			return nullptr;
		}

		Animation* getMutableAnimation(int animationId)
		{
			for (int i = 0; i < mObjects.size(); i++)
			{
				for (int ai = 0; ai < mObjects[i].animations.size(); ai++)
				{
					if (mObjects[i].animations[ai].id == animationId)
					{
						return &mObjects[i].animations[ai];
					}
				}
			}

			return nullptr;
		}

		const std::vector<AnimObject>& getAnimObjects()
		{
			return mObjects;
		}

		const AnimObject* getNextAnimObject(int animObjectId)
		{
			int timelineTrack = -1;
			int frameStart = -1;
			int objIndex = -1;
			for (int i = 0; i < mObjects.size(); i++)
			{
				if (objIndex != -1)
				{
					if (mObjects[i].timelineTrack == timelineTrack && mObjects[i].frameStart > mObjects[objIndex].frameStart)
					{
						g_logger_assert(mObjects[i].frameStart >= mObjects[objIndex].frameStart + mObjects[objIndex].duration, "These two objects are intersecting.");
						return &mObjects[i];
					}
				}

				if (mObjects[i].id == animObjectId)
				{
					g_logger_assert(objIndex == -1, "Multiple objects found with object id %d", animObjectId);
					timelineTrack = mObjects[i].timelineTrack;
					frameStart = mObjects[i].frameStart;
					objIndex = i;
				}
			}

			return nullptr;
		}

		void serialize(const char* savePath)
		{
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
			uint32 numAnimations = (uint32)mObjects.size();
			memory.write<uint32>(&numAnimations);

			// Write out each animation followed by 0xDEADBEEF
			for (int i = 0; i < mObjects.size(); i++)
			{
				mObjects[i].serialize(memory);
				memory.write<uint32>(&MAGIC_NUMBER);
			}
			memory.shrinkToFit();

			FILE* fp = fopen(savePath, "wb");
			fwrite(memory.data, memory.size, 1, fp);
			fclose(fp);

			memory.free();
		}

		void deserialize(const char* loadPath)
		{
			FILE* fp = fopen(loadPath, "rb");
			if (!fp)
			{
				g_logger_warning("Could not load '%s', does not exist.", loadPath);
				return;
			}

			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			RawMemory memory;
			memory.init(fileSize);
			fread(memory.data, fileSize, 1, fp);
			fclose(fp);

			// Read magic number and version then dispatch to appropraite
			// deserializer
			// magicNumber   -> uint32
			// version       -> uint32
			uint32 magicNumber;
			memory.read<uint32>(&magicNumber);
			uint32 serializerVersion;
			memory.read<uint32>(&serializerVersion);

			g_logger_assert(magicNumber == MAGIC_NUMBER, "File '%s' had invalid magic number '0x%8x. File must have been corrupted.", loadPath, magicNumber);
			g_logger_assert((serializerVersion != 0 && serializerVersion <= SERIALIZER_VERSION), "File '%s' saved with invalid version '%d'. Looks like corrupted data.", loadPath, serializerVersion);

			if (serializerVersion == 1)
			{
				deserializeAnimationManagerExV1(memory);
			}
			else
			{
				g_logger_error("AnimationManagerEx serialized with unknown version '%d'.", serializerVersion);
			}

			memory.free();

			// Need to sort animation objects to ensure they get animations 
			// applied in the correct order
			sortAnimObjects();
		}

		static bool compareAnimObject(const AnimObject& ob1, const AnimObject& ob2)
		{
			return ob1.frameStart < ob2.frameStart;
		}

		static bool compareAnimation(const Animation& a1, const Animation& a2)
		{
			return a1.frameStart < a2.frameStart;
		}

		void sortAnimObjects()
		{
			std::sort(mObjects.begin(), mObjects.end(), compareAnimObject);
			for (int i = 0; i < mObjects.size(); i++)
			{
				std::sort(mObjects[i].animations.begin(), mObjects[i].animations.end(), compareAnimation);
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

		// Internal Functions
		static void deserializeAnimationManagerExV1(RawMemory& memory)
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
				mObjects.push_back(animObject);
				uint32 magicNumber;
				memory.read<uint32>(&magicNumber);
				g_logger_assert(magicNumber == MAGIC_NUMBER, "Corrupted animation in file data. Bad magic number '0x%8x'", magicNumber);
			}
		}
	}
}