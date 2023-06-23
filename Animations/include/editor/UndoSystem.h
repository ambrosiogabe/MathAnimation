#ifndef MATH_ANIM_UNDO_SYSTEM_H
#define MATH_ANIM_UNDO_SYSTEM_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;
	struct UndoSystemData;
	struct AnimObject;

	namespace UndoSystem
	{
		UndoSystemData* init(AnimationManagerData* const am, int maxHistory);
		void free(UndoSystemData* us);

		void undo(UndoSystemData* us);
		void redo(UndoSystemData* us);

		void setObjFillColor(UndoSystemData* us, AnimObjId objId, const glm::u8vec4& oldColor, const glm::u8vec4& newColor);
		void setObjStrokeColor(UndoSystemData* us, AnimObjId objId, const Vec4& newColor);
		void addNewObjToScene(UndoSystemData* us, const AnimObject& obj);
		void removeObjFromScene(UndoSystemData* us, AnimObjId objId);
	}
}

#endif