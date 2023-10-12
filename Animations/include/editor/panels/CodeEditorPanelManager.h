#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;

	namespace CodeEditorPanelManager
	{
		void free();

		void update(AnimationManagerData const* am, ImGuiID parentDockId);

		void openFile(std::string const& filename);
		void closeFile(std::string const& filename);
	}
}