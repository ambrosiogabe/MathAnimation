#include "editor/ProjectScreen.h"
#include "core.h"
#include "core/Window.h"
#include "core/GladLayer.h"
#include "core/ImGuiLayer.h"
#include "multithreading/GlobalThreadPool.h"

#include <imgui.h>

namespace MathAnim
{
	namespace ProjectScreen
	{
		static GlobalThreadPool* globalThreadPool = nullptr;
		static Window* window = nullptr;

		static const char* winTitle = "Math Animations -- Project Selector";

		void init()
		{
			globalThreadPool = new GlobalThreadPool(std::thread::hardware_concurrency());

			// Initiaize GLFW/Glad
			window = new Window(1920, 1080, winTitle);
			window->setVSync(true);

			GladLayer::init();
			ImGuiLayer::init(*window);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// TODO: Initialize app settings
		}

		std::string run()
		{
			// Run game loop
			// Start with a 60 fps frame rate
			bool isRunning = true;
			double previousTime = glfwGetTime() - 0.16f;
			constexpr float fixedDeltaTime = 1.0f / 60.0f;
			while (isRunning && !window->shouldClose())
			{
				float deltaTime = (float)(glfwGetTime() - previousTime);
				previousTime = glfwGetTime();

				window->pollInput();

				glViewport(0, 0, window->width, window->height);
				glClearColor(0, 0, 0, 0);

				// Do ImGui stuff
				ImGuiLayer::beginFrame();
				ImGui::ShowDemoWindow();
				
				ImGuiLayer::endFrame();

				window->swapBuffers();
			}

			return "./myScene.bin";
		}

		void free()
		{
			ImGuiLayer::free();
			Window::cleanup();
			globalThreadPool->free();
		}
	}
}