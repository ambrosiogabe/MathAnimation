#include "editor/Clipboard.h"
#include "editor/panels/SceneHierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"

namespace MathAnim
{
	struct ClipboardData
	{
		ClipboardContents type;
		Animation lastCopiedAnimation;
		std::vector<AnimObject> lastCopiedObject;
	};

	namespace Clipboard
	{
		// ---------------- Internal functions ----------------
		static AnimObjId pasteObjectInternal(ClipboardData* clipboard, AnimationManagerData* am, AnimObjId newParent = NULL_ANIM_OBJECT, bool useNewParent = false);

		ClipboardData* create()
		{
			auto* data = g_memory_new ClipboardData();
			return data;
		}

		void free(ClipboardData* data)
		{
			freeContents(data);
			g_memory_delete(data);
		}

		ClipboardContents getType(const ClipboardData* data)
		{
			return data->type;
		}

		void freeContents(ClipboardData* board)
		{
			if (board->type == ClipboardContents::GameObject)
			{
				for (auto& lastObj : board->lastCopiedObject)
				{
					lastObj.free();
				}
			}
			else if (board->type == ClipboardContents::AnimationClip)
			{
				board->lastCopiedAnimation.free();
			}

			board->type = ClipboardContents::None;
		}

		void copyObjectTo(ClipboardData* clipboard, const AnimationManagerData* am, AnimObjId objId)
		{
			// Free the old contents
			const AnimObject* obj = AnimationManager::getObject(am, objId);
			if (obj)
			{
				freeContents(clipboard);
				clipboard->type = ClipboardContents::GameObject;
				clipboard->lastCopiedObject = AnimObject::createDeepCopyWithChildren(am, *obj);
			}
			else
			{
				g_logger_error("Tried to copy null object '{}' to clipboard.", objId);
			}
		}

		AnimObjId pasteObjectToParent(ClipboardData* clipboard, AnimationManagerData* am, AnimObjId newParent)
		{
			return pasteObjectInternal(clipboard, am, newParent, true);
		}

		AnimObjId pasteObjectToScene(ClipboardData* clipboard, AnimationManagerData* am)
		{
			return pasteObjectInternal(clipboard, am);
		}

		AnimObjId duplicateObject(ClipboardData* clipboard, AnimationManagerData* am, AnimObjId obj)
		{
			// Duplicate should just duplicate and not copy to the clipboard
			// so we'll save the old clipboard contents and restore them after the copy.
			ClipboardData oldClipboard = *clipboard;
			*clipboard = {};
			copyObjectTo(clipboard, am, obj);
			AnimObjId res = pasteObjectToScene(clipboard, am);
			freeContents(clipboard);
			// Then replace the old clipboard
			*clipboard = oldClipboard;

			return res;
		}

		// ---------------- Internal functions ----------------
		static AnimObjId pasteObjectInternal(ClipboardData* clipboard, AnimationManagerData* am, AnimObjId newParent, bool useNewParent)
		{
			if (clipboard->type != ClipboardContents::GameObject || clipboard->lastCopiedObject.size() == 0)
			{
				// No object to copy, so just exit early
				return NULL_ANIM_OBJECT;
			}

			// Create one more copy of the objects so they all get a unique ID and we can safely add them
			// to the scene without conflicting ID's
			std::vector<AnimObject> copiedObjects = {};
			std::unordered_map<AnimObjId, AnimObjId> copyIdMap = {};
			for (auto& obj : clipboard->lastCopiedObject)
			{
				AnimObject copy = obj.createDeepCopy();
				copyIdMap[obj.id] = copy.id;
				copiedObjects.emplace_back(copy);
			}

			if (useNewParent)
			{
				copiedObjects[0].parentId = newParent;
			}

			// Re-assign all the parent ID's to the appropriate newly created objects
			// And add them to the animation manager
			for (int i = 0; i < copiedObjects.size(); i++)
			{
				AnimObject& obj = copiedObjects[i];
				if (!isNull(obj.parentId))
				{
					auto iter = copyIdMap.find(obj.parentId);
					if (iter != copyIdMap.end())
					{
						obj.parentId = iter->second;
					}
					else if (i != 0)
					{
						g_logger_error("Failed to find suitable parent id '{}' while deep copying object '{}' in a paste operation.", obj.parentId);
					}
				}

				AnimationManager::addAnimObject(am, obj);
				SceneHierarchyPanel::addNewAnimObject(obj);
			}

			// Set the newly copied object as the active game object
			InspectorPanel::setActiveAnimObject(am, copiedObjects[0].id);
			return copiedObjects[0].id;
		}
	}
}