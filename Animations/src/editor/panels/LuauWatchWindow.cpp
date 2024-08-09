#include "editor/panels/LuauWatchWindow.h"

namespace MathAnim
{
	namespace LuauWatchWindow
	{
		static bool shouldShowWindow = false;

		void update()
		{
			if (!shouldShowWindow)
			{
				return;
			}

			ImGui::Begin("Watch Window", &shouldShowWindow);

			ImGui::End();
		}

		void showWindow()
		{
			shouldShowWindow = true;
		}
	}
}
