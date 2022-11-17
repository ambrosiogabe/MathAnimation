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
	struct SceneSnapshot;
	
	// Constants
	constexpr uint32 SERIALIZER_VERSION = 1;
	constexpr uint32 MAGIC_NUMBER = 0xDEADBEEF;
	constexpr AnimObjId NULL_ANIM_OBJECT = UINT64_MAX;
	constexpr AnimId NULL_ANIM = UINT64_MAX;

	inline bool isNull(AnimObjId animObj) { return animObj == NULL_ANIM_OBJECT; }

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

	struct ReplacementTransformData
	{
		AnimObjId srcAnimObjectId;
		AnimObjId dstAnimObjectId;

		void serialize(RawMemory& memory) const;
		static ReplacementTransformData deserialize(RawMemory& memory);
	};

	// Base Structs
	struct Animation
	{
		AnimTypeV1 type;
		int32 frameStart;
		int32 duration;
		AnimId id;
		int32 timelineTrack;
		EaseType easeType;
		EaseDirection easeDirection;
		PlaybackType playbackType;
		float lagRatio;
		std::vector<AnimObjId> animObjectIds;

		union
		{
			ModifyVec4AnimData modifyVec4;
			ModifyVec3AnimData modifyVec3;
			ModifyVec2AnimData modifyVec2;
			ModifyU8Vec4AnimData modifyU8Vec4;
			ReplacementTransformData replacementTransform;
		} as;

		// Apply the animation state using a interpolation t value
		// 
		//   t: is a float that ranges from [0, 1] where 0 is the
		//      beginning of the animation and 1 is the end of the
		//      animation
		void applyAnimation(AnimationManagerData* am, NVGcontext* vg, float t = 1.0f) const;
		void applyAnimationToSnapshot(SceneSnapshot& snapshot, NVGcontext* vg, float t = 1.0f) const;

		// Render the gizmo with relation to this object
		void onGizmo(const AnimObject* obj);
		// Render the gizmo for this animation with no relation to it's child objects
		void onGizmo();

		bool shouldDisplayAnimObjects() const;

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

	class AnimObjectBreadthFirstIter
	{
	public:
		AnimObjectBreadthFirstIter(AnimationManagerData* am, AnimObjId parentId);

		void operator++();

		inline bool operator==(AnimObjId other) const { return currentId == other; }
		inline bool operator!=(AnimObjId other) const { return currentId != other; }
		inline AnimObjId operator*() const { return currentId; }

	private: 
		AnimationManagerData* am;
		std::deque<AnimObjId> childrenLeft;
		AnimObjId currentId;
	};

	class AnimObjectBreadthFirstIterSnapshot
	{
	public:
		AnimObjectBreadthFirstIterSnapshot(SceneSnapshot& am, AnimObjId parentId)
			: snapshot(am)
		{
			init(parentId);
		}

		void init(AnimObjId parentId);

		void operator++();

		inline bool operator==(AnimObjId other) const { return currentId == other; }
		inline bool operator!=(AnimObjId other) const { return currentId != other; }
		inline AnimObjId operator*() const { return currentId; }

	private:
		SceneSnapshot& snapshot;
		std::deque<AnimObjId> childrenLeft;
		AnimObjId currentId;
	};

	struct AnimObjectState
	{
		AnimObjectTypeV1 objectType;
		Vec3 position;
		Vec3 rotation;
		Vec3 scale;
		SvgObject* svgObject;
		float svgScale;
		// This is the percent created ranging from [0.0-1.0] which determines 
		// what to pass to renderCreateAnimation(...)
		float percentCreated;
		float strokeWidth;
		glm::u8vec4 strokeColor;
		glm::u8vec4 fillColor;

		// Transform stuff
		// TODO: Consider moving this to a Transform class
		// This is the combined parent+child positions and transformations
		Vec3 globalPosition;
		glm::mat4 globalTransform;

		AnimObjId id;
		AnimObjId parentId;
		std::vector<AnimObjId> generatedChildrenIds;

		bool isTransparent;
		bool drawDebugBoxes;
		bool drawCurveDebugBoxes;
		bool drawCurves;
		bool drawControlPoints;
		bool is3D;
		bool isGenerated;

		union
		{
			TextObject textObject;
			LaTexObject laTexObject;
			Square square;
			Circle circle;
			Cube cube;
			Axis axis;
		} as;
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

		// Transform stuff
		// TODO: Consider moving this to a Transform class
		// This is the combined parent+child positions and transformations
		Vec3 _globalPositionStart;
		Vec3 globalPosition;
		glm::mat4 globalTransform;

		AnimObjId id;
		AnimObjId parentId;
		std::vector<AnimObjId> generatedChildrenIds;

		uint8* name;
		uint32 nameLength;

		SvgObject* _svgObjectStart;
		SvgObject* svgObject;
		float svgScale;
		AnimObjectStatus status;
		bool isTransparent;
		bool drawDebugBoxes;
		bool drawCurveDebugBoxes;
		bool drawCurves;
		bool drawControlPoints;
		bool is3D;
		bool isGenerated;
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

		void onGizmo(AnimationManagerData* am);
		void render(NVGcontext* vg, AnimationManagerData* am) const;
		void renderMoveToAnimation(NVGcontext* vg, AnimationManagerData* am, float t, const Vec3& target);
		void renderFadeInAnimation(NVGcontext* vg, AnimationManagerData* am, float t);
		void renderFadeOutAnimation(NVGcontext* vg, AnimationManagerData* am, float t);
		void takeAttributesFrom(const AnimObject& obj);
		void replacementTransform(AnimationManagerData* am, AnimObjId replacement, float t);

		void updateStatus(AnimationManagerData* am, AnimObjectStatus newStatus);
		void updateStatusSnapshot(SceneSnapshot& snapshot, AnimObjectStatus newStatus);
		void updateChildrenPercentCreated(AnimationManagerData* am, float newPercentCreated);
		void updateChildrenPercentCreatedSnapshot(SceneSnapshot& snapshot, float newPercentCreated);
		void copySvgScaleToChildren(AnimationManagerData* am) const;
		void copyStrokeWidthToChildren(AnimationManagerData* am) const;
		void copyStrokeColorToChildren(AnimationManagerData* am) const;
		void copyFillColorToChildren(AnimationManagerData* am) const;

		AnimObjectBreadthFirstIter beginBreadthFirst(AnimationManagerData* am) const;
		AnimObjectBreadthFirstIterSnapshot beginBreadthFirst(SceneSnapshot& snapshot) const;
		inline AnimObjId end() const { return NULL_ANIM_OBJECT; }
		
		void free();
		void serialize(RawMemory& memory) const;
		static AnimObject deserialize(AnimationManagerData* am, RawMemory& memory, uint32 version);
		static AnimObject createDefaultFromParent(AnimationManagerData* am, AnimObjectTypeV1 type, AnimObjId parentId, bool addChildAsGenerated = false);
		static AnimObject createDefaultFromObj(AnimationManagerData* am, AnimObjectTypeV1 type, const AnimObject& obj);
		static AnimObject createDefault(AnimationManagerData* am, AnimObjectTypeV1 type);
		static AnimObject createCopy(const AnimObject& from);
	};

	// Helpers
	inline bool isNull(const Animation& anim) { return anim.id == NULL_ANIM; }
	inline bool isNull(const AnimObject& animObject) { return animObject.id == NULL_ANIM_OBJECT; }
}

#endif
