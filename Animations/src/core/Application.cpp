#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/Shader.h"
#include "animation/Animation.h"
#include "animation/GridLines.h"
#include "animation/Settings.h"
#include "animation/Sandbox.h"

namespace MathAnim
{
	namespace Application
	{
		bool playAnim = false;
		void run()
		{
			// Initiaize GLFW/Glad
			bool isRunning = true;

			Window window = Window(1920, 1080, "Math Animations");
			window.setVSync(true);

			OrthoCamera camera;
			camera.position = glm::vec2(0, 0);
			camera.projectionSize = glm::vec2(6.0f * (1920.0f / 1080.0f), 6.0f);

			Renderer::init(camera);
			Sandbox::init();

			// Run game loop
			// Start with a 60 fps frame rate
			float previousTime = glfwGetTime() - 0.16f;
			while (isRunning && !window.shouldClose())
			{
				float deltaTime = glfwGetTime() - previousTime;
				previousTime = glfwGetTime();

				// Update components
				if (Settings::displayGrid)
				{
					GridLines::update(camera);
				}

				if (playAnim)
				{
					Sandbox::update(deltaTime);
					AnimationManager::update(deltaTime);
				}

				// Render
				Renderer::render();
				window.swapBuffers();

				if (Input::isKeyPressed(GLFW_KEY_ESCAPE))
				{
					isRunning = false;
				}
				else if (Input::isKeyPressed(GLFW_KEY_SPACE))
				{
					playAnim = true;
				}

				window.pollInput();
			}

			Window::cleanup();
		}
	}
}