#include "core.h"
#include "core/Application.h"
#include "core/ProjectApp.h"

using namespace MathAnim;

int main()
{
	g_logger_init();
	g_memory_init_padding(true, 5);

	ProjectApp::init();
	std::string projectFile = ProjectApp::run();
	ProjectApp::free();

	if (projectFile != "")
	{
		Application::init(projectFile.c_str());
		Application::run();
		Application::free();
	}

	g_memory_dumpMemoryLeaks();
	return 0;
}
