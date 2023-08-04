#ifndef MATH_ANIM_UNDO_SYSTEM_H
#define MATH_ANIM_UNDO_SYSTEM_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;
	struct UndoSystemData;
	struct AnimObject;

	typedef AnimObjId ObjOrAnimId;

	enum class U8Vec4PropType : uint8
	{
		FillColor = 0,
		StrokeColor
	};

	enum class Vec2PropType : uint8
	{
		MoveToTargetPos = 0,
		AnimateScaleTarget,
	};

	enum class Vec2iPropType : uint8
	{
		AspectRatio = 0,
	};
	
	enum class Vec3PropType : uint8
	{
		Position = 0,
		Scale,
		Rotation
	};

	enum class Vec4PropType : uint8
	{
		CameraBackgroundColor = 0,
	};

	enum class StringPropType : uint8
	{
		Name = 0,
		TextObjectText,
		CodeBlockText,
		LaTexText,
		SvgFilepath,
	};

	enum class FloatPropType : uint8
	{
		StrokeWidth = 0,
		LagRatio,
		CameraFieldOfView,
		CameraNearPlane,
		CameraFarPlane,
		CameraFocalDistance,
		CameraOrthoZoomLevel,
	};

	enum class EnumPropType : uint8
	{
		EaseType = 0,
		EaseDirection,
		PlaybackType,
		HighlighterLanguage,
		HighlighterTheme,
		CameraMode,
	};

	enum class AnimDragDropType : uint8
	{
		ReplacementTransformSrc = 0,
		ReplacementTransformDst,
		MoveToTarget,
		AnimateScaleTarget
	};

	namespace UndoSystem
	{
		UndoSystemData* init(AnimationManagerData* const am, int maxHistory);
		void free(UndoSystemData* us);

		void undo(UndoSystemData* us);
		void redo(UndoSystemData* us);

		void applyU8Vec4ToChildren(UndoSystemData* us, ObjOrAnimId id, U8Vec4PropType propType);
		void setU8Vec4Prop(UndoSystemData* us, ObjOrAnimId objId, const glm::u8vec4& oldVec, const glm::u8vec4& newVec, U8Vec4PropType propType);
		void setEnumProp(UndoSystemData* us, ObjOrAnimId id, int oldEnum, int newEnum, EnumPropType propType);
		void setFloatProp(UndoSystemData* us, ObjOrAnimId objId, float oldValue, float newValue, FloatPropType propType);
		void setVec2Prop(UndoSystemData* us, ObjOrAnimId id, const Vec2& oldVec, const Vec2& newVec, Vec2PropType propType);
		void setVec2iProp(UndoSystemData* us, ObjOrAnimId objId, const Vec2i& oldVec, const Vec2i& newVec, Vec2iPropType propType);
		void setVec3Prop(UndoSystemData* us, ObjOrAnimId objId, const Vec3& oldVec, const Vec3& newVec, Vec3PropType propType);
		void setVec4Prop(UndoSystemData* us, ObjOrAnimId objId, const Vec4& oldVec, const Vec4& newVec, Vec4PropType propType);
		void setStringProp(UndoSystemData* us, ObjOrAnimId objId, const std::string& oldString, const std::string& newString, StringPropType propType);
		void setFont(UndoSystemData* us, ObjOrAnimId objId, const std::string& oldFont, const std::string& newFont);

		void animDragDropInput(UndoSystemData* us, AnimObjId oldTarget, AnimObjId newTarget, AnimId animToAddTo, AnimDragDropType type);

		void addObjectToAnim(UndoSystemData* us, AnimObjId objToAdd, AnimId animToAddTo);
		void removeObjectFromAnim(UndoSystemData* us, AnimObjId objToAdd, AnimId animToAddTo);
		void addNewObjToScene(UndoSystemData* us, const AnimObject& obj);
		void removeObjFromScene(UndoSystemData* us, AnimObjId objId);
	}
}

#endif