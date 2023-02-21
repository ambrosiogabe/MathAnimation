#include "core/Application.h"
#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "core/Profiling.h"
#include "renderer/Colors.h"
#include "renderer/GladLayer.h"
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
#include "editor/Gizmos.h"
#include "editor/EditorCameraController.h"
#include "editor/EditorSettings.h"
#include "editor/timeline/Timeline.h"
#include "editor/imgui/ImGuiLayer.h"
#include "editor/panels/SceneManagementPanel.h"
#include "editor/panels/MenuBar.h"
#include "editor/panels/ExportPanel.h"
#include "parsers/SyntaxHighlighter.h"
#include "audio/Audio.h"
#include "latex/LaTexLayer.h"
#include "multithreading/GlobalThreadPool.h"
#include "video/Encoder.h"
#include "utils/TableOfContents.h"
#include "scripting/LuauLayer.h"
#include "platform/Platform.h"

#include <imgui.h>
#include <oniguruma.h>
#include <errno.h>

namespace MathAnim
{
	namespace Application
	{
		static AnimState animState = AnimState::Pause;

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
		static std::filesystem::path currentProjectRoot;
		static std::filesystem::path currentProjectTmpDir;
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
			GlVersion glVersion = GladLayer::init();
			window = new Window(1920, 1080, winTitle, WindowFlags::OpenMaximized);
			window->setVSync(true);

			// Initialize Onigiruma
			OnigEncoding use_encs[1];
			use_encs[0] = ONIG_ENCODING_ASCII;
			onig_initialize(use_encs, sizeof(use_encs) / sizeof(use_encs[0]));

			Fonts::init();
			Renderer::init();
			ImGuiLayer::init(*window, "./assets/layouts/Default.json");
			Audio::init();
			GizmoManager::init();
			Svg::init();
			SceneManagementPanel::init();
			SvgParser::init();
			Highlighters::init();

			LaTexLayer::init();

			mainFramebuffer = AnimationManager::prepareFramebuffer(outputWidth, outputHeight);
			editorFramebuffer = AnimationManager::prepareFramebuffer(outputWidth, outputHeight);

			currentProjectRoot = std::filesystem::path(projectFile).parent_path();
			currentProjectTmpDir = currentProjectRoot / "tmp";
			Platform::createDirIfNotExists(currentProjectTmpDir.string().c_str());

			initializeSceneSystems();
			loadProject(currentProjectRoot);

			EditorGui::init(am, currentProjectRoot, outputWidth, outputHeight);
			LuauLayer::init(currentProjectRoot / "scripts", am);

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

			// TODO: Make this customizable through the project settings
			const Vec4 greenBrown = "#272822FF"_hex;

			while (isRunning && !window->shouldClose())
			{
				MP_PROFILE_FRAME("MainLoop");

				deltaTime = (float)(glfwGetTime() - previousTime);
				previousTime = glfwGetTime();
				window->pollInput();

				// Update components
				switch (animState)
				{
				case AnimState::PlayForward:
				{
					accumulatedTime += deltaTime;
					absoluteCurrentFrame = (int)(accumulatedTime * 60.0f);
				}
				break;
				case AnimState::PlayForwardFixedFrameTime:
				{
					absoluteCurrentFrame++;
					accumulatedTime += (1.0f / 60.0f);
				}
				break;
				case AnimState::PlayReverse:
				{
					accumulatedTime -= deltaTime;
					absoluteCurrentFrame = (int)(accumulatedTime * 60.0f);
				}
				break;
				case AnimState::Pause:
					break;
				}

				deltaFrame = absoluteCurrentFrame - absolutePrevFrame;
				absolutePrevFrame = absoluteCurrentFrame;

				// Update systems all systems/collect systems draw calls
				GizmoManager::update(am);
				EditorCameraController::updateOrtho(editorCamera2D);
				// Update Animation logic and collect draw calls
				AnimationManager::render(am, deltaFrame);
				LaTexLayer::update();
				LuauLayer::update();

				// Render all animation draw calls to main framebuffer
				if (EditorGui::mainViewportActive() || ExportPanel::isExportingVideo())
				{
					MP_PROFILE_EVENT("MainLoop_RenderToMainViewport");
					Renderer::bindAndUpdateViewportForFramebuffer(mainFramebuffer);
					Renderer::clearFramebuffer(mainFramebuffer, greenBrown);
					Renderer::renderToFramebuffer(mainFramebuffer, am);

					Renderer::clearDrawCalls();
				}

				// Render active objects with outlines around them
				if (EditorGui::editorViewportActive())
				{
					Renderer::bindAndUpdateViewportForFramebuffer(editorFramebuffer);
					Renderer::clearFramebuffer(editorFramebuffer, Colors::Neutral[7]);

					{
						MP_PROFILE_EVENT("MainLoop_RenderActiveObjectOutlines");
						Renderer::renderStencilOutlineToFramebuffer(am, editorFramebuffer, editorCamera2D, editorCamera3D);

						Renderer::clearDrawCalls();
					}

					{
						// Then render the rest of the stuff
						MP_PROFILE_EVENT("MainLoop_RenderToEditorViewport");
						editorFramebuffer.clearDepthStencil();
						AnimationManager::render(am, deltaFrame, RenderType::InactiveObjectsOnly);
						Renderer::renderToFramebuffer(editorFramebuffer, editorCamera2D, editorCamera3D);

						Renderer::clearDrawCalls();
					}

					{
						// Collect gizmo draw calls and render on top of outlined object
						MP_PROFILE_EVENT("MainLoop_RenderGizmos");
						editorFramebuffer.clearDepthStencil();
						GizmoManager::render(am);
						Renderer::renderToFramebuffer(editorFramebuffer, editorCamera2D, editorCamera3D);

						Renderer::clearDrawCalls();
					}
				}

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
				globalThreadPool->processFinishedTasks();
				{
					MP_PROFILE_EVENT("MainThreadLoop_SwapBuffers");
					window->swapBuffers();
				}

				if (reloadCurrentScene)
				{
					MP_PROFILE_EVENT("MainThreadLoop_ReloadCurrentScene");
					reloadCurrentSceneInternal();
					reloadCurrentScene = false;
				}
			}

