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
#include "renderer/GLApi.h"
#include "animation/TextAnimations.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "svg/Svg.h"
#include "svg/SvgParser.h"
#include "svg/SvgCache.h"
#include "editor/EditorGui.h"
#include "editor/Timeline.h"
#include "editor/Gizmos.h"
#include "editor/EditorCameraController.h"
#include "editor/EditorSettings.h"
#include "editor/SceneManagementPanel.h"
#include "editor/MenuBar.h"
#include "parsers/SyntaxHighlighter.h"
#include "audio/Audio.h"
#include "latex/LaTexLayer.h"
#include "multithreading/GlobalThreadPool.h"
#include "video/Encoder.h"
#include "utils/TableOfContents.h"
#include "scripting/LuauLayer.h"

#include <imgui.h>
#include <oniguruma.h>

#include <errno.h>

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
		static AnimationManagerData* am = nullptr;

		static GlobalThreadPool* globalThreadPool = nullptr;
		static Window* window = nullptr;
		static Framebuffer mainFramebuffer;
		static Framebuffer editorFramebuffer;
		static OrthoCamera editorCamera2D;
		static PerspectiveCamera editorCamera3D;
		static int absoluteCurrentFrame = -1;
		static int absolutePrevFrame = -1;
		static float accumulatedTime = 0.0f;
		static std::string currentProjectRoot;
		static VideoEncoder encoder = {};
		static SceneData sceneData = {};
		static bool reloadCurrentScene = false;
		static bool saveCurrentSceneOnReload = true;
		static int sceneToChangeTo = -1;
		static SvgCache* svgCache = nullptr;
		static float deltaTime = 0.0f;

		static const char* winTitle = "Math Animations";

		// ------- Internal Functions -------
		static RawMemory serializeCameras();
		static void deserializeCameras(RawMemory& cameraData);
		static std::string sceneToFilename(const std::string& stringName);
		static void reloadCurrentSceneInternal();
		static void initializeSceneSystems();
		static void freeSceneSystems();

		void init(const char* projectFile)
		{
			// Initialize these just in case this is a new project
			editorCamera2D.position = Vec2{ viewportWidth / 2.0f, viewportHeight / 2.0f };
			editorCamera2D.projectionSize = Vec2{ viewportWidth, viewportHeight };
			editorCamera2D.zoom = 1.0f;

			editorCamera3D.position = glm::vec3(0.0f);
			editorCamera3D.fov = 70.0f;
			editorCamera3D.forward = glm::vec3(1.0f, 0.0f, 0.0f);

			// Initialize other global systems
			globalThreadPool = new GlobalThreadPool(std::thread::hardware_concurrency());
			//globalThreadPool = new GlobalThreadPool(true);

			// Initiaize GLFW/Glad
			GladLayer::init();
			window = new Window(1920, 1080, winTitle, WindowFlags::OpenMaximized);
			window->setVSync(true);

			// Initialize Onigiruma
			OnigEncoding use_encs[1];
			use_encs[0] = ONIG_ENCODING_ASCII;
			onig_initialize(use_encs, sizeof(use_encs) / sizeof(use_encs[0]));

			Fonts::init();
			Renderer::init();
			ImGuiLayer::init(*window);
			Audio::init();
			GizmoManager::init();
			Svg::init();
			SceneManagementPanel::init();
			SvgParser::init();
			Highlighters::init();

			LaTexLayer::init();

			mainFramebuffer = AnimationManager::prepareFramebuffer(outputWidth, outputHeight);
			editorFramebuffer = AnimationManager::prepareFramebuffer(outputWidth, outputHeight);

			currentProjectRoot = std::filesystem::path(projectFile).parent_path().string() + "/";

			initializeSceneSystems();
			loadProject(currentProjectRoot.c_str());
			if (sceneData.sceneNames.size() <= 0)
			{
				sceneData.sceneNames.push_back("New Scene");
				sceneData.currentScene = 0;
			}

			EditorGui::init(am, currentProjectRoot);
			LuauLayer::init(currentProjectRoot + "/scripts", am);

			svgCache = new SvgCache();
			svgCache->init();

			GL::enable(GL_BLEND);
			GL::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		void run()
		{
			// Run game loop
			// Start with a 60 fps frame rate
			bool isRunning = true;
			double previousTime = glfwGetTime() - 0.16f;
			int deltaFrame = 0;

			svgCache->clearAll();

			while (isRunning && !window->shouldClose())
			{
				deltaTime = (float)(glfwGetTime() - previousTime);
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
				if (deltaFrame != 0)
				{
					// TODO: Do something here...
					//svgCache->clearAll();
				}

				absolutePrevFrame = absoluteCurrentFrame;

				// Update systems all systems/collect systems draw calls
				GizmoManager::update(am);
				EditorCameraController::updateOrtho(editorCamera2D);
				// Update Animation logic and collect draw calls
				AnimationManager::render(am, deltaFrame);
				LaTexLayer::update();
				LuauLayer::update();

				// Render all animation draw calls to main framebuffer
				bool renderPickingOutline = false;

				if (EditorGui::mainViewportActive() || outputVideoFile)
				{
					Renderer::renderToFramebuffer(mainFramebuffer, colors[(uint8)Color::GreenBrown], am, renderPickingOutline);
				}
				// Collect gizmo draw calls
				GizmoManager::render(am);
				// Render all gizmo draw calls and animation draw calls to the editor framebuffer
				renderPickingOutline = true;
				if (EditorGui::editorViewportActive())
				{
					Renderer::renderToFramebuffer(editorFramebuffer, Colors::Neutral[7], editorCamera2D, editorCamera3D, renderPickingOutline);
				}
				Renderer::endFrame();

				// Bind the window framebuffer and render ImGui results
				GL::bindFramebuffer(GL_FRAMEBUFFER, 0);
				GL::viewport(0, 0, window->width, window->height);
				Renderer::clearColor(Vec4{ 0, 0, 0, 0 });

				// Do ImGui stuff
				int debugMsgId = 0;
				GL::pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, debugMsgId++, -1, "ImGui_Pass");
				ImGuiLayer::beginFrame();
				MenuBar::update();
				ImGui::ShowDemoWindow();
				SceneManagementPanel::update(sceneData);
				EditorGui::update(mainFramebuffer, editorFramebuffer, am);
				ImGuiLayer::endFrame();
				GL::popDebugGroup();

				AnimationManager::endFrame(am);

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

				if (reloadCurrentScene)
				{
					reloadCurrentSceneInternal();
					reloadCurrentScene = false;
				}
			}

			// If the window is closing, save the last rendered frame to a preview image
			// TODO: Do this a better way
			// Like no hard coded image path here and hard coded number of components
			AnimationManager::render(am, deltaFrame);
			Renderer::renderToFramebuffer(mainFramebuffer, colors[(uint8)Color::GreenBrown], am, false);
			Pixel* pixels = mainFramebuffer.readAllPixelsRgb8(0);
			std::filesystem::path currentPath = std::filesystem::path(currentProjectRoot);
			std::filesystem::path outputFile = (currentPath / "projectPreview.png");
			if (mainFramebuffer.width > 1280 || mainFramebuffer.height > 720)
			{
				constexpr int pngOutputWidth = 1280;
				constexpr int pngOutputHeight = 720;
				uint8* pngOutputPixels = (uint8*)g_memory_allocate(sizeof(uint8) * pngOutputWidth * pngOutputHeight * 3);
				stbir_resize_uint8(
					(uint8*)pixels,
					mainFramebuffer.width,
					mainFramebuffer.height,
					0,
					pngOutputPixels,
					pngOutputWidth,
					pngOutputHeight,
					0,
					3);
				stbi_flip_vertically_on_write(true);
				stbi_write_png(
					outputFile.string().c_str(),
					pngOutputWidth,
					pngOutputHeight,
					3,
					pngOutputPixels,
					sizeof(uint8) * pngOutputWidth * 3);
				g_memory_free(pngOutputPixels);
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
			svgCache->free();
			delete svgCache;

			// Free it just in case, if the encoder isn't active this does nothing
			VideoWriter::freeEncoder(encoder);

			saveProject();

			mainFramebuffer.destroy();
			editorFramebuffer.destroy();

			onig_end();
			Highlighters::free();
			LaTexLayer::free();
			EditorSettings::free();
			LuauLayer::free();
			SceneManagementPanel::free();
			EditorGui::free(am);
			AnimationManager::free(am);
			Fonts::unloadAllFonts();
			Renderer::free();
			GizmoManager::free();
			Audio::free();

			ImGuiLayer::free();
			Window::cleanup();
			globalThreadPool->free();
			delete globalThreadPool;

			GladLayer::deinit();
		}

		void saveProject()
		{
			RawMemory sceneDataMemory = SceneManagementPanel::serialize(sceneData);

			TableOfContents tableOfContents;
			tableOfContents.init();
			tableOfContents.addEntry(sceneDataMemory, "Scene_Data");

			std::string projectFilepath = currentProjectRoot + "project.bin";
			tableOfContents.serialize(projectFilepath.c_str());

			sceneDataMemory.free();
			tableOfContents.free();

			saveCurrentScene();
		}

		void saveCurrentScene()
		{
			RawMemory animationData = AnimationManager::serialize(am);
			RawMemory timelineData = Timeline::serialize(EditorGui::getTimelineData());
			RawMemory cameraData = serializeCameras();

			TableOfContents tableOfContents;
			tableOfContents.init();

			tableOfContents.addEntry(animationData, "Animation_Data");
			tableOfContents.addEntry(timelineData, "Timeline_Data");
			tableOfContents.addEntry(cameraData, "Camera_Data");

			std::string filepath = currentProjectRoot + sceneToFilename(sceneData.sceneNames[sceneData.currentScene]);
			tableOfContents.serialize(filepath.c_str());

			animationData.free();
			timelineData.free();
			cameraData.free();
			tableOfContents.free();
		}

		void loadProject(const std::string& projectRoot)
		{
			std::string projectFilepath = projectRoot + "project.bin";
			FILE* fp = fopen(projectFilepath.c_str(), "rb");
			if (!fp)
			{
				g_logger_warning("Could not load project '%s', error opening file: %s.", projectFilepath.c_str(), strerror(errno));
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

			RawMemory sceneDataMemory = toc.getEntry("Scene_Data");
			toc.free();

			if (sceneDataMemory.data)
			{
				sceneData = SceneManagementPanel::deserialize(sceneDataMemory);
				loadScene(sceneData.sceneNames[sceneData.currentScene]);
			}

			sceneDataMemory.free();
		}

		void loadScene(const std::string& sceneName)
		{
			std::string filepath = currentProjectRoot + sceneToFilename(sceneName);
			FILE* fp = fopen(filepath.c_str(), "rb");
			if (!fp)
			{
				g_logger_warning("Could not load scene '%s', error opening file.", filepath.c_str());
				resetToFrame(0);
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
			RawMemory cameraData = toc.getEntry("Camera_Data");
			toc.free();

			int loadedProjectCurrentFrame = 0;
			if (timelineData.data)
			{
				TimelineData timeline = Timeline::deserialize(timelineData);
				EditorGui::setTimelineData(timeline);
				loadedProjectCurrentFrame = timeline.currentFrame;
			}
			if (animationData.data)
			{
				AnimationManager::deserialize(am, animationData, loadedProjectCurrentFrame);
				// Flush any pending objects to be created for real
				AnimationManager::endFrame(am);

			}
			if (cameraData.data)
			{
				deserializeCameras(cameraData);
			}

			animationData.free();
			timelineData.free();
			cameraData.free();
		}

		void deleteScene(const std::string& sceneName)
		{
			std::string filepath = currentProjectRoot + sceneToFilename(sceneName);
			remove(filepath.c_str());
		}

		void changeSceneTo(const std::string& sceneName, bool saveCurrentScene)
		{
			for (int i = 0; i < sceneData.sceneNames.size(); i++)
			{
				if (sceneData.sceneNames[i] == sceneName)
				{
					sceneToChangeTo = i;
					reloadCurrentScene = true;
					saveCurrentSceneOnReload = saveCurrentScene;
					return;
				}
			}

			g_logger_warning("Cannot change to unknown scene name '%s'", sceneName.c_str());
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

		float getDeltaTime()
		{
			return deltaTime;
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

		glm::vec2 getAppWindowSize()
		{
			return glm::vec2(window->width, window->height);
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

		void resetToFrame(int frame)
		{
			absoluteCurrentFrame = frame;
			absolutePrevFrame = frame;
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

		OrthoCamera* getEditorCamera()
		{
			return &editorCamera2D;
		}

		SvgCache* getSvgCache()
		{
			return svgCache;
		}

		GlobalThreadPool* threadPool()
		{
			return globalThreadPool;
		}

		static RawMemory serializeCameras()
		{
			RawMemory cameraData;
			cameraData.init(sizeof(OrthoCamera) + sizeof(PerspectiveCamera));

			// Version    -> u32
			// camera2D   -> OrthoCamera
			// camera3D   -> PerspCamera
			const uint32 version = 1;
			cameraData.write<uint32>(&version);

			editorCamera2D.serialize(cameraData);
			editorCamera3D.serialize(cameraData);

			cameraData.shrinkToFit();
			return cameraData;
		}

		static void deserializeCameras(RawMemory& cameraData)
		{
			// Version    -> u32
			// camera2D   -> OrthoCamera
			// camera3D   -> PerspCamera
			uint32 version = 1;
			cameraData.read<uint32>(&version);
			editorCamera2D = OrthoCamera::deserialize(cameraData, version);
			editorCamera3D = PerspectiveCamera::deserialize(cameraData, version);
		}

		static std::string sceneToFilename(const std::string& stringName)
		{
			return "Scene_" + stringName + ".bin";
		}

		static void reloadCurrentSceneInternal()
		{
			if (saveCurrentSceneOnReload)
			{
				saveCurrentScene();
			}
			sceneData.currentScene = sceneToChangeTo;

			// Reset to a blank slate
			EditorGui::free(am);
			freeSceneSystems();
			initializeSceneSystems();

			loadScene(sceneData.sceneNames[sceneData.currentScene]);

			EditorGui::init(am, currentProjectRoot);
		}

		static void freeSceneSystems()
		{
			AnimationManager::free(am);
			EditorSettings::free();
		}

		static void initializeSceneSystems()
		{
			am = AnimationManager::create();
			EditorSettings::init();
		}
	}
}