#ifndef _MATH_ANIM_TESTS
#include "core.h"
#include "core/Application.h"
#include "core/ProjectApp.h"

using namespace MathAnim;

int main(int argc, char* argv[])
{
	g_logger_init();
	g_memory_init_padding(true, 5);

	std::string projectFile = "";
	if (argc < 2)
	{
		ProjectApp::init();
		projectFile = ProjectApp::run();
		ProjectApp::free();
	}
	else
	{
		projectFile = argv[1];
	}
	
	if (projectFile != "")
	{
		Application::init(projectFile.c_str());
		// Application::run();
		// Application::free();
	}
	
	// g_memory_dumpMemoryLeaks();
	return 0;
}

#endif