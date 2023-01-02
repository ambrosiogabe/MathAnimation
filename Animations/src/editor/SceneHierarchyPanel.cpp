#include "editor/SceneHierarchyPanel.h"
#include "editor/Timeline.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "utils/IconsFontAwesome5.h"
#include "core/Colors.h"
#include "core/Input.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace MathAnim
{
	// --------- Internal Structs ---------
	struct SceneTreeMetadata
	{
		AnimObjId animObjectId;
		int level;
		int index;
		bool selected;
		bool isOpen;
	};

	struct BetweenMetadata
	{
		ImRect rect;
		int index;
	};

	namespace SceneHierarchyPanel
	{
		// --------- Internal variables ---------
		// This is the in between spaces for all the different elements in the
		// scene heirarchy tree
		static std::vector<BetweenMetadata> inBetweenBuffer;
		static std::vector<SceneTreeMetadata> orderedEntities;
		static std::vector<SceneTreeMetadata> orderedEntitiesCopy;

		// --------- Internal functions ---------
		static void imGuiRightClickPopup(AnimationManagerData* am);
		static bool doTreeNode(AnimationManagerData* am, SceneTreeMetadata& element, const AnimObject& animObject, SceneTreeMetadata& nextElement);
		static bool isDescendantOf(AnimationManagerData* am, AnimObjId childAnimObjId, AnimObjId parentAnimObjId);
		static bool imGuiSceneHeirarchyWindow(int* inBetweenIndex);
		static void addElementAsChild(AnimationManagerData* am, int parentIndex, int newChildIndex);
		static void moveTreeTo(AnimationManagerData* am, int treeToMoveIndex, int placeToMoveToIndex, bool reparent = true);
		static void updateLevel(int parentIndex, int newLevel);
		static int getNumChildren(int parentIndex);
		static void addExistingAnimObject(AnimationManagerData* am, const AnimObject& obj, int level);

		void init(AnimationManagerData* am)
		{
			inBetweenBuffer = std::vector<BetweenMetadata>();
			orderedEntities = std::vector<SceneTreeMetadata>();
			orderedEntitiesCopy = std::vector<SceneTreeMetadata>();

			const std::vector<AnimObject>& animObjects = AnimationManager::getAnimObjects(am);
			for (int i = 0; i < animObjects.size(); i++)
			{
				// Root object
				if (isNull(animObjects[i].parentId))
				{
					addExistingAnimObject(am, animObjects[i], 0);
				}
			}
		}

		void free()
		{

		}

		void addNewAnimObject(const AnimObject& animObject)
		{
			// TODO: Consider making anim object creation a message then subscribing to this message type
			int newIndex = (int32)orderedEntities.size();
			int level = 0;
			if (!isNull(animObject.parentId))
			{
				// Find out where to insert object
				for (int i = 0; i < orderedEntities.size(); i++)
				{
					const auto& element = orderedEntities[i];
					if (element.animObjectId == animObject.parentId)
					{
						newIndex = i + 1;
						level = element.level + 1;
						break;
					}
				}
			}
			orderedEntities.insert(orderedEntities.begin() + newIndex, { animObject.id, level, newIndex, false });
			orderedEntitiesCopy.insert(orderedEntitiesCopy.begin() + newIndex, SceneTreeMetadata{ animObject.id, level, newIndex, false });

			// Update indices
			for (int i = newIndex; i < orderedEntities.size(); i++)
			{
				orderedEntities[i].index = i;
				orderedEntitiesCopy[i].index = i;
			}
		}

		void update(AnimationManagerData* am)
		{
			// TODO: Save when a tree node is open
			ImGui::Begin(ICON_FA_PROJECT_DIAGRAM " Scene");
			inBetweenBuffer.clear();

			// Now iterate through all the entities
			int activeElementIndex = -1;
			for (int i = 0; i < (int)orderedEntities.size(); i++)
			{
				SceneTreeMetadata& element = orderedEntities[i];
				const AnimObject* animObject = AnimationManager::getObject(am, element.animObjectId);
				if (!animObject)
				{
					animObject = AnimationManager::getPendingObject(am, element.animObjectId);
					g_logger_assert(animObject != nullptr, "Scene hierarchy tried to access anim object with id '%d' that does not exist and is not pending addition.", element.animObjectId);
				}

				if (element.selected)
				{
					activeElementIndex = i;
				}

				// Next element wraps around to 0, which plays nice with all of our sorting logic
				SceneTreeMetadata& nextElement = orderedEntities[(i + 1) % orderedEntities.size()];
				int isOpen = 1;
				if (!doTreeNode(am, element, *animObject, nextElement))
				{
					// If the tree node is not open, skip all the children
					int lastIndex = (int)orderedEntities.size() - 1;
					for (int j = i + 1; j < (int)orderedEntities.size(); j++)
					{
						if (orderedEntities[j].level <= element.level)
						{
							lastIndex = j - 1;
							break;
						}
					}
					i = lastIndex;
					nextElement = orderedEntities[(i + 1) % orderedEntities.size()];
					isOpen = 0;
				}

				if (nextElement.level <= element.level)
				{
					int numPops = element.level - nextElement.level + isOpen;
					for (int treePops = 0; treePops < numPops; treePops++)
					{
						ImGui::TreePop();
					}
				}
			}

			// We do this after drawing all the elements so that we can loop through all
			// the rects in the window
			static int inBetweenIndex = 0;
			if (imGuiSceneHeirarchyWindow(&inBetweenIndex))
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Timeline::getAnimObjectPayloadId()))
				{
					g_logger_assert(payload->DataSize == sizeof(AnimObjectPayload), "Invalid payload.");
					const AnimObjectPayload* objPayload = (const AnimObjectPayload*)payload->Data;
					int childIndex = objPayload->sceneHierarchyIndex;
					g_logger_assert(childIndex >= 0 && childIndex < orderedEntities.size(), "Invalid payload.");
					moveTreeTo(am, childIndex, inBetweenIndex);
				}
				ImGui::EndDragDropTarget();
			}

			// Synchronize the copies
			for (int i = 0; i < orderedEntitiesCopy.size(); i++)
			{
				// TODO: Maybe better way to do this?
				orderedEntitiesCopy[i].isOpen = orderedEntities[i].isOpen;
				orderedEntitiesCopy[i].selected = orderedEntities[i].selected;
				orderedEntities[i] = orderedEntitiesCopy[i];
			}

			imGuiRightClickPopup(am);

			// Handle delete animation object
			if (ImGui::IsWindowHovered() && activeElementIndex != -1 && Input::keyPressed(GLFW_KEY_DELETE))
			{
				const AnimObject* animObject = AnimationManager::getObject(am, orderedEntities[activeElementIndex].animObjectId);
				if (animObject)
				{
					// TODO: Have this create some sort of event that we can subscribe to like:
					//    EVENT --- DeleteAnimObject
					// That way I don't have to worry about who's responsibility it is to remove
					// the anim objects from the animation manager and the timeline
					deleteAnimObject(*animObject);
					AnimationManager::removeAnimObject(am, animObject->id);
					Timeline::setActiveAnimObject(NULL_ANIM_OBJECT);
				}
			}

			ImGui::End();
		}

		void deleteAnimObject(const AnimObject& animObjectToDelete)
		{
			int numChildren = -1;
			int parentIndex = -1;

			bool hasEntity = false;
			for (int index = 0; index < orderedEntities.size(); index++)
			{
				if (orderedEntities[index].animObjectId == animObjectToDelete.id)
				{
					numChildren = getNumChildren(index);
					parentIndex = index;
					hasEntity = true;
					break;
				}
			}

			if (!hasEntity)
			{
				g_logger_warning("Deleted entity that wasn't registered with the scene hierarchy tree.");
				return;
			}

			if (parentIndex >= 0)
			{
				// The +1 is for the parent to be deleted also
				for (int i = 0; i < numChildren + 1; i++)
				{
					orderedEntities.erase(orderedEntities.begin() + parentIndex);
					orderedEntitiesCopy.erase(orderedEntitiesCopy.begin() + parentIndex);
				}

				for (int i = parentIndex; i < orderedEntities.size(); i++)
				{
					orderedEntities[i].index = i;
					orderedEntitiesCopy[i].index = i;
				}
			}
		}

		void serialize(RawMemory&)
		{
			//json orderedEntitiesJson = {};
			//for (int i = 0; i < orderedEntities.size(); i++)
			//{
			//	SceneTreeMetadata metadata = orderedEntities[i];
			//	json entityId = {
			//		{ "Id", NEntity::getId(metadata.entity) },
			//		{ "Level", metadata.level },
			//		{ "Index", metadata.index },
			//		{ "Selected", metadata.selected },
			//		{ "IsOpen", metadata.isOpen }
			//	};
			//	orderedEntitiesJson.push_back(entityId);
			//}
			//j["SceneHeirarchyOrder"] = orderedEntitiesJson;
		}

		void deserialize(RawMemory&)
		{
			// TODO: See if this is consistent with how you load the rest of the assets
			//orderedEntities.clear(false);
			//orderedEntitiesCopy.clear(false);

			//if (j.contains("SceneHeirarchyOrder"))
			//{
			//	for (auto& entityJson : j["SceneHeirarchyOrder"])
			//	{
			//		if (entityJson.is_null()) continue;

			//		uint32 entityId = -1;
			//		JsonExtended::assignIfNotNull(entityJson, "Id", entityId);
			//		int level = -1;
			//		JsonExtended::assignIfNotNull(entityJson, "Level", level);
			//		Logger::Assert(level != -1, "Invalid entity level serialized for scene heirarchy tree.");
			//		int index = -1;
			//		JsonExtended::assignIfNotNull(entityJson, "Index", index);
			//		Logger::Assert(index == orderedEntities.size(), "Scene tree was not serialized in sorted order, this will cause problems.");
			//		bool selected = false;
			//		JsonExtended::assignIfNotNull(entityJson, "Selected", selected);
			//		bool isOpen = false;
			//		JsonExtended::assignIfNotNull(entityJson, "IsOpen", isOpen);

			//		Logger::Assert(entt::entity(entityId) != entt::null, "Somehow a null entity got serialized in the scene heirarchy panel.");
			//		Logger::Assert(Scene::isValid(scene, entityId), "Somehow an invalid entity id got serialized in the scene heirarchy panel.");
			//		Entity entity = Entity{ entt::entity(entityId) };
			//		orderedEntities.push({ entity, level, index, selected, isOpen });
			//		orderedEntitiesCopy.push({ entity, level, index, selected, isOpen });
			//	}
			//}
		}

		// --------- Internal functions ---------
		static void imGuiRightClickPopup(AnimationManagerData* am)
		{
			// String buffer to write object names
			char buffer[256];
			size_t bufferSize = sizeof(buffer);

			if (ImGui::BeginPopupContextWindow())
			{
				for (int i = 1; i < (int)AnimObjectTypeV1::Length; i++)
				{
					if (AnimObject::isInternalObjectOnly((AnimObjectTypeV1)i))
					{
						continue;
					}

					int strRes = sprintf_s(buffer, bufferSize, "Add %s\0", AnimObject::getAnimObjectName((AnimObjectTypeV1)i));
					if (strRes != -1)
					{
						if (ImGui::MenuItem(buffer))
						{
							AnimObject animObject = AnimObject::createDefault(am, (AnimObjectTypeV1)i);
							AnimationManager::addAnimObject(am, animObject);
							addNewAnimObject(animObject);
						}
					}
				}

				ImGui::EndPopup();
			}
		}

		static bool doTreeNode(AnimationManagerData* am, SceneTreeMetadata& element, const AnimObject& animObject, SceneTreeMetadata& nextElement)
		{
			const AnimObject* nextAnimObject = AnimationManager::getObject(am, nextElement.animObjectId);

			if (animObject.status == AnimObjectStatus::Inactive)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Neutral[4]);
			}
			else if (animObject.status == AnimObjectStatus::Animating)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentGreen[1]);
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Neutral[0]);
			}

			ImGui::PushID(element.index);
			ImGui::SetNextItemOpen(element.isOpen);
			bool open = ImGui::TreeNodeEx(
				animObject.name,
				ImGuiTreeNodeFlags_FramePadding |
				(element.selected ? ImGuiTreeNodeFlags_Selected : 0) |
				(nextAnimObject && nextAnimObject->parentId == element.animObjectId ? 0 : ImGuiTreeNodeFlags_Leaf) |
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_SpanFullWidth,
				"%s", animObject.name);
			ImGui::PopID();
			element.isOpen = open;

			// Pop style color for tree node text
			ImGui::PopStyleColor();

			// Track the in-between size for this tree node
			ImVec2 cursorPos = ImGui::GetCursorPos();
			ImVec2 elementSize = ImGui::GetItemRectSize();
			elementSize.x -= ImGui::GetStyle().FramePadding.x;
			elementSize.y = ImGui::GetStyle().FramePadding.y;
			cursorPos.y -= ImGui::GetStyle().FramePadding.y;
			ImVec2 windowPos = ImGui::GetCurrentWindow()->Pos;
			inBetweenBuffer.emplace_back(
				BetweenMetadata
				{
					ImRect(
						windowPos.x + cursorPos.x,
						windowPos.y + cursorPos.y,
						windowPos.x + cursorPos.x + elementSize.x,
						windowPos.y + cursorPos.y + elementSize.y
					),
					element.index
				}
			);

			bool clicked = ImGui::IsItemClicked();

			// We do drag drop right after the element we want it to effect, and this one will be a source and drag target
			if (ImGui::BeginDragDropSource())
			{
				// Set payload to carry AnimObjectPayload
				static AnimObjectPayload payload;
				payload.animObjectId = element.animObjectId;
				payload.sceneHierarchyIndex = element.index;
				ImGui::SetDragDropPayload(Timeline::getAnimObjectPayloadId(), &payload, sizeof(AnimObjectPayload));
				// TODO: Add anim object names
				ImGui::Text("%s", "Temporary Name");
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Timeline::getAnimObjectPayloadId()))
				{
					g_logger_assert(payload->DataSize == sizeof(AnimObjectPayload), "Invalid payload.");
					const AnimObjectPayload* objPayload = (const AnimObjectPayload*)payload->Data;
					int childIndex = objPayload->sceneHierarchyIndex;
					g_logger_assert(childIndex >= 0 && childIndex < orderedEntities.size(), "Invalid payload.");
					SceneTreeMetadata& childMetadata = orderedEntitiesCopy[childIndex];
					if (!isDescendantOf(am, element.animObjectId, childMetadata.animObjectId))
					{
						addElementAsChild(am, element.index, childIndex);
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (clicked)
			{
				//InspectorWindow::clearAllEntities();
				Timeline::setActiveAnimObject(element.animObjectId);
			}

			element.selected = Timeline::getActiveAnimObject() == element.animObjectId;
			return open;
		}

		static bool isDescendantOf(AnimationManagerData* am, AnimObjId childAnimObjId, AnimObjId parentAnimObjId)
		{
			const AnimObject* childObj = AnimationManager::getObject(am, childAnimObjId);
			if (childObj)
			{
				if (childObj->parentId == parentAnimObjId || childAnimObjId == parentAnimObjId)
				{
					return true;
				}
				else if (!isNull(childObj->parentId))
				{
					return isDescendantOf(am, childObj->parentId, parentAnimObjId);
				}
			}

			return false;
		}

		static bool imGuiSceneHeirarchyWindow(int* inBetweenIndex)
		{
			ImGuiContext& g = *GImGui;
			if (!g.DragDropActive)
				return false;

			if (std::strcmp(g.DragDropPayload.DataType, Timeline::getAnimObjectPayloadId()) != 0)
				return false;

			ImGuiWindow* window = g.CurrentWindow;
			if (window == NULL)
				return false;

			ImRect windowRect = window->Rect();
			if (!ImGui::IsMouseHoveringRect(windowRect.Min, windowRect.Max))
				return false;
			if (window->SkipItems)
				return false;

			// We need to find the in-between spaces of two tree nodes in the scene heirarchy tree.
			// So, if we just loop through all the objects in this window we should be able to 
			// maybe find out which in-between we are in
			bool hoveringBetween = false;
			for (int i = 0; i < inBetweenBuffer.size(); i++)
			{
				BetweenMetadata& meta = inBetweenBuffer[i];
				if (ImGui::IsMouseHoveringRect(meta.rect.Min, meta.rect.Max))
				{
					windowRect = meta.rect;
					// I just tweaked these values until I got a line of about 1 pixel.
					// TODO: Test this on different resolution sized screens and make sure it's 1 pixel there as well
					windowRect.Min.y += 4;
					windowRect.Max.y = windowRect.Min.y - 4;
					*inBetweenIndex = meta.index;
					hoveringBetween = true;
					break;
				}
			}

			g_logger_assert(inBetweenBuffer.size() > 0, "No tree elements, impossible to be dragging them...");
			ImVec2 mousePos = ImGui::GetMousePos();
			if (mousePos.y > inBetweenBuffer[inBetweenBuffer.size() - 1].rect.Max.y)
			{
				// If we are below all elements default to showing a place at the bottom
				// of the elements as where it will be added
				SceneTreeMetadata& lastElement = orderedEntitiesCopy[orderedEntitiesCopy.size() - 1];
				windowRect = inBetweenBuffer[inBetweenBuffer.size() - 1].rect;
				windowRect.Min.y += 4;
				windowRect.Max.y = windowRect.Min.y - 4;
				hoveringBetween = true;
				*inBetweenIndex = lastElement.index;
			}

			if (!hoveringBetween)
			{
				return false;
			}

			IM_ASSERT(g.DragDropWithinTarget == false);
			g.DragDropTargetRect = windowRect;
			g.DragDropTargetId = window->ID;
			g.DragDropWithinTarget = true;
			return true;
		}

		static void addElementAsChild(AnimationManagerData* am, int parentIndex, int newChildIndex)
		{
			g_logger_assert(parentIndex != newChildIndex, "Tried to child a parent to itself, not possible.");

			SceneTreeMetadata& parent = orderedEntitiesCopy[parentIndex];
			SceneTreeMetadata& newChild = orderedEntitiesCopy[newChildIndex];
			AnimObject* childAnimObj = AnimationManager::getMutableObject(am, newChild.animObjectId);
			AnimObject* parentAnimObj = AnimationManager::getMutableObject(am, parent.animObjectId);

			if (childAnimObj && parentAnimObj)
			{
				childAnimObj->parentId = parent.animObjectId;
				// TODO: This should automatically get updated since objects store local and absolute transformations
				// but double check that it works alright
				// 
				//childTransform.localPosition = childTransform.position - parentTransform.position;
				//childTransform.localEulerRotation = childTransform.eulerRotation - parentTransform.eulerRotation;
				//childTransform.localScale = childTransform.scale - parentTransform.scale;
				updateLevel(newChildIndex, parent.level + 1);
				int placeToMoveToIndex = parent.index < newChild.index ?
					parent.index + 1 :
					parent.index;

				moveTreeTo(am, newChildIndex, placeToMoveToIndex, false);
			}
		}

		static void moveTreeTo(AnimationManagerData* am, int treeToMoveIndex, int placeToMoveToIndex, bool reparent)
		{
			if (placeToMoveToIndex == treeToMoveIndex)
			{
				// We're in the right place already, exit early
				return;
			}

			SceneTreeMetadata& placeToMoveTo = orderedEntitiesCopy[placeToMoveToIndex];
			SceneTreeMetadata& treeToMove = orderedEntitiesCopy[treeToMoveIndex];
			if (isDescendantOf(am, placeToMoveTo.animObjectId, treeToMove.animObjectId))
			{
				return;
			}

			// We have to move this element and all it's children right into the parent's slot
			// so first find how many children this element has
			int numChildren = getNumChildren(treeToMoveIndex);
			int numItemsToCopy = numChildren + 1;

			if (reparent)
			{
				AnimObject* treeToMoveObj = AnimationManager::getMutableObject(am, treeToMove.animObjectId);
				AnimObject* placeToMoveToObj = AnimationManager::getMutableObject(am, placeToMoveTo.animObjectId);
				if (treeToMoveObj && placeToMoveToObj)
				{
					// AnimObject* newParentTransform = !AnimationManager::isObjectNull(placeToMoveToObj->parentId) ?
					// 	NEntity::getComponent<TransformData>(placeToMoveToTransform.parent) :
					// 	Transform::createTransform();

					// Check if parent is open or closed, if they are closed then we want to use their parent and level
					if (!isNull(placeToMoveToObj->parentId))
					{
						// Need to start at the root and go down until you find a closed parent and thats the correct new parent
						// TODO: Not sure if this is the actual root of the problem
					}

					updateLevel(treeToMove.index, placeToMoveTo.level);
					treeToMoveObj->parentId = placeToMoveToObj->parentId;
					// TODO: Should be fine, see TODO above
					// treeToMoveObj.localPosition = treeToMoveTransform.position - newParentTransform.position;
				}
			}

			// Temporarily copy the tree we are about to move
			SceneTreeMetadata* copyOfTreeToMove = (SceneTreeMetadata*)g_memory_allocate(sizeof(SceneTreeMetadata) * numItemsToCopy);
			memcpy(copyOfTreeToMove, &orderedEntitiesCopy[treeToMoveIndex], sizeof(SceneTreeMetadata) * numItemsToCopy);

			if (placeToMoveTo.index < treeToMove.index)
			{
				// Step 1
				SceneTreeMetadata* dataAboveTreeToMove = (SceneTreeMetadata*)g_memory_allocate(sizeof(SceneTreeMetadata) * (treeToMoveIndex - placeToMoveToIndex));
				memcpy(dataAboveTreeToMove, &orderedEntitiesCopy[placeToMoveToIndex], sizeof(SceneTreeMetadata) * (treeToMoveIndex - placeToMoveToIndex));

				// Step 2
				memcpy(&orderedEntitiesCopy[placeToMoveToIndex + numItemsToCopy], dataAboveTreeToMove, sizeof(SceneTreeMetadata) * (treeToMoveIndex - placeToMoveToIndex));
				g_memory_free(dataAboveTreeToMove);

				// Step 3
				memcpy(&orderedEntitiesCopy[placeToMoveToIndex], copyOfTreeToMove, sizeof(SceneTreeMetadata) * numItemsToCopy);
			}
			else
			{
				// Step 1
				int sizeOfData = placeToMoveToIndex - (treeToMoveIndex + numChildren);

				// Copying trees down is a bit trickier, because if we try to place this guy in the split of a tree going down,
				// then we have to move that whole tree up to compensate...
				if (placeToMoveToIndex + 1 < orderedEntitiesCopy.size())
				{
					if (isDescendantOf(
						am,
						orderedEntitiesCopy[placeToMoveToIndex + 1].animObjectId,
						orderedEntitiesCopy[placeToMoveToIndex].animObjectId))
					{
						int parentLevel = orderedEntitiesCopy[placeToMoveToIndex].level;
						// Tricky, now we have to move the whole subtree
						for (int i = placeToMoveToIndex + 1; i < orderedEntitiesCopy.size(); i++)
						{
							if (orderedEntitiesCopy[i].level <= parentLevel)
							{
								break;
							}
							sizeOfData++;
							placeToMoveToIndex++;
						}

						g_logger_assert(placeToMoveToIndex < orderedEntitiesCopy.size(), "Invalid place to move to calculation.");
					}
				}

				SceneTreeMetadata* dataBelowTreeToMove = (SceneTreeMetadata*)g_memory_allocate(sizeof(SceneTreeMetadata) * sizeOfData);
				memcpy(dataBelowTreeToMove, &orderedEntitiesCopy[placeToMoveToIndex - sizeOfData + 1], sizeof(SceneTreeMetadata) * sizeOfData);

				// Step 2
				memcpy(&orderedEntitiesCopy[treeToMoveIndex], dataBelowTreeToMove, sizeof(SceneTreeMetadata) * sizeOfData);
				g_memory_free(dataBelowTreeToMove);

				// Step 3
				memcpy(&orderedEntitiesCopy[placeToMoveToIndex - numChildren], copyOfTreeToMove, sizeof(SceneTreeMetadata) * numItemsToCopy);
			}

			// Update indices
			for (int i = 0; i < orderedEntitiesCopy.size(); i++)
			{
				orderedEntitiesCopy[i].index = i;
			}
			g_memory_free(copyOfTreeToMove);
		}

		static void updateLevel(int parentIndex, int newParentLevel)
		{
			g_logger_assert(parentIndex >= 0 && parentIndex < orderedEntitiesCopy.size(), "Out of bounds index.");
			SceneTreeMetadata& parent = orderedEntitiesCopy[parentIndex];
			for (int i = parent.index + 1; i < orderedEntitiesCopy.size(); i++)
			{
				// We don't have to worry about going out of bounds with that plus one, because if it is out
				// of bounds then the for loop won't execute anyways
				SceneTreeMetadata& element = orderedEntitiesCopy[i];
				if (element.level <= parent.level)
				{
					break;
				}

				element.level += (newParentLevel - parent.level);
			}
			parent.level = newParentLevel;
		}

		static int getNumChildren(int parentIndex)
		{
			g_logger_assert(parentIndex >= 0 && parentIndex < orderedEntitiesCopy.size(), "Out of bounds index.");
			SceneTreeMetadata& parent = orderedEntitiesCopy[parentIndex];
			int numChildren = 0;
			for (int i = parent.index + 1; i < orderedEntitiesCopy.size(); i++)
			{
				// We don't have to worry about going out of bounds with that plus one, because if it is out
				// of bounds then the for loop won't execute anyways
				SceneTreeMetadata& element = orderedEntitiesCopy[i];
				if (element.level <= parent.level)
				{
					break;
				}
				numChildren++;
			}

			return numChildren;
		}

		static void addExistingAnimObject(AnimationManagerData* am, const AnimObject& obj, int level)
		{
			// Add this object first
			addNewAnimObject(obj);

			// Recursively add all children
			std::vector<AnimObjId> children = AnimationManager::getChildren(am, obj.id);
			while (children.size() > 0)
			{
				const AnimObject* child = AnimationManager::getObject(am, children.back());
				addExistingAnimObject(am, *child, level + 1);
				children.pop_back();
			}
		}
	}
}
