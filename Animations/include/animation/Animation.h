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
		Length
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
		int32 objectId;
		int32 frameStart;
		int32 duration;
		int32 id;
		EaseType easeType;
		EaseDirection easeDirection;

		union
		{
			ModifyVec4AnimData modifyVec4;
			ModifyVec3AnimData modifyVec3;
			ModifyVec2AnimData modifyVec2;
			ModifyU8Vec4AnimData modifyU8Vec4;
		} as;

		// Render the animation state using a interpolation t value
		// 
		//   t: is a float that ranges from [0, 1] where 0 is the
		//      beginning of the animation and 1 is the end of the
		//      animation
		void render(NVGcontext* vg, float t) const;
		// Set the animation to the end state without rendering it
		void applyAnimation(NVGcontext* vg) const;

		const AnimObject* getParent() const;
		AnimObject* getMutableParent() const;
		void free();
		void serialize(RawMemory& memory) const;
		static Animation deserialize(RawMemory& memory, uint32 version);
		static Animation createDefault(AnimTypeV1 type, int32 frameStart, int32 duration, int32 animObjectId);
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
		int32 id;
		int32 frameStart;
		int32 duration;
		int32 timelineTrack;
		SvgObject* _svgObjectStart;
		SvgObject* svgObject;
		bool isAnimating;
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
		std::vector<Animation> animations;
		// TODO: Do we want to restructure this to only have
		// a parent pointer or something?
		std::vector<AnimObject> children;

		union
		{
			TextObject textObject;
			LaTexObject laTexObject;
			Square square;
			Circle circle;
			Cube cube;
			Axis axis;
		} as;

		void render(NVGcontext* vg) const;
		void renderMoveToAnimation(NVGcontext* vg, float t, const Vec3& target);
		void renderFadeInAnimation(NVGcontext* vg, float t);
		void renderFadeOutAnimation(NVGcontext* vg, float t);
		void takeParentAttributes(const AnimObject* parent);
		
		void free();
		void serialize(RawMemory& memory) const;
		static AnimObject deserialize(RawMemory& memory, uint32 version);
		static AnimObject createDefaultFromParent(AnimObjectTypeV1 type, const AnimObject* parent);
		static AnimObject createDefault(AnimObjectTypeV1 type, int32 frameStart, int32 duration);
	};
}

#endif
