#include "animation/Animation.h"
#include "core.h"
#include "core/Application.h"
#include "animation/AnimationManager.h"
#include "animation/Svg.h"
#include "renderer/Renderer.h"
#include "renderer/Fonts.h"
#include "utils/CMath.h"
#include "editor/Gizmos.h"

namespace MathAnim
{
	// ------- Private variables --------
	static int animObjectUidCounter = 0;
	static int animationUidCounter = 0;

	// ----------------------------- Internal Functions -----------------------------
	static AnimObject deserializeAnimObjectV1(AnimationManagerData* am, RawMemory& memory);
	static Animation deserializeAnimationExV1(RawMemory& memory);
	static void applyAnimationToObj(AnimationManagerData* am, NVGcontext* vg, AnimObject* obj, float t, const Animation* animation);

	static void onMoveToGizmo(AnimationManagerData* am, Animation* anim);

	// ----------------------------- Animation Functions -----------------------------
	void Animation::applyAnimation(AnimationManagerData* am, NVGcontext* vg, float t) const
	{
		if (this->shouldDisplayAnimObjects())
		{
			for (int i = 0; i < animObjectIds.size(); i++)
			{
				float startT = 0.0f;
				if (this->playbackType == PlaybackType::LaggedStart)
				{
					startT = (float)i / (float)animObjectIds.size() * lagRatio;
				}

				if (t >= startT)
				{
					float interpolatedT = CMath::mapRange(Vec2{ startT, 1.0f - startT }, Vec2{ 0.0f, 1.0f }, (t - startT));
					interpolatedT = glm::clamp(CMath::ease(interpolatedT, easeType, easeDirection), 0.0f, 1.0f);

					AnimObject* animObject = AnimationManager::getMutableObject(am, animObjectIds[i]);
					if (animObject != nullptr)
					{
						animObject->status = interpolatedT < 1.0f
							? AnimObjectStatus::Animating
							: AnimObjectStatus::Active;
						applyAnimationToObj(am, vg, animObject, interpolatedT, this);
					}
				}
			}
		}
		else
		{
			t = glm::clamp(CMath::ease(t, easeType, easeDirection), 0.0f, 1.0f);
			applyAnimationToObj(am, vg, nullptr, t, this);
		}
	}

	void Animation::onGizmo()
	{
		switch (type)
		{
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Transform:
		case AnimTypeV1::AnimateStrokeColor:
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeWidth:
			break;
		case AnimTypeV1::MoveTo:
			onMoveToGizmo(nullptr, this);
			break;
		case AnimTypeV1::RotateTo:
			// TODO: Render and handle rotate gizmo logic
			break;
		case AnimTypeV1::CameraMoveTo:
			// TODO: Render and handle move to gizmo logic
			break;
		default:
			g_logger_info("Unknown animation: %d", type);
			break;
		}
	}

	bool Animation::shouldDisplayAnimObjects() const
	{
		switch (type)
		{
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::AnimateStrokeColor:
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeWidth:
		case AnimTypeV1::MoveTo:
		case AnimTypeV1::RotateTo:
			return true;
		case AnimTypeV1::CameraMoveTo:
		case AnimTypeV1::Transform:
			return false;
		default:
			g_logger_info("Unknown animation: %d", type);
			break;
		}

		return true;
	}

	void Animation::onGizmo(const AnimObject* obj)
	{
		switch (type)
		{
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Transform:
		case AnimTypeV1::AnimateStrokeColor:
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeWidth:
			break;
		case AnimTypeV1::MoveTo:
			onMoveToGizmo(nullptr, this);
			break;
		case AnimTypeV1::RotateTo:
			// TODO: Render and handle rotate gizmo logic
			break;
		case AnimTypeV1::CameraMoveTo:
			// TODO: Render and handle move to gizmo logic
			break;
		default:
			g_logger_info("Unknown animation: %d", type);
			break;
		}
	}

	void Animation::free()
	{
		// TODO: Place any animation freeing in here
	}

