#include "animation/Animation.h"
#include "core.h"
#include "animation/AnimationManager.h"
#include "animation/Svg.h"
#include "renderer/Renderer.h"
#include "renderer/Fonts.h"
#include "utils/CMath.h"
#include "core/Application.h"

namespace MathAnim
{
	// ------- Private variables --------
	static int animObjectUidCounter = 0;
	static int animationUidCounter = 0;

	// ----------------------------- Internal Functions -----------------------------
	static AnimObject deserializeAnimObjectV1(RawMemory& memory);
	static Animation deserializeAnimationExV1(RawMemory& memory);

	// ----------------------------- Animation Functions -----------------------------
	void Animation::render(NVGcontext* vg, float t) const
	{
		t = CMath::ease(t, this->easeType, this->easeDirection);

		switch (type)
		{
		case AnimTypeV1::Create:
			g_logger_assert(getParent()->svgObject != nullptr, "Cannot render create animation for SVG object that is nullptr.");
			getParent()->svgObject->renderCreateAnimation(vg, t, getParent());
			if (getParent()->svgObject->is3D)
			{
				// Only fade in in for t = [0.8-1.0] 
				if (t >= 0.8f)
				{
					getMutableParent()->renderFadeInAnimation(vg, (t - 0.8f) / 0.2f);
				}
			}
			break;
		case AnimTypeV1::UnCreate:
			g_logger_assert(getParent()->svgObject != nullptr, "Cannot render un-create animation for SVG object that is nullptr.");
			getParent()->svgObject->renderCreateAnimation(vg, t, getParent(), true);
			if (getParent()->svgObject->is3D)
			{
				// Only fade out for t = [0.0-0.2]
				if (t < 0.2f)
				{
					getMutableParent()->renderFadeOutAnimation(vg, t / 0.2f);
				}
			}
			break;
		case AnimTypeV1::FadeIn:
			getMutableParent()->renderFadeInAnimation(vg, t);
			break;
		case AnimTypeV1::FadeOut:
			getMutableParent()->renderFadeOutAnimation(vg, t);
			break;
		case AnimTypeV1::WriteInText:
			getParent()->as.textObject.renderWriteInAnimation(vg, t, getParent());
			break;
		case AnimTypeV1::MoveTo:
			getMutableParent()->renderMoveToAnimation(vg, t, this->as.modifyVec3.target);
			break;
		case AnimTypeV1::Transform:
		{
			const SvgObject* obj1 = getParent()->svgObject;
			const AnimObject* nextObj = AnimationManager::getNextAnimObject(getParent()->id);
			if (obj1 != nullptr && nextObj != nullptr)
			{
				const SvgObject* obj2 = nextObj->svgObject;
				if (obj2 != nullptr)
				{
					Svg::renderInterpolation(vg, getParent(), obj1, nextObj, obj2, t);
				}
				else
				{
					g_logger_warning("One or more null svg objects in transform animation.");
				}
			}
			else
			{
				g_logger_warning("One or more null svg objects in transform animation.");
			}
		}
		break;
		case AnimTypeV1::RotateTo:
			getMutableParent()->rotation = CMath::interpolate(t, getParent()->rotation, as.modifyVec3.target);
			getParent()->render(vg);
			break;
		case AnimTypeV1::AnimateStrokeColor:
			getMutableParent()->strokeColor = CMath::interpolate(t, getParent()->strokeColor, as.modifyU8Vec4.target);
			getParent()->render(vg);
			break;
		case AnimTypeV1::AnimateFillColor:
			getMutableParent()->fillColor = CMath::interpolate(t, getParent()->fillColor, as.modifyU8Vec4.target);
			getParent()->render(vg);
			break;
		case AnimTypeV1::AnimateStrokeWidth:
			g_logger_warning("TODO: Implement me.");
			break;
		case AnimTypeV1::CameraMoveTo:
			Renderer::getMutableOrthoCamera()->position =
				CMath::interpolate(t, Renderer::getOrthoCamera()->position, as.modifyVec2.target);
			// We also need to call render here, I know it's not the most intuitive haha
			getParent()->render(vg);
			break;
		default:
			// TODO: Add magic_enum
			// g_logger_info("Unknown animation: '%s'", magic_enum::enum_name(type).data());
			g_logger_info("Unknown animation: %d", type);
			break;
		}
	}

