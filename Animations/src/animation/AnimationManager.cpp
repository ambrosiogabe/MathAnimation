#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "renderer/Renderer.h"
#include "renderer/Texture.h"
#include "renderer/Framebuffer.h"
#include "renderer/Camera.h"
#include "editor/EditorSettings.h"
#include "editor/panels/InspectorPanel.h"
#include "svg/Svg.h"
#include "math/CMath.h"
#include "core/Application.h"
#include "core/Profiling.h"
#include "core/Serialization.hpp"

#include <nlohmann/json.hpp>

namespace MathAnim
{
	struct AnimationManagerData
	{
		std::vector<AnimObject> objects;
		// Maps from AnimObjectId -> Index in objects vector
		std::unordered_map<AnimObjId, size_t> objectIdMap;

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
		AnimObjId activeCamera;
		int currentFrame;
	};

	namespace AnimationManager
	{
		// -------- Internal Functions --------
		static bool compareAnimation(const Animation& a1, const Animation& a2);
		static void updateGlobalTransform(AnimObject& obj, const glm::mat4& parentTransform, const glm::mat4& parentTransformStart);
		static void addQueuedAnimObject(AnimationManagerData* am, const AnimObject& obj);
		static void addQueuedAnimation(AnimationManagerData* am, const Animation& anim);
		static void removeQueuedAnimObject(AnimationManagerData* am, AnimObjId animObj);
		static void removeQueuedAnimation(AnimationManagerData* am, AnimId animation);
		static bool removeSingleAnimObject(AnimationManagerData* am, AnimObjId animObj);
		static void applyDelta(AnimationManagerData* am, int deltaFrame);
		static void applyAnimationsFrom(AnimationManagerData* am, int startIndex, int frame, bool calculateKeyframes = false);

		AnimationManagerData* create()
		{
			void* animManagerMemory = g_memory_allocate(sizeof(AnimationManagerData));
			// Placement new to ensure the vectors and stuff are appropriately constructed
			// but I can still use my memory tracker
			AnimationManagerData* res = new (animManagerMemory) AnimationManagerData();

			res->activeCamera = NULL_ANIM_OBJECT;
			res->currentFrame = 0;

			return res;
		}

