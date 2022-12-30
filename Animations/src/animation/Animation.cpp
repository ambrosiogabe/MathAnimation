#include "animation/Animation.h"
#include "core.h"
#include "core/Application.h"
#include "core/Colors.h"
#include "animation/AnimationManager.h"
#include "svg/Svg.h"
#include "svg/SvgCache.h"
#include "svg/Paths.h"
#include "renderer/Renderer.h"
#include "renderer/Fonts.h"
#include "utils/CMath.h"
#include "editor/Gizmos.h"

namespace MathAnim
{
	// ------- Private variables --------
	static AnimObjId animObjectUidCounter = 0;
	static AnimObjId animationUidCounter = 0;

	// ----------------------------- Internal Functions -----------------------------
	static AnimObject deserializeAnimObjectV1(AnimationManagerData* am, RawMemory& memory);
	static Animation deserializeAnimationExV1(RawMemory& memory);
	static CameraObject deserializeCameraObjectV1(RawMemory& memory);

	static void onMoveToGizmo(AnimationManagerData* am, Animation* anim);

	// ----------------------------- Animation Functions -----------------------------
	void Animation::applyAnimation(AnimationManagerData* am, float t) const
	{
		if (Animation::appliesToChildren(type))
		{
			int i = 0;
			for (auto animObjId : animObjectIds)
			{
				float startT = 0.0f;
				if (this->playbackType == PlaybackType::LaggedStart)
				{
					startT = (float)i / (float)animObjectIds.size() * lagRatio;
				}

				if (t >= startT)
				{
					float interpolatedT = CMath::mapRange(Vec2{ 0.0f, 1.0f - startT }, Vec2{ 0.0f, 1.0f }, (t - startT));
					interpolatedT = glm::clamp(CMath::ease(interpolatedT, easeType, easeDirection), 0.0f, 1.0f);

					applyAnimationToObj(am, animObjId, interpolatedT);
				}

				i++;
			}
		}
		else
		{
			float interpolatedT = glm::clamp(CMath::ease(t, easeType, easeDirection), 0.0f, 1.0f);
			applyAnimationToObj(am, NULL_ANIM_OBJECT, interpolatedT);
		}
	}

