#include "editor/UndoSystem.h"
#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "renderer/Fonts.h"

namespace MathAnim
{
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

		void applyU8Vec4ToChildren(UndoSystemData* us, AnimObjId id, U8Vec4PropType propType)
		{
			auto* newCommand = (ApplyU8Vec4ToChildrenCommand*)g_memory_allocate(sizeof(ApplyU8Vec4ToChildrenCommand));
			new(newCommand)ApplyU8Vec4ToChildrenCommand(id, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setU8Vec4Prop(UndoSystemData* us, AnimObjId objId, const glm::u8vec4& oldVec, const glm::u8vec4& newVec, U8Vec4PropType propType)
		{
			auto* newCommand = (ModifyU8Vec4Command*)g_memory_allocate(sizeof(ModifyU8Vec4Command));
			new(newCommand)ModifyU8Vec4Command(objId, oldVec, newVec, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setEnumProp(UndoSystemData* us, AnimObjId id, int oldEnum, int newEnum, EnumPropType propType)
		{
			auto* newCommand = (ModifyEnumCommand*)g_memory_allocate(sizeof(ModifyEnumCommand));
			new(newCommand)ModifyEnumCommand(id, oldEnum, newEnum, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setFloatProp(UndoSystemData* us, AnimObjId objId, float oldValue, float newValue, FloatPropType propType)
		{
			auto* newCommand = (ModifyFloatCommand*)g_memory_allocate(sizeof(ModifyFloatCommand));
			new(newCommand)ModifyFloatCommand(objId, oldValue, newValue, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setVec3Prop(UndoSystemData* us, AnimObjId objId, const Vec3& oldVec, const Vec3& newVec, Vec3PropType propType)
		{
			auto* newCommand = (ModifyVec3Command*)g_memory_allocate(sizeof(ModifyVec3Command));
			new(newCommand)ModifyVec3Command(objId, oldVec, newVec, propType);
			pushAndExecuteCommand(us, newCommand);
		}

		void setStringProp(UndoSystemData* us, AnimObjId objId, const std::string& oldString, const std::string& newString, StringPropType propType)
		{
			if (oldString != newString)
			{
				auto* newCommand = (ModifyStringCommand*)g_memory_allocate(sizeof(ModifyStringCommand));
				new(newCommand)ModifyStringCommand(objId, oldString, newString, propType);
				pushAndExecuteCommand(us, newCommand);
			}
		}

		void setFont(UndoSystemData* us, AnimObjId objId, const std::string& oldFont, const std::string& newFont)
		{
			auto* newCommand = (SetFontCommand*)g_memory_allocate(sizeof(SetFontCommand));
			new(newCommand)SetFontCommand(objId, oldFont, newFont);
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
			}
			AnimationManager::updateObjectState(am, this->objId);
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
			}
			AnimationManager::updateObjectState(am, this->objId);
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
				g_logger_assert(this->newEnum >= 0 && this->newEnum < (int)EaseType::Length, "How did this happen?");
				anim->easeType = (EaseType)this->newEnum;
				break;
			case EnumPropType::EaseDirection:
				g_logger_assert(this->newEnum >= 0 && this->newEnum < (int)EaseDirection::Length, "How did this happen?");
				anim->easeDirection = (EaseDirection)this->newEnum;
				break;
			case EnumPropType::PlaybackType:
				g_logger_assert(this->newEnum >= 0 && this->newEnum < (int)PlaybackType::Length, "How did this happen?");
				anim->playbackType = (PlaybackType)this->newEnum;
				break;
			// NOTE: These are animObjects, so they go in the if-block below
			case EnumPropType::HighlighterLanguage:
			case EnumPropType::HighlighterTheme:
				break;
			}
		}

		AnimObject* obj = AnimationManager::getMutableObject(am, this->id);
		if (obj)
		{
			switch (propType)
			{
			case EnumPropType::HighlighterLanguage:
				g_logger_assert(obj->objectType == AnimObjectTypeV1::CodeBlock, "How did this happen?");
				g_logger_assert(newEnum >= 0 && newEnum < (int)HighlighterLanguage::Length, "How did this happen?");
				obj->as.codeBlock.language = (HighlighterLanguage)newEnum;
				obj->as.codeBlock.reInit(am, obj);
				break;
			case EnumPropType::HighlighterTheme:
				g_logger_assert(obj->objectType == AnimObjectTypeV1::CodeBlock, "How did this happen?");
				g_logger_assert(newEnum >= 0 && newEnum < (int)HighlighterTheme::Length, "How did this happen?");
				obj->as.codeBlock.theme = (HighlighterTheme)newEnum;
				obj->as.codeBlock.reInit(am, obj);
				break;
			// NOTE: These are animation types, so they go in the if-block above
			case EnumPropType::EaseType:
			case EnumPropType::EaseDirection:
			case EnumPropType::PlaybackType:
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
			// NOTE: These are animObjects, so they go in the if-block below
			case EnumPropType::HighlighterLanguage:
			case EnumPropType::HighlighterTheme:
				break;
			}
		}

		AnimObject* obj = AnimationManager::getMutableObject(am, this->id);
		if (obj)
		{
			switch (propType)
			{
			case EnumPropType::HighlighterLanguage:
				g_logger_assert(obj->objectType == AnimObjectTypeV1::CodeBlock, "How did this happen?");
				obj->as.codeBlock.language = (HighlighterLanguage)oldEnum;
				obj->as.codeBlock.reInit(am, obj);
				break;
			case EnumPropType::HighlighterTheme:
				g_logger_assert(obj->objectType == AnimObjectTypeV1::CodeBlock, "How did this happen?");
				g_logger_assert(oldEnum >= 0 && oldEnum < (int)HighlighterTheme::Length, "How did this happen?");
				obj->as.codeBlock.theme = (HighlighterTheme)oldEnum;
				obj->as.codeBlock.reInit(am, obj);
				break;
			// NOTE: These are animation types, so they go in the if-block above
			case EnumPropType::EaseType:
			case EnumPropType::EaseDirection:
			case EnumPropType::PlaybackType:
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
			case FloatPropType::LagRatio:
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
			case FloatPropType::StrokeWidth:
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
			case FloatPropType::StrokeWidth:
				break;
			}
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
			}
			AnimationManager::updateObjectState(am, this->objId);
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
			}
			AnimationManager::updateObjectState(am, this->objId);
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
				g_logger_assert(obj->objectType == AnimObjectTypeV1::TextObject, "How did this happen?");
				obj->as.textObject.setText(newString);
				obj->as.textObject.reInit(am, obj);
				break;
			case StringPropType::CodeBlockText:
				g_logger_assert(obj->objectType == AnimObjectTypeV1::CodeBlock, "How did this happen?");
				obj->as.codeBlock.setText(newString);
				obj->as.codeBlock.reInit(am, obj);
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
				g_logger_assert(obj->objectType == AnimObjectTypeV1::TextObject, "How did this happen?");
				obj->as.textObject.setText(oldString);
				obj->as.textObject.reInit(am, obj);
				break;
			case StringPropType::CodeBlockText:
				g_logger_assert(obj->objectType == AnimObjectTypeV1::CodeBlock, "How did this happen?");
				obj->as.codeBlock.setText(oldString);
				obj->as.codeBlock.reInit(am, obj);
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
			if (object->as.textObject.font)
			{
				Fonts::unloadFont(object->as.textObject.font);
			}
			object->as.textObject.font = Fonts::loadFont(oldFont.c_str());
			object->as.textObject.reInit(am, object);

			AnimationManager::updateObjectState(am, objId);
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
