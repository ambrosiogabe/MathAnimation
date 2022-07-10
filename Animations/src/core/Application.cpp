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
#include "animation/Svg.h"
#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "editor/EditorGui.h"
#include "audio/Audio.h"
#include "latex/LaTexLayer.h"
#include "multithreading/GlobalThreadPool.h"

#include "animation/SvgParser.h"

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

		static GlobalThreadPool* globalThreadPool = nullptr;
		static Window* window = nullptr;
		static Framebuffer mainFramebuffer;
		static OrthoCamera camera2D;
		static PerspectiveCamera camera3D;
		static int currentFrame = 0;
		static float accumulatedTime = 0.0f;

		static const char* winTitle = "Math Animations";
		static const char* testLatex = R"raw(\sum_{n=5}^{\infty} n!)raw";
		static SvgObject* tmpObject = nullptr;
		static AnimObject tmpParent;

		void init()
		{
			globalThreadPool = new GlobalThreadPool(std::thread::hardware_concurrency());
			//globalThreadPool = new GlobalThreadPool(true);

			// Initiaize GLFW/Glad
			window = new Window(1920, 1080, winTitle);
			window->setVSync(true);

			camera2D.position = Vec2{ 1920.0f, 1080.0f };
			//camera2D.projectionSize = Vec2{ 6.0f * (1920.0f / 1080.0f), 6.0f };
			camera2D.projectionSize = Vec2{ 3840.0f, 2160.0f };

			camera3D.forward = glm::vec3(0, 0, 1);
			camera3D.fov = 70.0f;
			camera3D.orientation = glm::vec3(-15.0f, 80.0f, 0);
			camera3D.position = glm::vec3(
				-10.0f * glm::cos(glm::radians(-camera3D.orientation.y)), 
				2.5f, 
				10.0f * glm::sin(glm::radians(-camera3D.orientation.y))
			);

			Fonts::init();
			Renderer::init(camera2D, camera3D);
			ImGuiLayer::init(*window);
			Audio::init();
			// NOTE(voxel): Just to initialize the camera
			Svg::init(camera2D);
			TextAnimations::init(camera2D);
			AnimationManager::init(camera2D);

			vg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
			if (vg == NULL)
			{
				g_logger_error("Failed to initialize nanovg.");
				return;
			}

			LaTexLayer::init();

			mainFramebuffer = AnimationManager::prepareFramebuffer(outputWidth, outputHeight);

			AnimationManager::deserialize("./myScene.bin");

			EditorGui::init();

			LaTexLayer::laTexToSvg(testLatex, true);
			LaTexLayer::laTexToSvg(R"raw(\sum_{n=5}^{\infty} n!)raw", true);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			tmpParent = AnimObject::createDefault(AnimObjectTypeV1::LaTexObject, 0, 3000);
			tmpParent.objectType = AnimObjectTypeV1::LaTexObject;
			tmpParent._positionStart.x = 300.0f;
			tmpParent._positionStart.y = 700.0f;
			tmpParent._positionStart.z = 0.0f;
			tmpParent._strokeWidthStart = 5.0f;
			tmpParent._strokeColorStart.r = 255;
			tmpParent._strokeColorStart.g = 0;
			tmpParent._strokeColorStart.b = 0;
			tmpParent._strokeColorStart.a = 255;
			tmpParent._scaleStart.x = 600.0f;
			tmpParent._scaleStart.y = 600.0f;
			tmpParent._scaleStart.z = 600.0f;

			tmpParent.position.x = tmpParent._positionStart.x;
			tmpParent.position.y = tmpParent._positionStart.y;
			tmpParent.position.z = tmpParent._positionStart.z;
			tmpParent.strokeWidth= tmpParent._strokeWidthStart;
			tmpParent.strokeColor.r = tmpParent._strokeColorStart.r;
			tmpParent.strokeColor.g = tmpParent._strokeColorStart.g;
			tmpParent.strokeColor.b = tmpParent._strokeColorStart.b;
			tmpParent.strokeColor.a = tmpParent._strokeColorStart.a;
			tmpParent.scale.x = tmpParent._scaleStart.x;
			tmpParent.scale.y = tmpParent._scaleStart.y;
			tmpParent.scale.z = tmpParent._scaleStart.z;
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

				static bool parsed = false;
				if (LaTexLayer::laTexIsReady(testLatex) && !parsed)
				{
					std::string docFilepath = "latex/" + LaTexLayer::getLaTexMd5(testLatex) + ".svg";
					//tmpObject = SvgParser::parseSvgDoc(docFilepath.c_str());
					const char pathText[] = "M 4 8 L 10 1 L 13 0 L 12 3 L 5 9 C 6 10 6 11 7 10 C 7 11 8 12 7 12 A 1.42 1.42 0 0 1 6 13 A 5 5 0 0 0 4 10 Q 3.5 9.9 3.5 10.5 T 2 11.8 T 1.2 11 T 2.5 9.5 T 3 9 A 5 5 90 0 0 0 7 A 1.42 1.42 0 0 1 1 6 C 1 5 2 6 3 6 C 2 7 3 7 4 8 M 10 1 L 10 3 L 12 3 L 10.2 2.8 L 10 1";
					//const char pathText[] = "M2.769614-2.49066C2.769614-2.49066 2.769614-2.550436 2.719801-2.669988L.976339-7.252802C.926526-7.372354 .896638-7.47198 .757161-7.47198C.647572-7.47198 .557908-7.382316 .557908-7.272727C.557908-7.272727 .557908-7.212951 .607721-7.0934L2.361146-2.49066L.607721 2.11208C.557908 2.231631 .557908 2.291407 .557908 2.291407C.557908 2.400996 .647572 2.49066 .757161 2.49066C.886675 2.49066 .926526 2.400996 .966376 2.291407L2.719801-2.311333C2.769614-2.430884 2.769614-2.49066 2.769614-2.49066Z";
					tmpObject = SvgParser::parseSvgPath(pathText, sizeof(pathText), Vec4{ 0.0f, 0.0, 0.0f, 0.0f });
					parsed = true;
				}

				if (tmpObject)
				{
					static float t = -1.0f;
					float easeT = CMath::ease(t / 2.0f, EaseType::Cubic, EaseDirection::InOut);
					tmpObject->renderCreateAnimation(vg, easeT, &tmpParent);
					t += deltaTime;
				}

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

				camera3D.orientation.y += 45.0f * deltaTime;
				camera3D.position.x = glm::cos(-glm::radians(camera3D.orientation.y)) * -10.0f;
				camera3D.position.z = glm::sin(-glm::radians(camera3D.orientation.y)) * 10.0f;

				// Render to main framebuffer
				Renderer::renderToFramebuffer(vg, currentFrame, mainFramebuffer);

				// Render to window
				mainFramebuffer.unbind();
				glViewport(0, 0, window->width, window->height);
				Renderer::clearColor(Vec4{ 0, 0, 0, 0 });

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
			tmpObject->free();
			g_memory_free(tmpObject);
			AnimationManager::serialize("./myScene.bin");

			LaTexLayer::free();
			EditorGui::free();
			nvgDeleteGL3(vg);
			Fonts::unloadAllFonts();
			Renderer::free();
			Audio::free();

			ImGuiLayer::free();
			Window::cleanup();
			globalThreadPool->free();
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

		GlobalThreadPool* threadPool()
		{
			return globalThreadPool;
		}
	}
}