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
		TextObjectText,
		CodeBlockText,
	};

	enum class FloatPropType : uint8
	{
		StrokeWidth = 0,
		LagRatio,
	};

	enum class EnumPropType : uint8
	{
		EaseType = 0,
		EaseDirection,
		PlaybackType,
		HighlighterLanguage,
		HighlighterTheme,
	};

	namespace UndoSystem
	{
		UndoSystemData* init(AnimationManagerData* const am, int maxHistory);
		void free(UndoSystemData* us);

		void undo(UndoSystemData* us);
		void redo(UndoSystemData* us);

		void applyU8Vec4ToChildren(UndoSystemData* us, AnimObjId id, U8Vec4PropType propType);
		void setU8Vec4Prop(UndoSystemData* us, AnimObjId objId, const glm::u8vec4& oldVec, const glm::u8vec4& newVec, U8Vec4PropType propType);
		void setEnumProp(UndoSystemData* us, AnimObjId id, int oldEnum, int newEnum, EnumPropType propType);
		void setFloatProp(UndoSystemData* us, AnimObjId objId, float oldValue, float newValue, FloatPropType propType);
		void setVec3Prop(UndoSystemData* us, AnimObjId objId, const Vec3& oldVec, const Vec3& newVec, Vec3PropType propType);
		void setStringProp(UndoSystemData* us, AnimObjId objId, const std::string& oldString, const std::string& newString, StringPropType propType);
		void setFont(UndoSystemData* us, AnimObjId objId, const std::string& oldFont, const std::string& newFont);

		void addObjectToAnim(UndoSystemData* us, AnimObjId objToAdd, AnimId animToAddTo);
		void removeObjectFromAnim(UndoSystemData* us, AnimObjId objToAdd, AnimId animToAddTo);
		void addNewObjToScene(UndoSystemData* us, const AnimObject& obj);
		void removeObjFromScene(UndoSystemData* us, AnimObjId objId);
	}
}

#endif