	void Animation::serialize(RawMemory& memory) const
	{
		// type           -> uint32
		// frameStart     -> int32
		// duration       -> int32
		// id             -> int32
		// easeType       -> uint8
		// easeDirection  -> uint8
		// timelineTrack  -> int32
		// playbackType   -> uint8
		// lagRatio       -> f32
		// 
		// numObjects     -> uint32
		// objectIds      -> int32[numObjects]
		uint32 animType = (uint32)this->type;
		memory.write<uint32>(&animType);
		memory.write<int32>(&frameStart);
		memory.write<int32>(&duration);
		memory.write<int32>(&id);
		uint8 easeTypeInt = (uint8)easeType;
		memory.write<uint8>(&easeTypeInt);
		uint8 easeDirectionInt = (uint8)easeDirection;
		memory.write<uint8>(&easeDirectionInt);

		memory.write<int32>(&timelineTrack);
		uint8 playbackTypeInt = (uint8)playbackType;
		memory.write<uint8>(&playbackTypeInt);
		memory.write<float>(&lagRatio);

		uint32 numObjects = (uint32)animObjectIds.size();
		memory.write<uint32>(&numObjects);
		for (uint32 i = 0; i < numObjects; i++)
		{
			memory.write<int32>(&animObjectIds[i]);
		}

		switch (this->type)
		{
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
			// NOP
			break;
		case AnimTypeV1::MoveTo:
		case AnimTypeV1::Shift:
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
		case AnimTypeV1::Transform:
			this->as.replacementTransform.serialize(memory);
			break;
		default:
			g_logger_warning("Unknown animation type: %d", (int)this->type);
			break;
		}
	}

	void ReplacementTransformData::serialize(RawMemory& memory) const
	{
		// srcObjectId -> int32
		// dstObjectId -> int32
		memory.write<int32>(&this->srcAnimObjectId);
		memory.write<int32>(&this->dstAnimObjectId);
	}

	ReplacementTransformData ReplacementTransformData::deserialize(RawMemory& memory)
	{
		// srcObjectId -> int32
		// dstObjectId -> int32
		ReplacementTransformData res;
		memory.read<int32>(&res.srcAnimObjectId);
		memory.read<int32>(&res.dstAnimObjectId);

		return res;
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
		res.animObjectIds.clear();
		res.type = AnimTypeV1::None;
		res.duration = 0;
		res.frameStart = 0;
		return res;
	}

