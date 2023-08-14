#ifndef MATH_ANIM_CLIPBOARD_H
#define MATH_ANIM_CLIPBOARD_H
#include "core.h"

namespace MathAnim
{
	// Forward Decls
	struct AnimationManagerData;

	// Public API
	enum class ClipboardContents : uint8
	{
		None = 0,
		GameObject,
		AnimationClip
	};

	struct ClipboardData;

	namespace Clipboard
	{
		ClipboardData* create();
		void free(ClipboardData* data);

		ClipboardContents getType(const ClipboardData* clipboard);

		void freeContents(ClipboardData* clipboard);
		void copyObjectTo(ClipboardData* clipboard, const AnimationManagerData* am, AnimObjId objId);

		/**
		 * @brief Copies the clipboard contents (if it's an animation object) from the clipboard
		 *        to become a new child of the `parent`
		 * 
		 * @return Returns the newly copied object id
		*/
		AnimObjId pasteObjectToParent(ClipboardData* clipboard, AnimationManagerData* am, AnimObjId parent);

		/**
		 * @brief Copies the clipboard contents (if it's an animation object) from the clipboard
		 *        to become a new child of the scene. It takes the same parent as the original object.
		 *
		 * @return Returns the newly copied object id
		*/
		AnimObjId pasteObjectToScene(ClipboardData* clipboard, AnimationManagerData* am);

		/**
		 * @return Returns the newly copied object id
		*/
		AnimObjId duplicateObject(ClipboardData* clipboard, AnimationManagerData* am, AnimObjId obj);
	}
}

#endif 