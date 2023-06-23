#include "editor/UndoSystem.h"
#include "animation/AnimationManager.h"
#include "animation/Animation.h"

namespace MathAnim
{
	class Command
	{
	public:
		Command() {}
		virtual ~Command() {};

		virtual void execute(AnimationManagerData* const am) = 0;
		virtual void undo(AnimationManagerData* const am) = 0;
		virtual void free() = 0;
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
	class ModifyFillColorCommand : public Command
	{
	public:
		ModifyFillColorCommand(AnimObjId objId, const glm::u8vec4& oldColor, const glm::u8vec4& newColor)
			: objId(objId), oldColor(oldColor), newColor(newColor)
		{
		}

		virtual void execute(AnimationManagerData* const am) override;
		virtual void undo(AnimationManagerData* const am) override;
		virtual void free() override {}

	private:
		AnimObjId objId;
		glm::u8vec4 oldColor;
		glm::u8vec4 newColor;
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

				us->history[i]->free();
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

		void setObjFillColor(UndoSystemData* us, AnimObjId objId, const glm::u8vec4& oldColor, const glm::u8vec4& newColor)
		{
			const AnimObject* obj = AnimationManager::getObject(us->am, objId);
			if (obj)
			{
				ModifyFillColorCommand* newCommand = (ModifyFillColorCommand*)g_memory_allocate(sizeof(ModifyFillColorCommand));
				new(newCommand)ModifyFillColorCommand(objId, oldColor, newColor);
				pushAndExecuteCommand(us, newCommand);
			}

		}

#pragma warning( push )
#pragma warning( disable : 4100 )
		void setObjStrokeColor(UndoSystemData* us, AnimObjId objId, const Vec4& newColor)
		{

		}

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

					us->history[i]->free();
					g_memory_free(us->history[i]);
					numCommandsRemoved++;
				}

				// Set the number of commands to the appropriate number now
				us->numCommands -= numCommandsRemoved;
			}

			if (((us->undoCursorTail + 1) % us->maxHistorySize) == us->undoCursorHead)
			{
				us->history[us->undoCursorHead]->free();
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
	void ModifyFillColorCommand::execute(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			obj->_fillColorStart = this->newColor;
			AnimationManager::updateObjectState(am, this->objId);
		}
	}

	void ModifyFillColorCommand::undo(AnimationManagerData* const am)
	{
		AnimObject* obj = AnimationManager::getMutableObject(am, this->objId);
		if (obj)
		{
			obj->_fillColorStart = this->oldColor;
			AnimationManager::updateObjectState(am, this->objId);
		}
	}
}
