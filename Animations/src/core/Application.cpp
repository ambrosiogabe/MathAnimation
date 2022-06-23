#include "core/Application.h"
#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "core/ImGuiLayer.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/PerspectiveCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/VideoWriter.h"
#include "renderer/Fonts.h"
#include "renderer/Colors.h"
#include "animation/Svg.h"
#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "editor/EditorGui.h"
#include "audio/Audio.h"

#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

#include "imgui.h"

namespace MathAnim
{
	namespace Application
	{
		static AnimState animState = AnimState::Pause;
		static bool outputVideoFile = false;

		static int numFramesWritten = 0;
		static int framerate = 60;
		static int outputWidth = 3840;
		static int outputHeight = 2160;
		static NVGcontext* vg = NULL;

		static Window* window = nullptr;
		static Framebuffer mainFramebuffer;
		static OrthoCamera camera2D;
		static PerspectiveCamera camera3D;
		static int currentFrame = 0;
		static float accumulatedTime = 0.0f;

		static const char* winTitle = "Math Animations";

		void init()
		{
			// Initiaize GLFW/Glad
			window = new Window(1920, 1080, winTitle);
			window->setVSync(true);

			camera2D.position = Vec2{ 0, 0 };
			camera2D.projectionSize = Vec2{ 6.0f * (1920.0f / 1080.0f), 6.0f };
			
			camera3D.forward = glm::vec3(0, 0, 1);
			camera3D.fov = 70.0f;
			camera3D.orientation = glm::vec3(-15.0f, 45.0f, 0);
			camera3D.position = glm::vec3(-10.0f * glm::cos(glm::radians(-45.0f)), 2.5f, 10.0f * glm::sin(glm::radians(-45.0f)));

			Fonts::init();
			Renderer::init(camera2D, camera3D);
			ImGuiLayer::init(*window);
			Audio::init();
			// NOTE(voxel): Just to initialize the camera
			Svg::init(camera2D);
			TextAnimations::init(camera2D);

			vg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
			if (vg == NULL)
			{
				g_logger_error("Failed to initialize nanovg.");
				return;
			}

			mainFramebuffer = AnimationManager::prepareFramebuffer(outputWidth, outputHeight);

			AnimationManager::deserialize("./myScene.bin");

			EditorGui::init();

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		void run()
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
				window->setTitle(winTitle + std::string(" -- ") + std::to_string(deltaTime));

				// Bind main framebuffer
				mainFramebuffer.bind();
				glViewport(0, 0, mainFramebuffer.width, mainFramebuffer.height);
				Renderer::clearColor(colors[(uint8)Color::GreenBrown]);

				nvgBeginFrame(vg, (float)mainFramebuffer.width, (float)mainFramebuffer.height, 1.0f);

				// Update components
				if (animState == AnimState::PlayForward)
				{
					if (outputVideoFile)
					{
						if (numFramesWritten % 60 == 0)
						{
							g_logger_info("Number of seconds rendered: %d", numFramesWritten / 60);
						}
						numFramesWritten++;
					}

					accumulatedTime += deltaTime;
					currentFrame = (int)(accumulatedTime * 60.0f);
				}
				else if (animState == AnimState::PlayReverse)
				{
					currentFrame = glm::max(currentFrame - 1, 0);
				}

				// Render to main framebuffer
				Renderer::renderToFramebuffer(vg, currentFrame, mainFramebuffer);
				mainFramebuffer.unbind();

				// Render to window
				glViewport(0, 0, window->width, window->height);
				Renderer::renderFramebuffer(mainFramebuffer);

				// Do ImGui stuff
				ImGuiLayer::beginFrame();
				ImGui::ShowDemoWindow();
				EditorGui::update(mainFramebuffer.getColorAttachment(0).graphicsId);
				ImGuiLayer::endFrame();

				// Miscellaneous
				// TODO: Abstract this stuff out of here
				if (outputVideoFile)
				{
					Pixel* pixels = mainFramebuffer.readAllPixelsRgb8(0);
					VideoWriter::pushFrame(pixels, outputHeight * outputWidth);
					mainFramebuffer.freePixels(pixels);
				}

				if (Input::isKeyPressed(GLFW_KEY_F7) && !outputVideoFile)
				{
					currentFrame = 0;
					outputVideoFile = true;
					VideoWriter::startEncodingFile("output.mp4", outputWidth, outputHeight, framerate);
				}
				else if (Input::isKeyPressed(GLFW_KEY_F8) && outputVideoFile)
				{
					outputVideoFile = false;
					VideoWriter::finishEncodingFile();
				}

				window->swapBuffers();
			}
		}

		void free()
		{
			AnimationManager::serialize("./myScene.bin");

			Fonts::unloadAllFonts();
			EditorGui::free();
			nvgDeleteGL3(vg);
			Renderer::free();
			Audio::free();

			ImGuiLayer::free();
			Window::cleanup();
		}

		void setEditorPlayState(AnimState state)
		{
			if (state == AnimState::PlayForward || state == AnimState::PlayReverse)
			{
				accumulatedTime = (float)currentFrame / 60.0f;
			}
			animState = state;
		}

		AnimState getEditorPlayState()
		{
			return animState;
		}

		float getOutputTargetAspectRatio()
		{
			return (float)outputWidth / (float)outputHeight;
		}

		glm::vec2 getOutputSize()
		{
			return glm::vec2((float)outputWidth, (float)outputHeight);
		}

		void setFrameIndex(int frame)
		{
			currentFrame = frame;
		}

		int getFrameIndex()
		{
			return currentFrame;
		}

		int getFrameratePerSecond()
		{
			return framerate;
		}

		NVGcontext* getNvgContext()
		{
			return vg;
		}
	}
}