	void Animation::applyAnimation(NVGcontext* vg) const
	{
		switch (type)
		{
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Create:
		case AnimTypeV1::Transform:
		case AnimTypeV1::UnCreate:
			// NOP
			break;
		case AnimTypeV1::FadeIn:
			g_logger_warning("TODO: Have an opacity field on objects and fade in to that opacity.");
			getMutableParent()->fillColor.a = 255;
			getMutableParent()->strokeColor.a = 255;
			break;
		case AnimTypeV1::FadeOut:
			getMutableParent()->fillColor.a = 0;
			getMutableParent()->strokeColor.a = 0;
			break;
		case AnimTypeV1::MoveTo:
			getMutableParent()->position = this->as.modifyVec3.target;
			break;
		case AnimTypeV1::RotateTo:
			getMutableParent()->rotation = this->as.modifyVec3.target;
			break;
		case AnimTypeV1::AnimateFillColor:
			getMutableParent()->fillColor = this->as.modifyU8Vec4.target;
			break;
		case AnimTypeV1::AnimateStrokeColor:
			getMutableParent()->strokeColor = this->as.modifyU8Vec4.target;
			break;
		case AnimTypeV1::AnimateStrokeWidth:
			g_logger_warning("TODO: Implement me");
			break;
		case AnimTypeV1::CameraMoveTo:
			Renderer::getMutableOrthoCamera()->position = this->as.modifyVec2.target;
			break;
		default:
			// TODO: Add magic_enum
			// g_logger_info("Unknown animation: '%s'", magic_enum::enum_name(type).data());
			g_logger_info("Unknown animation: %d", type);
			break;
		}
	}

	const AnimObject* Animation::getParent() const
	{
		const AnimObject* res = AnimationManager::getObject(objectId);
		g_logger_assert(res != nullptr, "Invalid anim object.");
		return res;
	}

	AnimObject* Animation::getMutableParent() const
	{
		AnimObject* res = AnimationManager::getMutableObject(objectId);
		g_logger_assert(res != nullptr, "Invalid anim object.");
		return res;
	}

	void Animation::free()
	{
		// TODO: Place any animation freeing in here
	}

	void Animation::serialize(RawMemory& memory) const
	{
		// type           -> uint32
		// objectId       -> int32
		// frameStart     -> int32
		// duration       -> int32
		// id             -> int32
		// easeType       -> uint8
		// easeDirection  -> uint8
		uint32 animType = (uint32)this->type;
		memory.write<uint32>(&animType);
		memory.write<int32>(&objectId);
		memory.write<int32>(&frameStart);
		memory.write<int32>(&duration);
		memory.write<int32>(&id);
		uint8 easeTypeInt = (uint8)easeType;
		memory.write<uint8>(&easeTypeInt);
		uint8 easeDirectionInt = (uint8)easeDirection;
		memory.write<uint8>(&easeDirectionInt);

		switch (this->type)
		{
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Create:
		case AnimTypeV1::Transform:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
			// NOP
			break;
		case AnimTypeV1::MoveTo:
		case AnimTypeV1::RotateTo:
			CMath::serialize(memory, this->as.modifyVec3.target);
			break;
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeColor:
			CMath::serialize(memory, this->as.modifyU8Vec4.target);
			break;
		case AnimTypeV1::AnimateStrokeWidth:
			g_logger_warning("TODO: implement me");
			break;
		case AnimTypeV1::CameraMoveTo:
			CMath::serialize(memory, this->as.modifyVec2.target);
			break;
		default:
			g_logger_warning("Unknown animation type: %d", (int)this->type);
			break;
		}
	}

