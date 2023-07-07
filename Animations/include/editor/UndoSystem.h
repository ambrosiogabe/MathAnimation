#ifndef MATH_ANIM_UNDO_SYSTEM_H
#define MATH_ANIM_UNDO_SYSTEM_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;
	struct UndoSystemData;
	struct AnimObject;

	enum class U8Vec4PropType : uint8
	{
		FillColor = 0,
		StrokeColor
	};
	
	enum class Vec3PropType : uint8
	{
		Position = 0,
		Scale,
		Rotation
	};

	enum class StringPropType : uint8
	{
		Name = 0,
	};

	namespace UndoSystem
	{
		UndoSystemData* init(AnimationManagerData* const am, int maxHistory);
		void free(UndoSystemData* us);

		void undo(UndoSystemData* us);
		void redo(UndoSystemData* us);

		void applyU8Vec4ToChildren(UndoSystemData* us, AnimObjId id, U8Vec4PropType propType);
		void setU8Vec4Prop(UndoSystemData* us, AnimObjId objId, const glm::u8vec4& oldVec, const glm::u8vec4& newVec, U8Vec4PropType propType);
		void setVec3Prop(UndoSystemData* us, AnimObjId objId, const Vec3& oldVec, const Vec3& newVec, Vec3PropType propType);
		void setStringProp(UndoSystemData* us, AnimObjId objId, const std::string& oldString, const std::string& newString, StringPropType propType);
		void addNewObjToScene(UndoSystemData* us, const AnimObject& obj);
		void removeObjFromScene(UndoSystemData* us, AnimObjId objId);
	}
}

#endif