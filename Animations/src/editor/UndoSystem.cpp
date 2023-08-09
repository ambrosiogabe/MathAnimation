#include "editor/UndoSystem.h"
#include "editor/panels/ErrorPopups.h"
#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "renderer/Fonts.h"
#include "platform/Platform.h"

namespace MathAnim
{
	static inline void assertCorrectType(AnimObject const* obj, AnimObjectTypeV1 expectedType)
	{
		g_logger_assert(obj->objectType == expectedType, "AnimObject is invalid type. Expected '{}', got '{}'.", expectedType, obj->objectType);
	}

	static inline void assertCorrectType(Animation const* anim, AnimTypeV1 expectedType)
	{
		g_logger_assert(anim->type == expectedType, "Animation is invalid type. Expected '{}', got '{}'.", expectedType, anim->type);
	}

	static inline bool assertAnimIsModifyVec3Type(Animation const* anim)
	{
		switch (anim->type)
		{
		case AnimTypeV1::Shift:
			return true;
		default:
			break;
		}

		return false;
	}

	static inline bool assertAnimIsModifyU8Vec4Type(Animation const* anim)
	{
		switch (anim->type)
		{
		case AnimTypeV1::AnimateStrokeColor:
		case AnimTypeV1::AnimateFillColor:
			return true;
		default:
			break;
		}

		return false;
	}

	template<typename T>
	static inline void assertEnumInRange(int value)
	{
		g_logger_assert(value >= 0 && value < (int)T::Length, "Invalid enum, out of range. Got '{}', and enum range goes from [{}, {}).", value, 0, (int)T::Length);
	}

	class Command
	{
	public:
		Command() {}
		virtual ~Command() {};

		virtual void execute(AnimationManagerData* const am) = 0;
		virtual void undo(AnimationManagerData* const am) = 0;
	};

	struct UndoSystemData
	{
		AnimationManagerData* am;
		Command** history;
		// The total number of commands that we can redo up to
		uint32 numCommands;
		// The offset where the current undo ptr is set
		// This is a ring buffer, so this should be <= to
		// (undoCursorHead + numCommands) % maxHistorySize
		uint32 undoCursorTail;
		// The offset for undo history beginning.
		uint32 undoCursorHead;
		uint32 maxHistorySize;
	};

	// -------------------------------------
	// Command Forward Decls
	// -------------------------------------
	class AddObjectToAnimCommand : public Command
	{
	public:
		AddObjectToAnimCommand(AnimObjId objToAdd, AnimId animToAddTo)
			: objToAdd(objToAdd), animToAddTo(animToAddTo)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objToAdd;
		AnimId animToAddTo;
	};

	class RemoveObjectFromAnimCommand : public Command
	{
	public:
		RemoveObjectFromAnimCommand(AnimObjId objToAdd, AnimId animToAddTo)
			: objToAdd(objToAdd), animToAddTo(animToAddTo)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objToAdd;
		AnimId animToAddTo;
	};

	class ModifyU8Vec4Command : public Command
	{
	public:
		ModifyU8Vec4Command(AnimObjId objId, const glm::u8vec4& oldVec, const glm::u8vec4& newVec, U8Vec4PropType propType)
			: objId(objId), oldVec(oldVec), newVec(newVec), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objId;
		glm::u8vec4 oldVec;
		glm::u8vec4 newVec;
		U8Vec4PropType propType;
	};

	class ModifyEnumCommand : public Command
	{
	public:
		ModifyEnumCommand(AnimObjId id, int oldEnum, int newEnum, EnumPropType propType)
			: id(id), oldEnum(oldEnum), newEnum(newEnum), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId id;
		int oldEnum;
		int newEnum;
		EnumPropType propType;
	};

	class ModifyFloatCommand : public Command
	{
	public:
		ModifyFloatCommand(AnimObjId objId, float oldValue, float newValue, FloatPropType propType)
			: objId(objId), oldValue(oldValue), newValue(newValue), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objId;
		float oldValue;
		float newValue;
		FloatPropType propType;
	};

	class ModifyVec2Command : public Command
	{
	public:
		ModifyVec2Command(AnimObjId id, const Vec2& oldVec, const Vec2& newVec, Vec2PropType propType)
			: id(id), oldVec(oldVec), newVec(newVec), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId id;
		Vec2 oldVec;
		Vec2 newVec;
		Vec2PropType propType;
	};

	class ModifyVec2iCommand : public Command
	{
	public:
		ModifyVec2iCommand(AnimObjId objId, const Vec2i& oldVec, const Vec2i& newVec, Vec2iPropType propType)
			: objId(objId), oldVec(oldVec), newVec(newVec), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objId;
		Vec2i oldVec;
		Vec2i newVec;
		Vec2iPropType propType;
	};

	class ModifyVec3Command : public Command
	{
	public:
		ModifyVec3Command(AnimObjId objId, const Vec3& oldVec, const Vec3& newVec, Vec3PropType propType)
			: objId(objId), oldVec(oldVec), newVec(newVec), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objId;
		Vec3 oldVec;
		Vec3 newVec;
		Vec3PropType propType;
	};

