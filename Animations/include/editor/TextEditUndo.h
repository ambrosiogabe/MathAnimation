#ifndef MATH_ANIM_TEXT_EDIT_UNDO_H
#define MATH_ANIM_TEXT_EDIT_UNDO_H
#include "core.h"

namespace MathAnim
{
	struct TextEditorUndoSystem;
	struct CodeEditorPanelData;

	namespace UndoSystem
	{
		TextEditorUndoSystem* createTextEditorUndoSystem(CodeEditorPanelData* const codeEditor, int maxHistory);
		void free(TextEditorUndoSystem* us);

		void undo(TextEditorUndoSystem* us);
		void redo(TextEditorUndoSystem* us);

		void insertTextAction(TextEditorUndoSystem* us, uint8* text, size_t textSize, size_t insertOffset);
		void deleteTextAction(TextEditorUndoSystem* us, size_t deleteSize, size_t deleteOffset);

		void imguiStats(TextEditorUndoSystem const* undoSystem);
	}
}

#endif 