	Animation Animation::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeAnimationExV1(memory);
		}

		g_logger_error("AnimationEx serialized with unknown version '%d'. Memory potentially corrupted.", version);
		Animation res;
		res.id = -1;
		res.objectId = -1;
		res.type = AnimTypeV1::None;
		res.duration = 0;
		res.frameStart = 0;
		return res;
	}

	Animation Animation::createDefault(AnimTypeV1 type, int32 frameStart, int32 duration, int32 animObjectId)
	{
		Animation res;
		res.id = animationUidCounter++;
		res.frameStart = frameStart;
		res.duration = duration;
		res.objectId = animObjectId;
		res.type = type;
		res.easeType = EaseType::Cubic;
		res.easeDirection = EaseDirection::InOut;

		switch (type)
		{
		case AnimTypeV1::Create:
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Transform:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
			// NOP
			break;
		case AnimTypeV1::MoveTo:
		case AnimTypeV1::RotateTo:
			res.as.modifyVec3.target = Vec3{ 0.0f, 0.0f, 0.0f };
			break;
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeColor:
			res.as.modifyU8Vec4.target = glm::u8vec4(0);
			break;
		case AnimTypeV1::AnimateStrokeWidth:
			g_logger_warning("TODO: Implement me");
			break;
		case AnimTypeV1::CameraMoveTo:
			res.as.modifyVec2.target = Vec2{ 0.f, 0.f };
			break;
		default:
			g_logger_error("Cannot create default animation of type %d", (int)type);
			break;
		}

		return res;
	}

	void AnimObject::render(NVGcontext* vg) const
	{
		switch (objectType)
		{
		case AnimObjectTypeV1::Square:
		case AnimObjectTypeV1::Circle:
			// Default SVG objects will just render the svgObject component
			g_logger_assert(this->svgObject != nullptr, "Cannot render SVG object that is nullptr.");
			this->svgObject->render(vg, this);
			break;
		case AnimObjectTypeV1::Cube:
		{
			if (!this->isAnimating)
			{
				g_logger_assert(this->svgObject != nullptr, "Cannot render SVG object that is nullptr.");
				this->svgObject->render(vg, this);
			}
			float sideLength = this->as.cube.sideLength - 0.01f;
			Renderer::pushColor(this->fillColor);
			Renderer::drawFilledCube(this->position, Vec3{sideLength, sideLength, sideLength});
			Renderer::popColor();
		}
		break;
		case AnimObjectTypeV1::Axis:
			g_logger_assert(this->svgObject != nullptr, "Cannot render axis with null SVG object.");
			this->svgObject->render(vg, this);
			// TODO: Render the numbers
			break;
		case AnimObjectTypeV1::TextObject:
			this->as.textObject.render(vg, this);
			break;
		default:
			// TODO: Add magic_enum
			// g_logger_info("Unknown animation: '%s'", magic_enum::enum_name(objectType).data());
			g_logger_info("Unknown animation: %d", objectType);
			break;
		}
	}

	// ----------------------------- AnimObject Functions -----------------------------
	void AnimObject::renderMoveToAnimation(NVGcontext* vg, float t, const Vec3& target)
	{
		Vec3 pos = Vec3{
			((target.x - position.x) * t) + position.x,
			((target.y - position.y) * t) + position.y,
			((target.z - position.z) * t) + position.z,
		};
		this->position = pos;
		this->render(vg);
	}

	void AnimObject::renderFadeInAnimation(NVGcontext* vg, float t)
	{
		this->fillColor.a = this->fillColor.a * t;
		this->strokeColor.a = this->strokeColor.a * t;
		this->render(vg);
	}

	void AnimObject::renderFadeOutAnimation(NVGcontext* vg, float t)
	{
		this->fillColor.a = this->fillColor.a * (1.0f - t);
		this->strokeColor.a = this->strokeColor.a * (1.0f - t);
		this->render(vg);
	}

	void AnimObject::free()
	{
		if (this->svgObject)
		{
			this->svgObject->free();
			g_memory_free(this->svgObject);
			this->svgObject = nullptr;
		}

		if (this->_svgObjectStart)
		{
			this->_svgObjectStart->free();
			g_memory_free(this->_svgObjectStart);
			this->_svgObjectStart = nullptr;
		}

		switch (this->objectType)
		{
		case AnimObjectTypeV1::Square:
		case AnimObjectTypeV1::Circle:
		case AnimObjectTypeV1::Cube:
			// NOP
			break;
		case AnimObjectTypeV1::Axis:
			// NOP
			// TODO: Free any text objecty stuff with the numbers
			break;
		case AnimObjectTypeV1::TextObject:
			this->as.textObject.free();
			break;
		case AnimObjectTypeV1::LaTexObject:
			this->as.laTexObject.free();
			break;
		default:
			g_logger_error("Cannot free unknown animation object of type %d", (int)objectType);
			break;
		}
	}

	void AnimObject::serialize(RawMemory& memory) const
	{
		//   AnimObjectType     -> uint32
		//   _PositionStart
		//     X                -> f32
		//     Y                -> f32
		//     Z                -> f32
		//  _RotationStart
		//     X                -> f32
		//     Y                -> f32
		//     Z                -> f32
		//  _ScaleStart
		//     X                -> f32
		//     Y                -> f32
		//     Z                -> f32
		//   _FillColorStart
		//     R                -> u8
		//     G                -> u8
		//     B                -> u8
		//     A                -> u8
		//   _StrokeColorStart 
		//     R                -> u8
		//     G                -> u8
		//     B                -> u8
		//     A                -> u8
		//   _StrokeWidthStart  -> f32
		//   Id                 -> int32
		//   FrameStart         -> int32
		//   Duration           -> int32
		//   TimelineTrack      -> int32
		//   AnimationTypeSpecificData (This data will change depending on AnimObjectType)
		uint32 animObjectType = (uint32)objectType;
		memory.write<uint32>(&animObjectType);
		memory.write<float>(&_positionStart.x);
		memory.write<float>(&_positionStart.y);
		memory.write<float>(&_positionStart.z);

		memory.write<float>(&_rotationStart.x);
		memory.write<float>(&_rotationStart.y);
		memory.write<float>(&_rotationStart.z);

		memory.write<float>(&_scaleStart.x);
		memory.write<float>(&_scaleStart.y);
		memory.write<float>(&_scaleStart.z);

		memory.write<uint8>(&_fillColorStart.r);
		memory.write<uint8>(&_fillColorStart.g);
		memory.write<uint8>(&_fillColorStart.b);
		memory.write<uint8>(&_fillColorStart.a);

		memory.write<uint8>(&_strokeColorStart.r);
		memory.write<uint8>(&_strokeColorStart.g);
		memory.write<uint8>(&_strokeColorStart.b);
		memory.write<uint8>(&_strokeColorStart.a);

		memory.write<float>(&_strokeWidthStart);

		memory.write<int32>(&id);
		memory.write<int32>(&frameStart);
		memory.write<int32>(&duration);
		memory.write<int32>(&timelineTrack);

		switch (objectType)
		{
		case AnimObjectTypeV1::TextObject:
			this->as.textObject.serialize(memory);
			break;
		case AnimObjectTypeV1::LaTexObject:
			this->as.laTexObject.serialize(memory);
			break;
		case AnimObjectTypeV1::Square:
			this->as.square.serialize(memory);
			break;
		case AnimObjectTypeV1::Circle:
			this->as.circle.serialize(memory);
			break;
		case AnimObjectTypeV1::Cube:
			this->as.cube.serialize(memory);
			break;
		case AnimObjectTypeV1::Axis:
			this->as.axis.serialize(memory);
			break;
		default:
			g_logger_warning("Unknown object type %d when serializing.", (int)objectType);
			break;
		}

		// NumAnimations  -> uint32
		// Animations     -> dynamic
		uint32 numAnimations = (uint32)this->animations.size();
		memory.write<uint32>(&numAnimations);
		for (uint32 i = 0; i < numAnimations; i++)
		{
			animations[i].serialize(memory);
		}
	}

	AnimObject AnimObject::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeAnimObjectV1(memory);
		}

		g_logger_error("AnimObject serialized with unknown version '%d'. Potentially corrupted memory.", version);
		AnimObject res;
		res.animations = {};
		res.frameStart = 0;
		res.duration = 0;
		res.id = -1;
		res.objectType = AnimObjectTypeV1::None;
		res.position = {};
		res._positionStart = {};
		res.timelineTrack = -1;
		return res;
	}

	AnimObject AnimObject::createDefault(AnimObjectTypeV1 type, int32 frameStart, int32 duration)
	{
		AnimObject res;
		res.id = animObjectUidCounter++;
		res.animations = {};
		res.frameStart = frameStart;
		res.duration = duration;
		res.isAnimating = false;
		res.objectType = type;

		res.position = { 0, 0, 0 };
		res._positionStart = { 0, 0, 0 };

		res.rotation = { 0, 0, 0 };
		res._rotationStart = { 0, 0, 0 };

		res.scale = { 1, 1, 1 };
		res._scaleStart = { 1, 1, 1 };

		res.svgObject = nullptr;
		res._svgObjectStart = nullptr;

		res.strokeWidth = 0.0f;
		res._strokeWidthStart = 0.0f;
		res.strokeColor = glm::u8vec4(255);
		res._strokeColorStart = glm::u8vec4(255);
		res.fillColor = glm::u8vec4(255);
		res._fillColorStart = glm::u8vec4(255);

		constexpr float defaultSquareLength = 400.0f;
		constexpr float defaultCircleRadius = 200.0f;
		constexpr float defaultCubeLength = 2.0f;
		glm::vec2 outputSize = Application::getOutputSize();

		switch (type)
		{
		case AnimObjectTypeV1::TextObject:
			res.as.textObject = TextObject::createDefault();
			// TODO: Center on the screen
			break;
		case AnimObjectTypeV1::LaTexObject:
			res.as.laTexObject = LaTexObject::createDefault();
			// TODO: Center on the screen
			break;
		case AnimObjectTypeV1::Square:
			res.as.square.sideLength = defaultSquareLength;
			res.as.square.init(&res);
			res._positionStart = {
				outputSize.x / 2.0f,
				outputSize.y / 2.0f
			};
			break;
		case AnimObjectTypeV1::Circle:
			res.as.circle.radius = defaultCircleRadius;
			res.as.circle.is3D = false;
			res.as.circle.init(&res);
			res._positionStart = {
				outputSize.x / 2.0f,
				outputSize.y / 2.0f
			};
			break;
		case AnimObjectTypeV1::Cube:
			res.as.cube.sideLength = defaultCubeLength;
			res.as.cube.init(&res);
			break;
		case AnimObjectTypeV1::Axis:
			res.as.axis.axesLength = Vec3{ 3'000.0f, 1'700.0f, 1.0f };
			res.as.axis.xRange = { 0, 18 };
			res.as.axis.yRange = { 0, 10 };
			res.as.axis.zRange = { 0, 10 };
			res.as.axis.xStep = 1.0f;
			res.as.axis.yStep = 1.0f;
			res.as.axis.zStep = 1.0f;
			res.as.axis.tickWidth = 75.0f;
			res.as.axis.drawNumbers = true;
			res.as.axis.is3D = false;
			res._strokeWidthStart = 7.5f;
			res._fillColorStart.a = 0;
			res._positionStart = {
				outputSize.x / 2.0f,
				outputSize.y / 2.0f
			};
			res.as.axis.init(&res);
			break;
		default:
			g_logger_error("Cannot create default animation object of type %d", (int)type);
			break;
		}

		return res;
	}

	// ----------------------------- Internal Functions -----------------------------
	static AnimObject deserializeAnimObjectV1(RawMemory& memory)
	{
		AnimObject res;

		// AnimObjectType     -> uint32
		// _PositionStart
		//   X                -> f32
		//   Y                -> f32
		//   Z                -> f32
		// _RotationStart
		//   X				  -> f32
		//   Y				  -> f32
		//   Z                -> f32
		//  _ScaleStart
		//     X                -> f32
		//     Y                -> f32
		//     Z                -> f32
		// _FillColorStart
		//   R                -> u8
		//   G                -> u8
		//   B                -> u8
		//   A                -> u8
		// _StrokeColorStart 
		//   R                -> u8
		//   G                -> u8
		//   B                -> u8
		//   A                -> u8
		// _StrokeWidthStart  -> f32
		// Id                 -> int32
		// FrameStart         -> int32
		// Duration           -> int32
		// TimelineTrack      -> int32
		// TimelineTrack      -> int32
		// AnimationTypeDataSize -> uint64
		// AnimationTypeSpecificData (This data will change depending on AnimObjectType)
		uint32 animObjectType;
		memory.read<uint32>(&animObjectType);
		g_logger_assert(animObjectType < (uint32)AnimObjectTypeV1::Length, "Invalid AnimObjectType '%d' from memory. Must be corrupted memory.", animObjectType);
		res.objectType = (AnimObjectTypeV1)animObjectType;
		memory.read<float>(&res._positionStart.x);
		memory.read<float>(&res._positionStart.y);
		memory.read<float>(&res._positionStart.z);

		memory.read<float>(&res._rotationStart.x);
		memory.read<float>(&res._rotationStart.y);
		memory.read<float>(&res._rotationStart.z);

		memory.read<float>(&res._scaleStart.x);
		memory.read<float>(&res._scaleStart.y);
		memory.read<float>(&res._scaleStart.z);

		memory.read<uint8>(&res._fillColorStart.r);
		memory.read<uint8>(&res._fillColorStart.g);
		memory.read<uint8>(&res._fillColorStart.b);
		memory.read<uint8>(&res._fillColorStart.a);

		memory.read<uint8>(&res._strokeColorStart.r);
		memory.read<uint8>(&res._strokeColorStart.g);
		memory.read<uint8>(&res._strokeColorStart.b);
		memory.read<uint8>(&res._strokeColorStart.a);

		memory.read<float>(&res._strokeWidthStart);

		memory.read<int32>(&res.id);
		memory.read<int32>(&res.frameStart);
		memory.read<int32>(&res.duration);
		memory.read<int32>(&res.timelineTrack);
		animObjectUidCounter = glm::max(animObjectUidCounter, res.id + 1);

		res.position = res._positionStart;
		res.strokeColor = res._strokeColorStart;
		res.fillColor = res._fillColorStart;
		res.strokeWidth = res._strokeWidthStart;
		res.svgObject = nullptr;
		res._svgObjectStart = nullptr;

		// We're in V1 so this is version 1
		constexpr uint32 version = 1;
		switch (res.objectType)
		{
		case AnimObjectTypeV1::TextObject:
			res.as.textObject = TextObject::deserialize(memory, version);
			break;
		case AnimObjectTypeV1::LaTexObject:
			res.as.laTexObject = LaTexObject::deserialize(memory, version);
			break;
		case AnimObjectTypeV1::Square:
			res.as.square = Square::deserialize(memory, version);
			res.as.square.init(&res);
			break;
		case AnimObjectTypeV1::Circle:
			res.as.circle = Circle::deserialize(memory, version);
			res.as.circle.init(&res);
			break;
		case AnimObjectTypeV1::Cube:
			res.as.cube = Cube::deserialize(memory, version);
			res.as.cube.init(&res);
			break;
		case AnimObjectTypeV1::Axis:
			res.as.axis = Axis::deserialize(memory, version);
			res.as.axis.init(&res);
			break;
		default:
			g_logger_error("Unknown anim object type: %d. Corrupted memory.", res.objectType);
			break;
		}

		// NumAnimations  -> uint32
		// Animations     -> dynamic
		uint32 numAnimations;
		memory.read<uint32>(&numAnimations);
		for (uint32 i = 0; i < numAnimations; i++)
		{
			Animation animation = Animation::deserialize(memory, SERIALIZER_VERSION);
			animationUidCounter = glm::max(animationUidCounter, animation.id + 1);
			res.animations.push_back(animation);
		}

		return res;
	}

	Animation deserializeAnimationExV1(RawMemory& memory)
	{
		// type           -> uint32
		// objectId       -> int32
		// frameStart     -> int32
		// duration       -> int32
		// id             -> int32
		// easeType       -> uint8
		// easeDirection  -> uint8
		// Custom animation data -> dynamic
		Animation res;
		uint32 animType;
		memory.read<uint32>(&animType);
		g_logger_assert(animType < (uint32)AnimTypeV1::Length, "Invalid animation type '%d' from memory. Must be corrupted memory.", animType);
		res.type = (AnimTypeV1)animType;
		memory.read<int32>(&res.objectId);
		memory.read<int32>(&res.frameStart);
		memory.read<int32>(&res.duration);
		memory.read<int32>(&res.id);
		uint8 easeTypeInt, easeDirectionInt;
		res.easeType = EaseType::Cubic;
		res.easeDirection = EaseDirection::InOut;
		memory.read<uint8>(&easeTypeInt);
		memory.read<uint8>(&easeDirectionInt);
		g_logger_assert(easeTypeInt < (uint8)EaseType::Length, "Corrupted memory. Ease type was %d which is out of bounds.", easeTypeInt);
		res.easeType = (EaseType)easeTypeInt;
		g_logger_assert(easeDirectionInt < (uint8)EaseDirection::Length, "Corrupted memory. Ease direction was %d which is out of bounds", easeDirectionInt);
		res.easeDirection = (EaseDirection)easeDirectionInt;

		switch (res.type)
		{
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Transform:
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
			// NOP
			break;
		case AnimTypeV1::MoveTo:
		case AnimTypeV1::RotateTo:
			// TODO: How do I really feel about this?
			// It's not explicit what the data structure looks like
			// but since it's just a vec3 I don't think I care
			res.as.modifyVec3.target = CMath::deserializeVec3(memory);
			break;
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeColor:
			res.as.modifyU8Vec4.target = CMath::deserializeU8Vec4(memory);
			break;
		case AnimTypeV1::AnimateStrokeWidth:
			g_logger_warning("TODO: implement me");
			break;
		case AnimTypeV1::CameraMoveTo:
			res.as.modifyVec2.target = CMath::deserializeVec2(memory);
			break;
		default:
			g_logger_warning("Unhandled animation deserialize for type %d", res.type);
			break;
		}

		return res;
	}
}