	class ModifyVec4Command : public Command
	{
	public:
		ModifyVec4Command(AnimObjId objId, const Vec4& oldVec, const Vec4& newVec, Vec4PropType propType)
			: objId(objId), oldVec(oldVec), newVec(newVec), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objId;
		Vec4 oldVec;
		Vec4 newVec;
		Vec4PropType propType;
	};

	class ModifyStringCommand : public Command
	{
	public:
		ModifyStringCommand(AnimObjId objId, const std::string& oldString, const std::string& newString, StringPropType propType)
			: objId(objId), oldString(oldString), newString(newString), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objId;
		std::string oldString;
		std::string newString;
		StringPropType propType;
	};

	class SetFontCommand : public Command
	{
	public:
		SetFontCommand(AnimObjId objId, const std::string& oldFont, const std::string& newFont)
			: objId(objId), oldFont(oldFont), newFont(newFont)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objId;
		std::string oldFont;
		std::string newFont;
	};

	class AnimDragDropInputCommand : public Command
	{
	public:
		AnimDragDropInputCommand(AnimObjId oldTarget, AnimObjId newTarget, AnimId animToAddTo, AnimDragDropType type)
			: oldTarget(oldTarget), newTarget(newTarget), animToAddTo(animToAddTo), type(type)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId oldTarget;
		AnimObjId newTarget;
		AnimId animToAddTo;
		AnimDragDropType type;
	};

	class ApplyU8Vec4ToChildrenCommand : public Command
	{
	public:
		ApplyU8Vec4ToChildrenCommand(AnimObjId objId, U8Vec4PropType propType)
			: objId(objId), oldProps({}), propType(propType)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;

	private:
		AnimObjId objId;
		U8Vec4PropType propType;
		std::unordered_map<AnimObjId, glm::u8vec4> oldProps;
	};

	namespace UndoSystem
	{
		// ----------------------------------------
		// Internal Implementations
		// ----------------------------------------
		static void pushAndExecuteCommand(UndoSystemData* us, Command* command);

		UndoSystemData* init(AnimationManagerData* const am, int maxHistory)
		{
			g_logger_assert(maxHistory > 1, "Cannot have a history of size '{}'. Must be greater than 1.", maxHistory);

			UndoSystemData* data = (UndoSystemData*)g_memory_allocate(sizeof(UndoSystemData));
			// TODO: Is there a way around this ugly hack while initializing? (Without classes...)
			data->am = am;
			data->numCommands = 0;
			data->maxHistorySize = maxHistory;
			data->history = (Command**)g_memory_allocate(sizeof(Command*) * maxHistory);
			data->undoCursorHead = 0;
			data->undoCursorTail = 0;

			return data;
		}

		void free(UndoSystemData* us)
		{
			for (uint32 i = us->undoCursorHead;
				i != ((us->undoCursorHead + us->numCommands) % us->maxHistorySize);
				i = ((i + 1) % us->maxHistorySize))
			{
				// Don't free the tail, nothing ever gets placed there
				if (i == ((us->undoCursorHead + us->numCommands) % us->maxHistorySize)) break;

				us->history[i]->~Command();
				g_memory_free(us->history[i]);
			}

			g_memory_free(us->history);
			g_memory_zeroMem(us, sizeof(UndoSystemData));
			g_memory_free(us);
		}

		void undo(UndoSystemData* us)
		{
			// No commands to undo
			if (us->undoCursorHead == us->undoCursorTail)
			{
				return;
			}

			uint32 offsetToUndo = us->undoCursorTail - 1;
			// Wraparound behavior
			if (us->undoCursorTail == 0)
			{
				offsetToUndo = us->maxHistorySize - 1;
			}

			us->history[offsetToUndo]->undo(us->am);
			us->undoCursorTail = offsetToUndo;
		}

		void redo(UndoSystemData* us)
		{
			// No commands to redo
			if (((us->undoCursorHead + us->numCommands) % us->maxHistorySize) == us->undoCursorTail)
			{
				return;
			}

			uint32 offsetToRedo = us->undoCursorTail;
			us->history[offsetToRedo]->execute(us->am);
			us->undoCursorTail = (us->undoCursorTail + 1) % us->maxHistorySize;
		}