			// If the window is closing, save the last rendered frame to a preview image
			// TODO: Do this a better way
			// Like no hard coded image path here and hard coded number of components
			AnimationManager::render(am, deltaFrame);
			Renderer::bindAndUpdateViewportForFramebuffer(mainFramebuffer);
			Renderer::clearFramebuffer(mainFramebuffer, greenBrown);
			Renderer::renderToFramebuffer(mainFramebuffer, am);
			Pixel* pixels = mainFramebuffer.readAllPixelsRgb8(0);
			std::filesystem::path outputFile = (currentProjectRoot / "projectPreview.png");
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

			saveProject();

			// Empty tmp directory
			std::filesystem::remove_all(currentProjectTmpDir);

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

			std::string projectFilepath = (currentProjectRoot / "project.bin").string();
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

			std::string filepath = (currentProjectRoot / sceneToFilename(sceneData.sceneNames[sceneData.currentScene])).string();
			tableOfContents.serialize(filepath.c_str());

			animationData.free();
			timelineData.free();
			cameraData.free();
			tableOfContents.free();
		}

		void loadProject(const std::filesystem::path& projectRoot)
		{
			std::string projectFilepath = (projectRoot / "project.bin").string();
			if (!Platform::fileExists(projectFilepath.c_str()))
			{
				// Save empty project now
				sceneData.sceneNames.push_back("New Scene");
				sceneData.currentScene = 0;
				// This should create a default scene since nothing exists
				loadScene(sceneData.sceneNames[sceneData.currentScene]);
				saveProject();
				return;
			}

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
			std::string filepath = (currentProjectRoot / sceneToFilename(sceneName)).string();
			if (!Platform::fileExists(filepath.c_str()))
			{
				// Load default scene template
				filepath = "./assets/sceneTemplates/default.bin";
			}

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
			std::string filepath = (currentProjectRoot / sceneToFilename(sceneName)).string();
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

		const Window& getWindow()
		{
			return *window;
		}

		void setFrameIndex(int frame)
		{
			absoluteCurrentFrame = frame;
		}

		int getFrameIndex()
		{
			return absoluteCurrentFrame;
		}

		void resetToFrame(int frame)
		{
			absoluteCurrentFrame = frame;
			absolutePrevFrame = frame;
			accumulatedTime = frame * ExportPanel::getExportSecondsPerFrame();
		}

		const Framebuffer& getMainFramebuffer()
		{
			return mainFramebuffer;
		}

		const std::filesystem::path& getCurrentProjectRoot()
		{
			return currentProjectRoot;
		}

		const std::filesystem::path& getTmpDir()
		{
			return currentProjectTmpDir;
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

			EditorGui::init(am, currentProjectRoot, outputWidth, outputHeight);
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