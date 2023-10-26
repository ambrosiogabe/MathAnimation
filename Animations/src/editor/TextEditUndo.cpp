#include "editor/TextEditUndo.h"
#include "editor/UndoSystem.h"
#include "editor/panels/CodeEditorPanel.h"

namespace MathAnim
{
	struct TextEditorUndoSystem
	{
		CodeEditorPanelData* codeEditor;
		UndoSystemData* genericSystem;
		size_t totalMemoryAllocated;
	};

	class InsertTextCommand : public Command
	{
	public:
		InsertTextCommand(uint8 const* inTextToInsert, size_t textToInsertSize, size_t insertOffset, size_t numberCharactersInserted)
			: textToInsertSize(textToInsertSize), insertOffset(insertOffset), numberCharactersInserted(numberCharactersInserted)
		{
			this->textToInsert = (uint8*)g_memory_allocate(textToInsertSize);
			g_memory_copyMem(this->textToInsert, textToInsertSize, (void*)inTextToInsert, textToInsertSize);
		}

		virtual void execute(void* ctx) override;
		virtual void undo(void* ctx) override;

		virtual ~InsertTextCommand() override
		{
			g_memory_free(textToInsert);
			textToInsert = nullptr;
			textToInsertSize = 0;
			insertOffset = 0;
		}

		virtual void free(void* ctx) override
		{
			auto* us = (TextEditorUndoSystem*)ctx;
			us->totalMemoryAllocated -= textToInsertSize;
		}

	private:
		uint8* textToInsert;
		size_t textToInsertSize;
		size_t insertOffset;
		size_t numberCharactersInserted;
	};

	class BackspaceTextCommand : public Command
	{
	public:
		BackspaceTextCommand(uint8 const* inTextToDelete, size_t textToBeDeletedSize, size_t selectionStart, size_t selectionEnd, size_t cursorPosition, bool shouldSetTextSelected)
			: textToBeDeletedSize(textToBeDeletedSize), selectionStart(selectionStart), selectionEnd(selectionEnd), cursorPosition(cursorPosition), shouldSetTextSelected(shouldSetTextSelected)
		{
			this->textToBeDeleted = (uint8*)g_memory_allocate(textToBeDeletedSize);
			g_memory_copyMem(this->textToBeDeleted, textToBeDeletedSize, (void*)inTextToDelete, textToBeDeletedSize);
		}

		virtual void execute(void* ctx) override;
		virtual void undo(void* ctx) override;

		virtual ~BackspaceTextCommand() override
		{
			g_memory_free(textToBeDeleted);
			textToBeDeleted = nullptr;
			textToBeDeletedSize = 0;
			selectionStart = 0;
			selectionEnd = 0;
		}

		virtual void free(void* ctx) override
		{
			auto* us = (TextEditorUndoSystem*)ctx;
			us->totalMemoryAllocated -= textToBeDeletedSize;
		}

	private:
		uint8* textToBeDeleted;
		size_t textToBeDeletedSize;
		size_t selectionStart;
		size_t selectionEnd;
		size_t cursorPosition;
		bool shouldSetTextSelected;
	};

	class DeleteTextCommand : public Command
	{
	public:
		DeleteTextCommand(uint8 const* inTextToDelete, size_t textToBeDeletedSize, size_t selectionStart, size_t selectionEnd, size_t cursorPosition, bool shouldSetTextSelected)
			: textToBeDeletedSize(textToBeDeletedSize), selectionStart(selectionStart), selectionEnd(selectionEnd), cursorPosition(cursorPosition), shouldSetTextSelected(shouldSetTextSelected)
		{
			this->textToBeDeleted = (uint8*)g_memory_allocate(textToBeDeletedSize);
			g_memory_copyMem(this->textToBeDeleted, textToBeDeletedSize, (void*)inTextToDelete, textToBeDeletedSize);
		}

		virtual void execute(void* ctx) override;
		virtual void undo(void* ctx) override;

		virtual ~DeleteTextCommand() override
		{
			g_memory_free(textToBeDeleted);
			textToBeDeleted = nullptr;
			textToBeDeletedSize = 0;
			selectionStart = 0;
			selectionEnd = 0;
		}