		void applyU8Vec4ToChildren(UndoSystemData* us, ObjOrAnimId id, U8Vec4PropType propType)
		{
			auto* newCommand = (ApplyU8Vec4ToChildrenCommand*)g_memory_allocate(sizeof(ApplyU8Vec4ToChildrenCommand));
			new(newCommand)ApplyU8Vec4ToChildrenCommand(id, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setU8Vec4Prop(UndoSystemData* us, ObjOrAnimId objId, const glm::u8vec4& oldVec, const glm::u8vec4& newVec, U8Vec4PropType propType)
		{
			// Don't add this to the undo history if they don't actually change the stuff
			if (oldVec == newVec)
			{
				return;
			}

			auto* newCommand = (ModifyU8Vec4Command*)g_memory_allocate(sizeof(ModifyU8Vec4Command));
			new(newCommand)ModifyU8Vec4Command(objId, oldVec, newVec, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setEnumProp(UndoSystemData* us, ObjOrAnimId id, int oldEnum, int newEnum, EnumPropType propType)
		{
			// Don't add this to the undo history if they don't actually change the enum
			if (oldEnum == newEnum)
			{
				return;
			}

			switch (propType)
			{
			case EnumPropType::EaseType:
				assertEnumInRange<EaseType>(newEnum);
				assertEnumInRange<EaseType>(oldEnum);
				break;
			case EnumPropType::EaseDirection:
				assertEnumInRange<EaseDirection>(newEnum);
				assertEnumInRange<EaseDirection>(oldEnum);
				break;
			case EnumPropType::PlaybackType:
				assertEnumInRange<PlaybackType>(newEnum);
				assertEnumInRange<PlaybackType>(oldEnum);
				break;
			case EnumPropType::HighlighterLanguage:
				assertEnumInRange<HighlighterLanguage>(newEnum);
				assertEnumInRange<HighlighterLanguage>(oldEnum);
				break;
			case EnumPropType::HighlighterTheme:
				assertEnumInRange<HighlighterTheme>(newEnum);
				assertEnumInRange<HighlighterTheme>(oldEnum);
				break;
			case EnumPropType::CameraMode:
				assertEnumInRange<CameraMode>(newEnum);
				assertEnumInRange<CameraMode>(oldEnum);
				break;
			case EnumPropType::CircumscribeShape:
				assertEnumInRange<CircumscribeShape>(newEnum);
				assertEnumInRange<CircumscribeShape>(oldEnum);
				break;
			case EnumPropType::CircumscribeFade:
				assertEnumInRange<CircumscribeFade>(newEnum);
				assertEnumInRange<CircumscribeFade>(oldEnum);
				break;
			}

			auto* newCommand = (ModifyEnumCommand*)g_memory_allocate(sizeof(ModifyEnumCommand));
			new(newCommand)ModifyEnumCommand(id, oldEnum, newEnum, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setFloatProp(UndoSystemData* us, ObjOrAnimId objId, float oldValue, float newValue, FloatPropType propType)
		{
			// Don't add this to the undo history if they don't actually change the stuff
			if (oldValue == newValue)
			{
				return;
			}

			auto* newCommand = (ModifyFloatCommand*)g_memory_allocate(sizeof(ModifyFloatCommand));
			new(newCommand)ModifyFloatCommand(objId, oldValue, newValue, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setVec2Prop(UndoSystemData* us, ObjOrAnimId objId, const Vec2& oldVec, const Vec2& newVec, Vec2PropType propType)
		{
			// Don't add this to the undo history if they don't actually change the stuff
			if (oldVec == newVec)
			{
				return;
			}

			auto* newCommand = (ModifyVec2Command*)g_memory_allocate(sizeof(ModifyVec2Command));
			new(newCommand)ModifyVec2Command(objId, oldVec, newVec, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setVec2iProp(UndoSystemData* us, ObjOrAnimId objId, const Vec2i& oldVec, const Vec2i& newVec, Vec2iPropType propType)
		{
			// Don't add this to the undo history if they don't actually change the stuff
			if (oldVec == newVec)
			{
				return;
			}

			auto* newCommand = (ModifyVec2iCommand*)g_memory_allocate(sizeof(ModifyVec2iCommand));
			new(newCommand)ModifyVec2iCommand(objId, oldVec, newVec, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setVec3Prop(UndoSystemData* us, ObjOrAnimId objId, const Vec3& oldVec, const Vec3& newVec, Vec3PropType propType)
		{
			// Don't add this to the undo history if they don't actually change the stuff
			if (oldVec == newVec)
			{
				return;
			}

			auto* newCommand = (ModifyVec3Command*)g_memory_allocate(sizeof(ModifyVec3Command));
			new(newCommand)ModifyVec3Command(objId, oldVec, newVec, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setVec4Prop(UndoSystemData* us, ObjOrAnimId objId, const Vec4& oldVec, const Vec4& newVec, Vec4PropType propType)
		{
			// Don't add this to the undo history if they don't actually change the stuff
			if (oldVec == newVec)
			{
				return;
			}

			auto* newCommand = (ModifyVec4Command*)g_memory_allocate(sizeof(ModifyVec4Command));
			new(newCommand)ModifyVec4Command(objId, oldVec, newVec, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setStringProp(UndoSystemData* us, ObjOrAnimId objId, const std::string& oldString, const std::string& newString, StringPropType propType)
		{
			// Don't add this to the undo history if they don't actually change the stuff
			if (oldString == newString)
			{
				return;
			}

			auto* newCommand = (ModifyStringCommand*)g_memory_allocate(sizeof(ModifyStringCommand));
			new(newCommand)ModifyStringCommand(objId, oldString, newString, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setFont(UndoSystemData* us, ObjOrAnimId objId, const std::string& oldFont, const std::string& newFont)
		{
			// Don't add this to the undo history if they don't actually change the stuff
			if (oldFont == newFont)
			{
				return;
			}

			auto* newCommand = (SetFontCommand*)g_memory_allocate(sizeof(SetFontCommand));
			new(newCommand)SetFontCommand(objId, oldFont, newFont);
			pushAndExecuteCommand(us, newCommand);
		}

		void animDragDropInput(UndoSystemData* us, AnimObjId oldTarget, AnimObjId newTarget, AnimId animToAddTo, AnimDragDropType type)
		{
			auto* newCommand = (AnimDragDropInputCommand*)g_memory_allocate(sizeof(AnimDragDropInputCommand));
			new(newCommand)AnimDragDropInputCommand(oldTarget, newTarget, animToAddTo, type);
			pushAndExecuteCommand(us, newCommand);
		}

		void addObjectToAnim(UndoSystemData* us, AnimObjId objToAdd, AnimId animToAddTo)
		{
			auto* newCommand = (AddObjectToAnimCommand*)g_memory_allocate(sizeof(AddObjectToAnimCommand));
			new(newCommand)AddObjectToAnimCommand(objToAdd, animToAddTo);
			pushAndExecuteCommand(us, newCommand);
		}

		void removeObjectFromAnim(UndoSystemData* us, AnimObjId objToAdd, AnimId animToAddTo)
		{
			auto* newCommand = (RemoveObjectFromAnimCommand*)g_memory_allocate(sizeof(RemoveObjectFromAnimCommand));
			new(newCommand)RemoveObjectFromAnimCommand(objToAdd, animToAddTo);
			pushAndExecuteCommand(us, newCommand);
		}

#pragma warning( push )
#pragma warning( disable : 4100 )
		void addNewObjToScene(UndoSystemData* us, const AnimObject& obj)
		{

		}

		void removeObjFromScene(UndoSystemData* us, AnimObjId objId)
		{

		}
#pragma warning( pop )

		// ----------------------------------------
		// Internal Implementations
		// ----------------------------------------
		static void pushAndExecuteCommand(UndoSystemData* us, Command* command)
		{
			// Remove any commands after the current tail
			{
				uint32 numCommandsRemoved = 0;
				for (uint32 i = us->undoCursorTail;
					i != ((us->undoCursorHead + us->numCommands) % us->maxHistorySize);
					i = (i + 1) % us->maxHistorySize)
				{
					if (i == ((us->undoCursorHead + us->numCommands) % us->maxHistorySize)) break;

					us->history[i]->~Command();
					g_memory_free(us->history[i]);
					numCommandsRemoved++;
				}

				// Set the number of commands to the appropriate number now
				us->numCommands -= numCommandsRemoved;
			}

			if (((us->undoCursorTail + 1) % us->maxHistorySize) == us->undoCursorHead)
			{
				us->history[us->undoCursorHead]->~Command();
				g_memory_free(us->history[us->undoCursorHead]);

				us->undoCursorHead = (us->undoCursorHead + 1) % us->maxHistorySize;
				us->numCommands--;
			}

			uint32 offsetToPlaceNewCommand = us->undoCursorTail;
			us->history[offsetToPlaceNewCommand] = command;
			us->numCommands++;
			us->undoCursorTail = (us->undoCursorTail + 1) % us->maxHistorySize;

			us->history[offsetToPlaceNewCommand]->execute(us->am);
		}
	}

	// -------------------------------------
	// Command Implementations
	// -------------------------------------
	void AddObjectToAnimCommand::execute(AnimationManagerData* const am)
	{
		AnimationManager::addObjectToAnim(am, objToAdd, animToAddTo);
	}

	void AddObjectToAnimCommand::undo(AnimationManagerData* const am)
	{
		AnimationManager::removeObjectFromAnim(am, objToAdd, animToAddTo);
	}

	void RemoveObjectFromAnimCommand::execute(AnimationManagerData* const am)
	{
		AnimationManager::removeObjectFromAnim(am, objToAdd, animToAddTo);
	}

	void RemoveObjectFromAnimCommand::undo(AnimationManagerData* const am)
	{
		AnimationManager::addObjectToAnim(am, objToAdd, animToAddTo);
	}

	void ModifyU8Vec4Command::execute(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case U8Vec4PropType::FillColor:
				obj->_fillColorStart = this->newVec;
				break;
			case U8Vec4PropType::StrokeColor:
				obj->_strokeColorStart = this->newVec;
				break;
			// NOTE: These are animations
			case U8Vec4PropType::AnimateU8Vec4Target:
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}

		Animation* anim = AnimationManager::getMutableAnimation(am, this->objId);
		if (anim)
		{
			switch (propType)
			{
			case U8Vec4PropType::AnimateU8Vec4Target:
				assertAnimIsModifyU8Vec4Type(anim);
				anim->as.modifyU8Vec4.target = this->newVec;
				break;
			// NOTE: These are anim objects
			case U8Vec4PropType::FillColor:
			case U8Vec4PropType::StrokeColor:
				break;
			}
		}
	}

	void ModifyU8Vec4Command::undo(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case U8Vec4PropType::FillColor:
				obj->_fillColorStart = this->oldVec;
				break;
			case U8Vec4PropType::StrokeColor:
				obj->_strokeColorStart = this->oldVec;
				break;
			// NOTE: These are animations
			case U8Vec4PropType::AnimateU8Vec4Target:
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}

		Animation* anim = AnimationManager::getMutableAnimation(am, this->objId);
		if (anim)
		{
			switch (propType)
			{
			case U8Vec4PropType::AnimateU8Vec4Target:
				assertAnimIsModifyU8Vec4Type(anim);
				anim->as.modifyU8Vec4.target = this->oldVec;
				break;
				// NOTE: These are anim objects
			case U8Vec4PropType::FillColor:
			case U8Vec4PropType::StrokeColor:
				break;
			}
		}
	}

	void ModifyEnumCommand::execute(AnimationManagerData* const am)
	{
		Animation* anim = AnimationManager::getMutableAnimation(am, this->id);
		if (anim)
		{
			switch (propType)
			{
			case EnumPropType::EaseType:
				anim->easeType = (EaseType)this->newEnum;
				break;
			case EnumPropType::EaseDirection:
				anim->easeDirection = (EaseDirection)this->newEnum;
				break;
			case EnumPropType::PlaybackType:
				anim->playbackType = (PlaybackType)this->newEnum;
				break;
			case EnumPropType::CircumscribeFade:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.fade = (CircumscribeFade)this->newEnum;
				break;
			case EnumPropType::CircumscribeShape:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.shape = (CircumscribeShape)this->newEnum;
				break;
				// NOTE: These are animObjects, so they go in the if-block below
			case EnumPropType::HighlighterLanguage:
			case EnumPropType::HighlighterTheme:
			case EnumPropType::CameraMode:
				break;
			}
		}

		AnimObject* obj = AnimationManager::getMutableObject(am, this->id);
		if (obj)
		{
			switch (propType)
			{
			case EnumPropType::HighlighterLanguage:
				assertCorrectType(obj, AnimObjectTypeV1::CodeBlock);
				obj->as.codeBlock.language = (HighlighterLanguage)newEnum;
				obj->as.codeBlock.reInit(am, obj);
				break;
			case EnumPropType::HighlighterTheme:
				assertCorrectType(obj, AnimObjectTypeV1::CodeBlock);
				obj->as.codeBlock.theme = (HighlighterTheme)newEnum;
				obj->as.codeBlock.reInit(am, obj);
				break;
			case EnumPropType::CameraMode:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.mode = (CameraMode)newEnum;
				break;
				// NOTE: These are animation types, so they go in the if-block above
			case EnumPropType::EaseType:
			case EnumPropType::EaseDirection:
			case EnumPropType::PlaybackType:
			case EnumPropType::CircumscribeFade:
			case EnumPropType::CircumscribeShape:
				break;
			}

			AnimationManager::updateObjectState(am, id);
		}
	}

	void ModifyEnumCommand::undo(AnimationManagerData* const am)
	{
		Animation* anim = AnimationManager::getMutableAnimation(am, this->id);
		if (anim)
		{
			switch (propType)
			{
			case EnumPropType::EaseType:
				anim->easeType = (EaseType)this->oldEnum;
				break;
			case EnumPropType::EaseDirection:
				anim->easeDirection = (EaseDirection)this->oldEnum;
				break;
			case EnumPropType::PlaybackType:
				anim->playbackType = (PlaybackType)this->oldEnum;
				break;
			case EnumPropType::CircumscribeFade:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.fade = (CircumscribeFade)this->oldEnum;
				break;
			case EnumPropType::CircumscribeShape:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.shape = (CircumscribeShape)this->oldEnum;
				break;
				// NOTE: These are animObjects, so they go in the if-block below
			case EnumPropType::HighlighterLanguage:
			case EnumPropType::HighlighterTheme:
			case EnumPropType::CameraMode:
				break;
			}
		}

		AnimObject* obj = AnimationManager::getMutableObject(am, this->id);
		if (obj)
		{
			switch (propType)
			{
			case EnumPropType::HighlighterLanguage:
				assertCorrectType(obj, AnimObjectTypeV1::CodeBlock);
				obj->as.codeBlock.language = (HighlighterLanguage)oldEnum;
				obj->as.codeBlock.reInit(am, obj);
				break;
			case EnumPropType::HighlighterTheme:
				assertCorrectType(obj, AnimObjectTypeV1::CodeBlock);
				obj->as.codeBlock.theme = (HighlighterTheme)oldEnum;
				obj->as.codeBlock.reInit(am, obj);
				break;
			case EnumPropType::CameraMode:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.mode = (CameraMode)oldEnum;
				break;
				// NOTE: These are animation types, so they go in the if-block above
			case EnumPropType::EaseType:
			case EnumPropType::EaseDirection:
			case EnumPropType::PlaybackType:
			case EnumPropType::CircumscribeFade:
			case EnumPropType::CircumscribeShape:
				break;
			}

			AnimationManager::updateObjectState(am, id);
		}
	}

	void ModifyFloatCommand::execute(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case FloatPropType::StrokeWidth:
				obj->_strokeWidthStart = this->newValue;
				break;
			case FloatPropType::CameraFieldOfView:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.fov = this->newValue;
				break;
			case FloatPropType::CameraNearPlane:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.nearFarRange.min = this->newValue;
				break;
			case FloatPropType::CameraFarPlane:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.nearFarRange.max = this->newValue;
				break;
			case FloatPropType::CameraFocalDistance:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.focalDistance = this->newValue;
				break;
			case FloatPropType::CameraOrthoZoomLevel:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.orthoZoomLevel = this->newValue;
				break;
			case FloatPropType::SquareSideLength:
				assertCorrectType(obj, AnimObjectTypeV1::Square);
				obj->as.square.sideLength = this->newValue;
				obj->as.square.reInit(obj);
				break;
			case FloatPropType::CircleRadius:
				assertCorrectType(obj, AnimObjectTypeV1::Circle);
				obj->as.circle.radius = this->newValue;
				obj->as.circle.reInit(obj);
				break;
			// Animation types
			case FloatPropType::LagRatio:
			case FloatPropType::CircumscribeTimeWidth:
			case FloatPropType::CircumscribeBufferSize:
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}

		Animation* anim = AnimationManager::getMutableAnimation(am, this->objId);
		if (anim)
		{
			switch (propType)
			{
			case FloatPropType::LagRatio:
				anim->lagRatio = this->newValue;
				break;
			case FloatPropType::CircumscribeTimeWidth:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.timeWidth = this->newValue;
				break;
			case FloatPropType::CircumscribeBufferSize:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.bufferSize = this->newValue;
				break;
			// AnimObject types
			case FloatPropType::StrokeWidth:
			case FloatPropType::CameraFieldOfView:
			case FloatPropType::CameraNearPlane:
			case FloatPropType::CameraFarPlane:
			case FloatPropType::CameraFocalDistance:
			case FloatPropType::CameraOrthoZoomLevel:
			case FloatPropType::SquareSideLength:
			case FloatPropType::CircleRadius:
				break;
			}
		}
	}

	void ModifyFloatCommand::undo(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case FloatPropType::StrokeWidth:
				obj->_strokeWidthStart = this->oldValue;
				break;
			case FloatPropType::CameraFieldOfView:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.fov = this->oldValue;
				break;
			case FloatPropType::CameraNearPlane:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.nearFarRange.min = this->oldValue;
				break;
			case FloatPropType::CameraFarPlane:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.nearFarRange.max = this->oldValue;
				break;
			case FloatPropType::CameraFocalDistance:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.focalDistance = this->oldValue;
				break;
			case FloatPropType::CameraOrthoZoomLevel:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.orthoZoomLevel = this->oldValue;
				break;
			case FloatPropType::SquareSideLength:
				assertCorrectType(obj, AnimObjectTypeV1::Square);
				obj->as.square.sideLength = this->oldValue;
				obj->as.square.reInit(obj);
				break;
			case FloatPropType::CircleRadius:
				assertCorrectType(obj, AnimObjectTypeV1::Circle);
				obj->as.circle.radius = this->oldValue;
				obj->as.circle.reInit(obj);
				break;
			// Animation types
			case FloatPropType::LagRatio:
			case FloatPropType::CircumscribeTimeWidth:
			case FloatPropType::CircumscribeBufferSize:
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}

		Animation* anim = AnimationManager::getMutableAnimation(am, this->objId);
		if (anim)
		{
			switch (propType)
			{
			case FloatPropType::LagRatio:
				anim->lagRatio = this->oldValue;
				break;
			case FloatPropType::CircumscribeTimeWidth:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.timeWidth = this->oldValue;
				break;
			case FloatPropType::CircumscribeBufferSize:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.bufferSize = this->oldValue;
				break;
			case FloatPropType::StrokeWidth:
			case FloatPropType::CameraFieldOfView:
			case FloatPropType::CameraNearPlane:
			case FloatPropType::CameraFarPlane:
			case FloatPropType::CameraFocalDistance:
			case FloatPropType::CameraOrthoZoomLevel:
			case FloatPropType::SquareSideLength:
			case FloatPropType::CircleRadius:
				break;
			}
		}
	}

	void ModifyVec2Command::execute(AnimationManagerData* const am)
	{
		Animation* anim = AnimationManager::getMutableAnimation(am, this->id);
		if (anim)
		{
			switch (propType)
			{
			case Vec2PropType::MoveToTargetPos:
				assertCorrectType(anim, AnimTypeV1::MoveTo);
				anim->as.moveTo.target = this->newVec;
				break;
			case Vec2PropType::AnimateScaleTarget:
				assertCorrectType(anim, AnimTypeV1::AnimateScale);
				anim->as.animateScale.target = this->newVec;
				break;
			}
		}
	}

	void ModifyVec2Command::undo(AnimationManagerData* const am)
	{
		Animation* anim = AnimationManager::getMutableAnimation(am, this->id);
		if (anim)
		{
			switch (propType)
			{
			case Vec2PropType::MoveToTargetPos:
				assertCorrectType(anim, AnimTypeV1::MoveTo);
				anim->as.moveTo.target = this->oldVec;
				break;
			case Vec2PropType::AnimateScaleTarget:
				assertCorrectType(anim, AnimTypeV1::AnimateScale);
				anim->as.animateScale.target = this->oldVec;
				break;
			}
		}
	}

	void ModifyVec2iCommand::execute(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case Vec2iPropType::AspectRatio:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.aspectRatioFraction = this->newVec;
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}
	}

	void ModifyVec2iCommand::undo(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case Vec2iPropType::AspectRatio:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.aspectRatioFraction = this->oldVec;
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}
	}

	void ModifyVec3Command::execute(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case Vec3PropType::Position:
				obj->_positionStart = this->newVec;
				break;
			case Vec3PropType::Scale:
				obj->_scaleStart = this->newVec;
				break;
			case Vec3PropType::Rotation:
				obj->_rotationStart = this->newVec;
				break;
			// NOTE: These are animations
			case Vec3PropType::ModifyAnimationVec3Target:
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}

		Animation* anim = AnimationManager::getMutableAnimation(am, this->objId);
		if (anim)
		{
			switch (propType)
			{
			case Vec3PropType::ModifyAnimationVec3Target:
				assertAnimIsModifyVec3Type(anim);
				anim->as.modifyVec3.target = this->newVec;
				break;
			// NOTE: These are anim objects
			case Vec3PropType::Position:
			case Vec3PropType::Scale:
			case Vec3PropType::Rotation:
				break;
			}
		}
	}

	void ModifyVec3Command::undo(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case Vec3PropType::Position:
				obj->_positionStart = this->oldVec;
				break;
			case Vec3PropType::Scale:
				obj->_scaleStart = this->oldVec;
				break;
			case Vec3PropType::Rotation:
				obj->_rotationStart = this->oldVec;
				break;
			// NOTE: These are animations
			case Vec3PropType::ModifyAnimationVec3Target:
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}

		Animation* anim = AnimationManager::getMutableAnimation(am, this->objId);
		if (anim)
		{
			switch (propType)
			{
			case Vec3PropType::ModifyAnimationVec3Target:
				assertAnimIsModifyVec3Type(anim);
				anim->as.modifyVec3.target = this->oldVec;
				break;
			// NOTE: These are anim objects
			case Vec3PropType::Position:
			case Vec3PropType::Scale:
			case Vec3PropType::Rotation:
				break;
			}
		}
	}

	void ModifyVec4Command::execute(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case Vec4PropType::CameraBackgroundColor:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.fillColor = this->newVec;
				break;
				// NOTE: The following are animations
			case Vec4PropType::CircumscribeColor:
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}

		Animation* anim = AnimationManager::getMutableAnimation(am, this->objId);
		if (anim)
		{
			switch (propType)
			{
			case Vec4PropType::CircumscribeColor:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.color = this->newVec;
				break;
				// NOTE: The following are anim objects
			case Vec4PropType::CameraBackgroundColor:
				break;
			}
		}
	}

	void ModifyVec4Command::undo(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case Vec4PropType::CameraBackgroundColor:
				assertCorrectType(obj, AnimObjectTypeV1::Camera);
				obj->as.camera.fillColor = this->oldVec;
				break;
				// NOTE: The following are animations
			case Vec4PropType::CircumscribeColor:
				break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}

		Animation* anim = AnimationManager::getMutableAnimation(am, this->objId);
		if (anim)
		{
			switch (propType)
			{
			case Vec4PropType::CircumscribeColor:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.color = this->oldVec;
				break;
				// NOTE: The following are anim objects
			case Vec4PropType::CameraBackgroundColor:
				break;
			}
		}
	}

	void ModifyStringCommand::execute(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case StringPropType::Name:
				obj->setName(this->newString.c_str(), this->newString.length());
				break;
			case StringPropType::TextObjectText:
				assertCorrectType(obj, AnimObjectTypeV1::TextObject);
				obj->as.textObject.setText(newString);
				obj->as.textObject.reInit(am, obj);
				break;
			case StringPropType::CodeBlockText:
				assertCorrectType(obj, AnimObjectTypeV1::CodeBlock);
				obj->as.codeBlock.setText(newString);
				obj->as.codeBlock.reInit(am, obj);
				break;
			case StringPropType::LaTexText:
				assertCorrectType(obj, AnimObjectTypeV1::LaTexObject);
				obj->as.laTexObject.setText(newString);
				obj->as.laTexObject.parseLaTex();
				break;
			case StringPropType::SvgFilepath:
			{
				assertCorrectType(obj, AnimObjectTypeV1::SvgFileObject);
				if (Platform::fileExists(newString.c_str()))
				{
					if (obj->as.svgFile.setFilepath(newString))
					{
						obj->as.svgFile.reInit(am, obj);
					}
					else
					{
						ErrorPopups::popupSvgImportError();
					}
				}
				else if (newString != "")
				{
					ErrorPopups::popupMissingFileError(newString);
				}
			}
			break;
			}

			AnimationManager::updateObjectState(am, this->objId);
		}
	}

	void ModifyStringCommand::undo(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			switch (propType)
			{
			case StringPropType::Name:
				obj->setName(this->oldString.c_str(), this->oldString.length());
				break;
			case StringPropType::TextObjectText:
				assertCorrectType(obj, AnimObjectTypeV1::TextObject);
				obj->as.textObject.setText(oldString);
				obj->as.textObject.reInit(am, obj);
				break;
			case StringPropType::CodeBlockText:
				assertCorrectType(obj, AnimObjectTypeV1::CodeBlock);
				obj->as.codeBlock.setText(oldString);
				obj->as.codeBlock.reInit(am, obj);
				break;
			case StringPropType::LaTexText:
				assertCorrectType(obj, AnimObjectTypeV1::LaTexObject);
				obj->as.laTexObject.setText(oldString);
				obj->as.laTexObject.parseLaTex();
				break;
			case StringPropType::SvgFilepath:
			{
				assertCorrectType(obj, AnimObjectTypeV1::SvgFileObject);
				if (Platform::fileExists(oldString.c_str()))
				{
					if (obj->as.svgFile.setFilepath(oldString))
					{
						obj->as.svgFile.reInit(am, obj);
					}
					else
					{
						ErrorPopups::popupSvgImportError();
					}
				}
				else if (newString != "")
				{
					ErrorPopups::popupMissingFileError(oldString);
				}
			}
			break;
			}
			AnimationManager::updateObjectState(am, this->objId);
		}
	}

	void SetFontCommand::execute(AnimationManagerData* const am)
	{
		AnimObject* object = AnimationManager::getMutableObject(am, objId);
		if (object)
		{
			assertCorrectType(object, AnimObjectTypeV1::TextObject);

			if (object->as.textObject.font)
			{
				Fonts::unloadFont(object->as.textObject.font);
			}
			object->as.textObject.font = Fonts::loadFont(newFont.c_str());
			object->as.textObject.reInit(am, object);

			AnimationManager::updateObjectState(am, objId);
		}
	}

	void SetFontCommand::undo(AnimationManagerData* const am)
	{
		AnimObject* object = AnimationManager::getMutableObject(am, objId);
		if (object && oldFont != "")
		{
			assertCorrectType(object, AnimObjectTypeV1::TextObject);

			if (object->as.textObject.font)
			{
				Fonts::unloadFont(object->as.textObject.font);
			}
			object->as.textObject.font = Fonts::loadFont(oldFont.c_str());
			object->as.textObject.reInit(am, object);

			AnimationManager::updateObjectState(am, objId);
		}
	}

	void AnimDragDropInputCommand::execute(AnimationManagerData* const am)
	{
		Animation* anim = AnimationManager::getMutableAnimation(am, animToAddTo);
		if (anim)
		{
			switch (type)
			{
			case AnimDragDropType::ReplacementTransformSrc:
				assertCorrectType(anim, AnimTypeV1::Transform);
				anim->as.replacementTransform.srcAnimObjectId = newTarget;
				break;
			case AnimDragDropType::ReplacementTransformDst:
				assertCorrectType(anim, AnimTypeV1::Transform);
				anim->as.replacementTransform.dstAnimObjectId = newTarget;
				break;
			case AnimDragDropType::MoveToTarget:
				assertCorrectType(anim, AnimTypeV1::MoveTo);
				anim->as.moveTo.object = newTarget;
				break;
			case AnimDragDropType::AnimateScaleTarget:
				assertCorrectType(anim, AnimTypeV1::AnimateScale);
				anim->as.animateScale.object = newTarget;
				break;
			case AnimDragDropType::CircumscribeTarget:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.obj = newTarget;
				break;
			}
		}
	}

	void AnimDragDropInputCommand::undo(AnimationManagerData* const am)
	{
		Animation* anim = AnimationManager::getMutableAnimation(am, animToAddTo);
		if (anim)
		{
			switch (type)
			{
			case AnimDragDropType::ReplacementTransformSrc:
				assertCorrectType(anim, AnimTypeV1::Transform);
				anim->as.replacementTransform.srcAnimObjectId = oldTarget;
				break;
			case AnimDragDropType::ReplacementTransformDst:
				assertCorrectType(anim, AnimTypeV1::Transform);
				anim->as.replacementTransform.dstAnimObjectId = oldTarget;
				break;
			case AnimDragDropType::MoveToTarget:
				assertCorrectType(anim, AnimTypeV1::MoveTo);
				anim->as.moveTo.object = oldTarget;
				break;
			case AnimDragDropType::AnimateScaleTarget:
				assertCorrectType(anim, AnimTypeV1::AnimateScale);
				anim->as.animateScale.object = oldTarget;
				break;
			case AnimDragDropType::CircumscribeTarget:
				assertCorrectType(anim, AnimTypeV1::Circumscribe);
				anim->as.circumscribe.obj = oldTarget;
				break;
			}
		}
	}

	void ApplyU8Vec4ToChildrenCommand::execute(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			for (auto it = obj->beginBreadthFirst(am); it != obj->end(); ++it)
			{
				AnimObjId childId = *it;
				AnimObject* childObj = AnimationManager::getMutableObject(am, childId);
				if (childObj)
				{
					switch (propType)
					{
					case U8Vec4PropType::FillColor:
						this->oldProps[childId] = childObj->_fillColorStart;
						childObj->_fillColorStart = obj->_fillColorStart;
						break;
					case U8Vec4PropType::StrokeColor:
						this->oldProps[childId] = childObj->_strokeColorStart;
						childObj->_fillColorStart = obj->_strokeColorStart;
						break;
					}
				}
			}
			AnimationManager::updateObjectState(am, this->objId);
		}
	}

	void ApplyU8Vec4ToChildrenCommand::undo(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			for (auto it = obj->beginBreadthFirst(am); it != obj->end(); ++it)
			{
				AnimObjId childId = *it;
				AnimObject* childObj = AnimationManager::getMutableObject(am, childId);
				if (childObj)
				{
					if (auto childColorIter = this->oldProps.find(childId); childColorIter != this->oldProps.end())
					{
						switch (propType)
						{
						case U8Vec4PropType::FillColor:
							childObj->_fillColorStart = childColorIter->second;
							break;
						case U8Vec4PropType::StrokeColor:
							childObj->_strokeColorStart = childColorIter->second;
							break;
						}
					}
				}
			}
			AnimationManager::updateObjectState(am, this->objId);
		}
	}
}