	void Animation::applyAnimationToObj(AnimationManagerData* am, AnimObjId animObjId, float t) const
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, animObjId);

		// Apply animation to all children as well
		if (obj && Animation::appliesToChildren(this->type))
		{
			AnimObjectStatus newStatus = t < 1.0f
				? AnimObjectStatus::Animating
				: AnimObjectStatus::Active;
			obj->status = newStatus;

			std::vector<AnimObjId> children = AnimationManager::getChildren(am, obj->id);
			for (int i = 0; i < children.size(); i++)
			{
				// TODO: This is duplicating the lagged start logic above
				// group this together into one function that determines
				// a new t-value depending on lag position
				float startT = 0.0f;
				if (this->playbackType == PlaybackType::LaggedStart)
				{
					startT = (float)i / (float)children.size() * lagRatio;
				}

				float interpolatedT = CMath::mapRange(Vec2{ 0.0f, 1.0f - startT }, Vec2{ 0.0f, 1.0f }, (t - startT));
				applyAnimationToObj(am, children[i], interpolatedT);
			}
		}

		switch (type)
		{
		case AnimTypeV1::Create:
		{
			if (obj)
			{
				obj->percentCreated = t;
				// Start the fade in after 80% of the drawing is complete
				constexpr float fadeInStart = 0.8f;
				float amountToFadeIn = ((t - fadeInStart) / (1.0f - fadeInStart));
				float percentToFadeIn = glm::max(glm::min(amountToFadeIn, 1.0f), 0.0f);
				obj->fillColor.a = (uint8)(percentToFadeIn * (float)obj->fillColor.a);

				if (obj->strokeWidth <= 0.0f)
				{
					obj->strokeColor.a = (uint8)((1.0f - percentToFadeIn) * 255.0f);
				}
			}
		}
		break;
		case AnimTypeV1::Transform:
		{
			AnimObject* srcObject = AnimationManager::getMutableObject(am, this->as.replacementTransform.srcAnimObjectId);
			AnimObject* dstObject = AnimationManager::getMutableObject(am, this->as.replacementTransform.dstAnimObjectId);

			if (dstObject && srcObject)
			{
				srcObject->replacementTransform(am, dstObject->id, t);
			}
		}
		break;
		case AnimTypeV1::UnCreate:
		{
			if (obj)
			{
				obj->percentCreated = 1.0f - t;
				obj->fillColor.a = (uint8)((1.0f - t) * 255.0f);
			}
		}
		break;
		case AnimTypeV1::FadeIn:
		{
			if (obj)
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
		}
		break;
		case AnimTypeV1::FadeOut:
		{
			if (obj)
			{
				obj->fillColor.a = obj->fillColor.a - (uint8)((float)obj->fillColor.a * t);
				obj->strokeColor.a = obj->strokeColor.a - (uint8)((float)obj->strokeColor.a * t);
			}
		}
		break;
		case AnimTypeV1::MoveTo:
		{
			AnimObject* moveToObj = AnimationManager::getMutableObject(am, this->as.moveTo.object);
			if (moveToObj)
			{
				const Vec2& target = this->as.moveTo.target;
				const Vec2& source = this->as.moveTo.source;
				moveToObj->position = Vec3{
					((target.x - source.x) * t) + source.x,
					((target.y - source.y) * t) + source.y,
					moveToObj->position.z
				};
			}
		}
		break;
		case AnimTypeV1::AnimateScale:
		{
			AnimObject* animateScaleObj = AnimationManager::getMutableObject(am, this->as.animateScale.object);
			if (animateScaleObj)
			{
				const Vec2& target = this->as.animateScale.target;
				const Vec2& source = this->as.animateScale.source;
				animateScaleObj->scale = Vec3{
					((target.x - source.x) * t) + source.x,
					((target.y - source.y) * t) + source.y,
					animateScaleObj->scale.z
				};
			}
		}
		break;
		case AnimTypeV1::Shift:
		{
			if (obj)
			{
				obj->position += (this->as.modifyVec3.target * t);
			}
		}
		break;
		case AnimTypeV1::RotateTo:
		{
			if (obj)
			{
				obj->rotation = this->as.modifyVec3.target;
			}
		}
		break;
		case AnimTypeV1::AnimateFillColor:
		{
			if (obj)
			{
				obj->fillColor = this->as.modifyU8Vec4.target;
			}
		}
		break;
		case AnimTypeV1::AnimateStrokeColor:
		{
			if (obj)
			{
				obj->strokeColor = this->as.modifyU8Vec4.target;
			}
		}
		break;
		case AnimTypeV1::AnimateStrokeWidth:
		{
			g_logger_warning("TODO: Implement me");
		}
		break;
		case AnimTypeV1::Circumscribe:
		{
			// TODO: This is an ugly hack, find a better way to do this
			AnimObject* objToCircumscribe = AnimationManager::getMutableObject(am, this->as.circumscribe.obj);
			if (objToCircumscribe)
			{
				objToCircumscribe->circumscribeId = this->id;
				// TODO: Super super gross... Definitely fix this
				((Animation*)this)->as.circumscribe.tValue = t;
			}

			if (objToCircumscribe && (t <= 0.0f || t >= 1.0f))
			{
				objToCircumscribe->circumscribeId = NULL_ANIM;
			}
		}
		break;
		case AnimTypeV1::Length:
		case AnimTypeV1::None:
			break;
		}
	}

	void Animation::calculateKeyframes(AnimationManagerData* am)
	{
		if (Animation::appliesToChildren(type))
		{
			for (auto animObjId : animObjectIds)
			{
				calculateKeyframesForObj(am, animObjId);
			}
		}
		else
		{
			calculateKeyframesForObj(am, NULL_ANIM_OBJECT);
		}
	}

	void Animation::calculateKeyframesForObj(AnimationManagerData* am, AnimObjId animObjId)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, animObjId);

		// Apply animation to all children as well
		if (obj && Animation::appliesToChildren(this->type))
		{
			std::vector<AnimObjId> children = AnimationManager::getChildren(am, obj->id);
			for (int i = 0; i < children.size(); i++)
			{
				calculateKeyframesForObj(am, children[i]);
			}
		}

		switch (type)
		{
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::Circumscribe:
			// NOP;
			break;
		case AnimTypeV1::Transform:
		{
			// TODO: Implement me
		}
		break;
		case AnimTypeV1::FadeIn:
		{
			// TODO: Implement me
		}
		break;
		case AnimTypeV1::FadeOut:
		{
			// TODO: Implement me
		}
		break;
		case AnimTypeV1::MoveTo:
		{
			AnimObject* moveToObj = AnimationManager::getMutableObject(am, this->as.moveTo.object);
			if (moveToObj)
			{
				this->as.moveTo.source = CMath::vector2From3(moveToObj->position);
			}
		}
		break;
		case AnimTypeV1::AnimateScale:
		{
			AnimObject* animateScaleObj = AnimationManager::getMutableObject(am, this->as.animateScale.object);
			if (animateScaleObj)
			{
				this->as.animateScale.source = CMath::vector2From3(animateScaleObj->scale);
			}
		}
		break;
		case AnimTypeV1::Shift:
		{
			// TODO: Implement me
		}
		break;
		case AnimTypeV1::RotateTo:
		{
			// TODO: Implement me
		}
		break;
		case AnimTypeV1::AnimateFillColor:
		{
			// TODO: Implement me
		}
		break;
		case AnimTypeV1::AnimateStrokeColor:
		{
			// TODO: Implement me
		}
		break;
		case AnimTypeV1::AnimateStrokeWidth:
		{
			// TODO: Implement me
		}
		break;
		case AnimTypeV1::Length:
		case AnimTypeV1::None:
			break;
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
		case AnimTypeV1::Transform:
		case AnimTypeV1::AnimateStrokeColor:
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeWidth:
		case AnimTypeV1::Circumscribe:
			break;
		case AnimTypeV1::MoveTo:
			onMoveToGizmo(nullptr, this);
			break;
		case AnimTypeV1::AnimateScale:
			// TODO: Add scale gizmo here
			break;
		case AnimTypeV1::RotateTo:
			// TODO: Render and handle rotate gizmo logic
			break;
		case AnimTypeV1::Shift:
			// TODO: What should happen here?
			break;
		case AnimTypeV1::Length:
		case AnimTypeV1::None:
			break;
		}
	}

	void Animation::onGizmo(const AnimObject*)
	{
		switch (type)
		{
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
		case AnimTypeV1::Transform:
		case AnimTypeV1::AnimateStrokeColor:
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeWidth:
		case AnimTypeV1::Circumscribe:
			break;
		case AnimTypeV1::MoveTo:
			onMoveToGizmo(nullptr, this);
			break;
		case AnimTypeV1::AnimateScale:
			// TODO: Animate scale gizmo here
			break;
		case AnimTypeV1::RotateTo:
			// TODO: Render and handle rotate gizmo logic
			break;
		case AnimTypeV1::Shift:
			// TODO: What should we do here?
			break;
		case AnimTypeV1::Length:
		case AnimTypeV1::None:
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
		// objectIds      -> AnimObjId[numObjects]
		uint32 animType = (uint32)this->type;
		memory.write<uint32>(&animType);
		memory.write<int32>(&frameStart);
		memory.write<int32>(&duration);
		memory.write<AnimId>(&id);
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
		for (auto animObjId : animObjectIds)
		{
			memory.write<AnimObjId>(&animObjId);
		}

		switch (this->type)
		{
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
			// NOP
			break;
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
		case AnimTypeV1::Transform:
			this->as.replacementTransform.serialize(memory);
			break;
		case AnimTypeV1::MoveTo:
			this->as.moveTo.serialize(memory);
			break;
		case AnimTypeV1::AnimateScale:
			this->as.animateScale.serialize(memory);
			break;
		case AnimTypeV1::Circumscribe:
			this->as.circumscribe.serialize(memory);
			break;
		case AnimTypeV1::Length:
		case AnimTypeV1::None:
			break;
		}
	}

	void ReplacementTransformData::serialize(RawMemory& memory) const
	{
		// srcObjectId -> AnimObjId
		// dstObjectId -> AnimObjId
		memory.write<AnimObjId>(&this->srcAnimObjectId);
		memory.write<AnimObjId>(&this->dstAnimObjectId);
	}

	ReplacementTransformData ReplacementTransformData::deserialize(RawMemory& memory)
	{
		// srcObjectId -> AnimObjId
		// dstObjectId -> AnimObjId
		ReplacementTransformData res;
		memory.read<AnimObjId>(&res.srcAnimObjectId);
		memory.read<AnimObjId>(&res.dstAnimObjectId);

		return res;
	}

	void MoveToData::serialize(RawMemory& memory) const
	{
		// source -> Vec2
		// target -> Vec2
		// object -> AnimObjId
		CMath::serialize(memory, this->source);
		CMath::serialize(memory, this->target);
		memory.write<AnimObjId>(&this->object);
	}

	MoveToData MoveToData::deserialize(RawMemory& memory)
	{
		// source -> Vec2
		// target -> Vec2
		// object -> AnimObjId
		MoveToData res;
		res.source = CMath::deserializeVec2(memory);
		res.target = CMath::deserializeVec2(memory);
		memory.read<AnimObjId>(&res.object);
		return res;
	}

	void AnimateScaleData::serialize(RawMemory& memory) const
	{
		// source -> Vec2
		// target -> Vec2
		// object -> AnimObjId
		CMath::serialize(memory, this->source);
		CMath::serialize(memory, this->target);
		memory.write<AnimObjId>(&this->object);
	}

	AnimateScaleData AnimateScaleData::deserialize(RawMemory& memory)
	{
		// source -> Vec2
		// target -> Vec2
		// object -> AnimObjId
		AnimateScaleData res;
		res.source = CMath::deserializeVec2(memory);
		res.target = CMath::deserializeVec2(memory);
		memory.read<AnimObjId>(&res.object);
		return res;
	}

	void Circumscribe::render(const Vec3& objectPosition, const BBox& bbox) const
	{
		Vec2 size = (bbox.max - bbox.min) + ((bbox.max - bbox.min) * bufferSize);
		float radius = CMath::length(size) / 2.0f;
		Vec2 translation = CMath::vector2From3(objectPosition);
		glm::mat4 transformation = glm::identity<glm::mat4>();
		transformation = glm::translate(transformation, glm::vec3(translation.x, translation.y, 0.0f));

		if (fade != CircumscribeFade::FadeNone)
		{
			if (tValue < 0.5f)
			{
				if (fade == CircumscribeFade::FadeIn || fade == CircumscribeFade::FadeInOut)
				{
					float fadeAmount = glm::clamp(this->color.a * tValue * 4.0f, 0.0f, 1.0f);
					Renderer::pushColor(Vec4{ this->color.r, this->color.g, this->color.b, this->color.a * fadeAmount });

					Path2DContext* path = shape == CircumscribeShape::Rectangle
						? Paths::createRectangle(size, transformation)
						: Paths::createCircle(radius, transformation);
					Renderer::endPath(path, true);
					Renderer::free(path);

					Renderer::popColor();
				}
				else
				{
					// Do draw in animation
					Renderer::pushColor(this->color);

					Path2DContext* path = shape == CircumscribeShape::Rectangle
						? Paths::createRectangle(size, transformation)
						: Paths::createCircle(radius, transformation);
					Renderer::renderOutline(path, 0.0f, tValue * 2.0f, true);
					Renderer::free(path);

					Renderer::popColor();
				}
			}
			else
			{
				if (fade == CircumscribeFade::FadeOut || fade == CircumscribeFade::FadeInOut)
				{
					float fadeAmount = glm::clamp(this->color.a * (4.0f + (-tValue * 4.0f)), 0.0f, 1.0f);
					Renderer::pushColor(Vec4{ this->color.r, this->color.g, this->color.b, this->color.a * fadeAmount });

					Path2DContext* path = shape == CircumscribeShape::Rectangle
						? Paths::createRectangle(size, transformation)
						: Paths::createCircle(radius, transformation);
					Renderer::endPath(path, true);
					Renderer::free(path);

					Renderer::popColor();
				}
				else
				{
					// Do draw out animation
					Renderer::pushColor(this->color);
					Path2DContext* path = shape == CircumscribeShape::Rectangle
						? Paths::createRectangle(size, transformation)
						: Paths::createCircle(radius, transformation);

					// Range from t = [1.0-0.0]
					float t = (2.0f + (-tValue * 2.0f));
					Renderer::renderOutline(path, 0.0f, t, true);

					Renderer::free(path);
					Renderer::popColor();
				}
			}
		}
		else
		{
			// Do the lagged animation where you get a trailing line
			Renderer::pushColor(Vec4{ this->color.r, this->color.g, this->color.b, this->color.a });
			Path2DContext* path = shape == CircumscribeShape::Rectangle
				? Paths::createRectangle(size, transformation)
				: Paths::createCircle(radius, transformation);

			// T ranges from [0.0-1.0]
			float t = tValue;
			// Then map t to a range from [0.0-(1.0 + timeWidth)]
			t = CMath::mapRange(Vec2{ 0.0f, 1.0f }, Vec2{ 0.0f, 1.0f + timeWidth }, t);
			float tStart = t - timeWidth;
			float tEnd = t;

			Renderer::renderOutline(path, tStart, tEnd, false);

			Renderer::free(path);
			Renderer::popColor();
		}
	}

	void Circumscribe::serialize(RawMemory& memory) const
	{
		// color          -> Vec4
		// shape          -> u8
		// fade           -> u8
		// bufferSize     -> f32
		// timeWidth      -> f32
		// obj            -> AnimObjId
		CMath::serialize(memory, this->color);
		memory.write<CircumscribeShape>(&this->shape);
		memory.write<CircumscribeFade>(&this->fade);
		memory.write<float>(&this->bufferSize);
		memory.write<float>(&this->timeWidth);
		memory.write<AnimObjId>(&this->obj);
	}

	Circumscribe Circumscribe::deserialize(RawMemory& memory)
	{
		// color          -> Vec4
		// shape          -> u8
		// fade           -> u8
		// bufferSize     -> f32
		// timeWidth      -> f32
		// obj            -> AnimObjId
		Circumscribe res;
		res.color = CMath::deserializeVec4(memory);
		memory.read<CircumscribeShape>(&res.shape);
		memory.read<CircumscribeFade>(&res.fade);
		memory.read<float>(&res.bufferSize);
		memory.read<float>(&res.timeWidth);
		memory.read<AnimObjId>(&res.obj);

		return res;
	}

	Circumscribe Circumscribe::createDefault()
	{
		Circumscribe res;
		res.color = "#F9DB1BFF"_hex;
		res.shape = CircumscribeShape::Rectangle;
		res.fade = CircumscribeFade::FadeNone;
		res.bufferSize = 0.25f;
		res.timeWidth = 0.1f;
		res.obj = NULL_ANIM_OBJECT;

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
		res.id = NULL_ANIM;
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
		case AnimTypeV1::Transform:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
			// NOP
			break;
		case AnimTypeV1::MoveTo:
			res.as.moveTo.source = Vec2{ 0, 0 };
			res.as.moveTo.target = Vec2{ Application::getViewportSize().x / 3.0f, Application::getViewportSize().y / 3.0f };
			break;
		case AnimTypeV1::AnimateScale:
			res.as.animateScale.source = Vec2{ 1.0f, 1.0f };
			res.as.animateScale.target = Vec2{ 0.5f, 0.5f };
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
		case AnimTypeV1::Circumscribe:
			res.as.circumscribe = Circumscribe::createDefault();
			break;
		case AnimTypeV1::Length:
		case AnimTypeV1::None:
			break;
		}

		return res;
	}

	void CameraObject::serialize(RawMemory& memory) const
	{
		//OrthoCamera camera2D;
		//bool is2D;
		//bool isActiveCamera;
		camera2D.serialize(memory);
		uint8 is2DU8 = is2D ? 1 : 0;
		memory.write<uint8>(&is2DU8);
	}

	void CameraObject::free()
	{

	}

	CameraObject CameraObject::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeCameraObjectV1(memory);
		}

		CameraObject res = {};
		return res;
	}

	CameraObject CameraObject::createDefault()
	{
		CameraObject res = {};
		res.camera2D.projectionSize = Vec2{ 9.0f, 18.0f };
		res.camera2D.position = res.camera2D.projectionSize / 2.0f;
		res.camera2D.zoom = 1.0f;
		res.is2D = true;
		res.isActiveCamera = true;
		return res;
	}

	void ScriptObject::serialize(RawMemory& memory) const
	{
		// ScriptFilepathLength    -> u32
		// ScriptFilepath          -> u8[ScriptFilepathLength]
		uint32 scriptFilepathLengthU32 = (uint32)scriptFilepathLength;
		memory.write<uint32>(&scriptFilepathLengthU32);
		memory.writeDangerous((const uint8*)scriptFilepath, sizeof(uint8) * scriptFilepathLength);
	}

	void ScriptObject::free()
	{
		if (scriptFilepath)
		{
			g_memory_free((void*)scriptFilepath);
		}

		scriptFilepath = nullptr;
		scriptFilepathLength = 0;
	}

	ScriptObject ScriptObject::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			ScriptObject res;

			// ScriptFilepathLength    -> u32
			// ScriptFilepath          -> u8[ScriptFilepathLength]
			uint32 scriptFilepathLengthU32;
			memory.read<uint32>(&scriptFilepathLengthU32);
			res.scriptFilepathLength = (size_t)scriptFilepathLengthU32;
			res.scriptFilepath = (char*)g_memory_allocate(sizeof(uint8) * (res.scriptFilepathLength + 1));
			memory.readDangerous((uint8*)res.scriptFilepath, sizeof(uint8) * res.scriptFilepathLength);
			res.scriptFilepath[res.scriptFilepathLength] = '\0';
			return res;
		}

		static const ScriptObject dummy = { nullptr, 0 };
		return dummy;
	}

	ScriptObject ScriptObject::createDefault()
	{
		ScriptObject res;
		res.scriptFilepath = (char*)g_memory_allocate(sizeof(char) * 1);
		res.scriptFilepathLength = 0;

		return res;
	}

	void AnimObject::setName(const char* newName, size_t newNameLength)
	{
		if (newNameLength == 0)
		{
			newNameLength = std::strlen(newName);
		}

		name = (uint8*)g_memory_realloc(name, sizeof(char) * (newNameLength + 1));
		nameLength = (int32_t)newNameLength;
		g_memory_copyMem(name, (void*)newName, newNameLength * sizeof(char));
		name[newNameLength] = '\0';
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
				if (!isNull(this->parentId))
				{
					const AnimObject* parent = AnimationManager::getObject(am, this->parentId);
					localPosition = this->_globalPositionStart - parent->_globalPositionStart;
				}
				this->_positionStart = localPosition;
				AnimationManager::updateObjectState(am, this->id);
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
		case AnimObjectTypeV1::SvgFileObject:
		case AnimObjectTypeV1::ScriptObject:
		case AnimObjectTypeV1::CodeBlock:
		case AnimObjectTypeV1::Arrow:
			// NOP: No Special logic, but I'll leave this just in case I think
			// of something
			break;
		case AnimObjectTypeV1::Camera:
		{
			if (this->as.camera.is2D)
			{
				this->as.camera.camera2D.position = CMath::vector2From3(this->_globalPositionStart);
			}
			else
			{
				// TODO: Implement 3D camera logic stuff...
			}
		}
		break;
		case AnimObjectTypeV1::Length:
		case AnimObjectTypeV1::None:
			break;
		}
	}

	void AnimObject::render(AnimationManagerData* am) const
	{
		// TODO: This is gross fixme
		const Animation* circumscribeAnim = AnimationManager::getAnimation(am, this->circumscribeId);
		if (circumscribeAnim)
		{
			if (this->svgObject && circumscribeAnim->type == AnimTypeV1::Circumscribe)
			{
				BBox bbox = this->svgObject->bbox;
				glm::vec3 scaleFactor, skew, translation;
				glm::quat orientation;
				glm::vec4 perspective;
				glm::decompose(this->globalTransform, scaleFactor, orientation, translation, skew, perspective);

				bbox.min.x *= scaleFactor.x;
				bbox.max.x *= scaleFactor.x;
				bbox.min.y *= scaleFactor.y;
				bbox.max.y *= scaleFactor.y;
				circumscribeAnim->as.circumscribe.render(Vec3{ translation.x, translation.y, translation.z }, bbox);
			}
		}

		switch (objectType)
		{
		case AnimObjectTypeV1::Square:
		case AnimObjectTypeV1::Circle:
		case AnimObjectTypeV1::SvgObject:
		case AnimObjectTypeV1::Arrow:
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
			Application::getSvgCache()->render(am, this->svgObject, this->id);
			if (this->strokeWidth > 0.0f || this->percentCreated < 1.0f)
			{
				// Render outline
				this->svgObject->renderOutline(this->percentCreated, this);
			}
		}
		break;
		case AnimObjectTypeV1::Cube:
		{
			// if (!this->isAnimating)
			// {
			// 	g_logger_assert(this->svgObject != nullptr, "Cannot render SVG object that is nullptr.");
			// 	this->svgObject->render(this);
			// }
			// float sideLength = this->as.cube.sideLength - 0.01f;
			// Renderer::pushColor(this->fillColor);
			// Renderer::drawFilledCube(this->position, Vec3{sideLength, sideLength, sideLength});
			// Renderer::popColor();
		}
		// NOP: Cube just has a bunch of children anim objects that get rendered
		break;
		case AnimObjectTypeV1::Axis:
		case AnimObjectTypeV1::TextObject:
		case AnimObjectTypeV1::LaTexObject:
		case AnimObjectTypeV1::SvgFileObject:
		case AnimObjectTypeV1::Camera:
		case AnimObjectTypeV1::ScriptObject:
		case AnimObjectTypeV1::CodeBlock:
			// NOP: These just have a bunch of children anim objects that get rendered
			break;
		case AnimObjectTypeV1::Length:
		case AnimObjectTypeV1::None:
			break;
		}
	}

	// ----------------------------- Iterators -----------------------------
	AnimObjectBreadthFirstIter::AnimObjectBreadthFirstIter(AnimationManagerData* am, AnimObjId parentId)
	{
		this->am = am;
		std::vector<AnimObjId> children = AnimationManager::getChildren(am, parentId);
		if (children.size() > 0)
		{
			childrenLeft = std::deque<AnimObjId>(children.begin(), children.end());
			currentId = childrenLeft.front();
			childrenLeft.pop_front();
		}
		else
		{
			currentId = NULL_ANIM_OBJECT;
		}
	}

	void AnimObjectBreadthFirstIter::operator++()
	{
		if (childrenLeft.size() > 0)
		{
			// Push back all children's children
			AnimObjId childId = childrenLeft.front();
			childrenLeft.pop_front();
			AnimObject* child = AnimationManager::getMutableObject(am, childId);
			if (child)
			{
				std::vector<AnimObjId> childrensChildren = AnimationManager::getChildren(am, child->id);
				childrenLeft.insert(childrenLeft.end(), childrensChildren.begin(), childrensChildren.end());
			}

			currentId = childId;
		}
		else
		{
			currentId = NULL_ANIM_OBJECT;
		}
	}

	// ----------------------------- AnimObject Functions -----------------------------
	void AnimObject::renderMoveToAnimation(AnimationManagerData* am, float t, const Vec3& target)
	{
		Vec3 pos = Vec3{
			((target.x - position.x) * t) + position.x,
			((target.y - position.y) * t) + position.y,
			((target.z - position.z) * t) + position.z,
		};
		this->position = pos;
		this->render(am);
	}

	void AnimObject::renderFadeInAnimation(AnimationManagerData* am, float t)
	{
		this->fillColor.a = (uint8)((float)this->fillColor.a * t);
		this->strokeColor.a = (uint8)((float)this->strokeColor.a * t);
		this->render(am);
	}

	void AnimObject::renderFadeOutAnimation(AnimationManagerData* am, float t)
	{
		this->fillColor.a = (uint8)((float)this->fillColor.a * (1.0f - t));
		this->strokeColor.a = (uint8)((float)this->strokeColor.a * (1.0f - t));
		this->render(am);
	}

	void AnimObject::takeAttributesFrom(const AnimObject& obj)
	{
		this->strokeColor = obj.strokeColor;
		this->strokeWidth = obj.strokeWidth;
		this->drawCurveDebugBoxes = obj.drawCurveDebugBoxes;
		this->drawDebugBoxes = obj.drawDebugBoxes;
		this->fillColor = obj.fillColor;
		this->is3D = obj.is3D;
		this->status = obj.status;
		this->isTransparent = obj.isTransparent;
		this->svgScale = obj.svgScale;
		this->percentCreated = obj.percentCreated;

		this->_fillColorStart = obj._fillColorStart;
		this->_strokeColorStart = obj._strokeColorStart;
		this->_strokeWidthStart = obj._strokeWidthStart;
	}

	void AnimObject::replacementTransform(AnimationManagerData* am, AnimObjId replacementId, float t)
	{
		AnimObject* replacement = AnimationManager::getMutableObject(am, replacementId);
		if (!replacement)
		{
			return;
		}

		// Update statuses
		AnimObjectStatus thisNewStatus = t < 1.0f
			? AnimObjectStatus::Animating
			: AnimObjectStatus::Inactive;
		AnimObjectStatus replacementNewStatus = t >= 1.0f
			? AnimObjectStatus::Active
			: replacement->percentCreated > 0.0f 
			? AnimObjectStatus::Animating
			: AnimObjectStatus::Inactive;
		this->status = thisNewStatus;
		replacement->status = replacementNewStatus;

		// Interpolate between shared children recursively
		std::vector<AnimObjId> thisChildren = AnimationManager::getChildren(am, this->id);
		std::vector<AnimObjId> replacementChildren = AnimationManager::getChildren(am, replacement->id);

		for (int i = 0; i < thisChildren.size(); i++)
		{
			if (i < replacementChildren.size())
			{
				AnimObject* thisChild = AnimationManager::getMutableObject(am, thisChildren[i]);
				AnimObject* otherChild = AnimationManager::getMutableObject(am, replacementChildren[i]);
				g_logger_assert(thisChild != nullptr, "How did this happen?");
				g_logger_assert(otherChild != nullptr, "How did this happen?");
				thisChild->replacementTransform(am, otherChild->id, t);
			}
		}

		// Fade in any extra *other* children
		for (size_t i = thisChildren.size(); i < replacementChildren.size(); i++)
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

				std::vector<AnimObjId> childrensChildren = AnimationManager::getChildren(am, otherChild->id);
				replacementChildren.insert(replacementChildren.end(), childrensChildren.begin(), childrensChildren.end());
			}
		}

		// Fade out any extra *this* children
		for (size_t i = replacementChildren.size(); i < thisChildren.size(); i++)
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

				std::vector<AnimObjId> childrensChildren = AnimationManager::getChildren(am, thisChild->id);
				thisChildren.insert(thisChildren.end(), childrensChildren.begin(), childrensChildren.end());
			}
		}

		if (t < 1.0f)
		{
			// Interpolate between this svg and other svg
			if (this->svgObject && replacement->svgObject)
			{
				this->percentReplacementTransformed = t;
				SvgObject* interpolated = Svg::interpolate(this->svgObject, replacement->svgObject, t);
				this->svgObject->free();
				g_memory_free(this->svgObject);
				this->svgObject = interpolated;
			}

			// Interpolate other properties
			this->position = CMath::interpolate(t, this->position, replacement->position);
			this->globalPosition = CMath::interpolate(t, this->globalPosition, replacement->globalPosition);
			this->_globalPositionStart = CMath::interpolate(t, this->_globalPositionStart, replacement->_globalPositionStart);
			this->rotation = CMath::interpolate(t, this->rotation, replacement->rotation);
			this->scale = CMath::interpolate(t, this->scale, replacement->scale);
			this->fillColor = CMath::interpolate(t, this->fillColor, replacement->fillColor);
			this->strokeColor = CMath::interpolate(t, this->strokeColor, replacement->strokeColor);
			this->strokeWidth = CMath::interpolate(t, this->strokeWidth, replacement->strokeWidth);
			// TODO: Come up with _svgScaleStart so scales can be reset
			// srcObject->svgScale = CMath::interpolate(t, srcObject->svgScale, dstObject->svgScale);
			this->percentCreated = 1.0f;

			// Fade out dstObject
			if (replacement->percentCreated > 0.0f)
			{
				replacement->fillColor = CMath::interpolate(t,
					replacement->fillColor,
					glm::u8vec4(replacement->fillColor.r, replacement->fillColor.g, replacement->fillColor.b, 0)
				);
				replacement->strokeColor = CMath::interpolate(t,
					replacement->strokeColor,
					glm::u8vec4(replacement->strokeColor.r, replacement->strokeColor.g, replacement->strokeColor.b, 0)
				);
			}
		}
		else
		{
			replacement->percentCreated = 1.0f;
		}
	}

	void AnimObject::resetAllState()
	{
		if (_svgObjectStart != nullptr && svgObject != nullptr)
		{
			Svg::copy(svgObject, _svgObjectStart);
		}
		globalPosition = _globalPositionStart;
		position = _positionStart;
		rotation = _rotationStart;
		scale = _scaleStart;
		fillColor = _fillColorStart;
		strokeColor = _strokeColorStart;
		strokeWidth = _strokeWidthStart;
		percentCreated = 0.0f;
		status = AnimObjectStatus::Inactive;
		circumscribeId = NULL_ANIM;

		if (objectType == AnimObjectTypeV1::Camera && as.camera.isActiveCamera)
		{
			status = as.camera.isActiveCamera
				? AnimObjectStatus::Active
				: AnimObjectStatus::Inactive;
		}
	}

	void AnimObject::updateStatus(AnimationManagerData* am, AnimObjectStatus newStatus)
	{
		this->status = newStatus;

		for (auto iter = beginBreadthFirst(am); iter != end(); ++iter)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, *iter);
			if (child)
			{
				child->status = newStatus;
			}
		}
	}

	void AnimObject::updateChildrenPercentCreated(AnimationManagerData* am, float newPercentCreated)
	{
		for (auto iter = beginBreadthFirst(am); iter != end(); ++iter)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, *iter);
			if (child)
			{
				child->percentCreated = newPercentCreated;
			}
		}
	}

	void AnimObject::copySvgScaleToChildren(AnimationManagerData* am) const
	{
		for (auto iter = beginBreadthFirst(am); iter != end(); ++iter)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, *iter);
			if (child)
			{
				child->svgScale = this->svgScale;
			}
		}
	}

	void AnimObject::copyStrokeWidthToChildren(AnimationManagerData* am) const
	{
		for (auto iter = beginBreadthFirst(am); iter != end(); ++iter)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, *iter);
			if (child)
			{
				child->_strokeWidthStart = this->_strokeWidthStart;
			}
		}
	}

	void AnimObject::copyStrokeColorToChildren(AnimationManagerData* am) const
	{
		for (auto iter = beginBreadthFirst(am); iter != end(); ++iter)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, *iter);
			if (child)
			{
				child->_strokeColorStart = this->_strokeColorStart;
			}
		}
	}

	void AnimObject::copyFillColorToChildren(AnimationManagerData* am) const
	{
		for (auto iter = beginBreadthFirst(am); iter != end(); ++iter)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, *iter);
			if (child)
			{
				child->_fillColorStart = this->_fillColorStart;
			}
		}
	}

	AnimObjectBreadthFirstIter AnimObject::beginBreadthFirst(AnimationManagerData* am) const
	{
		return AnimObjectBreadthFirstIter(am, this->id);
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
		case AnimObjectTypeV1::Arrow:
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
		case AnimObjectTypeV1::SvgFileObject:
			this->as.svgFile.free();
			break;
		case AnimObjectTypeV1::Camera:
			this->as.camera.free();
			break;
		case AnimObjectTypeV1::ScriptObject:
			this->as.script.free();
			break;
		case AnimObjectTypeV1::CodeBlock:
			this->as.codeBlock.free();
			break;
		case AnimObjectTypeV1::Length:
		case AnimObjectTypeV1::None:
			break;
		}

		this->parentId = NULL_ANIM_OBJECT;
		this->id = NULL_ANIM_OBJECT;
	}

	void AnimObject::serialize(RawMemory& memory) const
	{
		// AnimObjectType     -> uint32
		// _PositionStart     -> Vec3
		// RotationStart      -> Vec3
		// ScaleStart         -> Vec3
		// _FillColorStart    -> Vec4U8
		// _StrokeColorStart  -> Vec4U8
		// 
		// _StrokeWidthStart  -> f32
		// svgScale           -> f32
		// 
		// isTransparent      -> u8
		// is3D               -> u8
		// drawDebugBoxes     -> u8
		// drawCurveDebugBoxes -> u8
		// 
		// Id                   -> AnimObjId
		// ParentId             -> AnimObjId
		// NumGeneratedChildren -> uint32 
		// GeneratedChildrenIds -> AnimObjId[numGeneratedChildren]
		// 
		// NumRefAnimations     -> uint32
		// ReferencedAnimations -> AnimId[NumRefAnimations]
		// 
		// NameLength         -> uint32
		// Name               -> uint8[NameLength + 1]
		// AnimObject Specific Data
		uint32 animObjectType = (uint32)objectType;
		memory.write<uint32>(&animObjectType);
		CMath::serialize(memory, _positionStart);
		CMath::serialize(memory, _rotationStart);
		CMath::serialize(memory, _scaleStart);
		CMath::serialize(memory, _fillColorStart);
		CMath::serialize(memory, _strokeColorStart);

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

		memory.write<AnimObjId>(&id);
		memory.write<AnimObjId>(&parentId);

		uint32 numGeneratedChildren = (uint32)generatedChildrenIds.size();
		memory.write<uint32>(&numGeneratedChildren);
		for (uint32 i = 0; i < numGeneratedChildren; i++)
		{
			memory.write<AnimObjId>(&generatedChildrenIds[i]);
		}

		uint32 numRefAnimations = (uint32)referencedAnimations.size();
		memory.write<uint32>(&numRefAnimations);
		for (auto animId : referencedAnimations)
		{
			memory.write<AnimId>(&animId);
		}

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
			g_logger_assert(this->_svgObjectStart != nullptr, "Somehow SVGObject has no object allocated.");
			this->_svgObjectStart->serialize(memory);
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
		case AnimObjectTypeV1::SvgFileObject:
			this->as.svgFile.serialize(memory);
			break;
		case AnimObjectTypeV1::Camera:
			this->as.camera.serialize(memory);
			break;
		case AnimObjectTypeV1::ScriptObject:
			this->as.script.serialize(memory);
			break;
		case AnimObjectTypeV1::CodeBlock:
			this->as.codeBlock.serialize(memory);
			break;
		case AnimObjectTypeV1::Arrow:
			this->as.arrow.serialize(memory);
			break;
		case AnimObjectTypeV1::Length:
		case AnimObjectTypeV1::None:
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
		res.id = NULL_ANIM;
		return res;
	}

	AnimObject AnimObject::createDefaultFromParent(AnimationManagerData* am, AnimObjectTypeV1 type, AnimObjId parentId, bool addChildAsGenerated)
	{
		AnimObject* parent = AnimationManager::getMutableObject(am, parentId);
		AnimObject res = createDefault(am, type);
		res.takeAttributesFrom(*parent);
		res.parentId = parentId;

		if (addChildAsGenerated)
		{
			parent->generatedChildrenIds.push_back(res.id);
			res.isGenerated = true;
		}

		return res;
	}

	AnimObject AnimObject::createDefaultFromObj(AnimationManagerData* am, AnimObjectTypeV1 type, const AnimObject& obj)
	{
		AnimObject res = createDefault(am, type);
		res.takeAttributesFrom(obj);
		return res;
	}

	AnimObject AnimObject::createDefault(AnimationManagerData* am, AnimObjectTypeV1 type)
	{
		AnimObject res;
		res.id = animObjectUidCounter++;
		g_logger_assert(animObjectUidCounter < INT32_MAX, "Somehow our UID counter reached '%d'. If this ever happens, re-map all ID's to a lower range since it's likely there's not actually 2 billion animations in the scene.", INT32_MAX);
		res.parentId = NULL_ANIM_OBJECT;
		res.percentCreated = 0.0f;
		res.circumscribeId = NULL_ANIM;

		const char* newObjName = "New Object";
		res.nameLength = (uint32)std::strlen(newObjName);
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

		res.globalTransform = glm::identity<glm::mat4>();

		switch (type)
		{
		case AnimObjectTypeV1::TextObject:
			res.svgScale = 150.0f;
			res.as.textObject = TextObject::createDefault();
			res.as.textObject.init(am, res.id);
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
		case AnimObjectTypeV1::SvgFileObject:
			res.as.svgFile = SvgFileObject::createDefault();
			break;
		case AnimObjectTypeV1::Camera:
			res.as.camera = CameraObject::createDefault();
			break;
		case AnimObjectTypeV1::ScriptObject:
			res.as.script = ScriptObject::createDefault();
			break;
		case AnimObjectTypeV1::CodeBlock:
			res.as.codeBlock = CodeBlock::createDefault();
			break;
		case AnimObjectTypeV1::Arrow:
			res.as.arrow.tipLength = 0.4f;
			res.as.arrow.tipWidth = 0.4f;
			res.as.arrow.stemLength = 1.8f;
			res.as.arrow.stemWidth = 0.12f;
			res.svgScale = 50.0f;
			res.as.arrow.init(&res);
			break;
		case AnimObjectTypeV1::Length:
		case AnimObjectTypeV1::None:
			break;
		}

		return res;
	}

	AnimObject AnimObject::createCopy(const AnimObject& from)
	{
		AnimObject res;
		res.id = from.id;
		res.parentId = from.parentId;
		res.percentCreated = from.percentCreated;

		res.nameLength = from.nameLength;
		res.name = (uint8*)g_memory_allocate(sizeof(uint8) * (res.nameLength + 1));
		g_memory_copyMem(res.name, (void*)from.name, sizeof(uint8) * (res.nameLength + 1));

		res.status = from.status;
		res.objectType = from.objectType;

		res.rotation = from.rotation;
		res._rotationStart = from._rotationStart;

		res.scale = from.scale;
		res._scaleStart = from._scaleStart;

		res.position = from.position;
		res._positionStart = from._positionStart;
		res._globalPositionStart = from._globalPositionStart;
		res.globalPosition = from.globalPosition;

		res.svgObject = nullptr;
		res._svgObjectStart = nullptr;

		if (from.svgObject)
		{
			res.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			*res.svgObject = Svg::createDefault();
			Svg::copy(res.svgObject, from.svgObject);
		}
		if (from._svgObjectStart)
		{
			res._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			*res._svgObjectStart = Svg::createDefault();
			Svg::copy(res._svgObjectStart, from._svgObjectStart);
		}

		res.isTransparent = from.isTransparent;
		res.drawDebugBoxes = from.drawDebugBoxes;
		res.drawCurveDebugBoxes = from.drawCurveDebugBoxes;
		res.is3D = from.is3D;
		res.isGenerated = from.isGenerated;
		res.svgScale = from.svgScale;

		res.strokeWidth = from.strokeWidth;
		res._strokeWidthStart = from._strokeWidthStart;
		res.strokeColor = from.strokeColor;
		res._strokeColorStart = from._strokeColorStart;
		res.fillColor = from.fillColor;
		res._fillColorStart = from._fillColorStart;

		res.objectType = from.objectType;

		res.generatedChildrenIds = from.generatedChildrenIds;

		switch (from.objectType)
		{
		case AnimObjectTypeV1::TextObject:
			res.as.textObject = TextObject::createCopy(from.as.textObject);
			break;
		case AnimObjectTypeV1::LaTexObject:
		case AnimObjectTypeV1::SvgObject:
		case AnimObjectTypeV1::Square:
		case AnimObjectTypeV1::Circle:
		case AnimObjectTypeV1::Cube:
		case AnimObjectTypeV1::Axis:
		case AnimObjectTypeV1::SvgFileObject:
		case AnimObjectTypeV1::Camera:
		case AnimObjectTypeV1::ScriptObject:
		case AnimObjectTypeV1::CodeBlock:
		case AnimObjectTypeV1::Arrow:
			// TODO: Implement Copy for these
			break;
		case AnimObjectTypeV1::Length:
		case AnimObjectTypeV1::None:
			break;
		}

		return res;
	}

	// ----------------------------- Internal Functions -----------------------------
	static AnimObject deserializeAnimObjectV1(AnimationManagerData*, RawMemory& memory)
	{
		// TODO: Replace this with some sort of library where you register 
		// an object layout and it automatically serializes/deserializes stuff
		// for you

		AnimObject res;
		// If the object is being read in from the file then it's not
		// generated since all generated objects don't get saved
		res.isGenerated = false;
		res.drawCurves = false;
		res.drawControlPoints = false;
		res.percentCreated = 0.0f;

		// AnimObjectType     -> uint32
		// _PositionStart     -> Vec3
		// RotationStart      -> Vec3
		// ScaleStart         -> Vec3
		// _FillColorStart    -> Vec4U8
		// _StrokeColorStart  -> Vec4U8
		// 
		// _StrokeWidthStart  -> f32
		// svgScale           -> f32
		// 
		// isTransparent      -> u8
		// is3D               -> u8
		// drawDebugBoxes     -> u8
		// drawCurveDebugBoxes -> u8
		// 
		// Id                   -> AnimObjId
		// ParentId             -> AnimObjId
		// NumGeneratedChildren -> uint32
		// GeneratedChildrenIds -> AnimObjId[numGeneratedChildren]
		//
		// numRefAnimations     -> uint32
		// referencedAnimations -> AnimId[numObjects]
		// 
		// NameLength         -> uint32
		// Name               -> uint8[NameLength + 1]
		// AnimObject Specific Data
		uint32 animObjectType;
		memory.read<uint32>(&animObjectType);
		g_logger_assert(animObjectType < (uint32)AnimObjectTypeV1::Length, "Invalid AnimObjectType '%d' from memory. Must be corrupted memory.", animObjectType);
		res.objectType = (AnimObjectTypeV1)animObjectType;
		res._positionStart = CMath::deserializeVec3(memory);
		res._rotationStart = CMath::deserializeVec3(memory);
		res._scaleStart = CMath::deserializeVec3(memory);
		res._fillColorStart = CMath::deserializeU8Vec4(memory);
		res._strokeColorStart = CMath::deserializeU8Vec4(memory);

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

		memory.read<AnimObjId>(&res.id);
		animObjectUidCounter = glm::max(animObjectUidCounter, res.id + 1);
		memory.read<AnimObjId>(&res.parentId);

		uint32 numGeneratedChildrenIds;
		memory.read<uint32>(&numGeneratedChildrenIds);
		res.generatedChildrenIds.reserve(numGeneratedChildrenIds);
		for (uint32 i = 0; i < numGeneratedChildrenIds; i++)
		{
			AnimObjId childId;
			memory.read<AnimObjId>(&childId);
			res.generatedChildrenIds.push_back(childId);
		}

		uint32 numReferencedAnimations;
		memory.read<uint32>(&numReferencedAnimations);
		for (uint32 i = 0; i < numReferencedAnimations; i++)
		{
			AnimId refAnim;
			memory.read<AnimId>(&refAnim);
			res.referencedAnimations.insert(refAnim);
		}

		if (!memory.read<uint32>(&res.nameLength))
		{
			g_logger_assert(false, "Corrupted project data. Irrecoverable.");
		}
		res.name = (uint8*)g_memory_allocate(sizeof(uint8) * (res.nameLength + 1));
		memory.readDangerous(res.name, res.nameLength + 1);

		// Initialize other variables
		res.position = res._positionStart;
		res.rotation = res._rotationStart;
		res.scale = res._scaleStart;
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
		case AnimObjectTypeV1::SvgObject:
			res._svgObjectStart = SvgObject::deserialize(memory, version);
			res.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			*res.svgObject = Svg::createDefault();
			Svg::copy(res.svgObject, res._svgObjectStart);
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
		case AnimObjectTypeV1::SvgFileObject:
			res.as.svgFile = SvgFileObject::deserialize(memory, version);
			break;
		case AnimObjectTypeV1::Camera:
			res.as.camera = CameraObject::deserialize(memory, version);
			break;
		case AnimObjectTypeV1::ScriptObject:
			res.as.script = ScriptObject::deserialize(memory, version);
			break;
		case AnimObjectTypeV1::CodeBlock:
			res.as.codeBlock = CodeBlock::deserialize(memory, version);
			break;
		case AnimObjectTypeV1::Arrow:
			res.as.arrow = Arrow::deserialize(memory, version);
			res.as.arrow.init(&res);
			break;
		case AnimObjectTypeV1::Length:
		case AnimObjectTypeV1::None:
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
		// objectIds      -> AnimObjId[numObjects]
		// Custom animation data -> dynamic
		Animation res;
		uint32 animType;
		memory.read<uint32>(&animType);
		g_logger_assert(animType < (uint32)AnimTypeV1::Length, "Invalid animation type '%d' from memory. Must be corrupted memory.", animType);
		res.type = (AnimTypeV1)animType;
		memory.read<int32>(&res.frameStart);
		memory.read<int32>(&res.duration);

		memory.read<AnimId>(&res.id);
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
		for (uint32 i = 0; i < numObjects; i++)
		{
			AnimObjId tmp;
			memory.read<AnimObjId>(&tmp);
			res.animObjectIds.insert(tmp);
		}

		switch (res.type)
		{
		case AnimTypeV1::Create:
		case AnimTypeV1::UnCreate:
		case AnimTypeV1::FadeIn:
		case AnimTypeV1::FadeOut:
			// NOP
			break;
		case AnimTypeV1::RotateTo:
		case AnimTypeV1::Shift:
		case AnimTypeV1::AnimateFillColor:
		case AnimTypeV1::AnimateStrokeColor:
			res.as.modifyU8Vec4.target = CMath::deserializeU8Vec4(memory);
			break;
		case AnimTypeV1::AnimateStrokeWidth:
			g_logger_warning("TODO: implement me");
			break;
		case AnimTypeV1::Transform:
			res.as.replacementTransform = ReplacementTransformData::deserialize(memory);
			break;
		case AnimTypeV1::MoveTo:
			res.as.moveTo = MoveToData::deserialize(memory);
			break;
		case AnimTypeV1::AnimateScale:
			res.as.animateScale = AnimateScaleData::deserialize(memory);
			break;
		case AnimTypeV1::Circumscribe:
			res.as.circumscribe = Circumscribe::deserialize(memory);
			break;
		case AnimTypeV1::Length:
		case AnimTypeV1::None:
			break;
		}

		return res;
	}

	static CameraObject deserializeCameraObjectV1(RawMemory& memory)
	{
		//OrthoCamera camera2D;
		//bool is2D;
		//bool isActiveCamera;
		CameraObject res;
		res.camera2D = OrthoCamera::deserialize(memory, 1);
		uint8 is2DU8;
		memory.read<uint8>(&is2DU8);
		res.is2D = is2DU8 == 1;

		return res;
	}

	static void onMoveToGizmo(AnimationManagerData*, Animation* anim)
	{
		// TODO: Render and handle 2D gizmo logic based on edit mode
		std::string gizmoName = "Move_To_" + std::to_string(anim->id);
		Vec3 tmp = CMath::vector3From2(anim->as.moveTo.target);
		GizmoManager::translateGizmo(gizmoName.c_str(), &tmp, GizmoVariant::Free);
		anim->as.moveTo.target = CMath::vector2From3(tmp);
	}
}