		virtual void free(void* ctx) override
		{
			auto* us = (TextEditorUndoSystem*)ctx;
			us->totalMemoryAllocated -= textToBeDeletedSize;
		}

	private:
		uint8* textToBeDeleted;
		size_t textToBeDeletedSize;
		size_t selectionStart;
		size_t selectionEnd;
		size_t cursorPosition;
		bool shouldSetTextSelected;
	};

	namespace UndoSystem
	{
		TextEditorUndoSystem* createTextEditorUndoSystem(CodeEditorPanelData* const codeEditor, int maxHistory)
		{
			auto* res = (TextEditorUndoSystem*)g_memory_allocate(sizeof(TextEditorUndoSystem));
			g_memory_zeroMem(res, sizeof(TextEditorUndoSystem));

			res->codeEditor = codeEditor;
			res->genericSystem = UndoSystem::init(res, maxHistory);

			return res;
		}

		void free(TextEditorUndoSystem* us)
		{
			UndoSystem::free(us->genericSystem);
			g_memory_free(us);

			if (us)
			{
				g_memory_zeroMem(us, sizeof(TextEditorUndoSystem));
			}
		}

		void undo(TextEditorUndoSystem* us)
		{
			UndoSystem::undo(us->genericSystem);
		}

		void redo(TextEditorUndoSystem* us)
		{
			UndoSystem::redo(us->genericSystem);
		}

		void insertTextAction(TextEditorUndoSystem* us, uint8 const* text, size_t textSize, size_t textOffset, size_t numberCharactersInserted)
		{
			auto* newCommand = g_memory_new InsertTextCommand(text, textSize, textOffset, numberCharactersInserted);
			pushCommand(us->genericSystem, newCommand);
			us->totalMemoryAllocated += textSize;
		}

		void deleteTextAction(TextEditorUndoSystem* us, uint8 const* textToBeDeleted, size_t textToBeDeletedLength, size_t selectionStart, size_t selectionEnd, size_t cursorPosition, bool shouldSetTextSelected)
		{
			// TODO: Merge delete commands somehow
			//       If two consecutive backspaces happen at the same position, they should be merged
			//       However, if the backspace is at a different position, they should remain distinct
			auto* newCommand = g_memory_new DeleteTextCommand(textToBeDeleted, textToBeDeletedLength, selectionStart, selectionEnd, cursorPosition, shouldSetTextSelected);
			pushCommand(us->genericSystem, newCommand);
			us->totalMemoryAllocated += textToBeDeletedLength;
		}

		void backspaceTextAction(TextEditorUndoSystem* us, uint8 const* textToBeDeleted, size_t textToBeDeletedLength, size_t selectionStart, size_t selectionEnd, size_t cursorPosition, bool shouldSetTextSelected)
		{
			// TODO: Merge backspace commands somehow
			//       If two consecutive backspaces happen at the same position, they should be merged
			//       However, if the backspace is at a different position, they should remain distinct
			auto* newCommand = g_memory_new BackspaceTextCommand(textToBeDeleted, textToBeDeletedLength, selectionStart, selectionEnd, cursorPosition, shouldSetTextSelected);
			pushCommand(us->genericSystem, newCommand);
			us->totalMemoryAllocated += textToBeDeletedLength;
		}

		void imguiStats(TextEditorUndoSystem const* undoSystem)
		{
			const char* suffix = "B";
			size_t totalMemoryAllocated = undoSystem->totalMemoryAllocated;
			size_t remainder = 0;
			size_t remainderPower = 0;
			if (totalMemoryAllocated >= 1024)
			{
				// Divide by 1024
				size_t newAllocation = totalMemoryAllocated >> 10;
				remainder += totalMemoryAllocated - (newAllocation << 10);
				totalMemoryAllocated = newAllocation;
				suffix = "KB";
				remainderPower += 10;
			}

			if (totalMemoryAllocated >= 1024)
			{
				// Divide by 1024
				size_t newAllocation = totalMemoryAllocated >> 10;
				remainder += totalMemoryAllocated - (newAllocation << 10);
				totalMemoryAllocated = newAllocation;
				suffix = "MB";
				remainderPower += 10;
			}

			if (totalMemoryAllocated >= 1024)
			{
				// Divide by 1024
				size_t newAllocation = totalMemoryAllocated >> 10;
				remainder += totalMemoryAllocated - (newAllocation << 10);
				totalMemoryAllocated = newAllocation;
				suffix = "GB";
				remainderPower += 10;
			}

			float remainderFloat = (float)remainder / (float)(1 << remainderPower);
			// Integer with 2 decimal places of precision
			int integerRemainder = (int)(remainderFloat * 100.0f);

			ImGui::Text("Total Allocated Memory: %d.%d%s", totalMemoryAllocated, integerRemainder, suffix);
		}
	}