		void free(AnimationManagerData* am)
		{
			if (am)
			{
				// Free animation objects
				for (int i = 0; i < am->objects.size(); i++)
				{
					am->objects[i].free();
				}

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
			MP_PROFILE_EVENT("AnimationManager_EndFrame");

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

			// End camera frames
			AnimObject* activeCamera = getMutableObject(am, am->activeCamera);
			if (activeCamera)
			{
				activeCamera->as.camera.endFrame();
			}
		}

		void resetToFrame(AnimationManagerData* am, uint32 absoluteFrame)
		{
			MP_PROFILE_EVENT("AnimationManager_ResetToFrame");
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Reset all object states
			for (auto objectIter = am->objects.begin(); objectIter != am->objects.end(); objectIter++)
			{
				// Reset to original state and apply animations in order
				objectIter->resetAllState();
			}

			// Update all children global transforms and stuff
			applyGlobalTransforms(am);

			// Then apply each animation up to the current frame
			if (absoluteFrame > 0)
			{
				applyAnimationsFrom(am, 0, absoluteFrame);
			}
			applyGlobalTransforms(am);
			calculateBBoxes(am);

			am->currentFrame = absoluteFrame;
		}

		void calculateAnimationKeyFrames(AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Reset all object states
			for (auto objectIter = am->objects.begin(); objectIter != am->objects.end(); objectIter++)
			{
				// Reset to original state and apply animations in order
				objectIter->resetAllState();
			}

			// Update all children global transforms and stuff
			applyGlobalTransforms(am);

			// Then apply each animation up to the current frame
			applyAnimationsFrom(am, 0, lastAnimatedFrame(am), true);
			applyGlobalTransforms(am);
			calculateBBoxes(am);
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

		void removeAnimObject(AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// These removes get queued until the end of the frame so that
			// pointers are stable for at least the duration of one frame
			am->queuedRemoveObjects.push_back(animObj);
			if (animObj == am->activeCamera)
			{
				am->activeCamera = NULL_ANIM_OBJECT;
			}
		}

		void removeAnimation(AnimationManagerData* am, AnimId anim)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// These removes get queued until the end of the frame so that
			// pointers are stable for at least the duration of one frame
			am->queuedRemoveAnimations.push_back(anim);
		}

		void addObjectToAnim(AnimationManagerData* am, AnimObjId animObjId, AnimId animationId)
		{
			Animation* anim = getMutableAnimation(am, animationId);
			AnimObject* obj = getMutableObject(am, animObjId);
			if (anim && obj)
			{
				anim->animObjectIds.insert(animObjId);
				obj->referencedAnimations.insert(animationId);
			}
		}

		void removeObjectFromAnim(AnimationManagerData* am, AnimObjId animObjId, AnimId animationId)
		{
			Animation* anim = getMutableAnimation(am, animationId);
			AnimObject* obj = getMutableObject(am, animObjId);
			if (anim && obj)
			{
				anim->animObjectIds.erase(animObjId);
				obj->referencedAnimations.erase(animationId);
			}
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

		void render(AnimationManagerData* am, int deltaFrame)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");
			MP_PROFILE_EVENT("AnimationManager_Render");

			if (deltaFrame != 0)
			{
				applyDelta(am, deltaFrame);
			}

			// NOTE: Render any active/animating objects
			{
				MP_PROFILE_EVENT("AnimationManager_UpdateActiveObjects");

				for (auto objectIter = am->objects.begin(); objectIter != am->objects.end(); objectIter++)
				{
					if (objectIter->status != AnimObjectStatus::Inactive)
					{
						objectIter->render(am);
					}

					// Update any updateable objects
					switch (objectIter->objectType)
					{
					case AnimObjectTypeV1::Image: objectIter->as.image.update(am, objectIter->id); break;
					case AnimObjectTypeV1::LaTexObject: objectIter->as.laTexObject.update(am, objectIter->id); break;
					case AnimObjectTypeV1::Camera:
						objectIter->as.camera.position = objectIter->globalPosition;
						objectIter->as.camera.orientation = CMath::quatFromEulerAngles(objectIter->rotation);
						break;
					default:
						break;
					}
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

			// Add 1 extra second of footage for good measure
			return lastFrame + 60;
		}

		bool isPastLastFrame(const AnimationManagerData* am)
		{
			return am->currentFrame >= AnimationManager::lastAnimatedFrame(am);
		}

		const Camera& getActiveCamera(const AnimationManagerData* am)
		{
			g_logger_assert(hasActiveCamera(am), "No Camera set on the scene, cannot retrieve it.");
			const AnimObject* obj = getObject(am, am->activeCamera);
			return obj->as.camera;
		}

		bool hasActiveCamera(const AnimationManagerData* am)
		{
			return am->activeCamera != NULL_ANIM_OBJECT;
		}

		void setActiveCamera(AnimationManagerData* am, AnimObjId cameraObj)
		{
			am->activeCamera = cameraObj;
		}

		void calculateCameraMatrices(AnimationManagerData* am)
		{
			AnimObject* camera = getMutableObject(am, am->activeCamera);
			if (camera)
			{
				camera->as.camera.calculateMatrices();
			}
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

		AnimObject* getMutableObject(AnimationManagerData* am, AnimObjId animObj)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			if (isNull(animObj))
			{
				return nullptr;
			}

			{
				auto iter = am->objectIdMap.find(animObj);
				if (iter != am->objectIdMap.end())
				{
					size_t objectIndex = iter->second;
					if (objectIndex >= 0 && objectIndex < am->objects.size())
					{
						return &am->objects[objectIndex];
					}
				}
			}

			{
				// Check if the object is queued for addition
				if (am->queuedAddObjects.size() > 0)
				{
					for (int i = 0; i < am->queuedAddObjects.size(); i++)
					{
						if (am->queuedAddObjects[i].id == animObj)
						{
							return &am->queuedAddObjects[i];
						}
					}
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

			{
				// Check if the object is queued for addition
				if (am->queuedAddAnimations.size() > 0)
				{
					for (int i = 0; i < am->queuedAddAnimations.size(); i++)
					{
						if (am->queuedAddAnimations[i].id == anim)
						{
							return &am->queuedAddAnimations[i];
						}
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
			std::vector<AnimObjId> res;
			for (size_t i = 0; i < am->objects.size(); i++)
			{
				if (am->objects[i].parentId == animObj)
				{
					res.push_back(am->objects[i].id);
				}
			}

			for (size_t i = 0; i < am->queuedAddObjects.size(); i++)
			{
				if (am->queuedAddObjects[i].parentId == animObj)
				{
					res.push_back(am->queuedAddObjects[i].id);
				}
			}

			return res;
		}

		AnimObjId getNextSibling(const AnimationManagerData* am, AnimObjId objId)
		{
			const AnimObject* obj = getObject(am, objId);
			if (obj)
			{
				bool passedMyself = false;
				for (int i = 0; i < am->objects.size(); i++)
				{
					if (am->objects[i].id == objId)
					{
						passedMyself = true;
					}
					else if (am->objects[i].parentId == obj->parentId && passedMyself)
					{
						return am->objects[i].id;
					}
				}
			}

			return objId;
		}

		void serialize(const AnimationManagerData* am, nlohmann::json& output)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Custom data starts here. Subject to change from version to version
			writeIdToJson("ActiveCamera", am->activeCamera, output);
			output["Animations"] = nlohmann::json::array();

			// Write out each animation
			for (int i = 0; i < am->animations.size(); i++)
			{
				nlohmann::json animJson = nlohmann::json();
				am->animations[i].serialize(animJson);
				output["Animations"].emplace_back(animJson);
			}

			// Write out each anim object
			output["AnimationObjects"] = nlohmann::json::array();
			for (int i = 0; i < am->objects.size(); i++)
			{
				nlohmann::json animObjectJson = nlohmann::json();
				am->objects[i].serialize(animObjectJson);
				output["AnimationObjects"].emplace_back(animObjectJson);
			}
		}

		void deserialize(AnimationManagerData* am, const nlohmann::json& j, int currentFrame, uint32 versionMajor, uint32 versionMinor)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			if (versionMajor < 2 || versionMajor > 3)
			{
				g_logger_error("AnimationManager tried to deserialize save file with unknown version '{}.{}'.", versionMajor, versionMinor);
			}

			// TODO: Deprecate 2D cameras. All cameras will be 3D from here on out.
			am->activeCamera = readIdFromJson(j, "StartingActiveCamera");
			// NOTE: This is for legacy projects. It would be better to bump the serialized file version
			//       but that's too much work. And this hack works fine.
			if (isNull(am->activeCamera))
			{
				am->activeCamera = readIdFromJson(j, "ActiveCamera");
			}
			if (isNull(am->activeCamera))
			{
				am->activeCamera = readIdFromJson(j, "ActiveCamera3D");
			}

			// Read each animation followed by 0xDEADBEEF
			for (size_t i = 0; i < j["Animations"].size(); i++)
			{
				Animation animation = Animation::deserialize(j["Animations"][i], versionMajor);
				am->animationIdMap[animation.id] = i;
				am->animations.emplace_back(animation);
			}

			// Read each anim object followed by 0xDEADBEEF
			for (size_t i = 0; i < j["AnimationObjects"].size(); i++)
			{
				AnimObject animObject = AnimObject::deserialize(j["AnimationObjects"][i], versionMajor);
				am->objectIdMap[animObject.id] = i;
				am->objects.emplace_back(animObject);
			}

			// Sort the animations afterwards since they could be inserted in random order
			sortAnimations(am);

			am->currentFrame = currentFrame;
			// Calculate all key frame starting points and stuff
			calculateAnimationKeyFrames(am);
			// Apply all  animations appropriately
			Application::resetToFrame(currentFrame);
			resetToFrame(am, currentFrame);
		}

		void sortAnimations(AnimationManagerData* am)
		{
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			std::sort(am->animations.begin(), am->animations.end(), compareAnimation);
		}

		void legacy_deserialize(AnimationManagerData* am, RawMemory& memory, int currentFrame)
		{
			// TODO: Deprecate me
			g_logger_assert(am != nullptr, "Null AnimationManagerData.");

			// Read magic number and version then dispatch to appropraite
			// deserializer
			// magicNumber   -> uint32
			// version       -> uint32
			uint32 magicNumber;
			memory.read<uint32>(&magicNumber);
			uint32 serializerVersion;
			memory.read<uint32>(&serializerVersion);

			g_logger_assert(magicNumber == MAGIC_NUMBER, "Project file had invalid magic number '{:#010x}'. File must have been corrupted.", magicNumber);
			g_logger_assert((serializerVersion != 0 && serializerVersion <= 1), "Project file saved with invalid version '{}'. Looks like corrupted data.", serializerVersion);

			if (serializerVersion == 1)
			{
				{
					// We're in function V1 so this is a version 1 for sure
					constexpr uint32 version = 1;

					// startingActiveCamera -> AnimObjId
					// numAnimations -> uint32
					// animations    -> dynamic
					memory.read<AnimObjId>(&am->activeCamera);
					uint32 numAnimations;
					memory.read<uint32>(&numAnimations);

					// Read each animation followed by 0xDEADBEEF
					for (uint32 i = 0; i < numAnimations; i++)
					{
						Animation animation = Animation::legacy_deserialize(memory, version);
						am->animations.push_back(animation);
						memory.read<uint32>(&magicNumber);
						g_logger_assert(magicNumber == MAGIC_NUMBER, "Corrupted animation in file data. Bad magic number '{:#010x}'", magicNumber);

						am->animationIdMap[animation.id] = i;
					}

					// numAnimObjects -> uint32
					// animObjects    -> dynamic
					uint32 numAnimObjects;
					memory.read<uint32>(&numAnimObjects);

					// Read each anim object followed by 0xDEADBEEF
					for (uint32 i = 0; i < numAnimObjects; i++)
					{
						AnimObject animObject = AnimObject::legacy_deserialize(am, memory, version);
						am->objects.push_back(animObject);
						memory.read<uint32>(&magicNumber);
						g_logger_assert(magicNumber == MAGIC_NUMBER, "Corrupted animation in file data. Bad magic number '{:#010x}'", magicNumber);

						am->objectIdMap[animObject.id] = i;
					}
				}

				am->currentFrame = currentFrame;
				// Calculate all key frame starting points and stuff
				calculateAnimationKeyFrames(am);
				// Apply all  animations appropriately
				Application::resetToFrame(currentFrame);
				resetToFrame(am, currentFrame);
			}
			else
			{
				g_logger_error("AnimationManagerEx serialized with unknown version '{}'.", serializerVersion);
			}

			// Need to sort animations they get applied in the correct order
			sortAnimations(am);
		}

		void retargetSvgScales(AnimationManagerData* am)
		{
			for (int i = 0; i < am->objects.size(); i++)
			{
				am->objects[i].retargetSvgScale();
			}
		}

		void applyGlobalTransforms(AnimationManagerData* am)
		{
			MP_PROFILE_EVENT("AnimationManager_ApplyGlobalTransforms");
			// ----- Apply the parent->child transformations -----
			// Find all root objects and update recursively
			for (auto objIter = am->objects.begin(); objIter != am->objects.end(); objIter++)
			{
				// If the object has no parent, it's a root object.
				// Update the transform then update children recursively
				// and in order from parent->child
				if (isNull(objIter->parentId))
				{
					applyGlobalTransformsTo(am, objIter->id);
				}
			}
		}

		void applyGlobalTransformsTo(AnimationManagerData* am, AnimObjId obj)
		{
			// Initialize the queue with the object to update
			std::queue<AnimObjId> objects = {};
			objects.push(obj);

			// Then update all children
			// Loop through each object and recursively update the transforms by appending children to the queue
			while (objects.size() > 0)
			{
				AnimObjId nextObjId = objects.front();
				objects.pop();

				// Update child transform
				AnimObject* nextObj = getMutableObject(am, nextObjId);
				if (nextObj)
				{
					glm::mat4 parentTransform = glm::mat4(1.0f);
					glm::mat4 parentTransformStart = glm::mat4(1.0f);
					const AnimObject* parent = getObject(am, nextObj->parentId);
					if (parent)
					{
						parentTransform = parent->globalTransform;
						parentTransformStart = parent->_globalTransformStart;
					}
					updateGlobalTransform(*nextObj, parentTransform, parentTransformStart);

					// Then append all direct children to the queue so they are
					// recursively updated
					for (auto childIter = am->objects.begin(); childIter != am->objects.end(); childIter++)
					{
						if (childIter->parentId == nextObj->id)
						{
							objects.push(childIter->id);
						}
					}

					for (auto childIter = am->queuedAddObjects.begin(); childIter != am->queuedAddObjects.end(); childIter++)
					{
						if (childIter->parentId == nextObj->id)
						{
							objects.push(childIter->id);
						}
					}
				}
			}
		}

		void calculateBBoxes(AnimationManagerData* am)
		{
			MP_PROFILE_EVENT("AnimationManager_CalculateBBoxes");
			// ----- Calculate child bbox first then parent -----
			// Find all root objects and update recursively
			for (auto objIter = am->objects.begin(); objIter != am->objects.end(); objIter++)
			{
				// If the object has no parent, it's a root object.
				// Update the transform then update children recursively
				// and in order from parent->child
				if (isNull(objIter->parentId))
				{
					calculateBBoxFor(am, objIter->id);
				}
			}
		}

		void calculateBBoxFor(AnimationManagerData* am, AnimObjId obj)
		{
			// Parent
			//   -> Child1
			//   -> Child2
			//      -> Grandchild1
			//         -> GGChild1
			//         -> GGChild2
			//   -> Child3
			//      -> Grandchild1
			//      -> Grandchild2

			// Update all children
			// Loop through each object and recursively update the bboxes by appending children to the queue
			// Update child transform
			AnimObject* nextObj = getMutableObject(am, obj);
			if (nextObj)
			{
				glm::vec3 scaleFactor, skew, translation;
				glm::quat orientation;
				glm::vec4 perspective;
				glm::decompose(nextObj->globalTransform, scaleFactor, orientation, translation, skew, perspective);

				BBox finalBoundingBox = BBox{};
				if (nextObj->svgObject)
				{
					nextObj->svgObject->calculateBBox();
					finalBoundingBox = nextObj->svgObject->bbox;

					finalBoundingBox.min.x *= scaleFactor.x;
					finalBoundingBox.max.x *= scaleFactor.x;
					finalBoundingBox.min.y *= scaleFactor.y;
					finalBoundingBox.max.y *= scaleFactor.y;
					finalBoundingBox.min += CMath::vector2From3(nextObj->globalPosition);
					finalBoundingBox.max += CMath::vector2From3(nextObj->globalPosition);
					Vec2 halfSize = (finalBoundingBox.max - finalBoundingBox.min) / 2.0f;
					finalBoundingBox.min -= halfSize;
					finalBoundingBox.max -= halfSize;
				}
				else
				{
					finalBoundingBox.min = Vec2{ FLT_MAX, FLT_MAX };
					finalBoundingBox.max = Vec2{ -FLT_MAX, -FLT_MAX };
				}

				// Then append all direct children to the queue so they are
				// recursively updated
				for (auto childIter = am->objects.begin(); childIter != am->objects.end(); childIter++)
				{
					if (childIter->parentId == nextObj->id)
					{
						calculateBBoxFor(am, childIter->id);
						finalBoundingBox.min = CMath::min(finalBoundingBox.min, childIter->bbox.min);
						finalBoundingBox.max = CMath::max(finalBoundingBox.max, childIter->bbox.max);
					}
				}

				nextObj->bbox = finalBoundingBox;
			}
		}

		void updateObjectState(AnimationManagerData* am, AnimObjId animObjId)
		{
			AnimObject* obj = getMutableObject(am, animObjId);
			if (!obj)
			{
				g_logger_warning("Cannot update state of object that does not exist for AnimObjID: '{}'", animObjId);
				return;
			}

			// It's easiest to just apply all updates from the
			// root of the scene, so we'll find the root of this
			// object, reset all the children then update from there

			if (!isNull(obj->parentId))
			{
				AnimObject* parent;
				while ((parent = getMutableObject(am, obj->parentId)) != nullptr)
				{
					obj = parent;
					animObjId = parent->id;
				}
			}

			obj->resetAllState();
			for (auto childIter = obj->beginBreadthFirst(am); childIter != obj->end(); ++childIter)
			{
				AnimObjId childId = *childIter;
				AnimObject* child = getMutableObject(am, childId);
				if (child)
				{
					child->resetAllState();
				}
			}

			// Reset all global transforms
			applyGlobalTransformsTo(am, animObjId);

			// Apply any changes from animations in order
			for (auto animIter = obj->referencedAnimations.begin(); animIter != obj->referencedAnimations.end(); animIter++)
			{
				Animation* anim = AnimationManager::getMutableAnimation(am, *animIter);
				if (anim)
				{
					float frameStart = (float)anim->frameStart;
					if (frameStart <= am->currentFrame)
					{
						// Calculate key frames
						anim->calculateKeyframes(am);

						// Then apply the animation
						float interpolatedT = glm::clamp(((float)am->currentFrame - frameStart) / (float)anim->duration, 0.0f, 1.0f);
						anim->applyAnimationToObj(am, animObjId, interpolatedT);
					}
				}
			}

			// Apply global transform after the object has animations applied since
			// the animations may change the positions
			applyGlobalTransformsTo(am, animObjId);
			calculateBBoxFor(am, animObjId);
		}

		// -------- Internal Functions --------
		static bool compareAnimation(const Animation& a1, const Animation& a2)
		{
			// Sort by timeline track secondarily
			if (a1.frameStart == a2.frameStart)
			{
				return a1.timelineTrack < a2.timelineTrack;
			}

			return a1.frameStart < a2.frameStart;
		}

		static void updateGlobalTransform(AnimObject& obj, const glm::mat4& parentTransform, const glm::mat4& parentTransformStart)
		{
			// Calculate global transformation
			obj.globalTransform = parentTransform * CMath::calculateTransform(obj.rotation, obj.scale, obj.position);
			obj._globalTransformStart = parentTransformStart * CMath::calculateTransform(obj._rotationStart, obj._scaleStart, obj._positionStart);

			// Extract positions
			obj.globalPosition = CMath::extractPosition(obj.globalTransform);
			obj._globalPositionStart = CMath::extractPosition(obj._globalTransformStart);
		}

		static void addQueuedAnimObject(AnimationManagerData* am, const AnimObject& obj)
		{
			am->objects.push_back(obj);
			am->objectIdMap[obj.id] = am->objects.size() - 1;
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

			// First remove all children objects recursively
			std::vector<AnimObjId> children = getChildren(am, animObj);
			for (int i = 0; i < children.size(); i++)
			{
				removeQueuedAnimObject(am, children[i]);
			}

			// Then remove the parent
			if (!removeSingleAnimObject(am, animObj))
			{
				g_logger_warning("Tried to delete AnimObject<ID: '{}'>, which does not exist.", animObj);
			}
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

		static bool removeSingleAnimObject(AnimationManagerData* am, AnimObjId animObj)
		{
			// Remove the anim object
			auto iter = am->objectIdMap.find(animObj);
			if (iter == am->objectIdMap.end())
			{
				return false;
			}

			size_t animObjectIndex = iter->second;
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
			}

			// Remove any references from old animations
			for (auto animIter = am->animations.begin(); animIter != am->animations.end(); animIter++)
			{
				// TODO: The animIterIds should be a set not a vector
				auto deleteIterAnimRef = std::find(animIter->animObjectIds.begin(), animIter->animObjectIds.end(), animObj);
				if (deleteIterAnimRef != animIter->animObjectIds.end())
				{
					removeObjectFromAnim(am, animObj, animIter->id);
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

		static void applyDelta(AnimationManagerData* am, int deltaFrame)
		{
			MP_PROFILE_EVENT("AnimationManager_ApplyDelta");
			// int previousFrame = am->currentFrame;
			am->currentFrame += deltaFrame;
			int newFrame = am->currentFrame;

			resetToFrame(am, newFrame);

			// TODO: FIXME
			// if (am->animations.size() == 0)
			//{
			//	return;
			//}

			// if (deltaFrame < 0)
			//{
			//	// Assume newFrame < previousFrame
			//	int animIndexToStartFrom = (int)am->animations.size() - 1;
			//	for (size_t i = 0; i < am->animations.size(); i++)
			//	{
			//		const Animation& anim = am->animations[i];
			//		if (anim.frameStart > previousFrame)
			//		{
			//			g_logger_assert(i > 0, "Somehow the timeline cursor is at -1.");
			//			animIndexToStartFrom = (int)i - 1;
			//			break;
			//		}
			//	}

			//	int reapplyAnimationsFrom = 0;
			//	for (int i = animIndexToStartFrom; i >= 0; i--)
			//	{
			//		const Animation& anim = am->animations[i];
			//		int animStart = anim.frameStart;
			//		int animEnd = anim.frameStart + anim.duration;
			//		bool intersecting = newFrame <= animStart && animEnd <= previousFrame;
			//		if (intersecting)
			//		{
			//			anim.applyAnimation(am, 0.0f);
			//		}
			//		else
			//		{
			//			g_logger_assert(i < am->animations.size(), "Somehow we didn't reapply anything.");
			//			reapplyAnimationsFrom = i + 1;
			//			break;
			//		}
			//	}
			//	applyAnimationsFrom(am, animIndexToStartFrom, newFrame);
			//}
			// else
			//{
			//	// Assume newFrame >= previousFrame
			//	int animIndexToStartFrom = 0;
			//	// Start from the earliest animation that encompasses all the changes
			//	for (size_t i = am->animations.size(); i > 0; i--)
			//	{
			//		const Animation& anim = am->animations[i - 1];
			//		if (anim.frameStart >= previousFrame)
			//		{
			//			g_logger_assert(i > 0, "Somehow the timeline cursor is at -1.");
			//			animIndexToStartFrom = (int)i - 1;
			//		}
			//		else if (anim.frameStart + anim.duration >= previousFrame)
			//		{
			//			g_logger_assert(i > 0, "Somehow the timeline cursor is at -1.");
			//			animIndexToStartFrom = (int)i - 1;
			//		}
			//	}

			//	int reapplyAnimationsFrom = 0;
			//	for (int i = animIndexToStartFrom; i >= 0; i--)
			//	{
			//		const Animation& anim = am->animations[i];
			//		int animStart = anim.frameStart;
			//		int animEnd = anim.frameStart + anim.duration;
			//		bool intersecting = newFrame <= animStart && animEnd <= previousFrame;
			//		if (intersecting)
			//		{
			//			anim.applyAnimation(am, 0.0f);
			//		}
			//		else
			//		{
			//			g_logger_assert(i < am->animations.size(), "Somehow we didn't reapply anything.");
			//			reapplyAnimationsFrom = i + 1;
			//			break;
			//		}
			//	}
			//	applyAnimationsFrom(am, animIndexToStartFrom, newFrame);
			//}
			// applyGlobalTransforms(am);
		}

		static void applyAnimationsFrom(AnimationManagerData* am, int startIndex, int currentFrame, bool calculateKeyframes)
		{
			MP_PROFILE_EVENT("AnimationManager_ApplyAnimationsFrom");

			// Apply any changes from animations in order
			for (auto animIter = am->animations.begin() + startIndex; animIter != am->animations.end(); animIter++)
			{
				float frameStart = (float)animIter->frameStart;
				if (frameStart <= currentFrame)
				{
					// Then apply the animation
					float interpolatedT = ((float)currentFrame - frameStart) / (float)animIter->duration;
					if (calculateKeyframes)
					{
						animIter->calculateKeyframes(am);
					}
					animIter->applyAnimation(am, interpolatedT);
				}
			}
		}
	}
}