	Animation Animation::createDefault(AnimTypeV1 type, int32 frameStart, int32 duration)
	{
		Animation res;
		res.id = animationUidCounter++;
		g_logger_assert(animationUidCounter < INT32_MAX, "Somehow our UID counter reached 65'536. If this ever happens, re-map all ID's to a lower range since it's likely there's not actually 65'000 animations in the scene.");
		res.frameStart = frameStart;
		res.duration = duration;
		res.animObjectIds.clear();
		res.type = type;
		res.easeType = EaseType::Cubic;
		res.easeDirection = EaseDirection::InOut;
		res.playbackType = PlaybackType::LaggedStart;
		res.lagRatio = 0.1f;

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
			res.as.modifyVec3.target = Vec3{ Application::getViewportSize().x / 3.0f, Application::getViewportSize().y / 3.0f, 0.0f };
			break;
		case AnimTypeV1::RotateTo:
			res.as.modifyVec3.target = Vec3{ 0.0f, 0.0f, 0.0f };
			break;
		case AnimTypeV1::Shift:
			res.as.modifyVec3.target = Vec3{ 0.0f, 1.0f, 0.0f };
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

	void AnimObject::onGizmo(AnimationManagerData* am)
	{
		if (is3D)
		{
			// TODO: Render and handle 3D gizmo logic based on edit mode
		}
		else
		{
			// TODO: Render and handle 2D gizmo logic based on edit mode
			std::string gizmoName = std::string((char*)this->name) + std::to_string(this->id);

			// It's important to render the gizmo at the global location, and then transform
			// the result back to local coordinates since the user is editing with gizmos in global
			// space
			Vec3 globalPositionStart = this->_globalPositionStart;
			if (GizmoManager::translateGizmo(gizmoName.c_str(), &globalPositionStart))
			{
				this->_globalPositionStart = globalPositionStart;

				// Then get local position
				Vec3 localPosition = this->_globalPositionStart;
				if (this->parentId != INT32_MAX)
				{
					const AnimObject* parent = AnimationManager::getObject(am, this->parentId);
					localPosition = this->_globalPositionStart - parent->_globalPositionStart;
				}
				this->_positionStart = localPosition;
			}
		}

		switch (objectType)
		{
		case AnimObjectTypeV1::Square:
		case AnimObjectTypeV1::Circle:
		case AnimObjectTypeV1::SvgObject:
		case AnimObjectTypeV1::Axis:
		case AnimObjectTypeV1::TextObject:
		case AnimObjectTypeV1::LaTexObject:
		case AnimObjectTypeV1::Cube:
			// NOP: No Special logic, but I'll leave this just in case I think
			// of something
			break;
		default:
			g_logger_info("Unknown animation object: %d", objectType);
			break;
		}
	}

	void AnimObject::render(NVGcontext* vg) const
	{
		switch (objectType)
		{
		case AnimObjectTypeV1::Square:
		case AnimObjectTypeV1::Circle:
		case AnimObjectTypeV1::SvgObject:
		{
			// If svgObject is null, don't do anything with it
			static bool warned = false;
			if (this->svgObject == nullptr)
			{
				if (!warned)
				{
					g_logger_warning("Tried to render Square, Circle, SvgObject or some other object that had a nullptr svgObject. Skipping the rendering of this object.\nThis warning will supress itself now.");
					warned = true;
				}
				break;
			}

			// Default SVG objects will just render the svgObject component
			this->svgObject->renderCreateAnimation(vg, this->percentCreated, this);
		}
		break;
		case AnimObjectTypeV1::Cube:
		{
			// if (!this->isAnimating)
			// {
			// 	g_logger_assert(this->svgObject != nullptr, "Cannot render SVG object that is nullptr.");
			// 	this->svgObject->render(vg, this);
			// }
			// float sideLength = this->as.cube.sideLength - 0.01f;
			// Renderer::pushColor(this->fillColor);
			// Renderer::drawFilledCube(this->position, Vec3{sideLength, sideLength, sideLength});
			// Renderer::popColor();
		}
		// NOP: Cube just has a bunch of children anim objects that get rendered
		break;
		case AnimObjectTypeV1::Axis:
			// NOP: Axis just has a bunch of children anim objects that get rendered
			break;
		case AnimObjectTypeV1::TextObject:
			this->as.textObject.renderWriteInAnimation(vg, this->percentCreated, this);
			break;
		case AnimObjectTypeV1::LaTexObject:
			this->as.laTexObject.renderCreateAnimation(vg, this->percentCreated, this, false);
			break;
		default:
			// TODO: Add magic_enum
			// g_logger_info("Unknown animation: '%s'", magic_enum::enum_name(objectType).data());
			g_logger_info("Unknown animation object: %d", objectType);
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

	void AnimObject::takeParentAttributes(const AnimObject* parent)
	{
		this->strokeColor = parent->strokeColor;
		this->strokeWidth = parent->strokeWidth;
		this->drawCurveDebugBoxes = parent->drawCurveDebugBoxes;
		this->drawDebugBoxes = parent->drawDebugBoxes;
		this->fillColor = parent->fillColor;
		this->is3D = parent->is3D;
		this->status = parent->status;
		this->isTransparent = parent->isTransparent;

		this->_fillColorStart = parent->_fillColorStart;
		this->_strokeColorStart = parent->_strokeColorStart;
		this->_strokeWidthStart = parent->_strokeWidthStart;
	}

	static void replacementTransformTextObjs(AnimationManagerData* am, AnimObject* src, AnimObject* replacement, float t);
	void AnimObject::replacementTransform(AnimationManagerData* am, AnimObject* replacement, float t)
	{
		// Update statuses
		AnimObjectStatus thisNewStatus = t < 1.0f
			? AnimObjectStatus::Animating
			: AnimObjectStatus::Inactive;
		AnimObjectStatus replacementNewStatus = t >= 1.0f
			? AnimObjectStatus::Active
			: AnimObjectStatus::Animating;
		this->status = thisNewStatus;
		replacement->status = replacementNewStatus;

		// Interpolate between shared children recursively
		std::vector<int32> thisChildren = AnimationManager::getChildren(am, this);
		std::vector<int32> replacementChildren = AnimationManager::getChildren(am, replacement);

		for (int i = 0; i < thisChildren.size(); i++)
		{
			if (i < replacementChildren.size())
			{
				AnimObject* thisChild = AnimationManager::getMutableObject(am, thisChildren[i]);
				AnimObject* otherChild = AnimationManager::getMutableObject(am, replacementChildren[i]);
				g_logger_assert(thisChild != nullptr, "How did this happen?");
				g_logger_assert(otherChild != nullptr, "How did this happen?");
				thisChild->replacementTransform(am, otherChild, t);
			}
		}

		// Fade in any extra *other* children
		for (int i = thisChildren.size(); i < replacementChildren.size(); i++)
		{
			AnimObject* otherChild = AnimationManager::getMutableObject(am, replacementChildren[i]);
			if (otherChild)
			{
				otherChild->fillColor = CMath::interpolate(t,
					glm::u8vec4(otherChild->fillColor.r, otherChild->fillColor.g, otherChild->fillColor.b, 0),
					otherChild->fillColor
				);
				otherChild->strokeColor = CMath::interpolate(t,
					glm::u8vec4(otherChild->strokeColor.r, otherChild->strokeColor.g, otherChild->strokeColor.b, 0),
					otherChild->strokeColor
				);
				otherChild->percentCreated = 1.0f;
				otherChild->status = replacementNewStatus;

				std::vector<int32> childrensChildren = AnimationManager::getChildren(am, otherChild);
				replacementChildren.insert(replacementChildren.end(), childrensChildren.begin(), childrensChildren.end());
			}
		}

		// Fade out any extra *this* children
		for (int i = replacementChildren.size(); i < thisChildren.size(); i++)
		{
			AnimObject* thisChild = AnimationManager::getMutableObject(am, thisChildren[i]);
			if (thisChild)
			{
				thisChild->fillColor = CMath::interpolate(t,
					thisChild->fillColor,
					glm::u8vec4(thisChild->fillColor.r, thisChild->fillColor.g, thisChild->fillColor.b, 0)
				);
				thisChild->strokeColor = CMath::interpolate(t,
					thisChild->strokeColor,
					glm::u8vec4(thisChild->strokeColor.r, thisChild->strokeColor.g, thisChild->strokeColor.b, 0)
				);
				thisChild->status = thisNewStatus;

				std::vector<int32> childrensChildren = AnimationManager::getChildren(am, thisChild);
				thisChildren.insert(thisChildren.end(), childrensChildren.begin(), childrensChildren.end());
			}
		}

		// Interpolate between this svg and other svg
		if (this->svgObject && replacement->svgObject)
		{
			if (t < 1.0f)
			{
				SvgObject* interpolated = Svg::interpolate(this->svgObject, replacement->svgObject, t);
				this->svgObject->free();
				g_memory_free(this->svgObject);
				this->svgObject = interpolated;

				// Interpolate other properties
				this->position = CMath::interpolate(t, this->position, replacement->position);
				this->rotation = CMath::interpolate(t, this->rotation, replacement->rotation);
				this->scale = CMath::interpolate(t, this->scale, replacement->scale);
				this->fillColor = CMath::interpolate(t, this->fillColor, replacement->fillColor);
				this->strokeColor = CMath::interpolate(t, this->strokeColor, replacement->strokeColor);
				this->strokeWidth = CMath::interpolate(t, this->strokeWidth, replacement->strokeWidth);
				// TODO: Come up with _svgScaleStart so scales can be reset
				// srcObject->svgScale = CMath::interpolate(t, srcObject->svgScale, dstObject->svgScale);
				this->percentCreated = 1.0f;

				// Fade out dstObject
				replacement->fillColor = CMath::interpolate(t,
					replacement->fillColor,
					glm::u8vec4(replacement->fillColor.r, replacement->fillColor.g, replacement->fillColor.b, 0)
				);
				replacement->strokeColor = CMath::interpolate(t,
					replacement->strokeColor,
					glm::u8vec4(replacement->strokeColor.r, replacement->strokeColor.g, replacement->strokeColor.b, 0)
				);
			}
			else
			{
				// Set all children as inactive as well
				replacement->percentCreated = 1.0f;
			}
		}
		else if (this->objectType == AnimObjectTypeV1::TextObject && replacement->objectType == AnimObjectTypeV1::TextObject)
		{
			if (t < 1.0f)
			{
				//SvgObject* interpolated = Svg::interpolate(this->svgObject, replacement->svgObject, t);
				//this->svgObject->free();
				//g_memory_free(this->svgObject);
				//this->svgObject = interpolated;

				//// Interpolate other properties
				//this->position = CMath::interpolate(t, this->position, replacement->position);
				//this->rotation = CMath::interpolate(t, this->rotation, replacement->rotation);
				//this->scale = CMath::interpolate(t, this->scale, replacement->scale);
				//this->fillColor = CMath::interpolate(t, this->fillColor, replacement->fillColor);
				//this->strokeColor = CMath::interpolate(t, this->strokeColor, replacement->strokeColor);
				//this->strokeWidth = CMath::interpolate(t, this->strokeWidth, replacement->strokeWidth);
				//// TODO: Come up with _svgScaleStart so scales can be reset
				//// srcObject->svgScale = CMath::interpolate(t, srcObject->svgScale, dstObject->svgScale);
				//this->percentCreated = 1.0f;

				//// Fade out dstObject
				//replacement->fillColor = CMath::interpolate(t,
				//	replacement->fillColor,
				//	glm::u8vec4(replacement->fillColor.r, replacement->fillColor.g, replacement->fillColor.b, 0)
				//);
				//replacement->strokeColor = CMath::interpolate(t,
				//	replacement->strokeColor,
				//	glm::u8vec4(replacement->strokeColor.r, replacement->strokeColor.g, replacement->strokeColor.b, 0)
				//);
			}
			else
			{
				// Set all children as inactive as well
				replacement->percentCreated = 1.0f;
			}
		}
	}

	static void replacementTransformTextObjs(AnimationManagerData* am, AnimObject* src, AnimObject* replacement, float t)
	{ }

	void AnimObject::updateStatus(AnimationManagerData* am, AnimObjectStatus newStatus)
	{
		this->status = newStatus;

		std::vector<int32> children = AnimationManager::getChildren(am, this);
		for (int i = 0; i < children.size(); i++)
		{
			// Push back all children's children
			AnimObject* child = AnimationManager::getMutableObject(am, children[i]);
			if (child)
			{
				std::vector<int32> childrensChildren = AnimationManager::getChildren(am, child);
				children.insert(children.end(), childrensChildren.begin(), childrensChildren.end());
				child->status = newStatus;
			}
		}
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

		if (this->name)
		{
			g_memory_free(this->name);
			this->name = nullptr;
		}
		this->nameLength = 0;

		switch (this->objectType)
		{
		case AnimObjectTypeV1::Square:
		case AnimObjectTypeV1::Circle:
		case AnimObjectTypeV1::Cube:
		case AnimObjectTypeV1::SvgObject:
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

		this->parentId = INT32_MAX;
		this->id = INT32_MAX;
	}

	void AnimObject::serialize(RawMemory& memory) const
	{
		// AnimObjectType     -> uint32
		// _PositionStart
		//   X                -> f32
		//   Y                -> f32
		//   Z                -> f32
		// RotationStart
		//   X                -> f32
		//   Y                -> f32
		//   Z                -> f32
		// ScaleStart
		//   X                -> f32
		//   Y                -> f32
		//   Z                -> f32
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
		// svgScale           -> f32
		// isTransparent      -> u8
		// is3D               -> u8
		// drawDebugBoxes     -> u8
		// drawCurveDebugBoxes -> u8
		// Id                 -> int32
		// NameLength         -> uint32
		// Name               -> uint8[NameLength]
		// 
		// ParentId           -> int32
		// FrameStart         -> int32
		// Duration           -> int32
		// TimelineTrack      -> int32
		// AnimationTypeSpecificData (This data will change depending on AnimObjectType)
		// NumAnimations  -> uint32
		// Animations     -> Animation[numAnimations]
		// NumChildren    -> uint32
		// Children       -> AnimObject[numChildren]
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
		memory.write<float>(&svgScale);

		uint8 isTransparentU8 = isTransparent ? 1 : 0;
		memory.write<uint8>(&isTransparentU8);

		uint8 is3DU8 = is3D ? 1 : 0;
		memory.write<uint8>(&is3DU8);

		uint8 drawDebugBoxesU8 = drawDebugBoxes ? 1 : 0;
		memory.write<uint8>(&drawDebugBoxesU8);

		uint8 drawCurveDebugBoxesU8 = drawCurveDebugBoxes ? 1 : 0;
		memory.write<uint8>(&drawCurveDebugBoxesU8);

		memory.write<int32>(&id);
		memory.write<int32>(&parentId);
		memory.write<uint32>(&nameLength);
		memory.writeDangerous(name, (nameLength + 1));

		switch (objectType)
		{
		case AnimObjectTypeV1::TextObject:
			this->as.textObject.serialize(memory);
			break;
		case AnimObjectTypeV1::LaTexObject:
			this->as.laTexObject.serialize(memory);
			break;
		case AnimObjectTypeV1::SvgObject:
			g_logger_warning("No serialization for SVG objects yet. Maybe there should be a NOP svg object for objects that don't require serialization.");
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
	}

	AnimObject AnimObject::deserialize(AnimationManagerData* am, RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeAnimObjectV1(am, memory);
		}

		g_logger_error("AnimObject serialized with unknown version '%d'. Potentially corrupted memory.", version);
		AnimObject res = {};
		res.id = -1;
		return res;
	}

	AnimObject AnimObject::createDefaultFromParent(AnimationManagerData* am, AnimObjectTypeV1 type, const AnimObject* parent)
	{
		AnimObject res = createDefault(am, type);
		res.takeParentAttributes(parent);
		return res;
	}

	AnimObject AnimObject::createDefault(AnimationManagerData* am, AnimObjectTypeV1 type)
	{
		AnimObject res;
		res.id = animObjectUidCounter++;
		g_logger_assert(animObjectUidCounter < INT32_MAX, "Somehow our UID counter reached '%d'. If this ever happens, re-map all ID's to a lower range since it's likely there's not actually 2 billion animations in the scene.", INT32_MAX);
		res.parentId = INT32_MAX;

		const char* newObjName = "New Object";
		res.nameLength = std::strlen(newObjName);
		res.name = (uint8*)g_memory_allocate(sizeof(uint8) * (res.nameLength + 1));
		g_memory_copyMem(res.name, (void*)newObjName, sizeof(uint8) * (res.nameLength + 1));

		res.status = AnimObjectStatus::Active;
		res.objectType = type;

		res.rotation = { 0, 0, 0 };
		res._rotationStart = { 0, 0, 0 };

		res.scale = { 1, 1, 1 };
		res._scaleStart = { 1, 1, 1 };

		res.svgObject = nullptr;
		res._svgObjectStart = nullptr;

		res.isTransparent = false;
		res.drawDebugBoxes = false;
		res.drawCurveDebugBoxes = false;
		res.is3D = false;
		res.isGenerated = false;
		res.svgScale = 1.0f;

		res.strokeWidth = 0.0f;
		res._strokeWidthStart = 0.0f;
		res.strokeColor = glm::u8vec4(255);
		res._strokeColorStart = glm::u8vec4(255);
		res.fillColor = glm::u8vec4(255);
		res._fillColorStart = glm::u8vec4(255);

		constexpr float defaultSquareLength = 3.0f;
		constexpr float defaultCircleRadius = 1.5f;
		constexpr float defaultCubeLength = 2.0f;

		glm::vec2 viewportSize = Application::getViewportSize();
		res._positionStart = {
				viewportSize.x / 2.0f,
				viewportSize.y / 2.0f,
				0.0f
		};
		res.position = res._positionStart;

		switch (type)
		{
		case AnimObjectTypeV1::TextObject:
			res.as.textObject = TextObject::createDefault();
			res.as.textObject.init(am, &res);
			break;
		case AnimObjectTypeV1::LaTexObject:
			res.as.laTexObject = LaTexObject::createDefault();
			// TODO: Center on the screen
			break;
		case AnimObjectTypeV1::SvgObject:
			// TODO: Center on the screen
			break;
		case AnimObjectTypeV1::Square:
			res.as.square.sideLength = defaultSquareLength;
			res.svgScale = 50.0f;
			res.as.square.init(&res);
			break;
		case AnimObjectTypeV1::Circle:
			res.as.circle.radius = defaultCircleRadius;
			res.svgScale = 150.0f;
			res.as.circle.init(&res);
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
			res._strokeWidthStart = 7.5f;
			res._fillColorStart.a = 0;
			res.as.axis.fontSizePixels = 9.5f;
			res.as.axis.labelPadding = 25.0f;
			res.as.axis.init(&res);
			break;
		default:
			g_logger_error("Cannot create default animation object of type %d", (int)type);
			break;
		}

		return res;
	}

	// ----------------------------- Internal Functions -----------------------------
	static AnimObject deserializeAnimObjectV1(AnimationManagerData* am, RawMemory& memory)
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
		// svgScale           -> f32
		// isTransparent      -> u8
		// is3D               -> u8
		// drawDebugBoxes     -> u8
		// drawCurveDebugBoxes -> u8
		// Id                 -> int32
		// NameLength         -> uint32
		// Name               -> uint8[NameLength]
		// 
		// ParentId           -> int32
		// FrameStart         -> int32
		// Duration           -> int32
		// TimelineTrack      -> int32
		// TimelineTrack      -> int32
		// AnimObjectTypeDataSize -> uint64
		// AnimObjectTypeSpecificData (This data will change depending on AnimObjectType)
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
		memory.read<float>(&res.svgScale);

		uint8 isTransparent;
		memory.read<uint8>(&isTransparent);
		res.isTransparent = isTransparent != 0;

		uint8 is3D;
		memory.read<uint8>(&is3D);
		res.is3D = is3D != 0;

		uint8 drawDebugBoxes;
		memory.read<uint8>(&drawDebugBoxes);
		res.drawDebugBoxes = drawDebugBoxes != 0;

		uint8 drawCurveDebugBoxes;
		memory.read<uint8>(&drawCurveDebugBoxes);
		res.drawCurveDebugBoxes = drawCurveDebugBoxes != 0;

		memory.read<int32>(&res.id);
		animObjectUidCounter = glm::max(animObjectUidCounter, res.id + 1);

		memory.read<int32>(&res.parentId);
		if (!memory.read<uint32>(&res.nameLength))
		{
			g_logger_assert(false, "Corrupted project data. Irrecoverable.");
		}
		res.name = (uint8*)g_memory_allocate(sizeof(uint8) * (res.nameLength + 1));
		memory.readDangerous(res.name, res.nameLength + 1);

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
			res.as.textObject.init(am, &res);
			break;
		case AnimObjectTypeV1::LaTexObject:
			res.as.laTexObject = LaTexObject::deserialize(memory, version);
			break;
		case AnimObjectTypeV1::Square:
			res.as.square = Square::deserialize(memory, version);
			res.as.square.init(&res);
			break;
		case AnimObjectTypeV1::SvgObject:
			g_logger_warning("No deserialization available for svg objects yet.");
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

		return res;
	}

	Animation deserializeAnimationExV1(RawMemory& memory)
	{
		// type           -> uint32
		// frameStart     -> int32
		// duration       -> int32
		// id             -> int32
		// easeType       -> uint8
		// easeDirection  -> uint8
		// timelineTrack  -> int32
		// playbackType   -> uint8
		// lagRatio       -> f32
		// 
		// numObjects     -> uint32
		// objectIds      -> int32[numObjects]
		// Custom animation data -> dynamic
		Animation res;
		uint32 animType;
		memory.read<uint32>(&animType);
		g_logger_assert(animType < (uint32)AnimTypeV1::Length, "Invalid animation type '%d' from memory. Must be corrupted memory.", animType);
		res.type = (AnimTypeV1)animType;
		memory.read<int32>(&res.frameStart);
		memory.read<int32>(&res.duration);

		memory.read<int32>(&res.id);
		animationUidCounter = glm::max(animationUidCounter, res.id + 1);

		uint8 easeTypeInt, easeDirectionInt;
		memory.read<uint8>(&easeTypeInt);
		memory.read<uint8>(&easeDirectionInt);
		g_logger_assert(easeTypeInt < (uint8)EaseType::Length, "Corrupted memory. Ease type was %d which is out of bounds.", easeTypeInt);
		res.easeType = (EaseType)easeTypeInt;
		g_logger_assert(easeDirectionInt < (uint8)EaseDirection::Length, "Corrupted memory. Ease direction was %d which is out of bounds.", easeDirectionInt);
		res.easeDirection = (EaseDirection)easeDirectionInt;

		memory.read<int32>(&res.timelineTrack);
		uint8 playbackType;
		memory.read<uint8>(&playbackType);
		g_logger_assert(playbackType < (uint8)PlaybackType::Length, "Corrupted memory. PlaybackType was %d which is out of bounds.", playbackType);
		res.playbackType = (PlaybackType)playbackType;
		memory.read<float>(&res.lagRatio);

		uint32 numObjects;
		memory.read<uint32>(&numObjects);
		res.animObjectIds.resize(numObjects, INT32_MAX);
		for (uint32 i = 0; i < numObjects; i++)
		{
			memory.read<int32>(&res.animObjectIds[i]);
		}

		switch (res.type)
		{
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
			// NOP
			break;
		case AnimTypeV1::MoveTo:
		case AnimTypeV1::RotateTo:
		case AnimTypeV1::Shift:
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
		case AnimTypeV1::Transform:
			res.as.replacementTransform = ReplacementTransformData::deserialize(memory);
			break;
		default:
			g_logger_warning("Unhandled animation deserialize for type %d", res.type);
			break;
		}

		return res;
	}

	static void applyAnimationToObj(AnimationManagerData* am, NVGcontext* vg, AnimObject* obj, float t, const Animation* animation)
	{
		// TODO: How to handle this... Certain animations like moveTo
		// should only move the parent because the changes propagate to the
		// children when the transform updates. Other changes like creationPercent and fillColor
		// don't propagate. So should those changes propagate, or should I automatically
		// handle it here?
		// 
		// Apply animation to all children as well
		//if (obj)
		//{
		//	std::vector<int32> children = AnimationManager::getChildren(am, obj);
		//	for (int i = 0; i < children.size(); i++)
		//	{
		//		AnimObject* childObj = AnimationManager::getMutableObject(am, children[i]);
		//		if (childObj)
		//		{
		//			applyAnimationToObj(am, vg, childObj, t, animation);
		//		}
		//	}
		//}

		switch (animation->type)
		{
		case AnimTypeV1::WriteInText:
		case AnimTypeV1::Create:
			obj->percentCreated = t;
			break;
		case AnimTypeV1::Transform:
		{
			AnimObject* srcObject = AnimationManager::getMutableObject(am, animation->as.replacementTransform.srcAnimObjectId);
			AnimObject* dstObject = AnimationManager::getMutableObject(am, animation->as.replacementTransform.dstAnimObjectId);

			if (dstObject && srcObject)
			{
				srcObject->replacementTransform(am, dstObject, t);
			}
		}
		break;
		case AnimTypeV1::UnCreate:
			obj->percentCreated = 1.0f - t;
			break;
		case AnimTypeV1::FadeIn:
		{
			static bool logWarning = true;
			if (logWarning)
			{
				g_logger_warning("TODO: Have an opacity field on objects and fade in to that opacity.");
				logWarning = false;
			}
			obj->fillColor.a = (uint8)(255.0f * t);
			obj->strokeColor.a = (uint8)(255.0f * t);
		}
		break;
		case AnimTypeV1::FadeOut:
			obj->fillColor.a = 255 - (uint8)(255.0f * t);
			obj->strokeColor.a = 255 - (uint8)(255.0f * t);
			break;
		case AnimTypeV1::MoveTo:
		{
			const Vec3& target = animation->as.modifyVec3.target;
			obj->position = Vec3{
				((target.x - obj->position.x) * t) + obj->position.x,
				((target.y - obj->position.y) * t) + obj->position.y,
				((target.z - obj->position.z) * t) + obj->position.z,
			};
		}
		break;
		case AnimTypeV1::Shift:
			obj->position += (animation->as.modifyVec3.target * t);
			break;
		case AnimTypeV1::RotateTo:
			obj->rotation = animation->as.modifyVec3.target;
			break;
		case AnimTypeV1::AnimateFillColor:
			obj->fillColor = animation->as.modifyU8Vec4.target;
			break;
		case AnimTypeV1::AnimateStrokeColor:
			obj->strokeColor = animation->as.modifyU8Vec4.target;
			break;
		case AnimTypeV1::AnimateStrokeWidth:
			g_logger_warning("TODO: Implement me");
			break;
		case AnimTypeV1::CameraMoveTo:
			//Renderer::getMutableOrthoCamera()->position = animation->as.modifyVec2.target;
			g_logger_warning("TODO: Implement me");
			break;
		default:
			// TODO: Add magic_enum
			// g_logger_info("Unknown animation: '%s'", magic_enum::enum_name(type).data());
			g_logger_info("Unknown animation: %d", animation->type);
			break;
		}
	}

	static void onMoveToGizmo(AnimationManagerData* am, Animation* anim)
	{
		// TODO: Render and handle 2D gizmo logic based on edit mode
		std::string gizmoName = "Move_To_" + std::to_string(anim->id);
		GizmoManager::translateGizmo(gizmoName.c_str(), &anim->as.modifyVec3.target, GizmoVariant::Free);
	}
}