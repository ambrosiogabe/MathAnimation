#include "editor/TextEditUndo.h"
#include "editor/UndoSystem.h"
#include "editor/panels/CodeEditorPanel.h"

namespace MathAnim
{
	struct TextEditorUndoSystem
	{
		CodeEditorPanelData* codeEditor;
		UndoSystemData* genericSystem;
		size_t totalAllocatedMemory;
	};

	class InsertTextCommand : public Command
	{
	public:
		InsertTextCommand(uint8* inTextToInsert, size_t textToInsertSize, size_t insertOffset)
			: textToInsertSize(textToInsertSize), insertOffset(insertOffset)
		{
			this->textToInsert = (uint8*)g_memory_allocate(textToInsertSize);
			g_memory_copyMem(this->textToInsert, textToInsertSize, inTextToInsert, textToInsertSize);
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

	private:
		uint8* textToInsert;
		size_t textToInsertSize;
		size_t insertOffset;
	};

	namespace UndoSystem
	{
		TextEditorUndoSystem* createTextEditorUndoSystem(CodeEditorPanelData* const codeEditor, int maxHistory)
		{
			auto* res = (TextEditorUndoSystem*)g_memory_allocate(sizeof(TextEditorUndoSystem));
			g_memory_zeroMem(res, sizeof(TextEditorUndoSystem));

			res->codeEditor = codeEditor;
			res->genericSystem = UndoSystem::init(res, maxHistory);
			res->totalAllocatedMemory = 0;

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

		void insertTextAction(TextEditorUndoSystem* us, uint8* text, size_t textSize, size_t textOffset)
		{
			auto* newCommand = g_memory_new InsertTextCommand(text, textSize, textOffset);
			pushCommand(us->genericSystem, newCommand);
		}

		void deleteTextAction(TextEditorUndoSystem* us, size_t deleteOffset, size_t deleteNumBytes)
		{
			// TODO: Remove me, compiler warning suppression for now
			*us;
			deleteOffset++;
			deleteNumBytes++;
		}

		void imguiStats(TextEditorUndoSystem const* undoSystem)
		{
			// TODO: Remove me, compiler warning suppression for now
			*undoSystem;
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

		CodeEditorPanel::removeTextWithBackspace(*us->codeEditor, (int32)this->insertOffset, (int32)this->textToInsertSize);
	}
}