#ifndef MATH_ANIM_ANIMATION_H
#define MATH_ANIM_ANIMATION_H
#include "core.h"
#include "utils/CMath.h"

#include "animation/TextAnimations.h"
#include "animation/SvgFileObject.h"
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
		SvgFileObject,
		Camera,
		ScriptObject,
		CodeBlock,
		Arrow,
		Length
	};

	constexpr auto _animationObjectTypeNames = fixedSizeArray<const char*, (size_t)AnimObjectTypeV1::Length>(
		"None",
		"Text Object",
		"LaTex Object",
		"Square",
		"Circle",
		"Cube",
		"Axis",
		"SVG Object",
		"SVG File Object",
		"Camera",
		"Script Object",
		"Code Block",
		"Arrow"
	);

	constexpr auto _isInternalObjectOnly = fixedSizeArray<bool, (size_t)AnimObjectTypeV1::Length>(
		false, // "None",
		false, // "Text Object",
		false, // "LaTex Object",
		false, // "Square",
		false, // "Circle",
		false, // "Cube",
		false, // "Axis",
		true,  // "SVG Object",
		false, // "SVG File Object",
		false, // "Camera"
		false, // "Script Object"
		false, // "Code Block"
		false  // "Arrow"
	);

	enum class AnimTypeV1 : uint32
	{
		None = 0,
		MoveTo,
		Create,
		UnCreate,
		FadeIn,
		FadeOut,
		Transform,
		RotateTo,
		AnimateStrokeColor,
		AnimateFillColor,
		AnimateStrokeWidth,
		Shift,
		Circumscribe,
		AnimateScale,
		Length
	};

	constexpr auto _animationTypeNames = fixedSizeArray<const char*, (size_t)AnimTypeV1::Length>(
		"None",
		"Move To",
		"Create",
		"Un-Create",
		"Fade In",
		"Fade Out",
		"Replacement Transform",
		"Rotate To",
		"Animate Stroke Color",
		"Animate Fill Color",
		"Animate Stroke Width",
		"Shift",
		"Circumscribe",
		"Animate Scale"
	);

	enum class PlaybackType : uint8
	{
		None = 0,
		Synchronous,
		LaggedStart,
		Length
	};

	constexpr auto _playbackTypeNames = fixedSizeArray<const char*, (size_t)PlaybackType::Length>(
		"None",
		"Synchronous",
		"Lagged Start"
	);

	constexpr auto _appliesToChildrenData = fixedSizeArray<bool, (size_t)AnimTypeV1::Length>(
		false, // None = 0,
		false, // MoveTo,
		true,  // Create,
		true,  // UnCreate,
		true,  // FadeIn,
		true,  // FadeOut,
		false, // Transform,
		false, // RotateTo,
		false, // AnimateStrokeColor,
		false, // AnimateFillColor,
		false, // AnimateStrokeWidth,
		false, // Shift,
		false, // Circumscribe
		false  // AnimateScale
	);

	constexpr auto _isAnimationGroupData = fixedSizeArray<bool, (size_t)AnimTypeV1::Length>(
		false, // None = 0,
		false, // MoveTo,
		true,  // Create,
		true,  // UnCreate,
		true,  // FadeIn,
		true,  // FadeOut,
		false, // Transform,
		false, // RotateTo,
		false, // AnimateStrokeColor,
		false, // AnimateFillColor,
		false, // AnimateStrokeWidth,
		true,  // Shift,
		false, // Circumscribe
		false  // AnimateScale
	);

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

	struct MoveToData
	{
		Vec2 source;
		Vec2 target;
		AnimObjId object;

		void serialize(RawMemory& memory) const;
		static MoveToData deserialize(RawMemory& memory);
	};

	struct AnimateScaleData
	{
		Vec2 source;
		Vec2 target;
		AnimObjId object;

		void serialize(RawMemory& memory) const;
		static AnimateScaleData deserialize(RawMemory& memory);
	};

	enum class CircumscribeShape : uint8
	{
		Rectangle = 0,
		Circle,
		Length
	};

	constexpr auto _circumscribeShapeNames = fixedSizeArray<const char*, (size_t)CircumscribeShape::Length>(
		"Rectangle",
		"Circle"
	);

	enum class CircumscribeFade : uint8
	{
		FadeInOut = 0,
		FadeIn,
		FadeOut,
		FadeNone,
		Length
	};

	constexpr auto _circumscribeFadeNames = fixedSizeArray<const char*, (size_t)CircumscribeFade::Length>(
		"Fade In-Out",
		"Fade In",
		"Fade Out",
		"No Fade"
	);

	struct Circumscribe
	{
		Vec4 color;
		CircumscribeShape shape;
		CircumscribeFade fade;
		float bufferSize;
		AnimObjId obj;
		float timeWidth;
		float tValue;

		void render(const Vec3& position, const BBox& bbox) const;
		void serialize(RawMemory& memory) const;
		static Circumscribe deserialize(RawMemory& memory);
		static Circumscribe createDefault();
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
		std::unordered_set<AnimObjId> animObjectIds;

		union
		{
			ModifyVec4AnimData modifyVec4;
			ModifyVec3AnimData modifyVec3;
			ModifyVec2AnimData modifyVec2;
			ModifyU8Vec4AnimData modifyU8Vec4;
			ReplacementTransformData replacementTransform;
			MoveToData moveTo;
			Circumscribe circumscribe;
			AnimateScaleData animateScale;
		} as;

		// Apply the animation state using a interpolation t value
		// 
		//   t: is a float that ranges from [0, 1] where 0 is the
		//      beginning of the animation and 1 is the end of the
		//      animation
		void applyAnimation(AnimationManagerData* am, float t = 1.0f) const;
		void applyAnimationToObj(AnimationManagerData* am, AnimObjId animObj, float t = 1.0f) const;
		void calculateKeyframes(AnimationManagerData* am);
		void calculateKeyframesForObj(AnimationManagerData* am, AnimObjId animObj);

		// Render the gizmo with relation to this object
		void onGizmo(const AnimObject* obj);
		// Render the gizmo for this animation with no relation to it's child objects
		void onGizmo();

		void free();
		void serialize(RawMemory& memory) const;
		static Animation deserialize(RawMemory& memory, uint32 version);
		static Animation createDefault(AnimTypeV1 type, int32 frameStart, int32 duration);

		static inline bool appliesToChildren(AnimTypeV1 type) { g_logger_assert((size_t)type < (size_t)AnimTypeV1::Length, "Type name out of bounds."); return _appliesToChildrenData[(size_t)type]; }
		static inline bool isAnimationGroup(AnimTypeV1 type) { g_logger_assert((size_t)type < (size_t)AnimTypeV1::Length, "Type name out of bounds."); return _isAnimationGroupData[(size_t)type]; }
		static inline const char* getAnimationName(AnimTypeV1 type) { g_logger_assert((size_t)type < (size_t)AnimTypeV1::Length, "Type name out of bounds."); return _animationTypeNames[(size_t)type]; }
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

	struct CameraObject
	{
		OrthoCamera camera2D;
		bool is2D;
		bool isActiveCamera;

		void serialize(RawMemory& memory) const;
		void free();

		static CameraObject deserialize(RawMemory& memory, uint32 version);
		static CameraObject createDefault();
	};

	struct ScriptObject
	{
		char* scriptFilepath;
		size_t scriptFilepathLength;

		void serialize(RawMemory& memory) const;
		void free();

		static ScriptObject deserialize(RawMemory& memory, uint32 version);
		static ScriptObject createDefault();
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
		float percentReplacementTransformed;

		// TODO: This is an ugly hack think of a better way for this stuff
		AnimId circumscribeId;

		// Transform stuff
		// TODO: Consider moving this to a Transform class
		// This is the combined parent+child positions and transformations
		Vec3 _globalPositionStart;
		Vec3 globalPosition;
		glm::mat4 globalTransform;

		AnimObjId id;
		AnimObjId parentId;
		std::vector<AnimObjId> generatedChildrenIds;
		std::unordered_set<AnimId> referencedAnimations;

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
			SvgFileObject svgFile;
			CameraObject camera;
			ScriptObject script;
			CodeBlock codeBlock;
			Arrow arrow;
		} as;

		void setName(const char* newName, size_t newNameLength = 0);
		void onGizmo(AnimationManagerData* am);
		void render(AnimationManagerData* am) const;
		void renderMoveToAnimation(AnimationManagerData* am, float t, const Vec3& target);
		void renderFadeInAnimation(AnimationManagerData* am, float t);
		void renderFadeOutAnimation(AnimationManagerData* am, float t);
		void takeAttributesFrom(const AnimObject& obj);
		void replacementTransform(AnimationManagerData* am, AnimObjId replacement, float t);

		void resetAllState();
		void updateStatus(AnimationManagerData* am, AnimObjectStatus newStatus);
		void updateChildrenPercentCreated(AnimationManagerData* am, float newPercentCreated);
		void copySvgScaleToChildren(AnimationManagerData* am) const;
		void copyStrokeWidthToChildren(AnimationManagerData* am) const;
		void copyStrokeColorToChildren(AnimationManagerData* am) const;
		void copyFillColorToChildren(AnimationManagerData* am) const;

		AnimObjectBreadthFirstIter beginBreadthFirst(AnimationManagerData* am) const;
		inline AnimObjId end() const { return NULL_ANIM_OBJECT; }
		
		void free();
		void serialize(RawMemory& memory) const;
		static AnimObject deserialize(AnimationManagerData* am, RawMemory& memory, uint32 version);
		static AnimObject createDefaultFromParent(AnimationManagerData* am, AnimObjectTypeV1 type, AnimObjId parentId, bool addChildAsGenerated = false);
		static AnimObject createDefaultFromObj(AnimationManagerData* am, AnimObjectTypeV1 type, const AnimObject& obj);
		static AnimObject createDefault(AnimationManagerData* am, AnimObjectTypeV1 type);
		static AnimObject createCopy(const AnimObject& from);

		static inline bool isInternalObjectOnly(AnimObjectTypeV1 type) { g_logger_assert((size_t)type < (size_t)AnimObjectTypeV1::Length, "Name out of bounds."); return _isInternalObjectOnly[(size_t)type]; }
		static inline const char* getAnimObjectName(AnimObjectTypeV1 type) { g_logger_assert((size_t)type < (size_t)AnimObjectTypeV1::Length, "Name out of bounds."); return _animationObjectTypeNames[(size_t)type]; }
	};

	// Helpers
	inline bool isNull(const Animation& anim) { return anim.id == NULL_ANIM; }
	inline bool isNull(const AnimObject& animObject) { return animObject.id == NULL_ANIM_OBJECT; }
}

#endif
