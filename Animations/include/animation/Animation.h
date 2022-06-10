#ifndef MATH_ANIM_ANIMATION_H
#define MATH_ANIM_ANIMATION_H
#include "core.h"

#include "animation/TextAnimations.h"
#include "animation/Shapes.h"

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
		Length
	};

	// Animation Structs
	struct MoveToAnimData
	{
		Vec2 target;

		void serialize(RawMemory& memory) const;
		static MoveToAnimData deserialize(RawMemory& memory, uint32 version);
	};

	// Base Structs
	struct Animation
	{
		AnimTypeV1 type;
		int32 objectId;
		int32 frameStart;
		int32 duration;
		int32 id;

		union
		{
			MoveToAnimData moveTo;
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
		Vec2 position;
		// This is the position before any animations are applied
		Vec2 _positionStart;
		int32 id;
		int32 frameStart;
		int32 duration;
		int32 timelineTrack;
		SvgObject* _svgObjectStart;
		SvgObject* svgObject;
		bool isAnimating;
		float _strokeWidthStart;
		float strokeWidth;
		glm::u8vec4 _strokeColorStart;
		glm::u8vec4 strokeColor;
		glm::u8vec4 _fillColorStart;
		glm::u8vec4 fillColor;
		std::vector<Animation> animations;

		union
		{
			TextObject textObject;
			LaTexObject laTexObject;
			Square square;
			Circle circle;
		} as;

		void render(NVGcontext* vg) const;
		void renderMoveToAnimation(NVGcontext* vg, float t, const Vec2& target);
		void renderFadeInAnimation(NVGcontext* vg, float t);
		void renderFadeOutAnimation(NVGcontext* vg, float t);
		void free();
		void serialize(RawMemory& memory) const;
		static AnimObject deserialize(RawMemory& memory, uint32 version);
		static AnimObject createDefault(AnimObjectTypeV1 type, int32 frameStart, int32 duration);
	};
}

#endif
