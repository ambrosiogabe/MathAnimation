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
		// Base
		FillColor = 0,
		StrokeColor,
		// Generic
		AnimateU8Vec4Target,
	};

	enum class Vec2PropType : uint8
	{
		// MoveTo
		MoveToTargetPos = 0,
		// Scale
		AnimateScaleTarget,
	};

	enum class Vec2iPropType : uint8
	{
		// Camera
		AspectRatio = 0,
		// Axis
		AxisXRange,
		AxisYRange,
		AxisZRange,
	};
	
	enum class Vec3PropType : uint8
	{
		// Base
		Position = 0,
		Scale,
		Rotation,
		// Generic
		ModifyAnimationVec3Target,
		// Axis
		AxisAxesLength,
	};

	enum class Vec4PropType : uint8
	{
		// Camera
		CameraBackgroundColor = 0,
		// Circumscribe
		CircumscribeColor,
	};

	enum class StringPropType : uint8
	{
		// Base
		Name = 0,
		// Text Object
		TextObjectText,
		// Codeblock
		CodeBlockText,
		// LaTeX Object
		LaTexText,
		// Svg Object
		SvgFilepath,
		// Image Object
		ImageFilepath,
		// Script Object
		ScriptFile,
	};

	enum class FloatPropType : uint8
	{
		// Base
		StrokeWidth = 0,
		LagRatio,
		// Camera
		CameraFieldOfView,
		CameraNearPlane,
		CameraFarPlane,
		CameraFocalDistance,
		CameraOrthoZoomLevel,
		// Circumscribe
		CircumscribeTimeWidth,
		CircumscribeBufferSize,
		// Square
		SquareSideLength,
		// Circle
		CircleRadius,
		// Arrow
		ArrowStemLength,
		ArrowStemWidth,
		ArrowTipLength,
		ArrowTipWidth,
		// Cube
		CubeSideLength,
		// Axis
		AxisXStep,
		AxisYStep,
		AxisZStep,
		AxisTickWidth,
		AxisFontSizePixels,
		AxisLabelPadding,
		AxisLabelStrokeWidth,
	};

	enum class EnumPropType : uint8
	{
		// Base
		EaseType = 0,
		EaseDirection,
		PlaybackType,
		// Codeblock
		HighlighterLanguage,
		HighlighterTheme,
		// Camera
		CameraMode,
		// Circumscribe
		CircumscribeShape,
		CircumscribeFade,
		// Image Object
		ImageSampleMode,
		ImageRepeat,
	};

	enum class AnimDragDropType : uint8
	{
		// Replacement
		ReplacementTransformSrc = 0,
		ReplacementTransformDst,
		// MoveTo
		MoveToTarget,
		// Scale
		AnimateScaleTarget,
		// Circumscribe
		CircumscribeTarget,
	};

	enum class BoolPropType : uint8
	{
		// Axis
		AxisDrawNumbers,
	};

	namespace UndoSystem
	{
		UndoSystemData* init(AnimationManagerData* const am, int maxHistory);
		void free(UndoSystemData* us);

		void undo(UndoSystemData* us);
		void redo(UndoSystemData* us);

		void setBoolProp(UndoSystemData* us, ObjOrAnimId id, bool oldBool, bool newBool, BoolPropType propType);
		void applyU8Vec4ToChildren(UndoSystemData* us, ObjOrAnimId id, U8Vec4PropType propType);
		void setU8Vec4Prop(UndoSystemData* us, ObjOrAnimId id, const glm::u8vec4& oldVec, const glm::u8vec4& newVec, U8Vec4PropType propType);
		void setEnumProp(UndoSystemData* us, ObjOrAnimId id, int oldEnum, int newEnum, EnumPropType propType);
		void setFloatProp(UndoSystemData* us, ObjOrAnimId id, float oldValue, float newValue, FloatPropType propType);
		void setVec2Prop(UndoSystemData* us, ObjOrAnimId id, const Vec2& oldVec, const Vec2& newVec, Vec2PropType propType);
		void setVec2iProp(UndoSystemData* us, ObjOrAnimId id, const Vec2i& oldVec, const Vec2i& newVec, Vec2iPropType propType);
		void setVec3Prop(UndoSystemData* us, ObjOrAnimId id, const Vec3& oldVec, const Vec3& newVec, Vec3PropType propType);
		void setVec4Prop(UndoSystemData* us, ObjOrAnimId id, const Vec4& oldVec, const Vec4& newVec, Vec4PropType propType);
		void setStringProp(UndoSystemData* us, ObjOrAnimId id, const std::string& oldString, const std::string& newString, StringPropType propType);
		void setFont(UndoSystemData* us, ObjOrAnimId id, const std::string& oldFont, const std::string& newFont);

		void animDragDropInput(UndoSystemData* us, AnimObjId oldTarget, AnimObjId newTarget, AnimId animToAddTo, AnimDragDropType type);

		void addObjectToAnim(UndoSystemData* us, AnimObjId objToAdd, AnimId animToAddTo);
		void removeObjectFromAnim(UndoSystemData* us, AnimObjId objToAdd, AnimId animToAddTo);

		void addNewObjToScene(UndoSystemData* us, int animObjType);
		void removeObjFromScene(UndoSystemData* us, AnimObjId objId);
	}
}

#endif