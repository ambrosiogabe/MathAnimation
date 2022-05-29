#include "core/Application.h"
#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "core/ImGuiLayer.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/VideoWriter.h"
#include "renderer/Fonts.h"
#include "animation/Animation.h"
#include "animation/GridLines.h"
#include "animation/Settings.h"
#include "animation/Sandbox.h"
#include "animation/Styles.h"
#include "editor/EditorGui.h"

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
		Font* baskVillFont;

		static Window* window = nullptr;
		static Framebuffer mainFramebuffer;
		static Texture mainTexture;
		static OrthoCamera camera;
		static float timeAccumulation = 0.0f;

		static const char* winTitle = "Math Animations";

		void init()
		{
			// Initiaize GLFW/Glad
			window = new Window(1920, 1080, winTitle);
			window->setVSync(true);

			camera.position = Vec2{ 0, 0 };
			camera.projectionSize = Vec2{ 6.0f * (1920.0f / 1080.0f), 6.0f };

			Fonts::init();
			Renderer::init(camera);
			Sandbox::init();
			ImGuiLayer::init(*window);
			EditorGui::init();

			vg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
			if (vg == NULL)
			{
				g_logger_error("Failed to initialize nanovg.");
				return;
			}

			baskVillFont = Fonts::loadFont("C:/Windows/Fonts/BASKVILL.TTF", vg);
			if (baskVillFont == nullptr)
			{
				g_logger_error("Failed to load BASKVILL.ttf");
				return;
			}

			mainTexture = TextureBuilder()
				.setFormat(ByteFormat::RGBA8_UI)
				.setWidth(outputWidth)
				.setHeight(outputHeight)
				.build();

			mainFramebuffer = FramebufferBuilder(outputWidth, outputHeight)
				.addColorAttachment(mainTexture)
				.includeDepthStencil()
				.generate();

			AnimObject textObject;
			textObject.id = 0;
			textObject.objectType = AnimObjectType::TextObject;
			textObject.duration = 12.0f;
			textObject.startTime = 3.0f;
			textObject.as.textObject.font = baskVillFont;
			textObject.as.textObject.fontSizePixels = 500.0f;
			textObject.as.textObject.text = "Hello World!";

			glm::vec2 textSize = baskVillFont->getSizeOfString(textObject.as.textObject.text, textObject.as.textObject.fontSizePixels);
			glm::vec2 centeredText = glm::vec2(mainFramebuffer.width / 2.0f, mainFramebuffer.height / 2.0f) - (textSize * 0.5f);

			textObject.position = Vec2{ centeredText.x, centeredText.y };
			AnimationManagerEx::addAnimObject(textObject);

			AnimationEx animObject;
			animObject.objectId = 0;
			animObject.duration = 3.0f;
			animObject.startTime = 0.0f;
			animObject.type = AnimTypeEx::WriteInText;
			AnimationManagerEx::addAnimation(animObject);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		void run()
		{
			// Run game loop
			// Start with a 60 fps frame rate
			bool isRunning = true;
			float previousTime = glfwGetTime() - 0.16f;
			constexpr float fixedDeltaTime = 1.0f / 60.0f;
			while (isRunning && !window->shouldClose())
			{
				float deltaTime = glfwGetTime() - previousTime;
				previousTime = glfwGetTime();

				window->pollInput();
				window->setTitle(winTitle + std::string(" -- ") + std::to_string(deltaTime));

				// Bind main framebuffer
				mainFramebuffer.bind();
				glViewport(0, 0, mainFramebuffer.width, mainFramebuffer.height);
				Renderer::clearColor(Colors::greenBrown);

				nvgBeginFrame(vg, (float)mainFramebuffer.width, (float)mainFramebuffer.height, 1.0f);

				// Update components
				if (Settings::displayGrid)
				{
					GridLines::update(camera);
				}

				if (animState == AnimState::PlayForward)
				{
					float customDeltaTime = deltaTime;
					if (outputVideoFile)
					{
						customDeltaTime = 1.0f / 60.0f;
						if (numFramesWritten % 60 == 0)
						{
							g_logger_info("Number of seconds rendered: %d", numFramesWritten / 60);
						}
						numFramesWritten++;
					}
					//AnimationManager::update(customDeltaTime);
					//Sandbox::update(customDeltaTime);

					timeAccumulation += fixedDeltaTime;
				}
				else if (animState == AnimState::PlayReverse)
				{
					timeAccumulation -= fixedDeltaTime;
					timeAccumulation = glm::max(timeAccumulation, 0.0f);
				}

				AnimationManagerEx::render(vg, timeAccumulation);

				nvgEndFrame(vg);

				// Render to main framebuffer
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

				if (Input::isKeyPressed(GLFW_KEY_ESCAPE))
				{
					isRunning = false;
				}
				else if (Input::isKeyPressed(GLFW_KEY_F1))
				{
					animState = AnimState::PlayReverse;
				}
				else if (Input::isKeyPressed(GLFW_KEY_F2))
				{
					animState = AnimState::PlayForward;
				}
				else if (Input::isKeyPressed(GLFW_KEY_F3))
				{
					animState = AnimState::Pause;
				}
				else if (Input::isKeyPressed(GLFW_KEY_F4))
				{
					animState = AnimState::PlayForward;
					timeAccumulation = 0.0f;
				}
				else if (Input::isKeyPressed(GLFW_KEY_F7) && !outputVideoFile)
				{
					timeAccumulation = 0.0f;
					outputVideoFile = true;
					AnimationManager::reset();
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
			EditorGui::free();
			Fonts::unloadFont(baskVillFont);
			nvgDeleteGL3(vg);

			ImGuiLayer::free();
			Window::cleanup();
		}

		void setEditorPlayState(AnimState state)
		{
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

		void setFrameIndex(int frame)
		{
			timeAccumulation = (float)frame / 60.0f;
		}

		int getFrameIndex()
		{
			return (int)glm::round(timeAccumulation * 60.0f);
		}
	}
}