	void InsertTextCommand::execute(void* ctx)
	{
		auto* us = (TextEditorUndoSystem*)ctx;

		CodeEditorPanel::addUtf8StringToBuffer(*us->codeEditor, this->textToInsert, (int32)this->textToInsertSize, (int32)this->insertOffset);
	}

	void InsertTextCommand::undo(void* ctx)
	{
		auto* us = (TextEditorUndoSystem*)ctx;

		CodeEditorPanel::removeTextWithBackspace(*us->codeEditor, (int32)this->insertOffset, (int32)this->numberCharactersInserted);
	}

	void BackspaceTextCommand::execute(void* ctx)
	{
		auto* us = (TextEditorUndoSystem*)ctx;

		CodeEditorPanel::removeTextWithBackspace(*us->codeEditor, (int32)this->selectionStart, (int32)(this->selectionEnd - this->selectionStart));
	}

	void BackspaceTextCommand::undo(void* ctx)
	{
		auto* us = (TextEditorUndoSystem*)ctx;

		CodeEditorPanel::addUtf8StringToBuffer(*us->codeEditor, this->textToBeDeleted, (int32)this->textToBeDeletedSize, (int32)this->selectionStart);
		us->codeEditor->cursor.bytePos = (int32)this->cursorPosition;
		if (this->shouldSetTextSelected)
		{
			us->codeEditor->firstByteInSelection = (int32)this->selectionStart;
			us->codeEditor->lastByteInSelection = (int32)this->selectionEnd;
			us->codeEditor->mouseByteDragStart = this->cursorPosition == this->selectionEnd ? (int32)this->selectionStart : (int32)this->selectionEnd;
		}
		else
		{
			us->codeEditor->firstByteInSelection = (int32)us->codeEditor->cursor.bytePos;
			us->codeEditor->lastByteInSelection = (int32)us->codeEditor->cursor.bytePos;
			us->codeEditor->mouseByteDragStart = (int32)us->codeEditor->cursor.bytePos;
		}
	}

	void DeleteTextCommand::execute(void* ctx)
	{
		auto* us = (TextEditorUndoSystem*)ctx;

		CodeEditorPanel::removeTextWithDelete(*us->codeEditor, (int32)this->selectionStart, (int32)(this->selectionEnd - this->selectionStart));
	}

	void DeleteTextCommand::undo(void* ctx)
	{
		auto* us = (TextEditorUndoSystem*)ctx;

		CodeEditorPanel::addUtf8StringToBuffer(*us->codeEditor, this->textToBeDeleted, (int32)this->textToBeDeletedSize, (int32)this->selectionStart);
		us->codeEditor->cursor.bytePos = (int32)this->cursorPosition;
		if (this->shouldSetTextSelected)
		{
			us->codeEditor->firstByteInSelection = (int32)this->selectionStart;
			us->codeEditor->lastByteInSelection = (int32)this->selectionEnd;
			us->codeEditor->mouseByteDragStart = this->cursorPosition == this->selectionEnd ? (int32)this->selectionStart : (int32)this->selectionEnd;
		}
		else
		{
			us->codeEditor->firstByteInSelection = (int32)us->codeEditor->cursor.bytePos;
			us->codeEditor->lastByteInSelection = (int32)us->codeEditor->cursor.bytePos;
			us->codeEditor->mouseByteDragStart = (int32)us->codeEditor->cursor.bytePos;
		}
	}
}