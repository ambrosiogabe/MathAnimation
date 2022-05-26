#include "core.h"
#include "core/Application.h"

using namespace MathAnim;
int main()
{
	g_memory_init(true);

	Application::init();
	Application::run();
	Application::free();

	g_memory_dumpMemoryLeaks();
	return 0;
}

