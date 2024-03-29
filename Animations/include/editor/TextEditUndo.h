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

		void insertTextAction(TextEditorUndoSystem* us, uint8 const* utf8String, size_t utf8StringNumBytes, size_t insertOffset, size_t numberCharactersInserted);
		void deleteTextAction(TextEditorUndoSystem* us, uint8 const* textToBeDeleted, size_t textToBeDeletedLength, size_t selectionStart, size_t selectionEnd, size_t cursorPosition, bool shouldSetTextSelected);
		void backspaceTextAction(TextEditorUndoSystem* us, uint8 const* textToBeDeleted, size_t textToBeDeletedLength, size_t selectionStart, size_t selectionEnd, size_t cursorPosition, bool shouldSetTextSelected);

		void imguiStats(TextEditorUndoSystem const* undoSystem);
	}
}

#endif 