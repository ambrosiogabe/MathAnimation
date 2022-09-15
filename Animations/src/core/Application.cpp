#include "core/Application.h"
#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "core/GladLayer.h"
#include "core/ImGuiLayer.h"
#include "core/Colors.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/PerspectiveCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/Fonts.h"
#include "renderer/Colors.h"
#include "animation/Svg.h"
#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "editor/EditorGui.h"
#include "editor/Timeline.h"
#include "editor/Gizmos.h"
#include "editor/EditorCameraController.h"
#include "editor/EditorSettings.h"
#include "audio/Audio.h"
#include "latex/LaTexLayer.h"
#include "multithreading/GlobalThreadPool.h"
#include "video/Encoder.h"
#include "utils/TableOfContents.h"

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
		static std::string outputVideoFilename = "";

		static int framerate = 60;
		static int outputWidth = 3840;
		static int outputHeight = 2160;
		static float viewportWidth = 18.0f;
		static float viewportHeight = 9.0f;
		static NVGcontext* vg = NULL;
		static AnimationManagerData* am = nullptr;

		static GlobalThreadPool* globalThreadPool = nullptr;
		static Window* window = nullptr;
		static Framebuffer mainFramebuffer;
		static Framebuffer editorFramebuffer;
		static OrthoCamera camera2D;
		static PerspectiveCamera camera3D;
		static OrthoCamera editorCamera2D;
		static PerspectiveCamera editorCamera3D;
		static int absoluteCurrentFrame = -1;
		static int absolutePrevFrame = -1;
		static float accumulatedTime = 0.0f;
		static std::string currentProjectFilepath;
		static VideoEncoder encoder = {};

		static const char* winTitle = "Math Animations";

		void init(const char* projectFile)
		{
			globalThreadPool = new GlobalThreadPool(std::thread::hardware_concurrency());
			//globalThreadPool = new GlobalThreadPool(true);

			// Initiaize GLFW/Glad
			window = new Window(1920, 1080, winTitle);
			window->setVSync(true);

			camera2D.position = Vec2{ 0.0f, 0.0f };
			camera2D.projectionSize = Vec2{ viewportWidth, viewportHeight };
			camera2D.zoom = 1.0f;
			editorCamera2D = camera2D;

			camera3D.forward = glm::vec3(0, 0, 1);
			camera3D.fov = 70.0f;
			camera3D.orientation = glm::vec3(-15.0f, 50.0f, 0);
			camera3D.position = glm::vec3(
				-10.0f * glm::cos(glm::radians(-camera3D.orientation.y)),
				2.5f,
				10.0f * glm::sin(glm::radians(-camera3D.orientation.y))
			);
			editorCamera3D = camera3D;

			Fonts::init();
			GladLayer::init();
			Renderer::init(camera2D, camera3D);
			ImGuiLayer::init(*window);
			Audio::init();
			GizmoManager::init();
			// NOTE(voxel): Just to initialize the camera
			Svg::init(camera2D, camera3D);
			TextAnimations::init(camera2D);
			am = AnimationManager::create(camera2D);
			EditorSettings::init();

			vg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
			if (vg == NULL)
			{
				g_logger_error("Failed to initialize nanovg.");
				return;
			}

			LaTexLayer::init();

			mainFramebuffer = AnimationManager::prepareFramebuffer(outputWidth, outputHeight);
			editorFramebuffer = AnimationManager::prepareFramebuffer(outputWidth, outputHeight);

			currentProjectFilepath = projectFile;
			loadProject(currentProjectFilepath.c_str());

			EditorGui::init(am);

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
			int deltaFrame = 0;

			while (isRunning && !window->shouldClose())
			{
				float deltaTime = (float)(glfwGetTime() - previousTime);
				previousTime = glfwGetTime();
				window->pollInput();

				if (outputVideoFile)
				{
					accumulatedTime += (1.0f / 60.0f);
					absoluteCurrentFrame++;
				}

				// Update components
				if (animState == AnimState::PlayForward)
				{
					accumulatedTime += deltaTime;
					absoluteCurrentFrame = (int)(accumulatedTime * 60.0f);
				}
				else if (animState == AnimState::PlayReverse)
				{
					absoluteCurrentFrame = glm::max(absoluteCurrentFrame - 1, 0);
				}

				deltaFrame = absoluteCurrentFrame - absolutePrevFrame;
				absolutePrevFrame = absoluteCurrentFrame;

				// Update systems all systems/collect systems draw calls
				GizmoManager::update(am, camera2D, camera3D, editorCamera2D);
				EditorCameraController::updateOrtho(editorCamera2D);
				// Update Animation logic and collect draw calls
				AnimationManager::render(am, vg, deltaFrame);

				// Render all animation draw calls to main framebuffer
				bool renderPickingOutline = false;
				if (EditorGui::mainViewportActive() || outputVideoFile)
				{
					Renderer::renderToFramebuffer(mainFramebuffer, colors[(uint8)Color::GreenBrown], camera2D, camera3D, renderPickingOutline);
				}
				// Collect gizmo draw calls
				GizmoManager::render();
				// Render all gizmo draw calls and animation draw calls to the editor framebuffer
				renderPickingOutline = true;
				if (EditorGui::editorViewportActive())
				{
					Renderer::renderToFramebuffer(editorFramebuffer, Colors::Neutral[7], editorCamera2D, editorCamera3D, renderPickingOutline);
				}
				Renderer::endFrame();

				// Bind the window framebuffer and render ImGui results
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, window->width, window->height);
				Renderer::clearColor(Vec4{ 0, 0, 0, 0 });

				// Do ImGui stuff
				ImGuiLayer::beginFrame();
				ImGui::ShowDemoWindow();
				EditorGui::update(mainFramebuffer, editorFramebuffer, am);
				ImGuiLayer::endFrame();
				Svg::endFrame();

				// Miscellaneous
				// TODO: Abstract this stuff out of here
				if (outputVideoFile && absoluteCurrentFrame > -1)
				{
					Pixel* pixels = mainFramebuffer.readAllPixelsRgb8(0, true);

					// TODO: Add a hardware accelerated version that usee CUDA and NVENC
					VideoWriter::pushFrame(pixels, outputWidth * outputHeight, encoder);
					mainFramebuffer.freePixels(pixels);

					if (absoluteCurrentFrame >= AnimationManager::lastAnimatedFrame(am))
					{
						endExport();
					}
				}

				window->swapBuffers();
			}

			// If the window is closing, save the last rendered frame to a preview image
			// TODO: Do this a better way
			// Like no hard coded image path here and hard coded number of components
			AnimationManager::render(am, vg, deltaFrame);
			Renderer::renderToFramebuffer(mainFramebuffer, colors[(uint8)Color::GreenBrown], camera2D, camera3D, false);
			Pixel* pixels = mainFramebuffer.readAllPixelsRgb8(0);
			std::filesystem::path currentPath = std::filesystem::path(currentProjectFilepath).parent_path();
			std::filesystem::path outputFile = (currentPath / "projectPreview.png");
			if (mainFramebuffer.width > 1280 || mainFramebuffer.height > 720)
			{
				constexpr int outputWidth = 1280;
				constexpr int outputHeight = 720;
				uint8* outputPixels = (uint8*)g_memory_allocate(sizeof(uint8) * outputWidth * outputHeight * 3);
				stbir_resize_uint8(
					(uint8*)pixels,
					mainFramebuffer.width,
					mainFramebuffer.height,
					0,
					outputPixels,
					outputWidth,
					outputHeight,
					0,
					3);
				stbi_flip_vertically_on_write(true);
				stbi_write_png(
					outputFile.string().c_str(),
					outputWidth,
					outputHeight,
					3,
					outputPixels,
					sizeof(uint8) * outputWidth * 3);
				g_memory_free(outputPixels);
			}
			else
			{
				stbi_write_png(
					outputFile.string().c_str(),
					mainFramebuffer.width,
					mainFramebuffer.height,
					3,
					pixels,
					sizeof(Pixel) * mainFramebuffer.width);
			}
			mainFramebuffer.freePixels(pixels);
		}

		void free()
		{
			// Free it just in case, if the encoder isn't active this does nothing
			VideoWriter::freeEncoder(encoder);

			saveProject();

			mainFramebuffer.destroy();
			editorFramebuffer.destroy();

			LaTexLayer::free();
			EditorSettings::free();
			EditorGui::free(am);
			AnimationManager::free(am);
			nvgDeleteGL3(vg);
			Fonts::unloadAllFonts();
			Svg::free();
			Renderer::free();
			GizmoManager::free();
			Audio::free();

			ImGuiLayer::free();
			Window::cleanup();
			globalThreadPool->free();
		}

		void saveProject()
		{
			RawMemory animationData = AnimationManager::serialize(am);
			RawMemory timelineData = Timeline::serialize(EditorGui::getTimelineData());

			TableOfContents tableOfContents;
			tableOfContents.init();

			tableOfContents.addEntry(animationData, "Animation_Data");
			tableOfContents.addEntry(timelineData, "Timeline_Data");

			tableOfContents.serialize(currentProjectFilepath.c_str());

			animationData.free();
			timelineData.free();
			tableOfContents.free();
		}

		void loadProject(const char* filepath)
		{
			FILE* fp = fopen(filepath, "rb");
			if (!fp)
			{
				g_logger_warning("Could not load project '%s', error opening file.", filepath);
				return;
			}

			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			RawMemory memory;
			memory.init(fileSize);
			fread(memory.data, fileSize, 1, fp);
			fclose(fp);

			TableOfContents toc = TableOfContents::deserialize(memory);
			memory.free();

			RawMemory animationData = toc.getEntry("Animation_Data");
			RawMemory timelineData = toc.getEntry("Timeline_Data");
			toc.free();

			AnimationManager::deserialize(am, animationData);
			TimelineData timeline = Timeline::deserialize(timelineData);
			EditorGui::setTimelineData(timeline);

			animationData.free();
			timelineData.free();
		}

		void setEditorPlayState(AnimState state)
		{
			if (state == AnimState::PlayForward || state == AnimState::PlayReverse)
			{
				accumulatedTime = (float)absoluteCurrentFrame / 60.0f;
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

		glm::vec2 getViewportSize()
		{
			return glm::vec2(viewportWidth, viewportHeight);
		}

		void setFrameIndex(int frame)
		{
			absoluteCurrentFrame = frame;
		}

		int getFrameIndex()
		{
			return absoluteCurrentFrame;
		}

		int getFrameratePerSecond()
		{
			return framerate;
		}

		void exportVideoTo(const std::string& filename)
		{
			outputVideoFilename = filename;
			if (VideoWriter::startEncodingFile(&encoder, outputVideoFilename.c_str(), outputWidth, outputHeight, framerate, 60, true))
			{
				absoluteCurrentFrame = -1;
				outputVideoFile = true;
			}
		}

		bool isExportingVideo()
		{
			return outputVideoFile;
		}

		void endExport()
		{
			if (VideoWriter::finalizeEncodingFile(encoder))
			{
				g_logger_info("Finished exporting video file.");
			}
			else
			{
				g_logger_error("Failed to finalize encoding video file: %s", encoder.filename);
			}
			VideoWriter::freeEncoder(encoder);
			outputVideoFile = false;
		}

		OrthoCamera* getCamera()
		{
			return &camera2D;
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