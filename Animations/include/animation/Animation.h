#ifndef MATH_ANIM_ANIMATION_H
#define MATH_ANIM_ANIMATION_H
#include "core.h"
#include "utils/CMath.h"

#include "animation/TextAnimations.h"
#include "animation/Shapes.h"
#include "animation/Axis.h"
#include "renderer/OrthoCamera.h"

namespace MathAnim
{
	struct Font;
	struct SvgObject;
	struct AnimationManagerData;
	
	// Constants
	constexpr uint32 SERIALIZER_VERSION = 1;
	constexpr uint32 MAGIC_NUMBER = 0xDEADBEEF;

	// Types
	enum class AnimObjectTypeV1 : uint32
	{
		None = 0,
		TextObject,
		LaTexObject,
		Square,
		Circle,
		Cube,
		Axis,
		SvgObject,
		Length
	};

	enum class AnimTypeV1 : uint32
	{
		None = 0,
		WriteInText,
		MoveTo,
		Create,
		Transform,
		UnCreate,
		FadeIn,
		FadeOut,
		RotateTo,
		AnimateStrokeColor,
		AnimateFillColor,
		AnimateStrokeWidth,
        CameraMoveTo,
		Shift,
		Length
	};

	enum class PlaybackType : uint8
	{
		None = 0,
		Synchronous,
		LaggedStart,
		Length
	};

	constexpr const char* playbackTypeNames[(uint8)PlaybackType::Length] = {
		"None",
		"Synchronous",
		"Lagged Start"
	};

	// Animation Structs
	struct ModifyVec4AnimData
	{
		Vec4 target;
	};

	struct ModifyU8Vec4AnimData
	{
		glm::u8vec4 target;
	};

	struct ModifyVec3AnimData
	{
		Vec3 target;
	};

	struct ModifyVec2AnimData
	{
		Vec2 target;
	};

	// Base Structs
	struct Animation
	{
		AnimTypeV1 type;
		int32 frameStart;
		int32 duration;
		int32 id;
		int32 timelineTrack;
		EaseType easeType;
		EaseDirection easeDirection;
		PlaybackType playbackType;
		float lagRatio;
		std::vector<int32> animObjectIds;

		union
		{
			ModifyVec4AnimData modifyVec4;
			ModifyVec3AnimData modifyVec3;
			ModifyVec2AnimData modifyVec2;
			ModifyU8Vec4AnimData modifyU8Vec4;
		} as;

		// Apply the animation state using a interpolation t value
		// 
		//   t: is a float that ranges from [0, 1] where 0 is the
		//      beginning of the animation and 1 is the end of the
		//      animation
		void applyAnimation(AnimationManagerData* am, NVGcontext* vg, float t = 1.0) const;

		// Render the gizmo with relation to this object
		void onGizmo(const AnimObject* obj);
		// Render the gizmo for this animation with no relation to it's child objects
		void onGizmo();

		void free();
		void serialize(RawMemory& memory) const;
		static Animation deserialize(RawMemory& memory, uint32 version);
		static Animation createDefault(AnimTypeV1 type, int32 frameStart, int32 duration);
	};

	enum class AnimObjectStatus : uint8
	{
		Inactive,
		Animating,
		Active
	};

	struct AnimObject
	{
		AnimObjectTypeV1 objectType;
		Vec3 position;
		Vec3 rotation;
		Vec3 scale;
		// Rotation is stored by rotX, rotY, rotZ order of rotations
		Vec3 _rotationStart;
		// This is the position before any animations are applied
		Vec3 _positionStart;
		Vec3 _scaleStart;
		// This is the percent created ranging from [0.0-1.0] which determines 
		// what to pass to renderCreateAnimation(...)
		float percentCreated;

		int32 id;
		int32 parentId;
		uint8* name;
		uint32 nameLength;

		SvgObject* _svgObjectStart;
		SvgObject* svgObject;
		float svgScale;
		AnimObjectStatus status;
		bool isTransparent;
		bool drawDebugBoxes;
		bool drawCurveDebugBoxes;
		bool is3D;
		float _strokeWidthStart;
		float strokeWidth;
		glm::u8vec4 _strokeColorStart;
		glm::u8vec4 strokeColor;
		glm::u8vec4 _fillColorStart;
		glm::u8vec4 fillColor;

		union
		{
			TextObject textObject;
			LaTexObject laTexObject;
			Square square;
			Circle circle;
			Cube cube;
			Axis axis;
		} as;

		void onGizmo();
		void render(NVGcontext* vg) const;
		void renderMoveToAnimation(NVGcontext* vg, float t, const Vec3& target);
		void renderFadeInAnimation(NVGcontext* vg, float t);
		void renderFadeOutAnimation(NVGcontext* vg, float t);
		void takeParentAttributes(const AnimObject* parent);
		
		void free();
		void serialize(RawMemory& memory) const;
		static AnimObject deserialize(RawMemory& memory, uint32 version);
		static AnimObject createDefaultFromParent(AnimObjectTypeV1 type, const AnimObject* parent);
		static AnimObject createDefault(AnimObjectTypeV1 type);
	};
}

#endif
