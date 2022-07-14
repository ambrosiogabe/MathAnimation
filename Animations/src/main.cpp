#include "core.h"
#include "core/Application.h"
#include "editor/ProjectScreen.h"

using namespace MathAnim;
int main()
{
	g_memory_init(true);

	ProjectScreen::init();
	std::string projectFile = ProjectScreen::run();
	ProjectScreen::free();

	if (projectFile != "")
	{
		Application::init(projectFile.c_str());
		Application::run();
		Application::free();
	}

	g_memory_dumpMemoryLeaks();
	return 0;
}

