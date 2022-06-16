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
		static Texture mainTexture;
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
			camera3D.orientation = glm::vec3(0, 45.0f, 0);
			camera3D.position = glm::vec3(0, 1, -10);

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

			mainTexture = TextureBuilder()
				.setFormat(ByteFormat::RGBA8_UI)
				.setMinFilter(FilterMode::Linear)
				.setMagFilter(FilterMode::Linear)
				.setWidth(outputWidth)
				.setHeight(outputHeight)
				.build();

			mainFramebuffer = FramebufferBuilder(outputWidth, outputHeight)
				.addColorAttachment(mainTexture)
				.includeDepthStencil()
				.generate();

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
				AnimationManager::render(vg, currentFrame);
				nvgEndFrame(vg);

				Renderer::pushStrokeWidth(0.1f);
				Renderer::pushColor(colors[(int)Color::Red]);

				Renderer::beginPath3D(Vec3{ 0, 0, 0 });
				Renderer::lineTo3D(Vec3{ 0, 1, 0 });
				Renderer::lineTo3D(Vec3{ 1, 1, 0 });
				Renderer::lineTo3D(Vec3{ 1, 0, 0 });
				Renderer::endPath3D();

				Renderer::beginPath3D(Vec3{ 0, 0, 0 });
				Renderer::lineTo3D(Vec3{ 0, 1, 0 });
				Renderer::lineTo3D(Vec3{ 0, 1, 1 });
				Renderer::lineTo3D(Vec3{ 0, 0, 1 });
				Renderer::endPath3D();

				Renderer::beginPath3D(Vec3{ 1, 0, 1 });
				Renderer::lineTo3D(Vec3{ 1, 1, 1 });
				Renderer::lineTo3D(Vec3{ 1, 1, 0 });
				Renderer::lineTo3D(Vec3{ 1, 0, 0 });
				Renderer::endPath3D();

				Renderer::beginPath3D(Vec3{ 1, 0, 1 });
				Renderer::lineTo3D(Vec3{ 1, 1, 1 });
				Renderer::lineTo3D(Vec3{ 0, 1, 1 });
				Renderer::lineTo3D(Vec3{ 0, 0, 1 });
				Renderer::endPath3D();

				Renderer::popColor();
				Renderer::popStrokeWidth();

				camera3D.orientation.y += 30.0f * deltaTime;
				camera3D.position.x = -glm::cos(glm::radians(-camera3D.orientation.y)) * 10.0f;
				camera3D.position.z = glm::sin(glm::radians(-camera3D.orientation.y)) * 10.0f;

				Renderer::render();

				// Render to screen
				mainFramebuffer.unbind();
				glViewport(0, 0, window->width, window->height);
				Renderer::renderFramebuffer(mainFramebuffer);

				ImGuiLayer::beginFrame();

				// Do ImGui stuff
				ImGui::ShowDemoWindow();
				EditorGui::update(mainFramebuffer.getColorAttachment(0).graphicsId);

				ImGuiLayer::endFrame();

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