#include "core/Application.h"
#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "core/Profiling.h"
#include "renderer/Colors.h"
#include "renderer/GladLayer.h"
#include "renderer/Renderer.h"
#include "renderer/Camera.h"
#include "renderer/Deprecated_OrthoCamera.h"
#include "renderer/Deprecated_PerspectiveCamera.h"
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
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/MenuBar.h"
#include "editor/panels/ExportPanel.h"
#include "editor/UndoSystem.h"
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
#include <nlohmann/json.hpp>

namespace MathAnim
{
	namespace Application
	{
		static uint32 MAX_UNDO_HISTORY = 300;
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
		static EditorCamera* editorCamera = nullptr;
		static UndoSystemData* undoSystem = nullptr;
		static int absoluteCurrentFrame = -1;
		static int absolutePrevFrame = -1;
		static float accumulatedTime = 0.0f;
		static std::filesystem::path currentProjectRoot;
		static std::filesystem::path currentProjectTmpDir;
		static std::filesystem::path currentProjectSceneDir;
		static SceneData sceneData = {};
		static bool reloadCurrentScene = false;
		static bool saveCurrentSceneOnReload = true;
		static int sceneToChangeTo = -1;
		static SvgCache* svgCache = nullptr;
		static float deltaTime = 0.0f;

		static const char* winTitle = "Math Animations";

		// ------- Internal Functions -------
		static nlohmann::json serializeCameras();
		static void deserializeCameras(const nlohmann::json& cameraData, uint32 version);
		static std::string sceneToFilename(const std::string& stringName, const char* ext);
		static void reloadCurrentSceneInternal();
		static void initializeSceneSystems();
		static void freeSceneSystems();

		[[deprecated("This is for upgrading legacy projects created in beta")]]
		static void legacy_loadScene(const std::string& sceneName);
		[[deprecated("This is for upgrading legacy projects created in beta")]]
		static void legacy_loadProject(const std::filesystem::path& projectRoot);

		void init(const char* projectFile)
		{
			auto begin = std::chrono::high_resolution_clock::now();

			// Initialize these just in case this is a new project
			editorCamera = EditorCameraController::init(Camera::createDefault());

			// Initialize other global systems
			globalThreadPool = new GlobalThreadPool(std::thread::hardware_concurrency());
			//globalThreadPool = new GlobalThreadPool(true);

			// Initialize GLFW
			GladLayer::initGlfw();

			// Create the window
			window = new Window(1920, 1080, winTitle, WindowFlags::OpenMaximized);
			window->setVSync(true);

			// Initialize OpenGL functions
			GlVersion glVersion = GladLayer::init();

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

			mainFramebuffer = Renderer::prepareFramebuffer(outputWidth, outputHeight);
			editorFramebuffer = Renderer::prepareFramebuffer(outputWidth, outputHeight);

			currentProjectRoot = std::filesystem::path(projectFile).parent_path();
			currentProjectTmpDir = currentProjectRoot / "tmp";
			Platform::createDirIfNotExists(currentProjectTmpDir.string().c_str());
			currentProjectSceneDir = currentProjectRoot / "scenes";
			Platform::createDirIfNotExists(currentProjectSceneDir.string().c_str());

			initializeSceneSystems();
			loadProject(currentProjectRoot);

			EditorGui::init(am, currentProjectRoot, outputWidth, outputHeight);
			LuauLayer::init(currentProjectRoot / "scripts", am);

			svgCache = new SvgCache();
			svgCache->init();

			GL::enable(GL_BLEND);
			GL::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			auto end = std::chrono::high_resolution_clock::now();
			auto numMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
			if (numMs < 200)
			{
				CppUtils::IO::setForegroundColor(CppUtils::DARKGREEN);
			}
			else if (numMs < 400)
			{
				CppUtils::IO::setForegroundColor(CppUtils::DARKYELLOW);
			}
			else
			{
				CppUtils::IO::setForegroundColor(CppUtils::RED);
			}

			CppUtils::IO::printf("Initialization took: {}ms", numMs);

			CppUtils::IO::resetColor();
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

				// Update systems all systems
				GizmoManager::update(am);
				EditorCameraController::update(deltaTime, editorCamera);
				LaTexLayer::update();
				LuauLayer::update();

				// Update camera matrices
				// NOTE: The editor camera matrices are updated in EditorCameraController::update
				AnimationManager::calculateCameraMatrices(am);

				// Render all animation draw calls to main framebuffer
				if (EditorGui::mainViewportActive() || ExportPanel::isExportingVideo())
				{
					MP_PROFILE_EVENT("MainLoop_RenderToMainViewport");
					if (//AnimationManager::hasActive2DCamera(am) && 
						AnimationManager::hasActive3DCamera(am))
					{
						// TODO: Either come up with multi-camera scenes or get rid of the idea of 2D cameras altogether
						Renderer::pushCamera2D(&AnimationManager::getActiveCamera2D(am));
						Renderer::pushCamera3D(&AnimationManager::getActiveCamera3D(am));
						AnimationManager::render(am, deltaFrame);
						Renderer::popCamera2D();
						Renderer::popCamera3D();

						Renderer::bindAndUpdateViewportForFramebuffer(mainFramebuffer);
						Renderer::renderToFramebuffer(mainFramebuffer, am, "OutputVP_Main_Framebuffer_Pass");

						Renderer::clearDrawCalls();
					}
					else
					{
						// TODO: Display some sort of graphic on screen to let user know they don't have active camera
					}
				}

				// Render editor viewport and active objects with outlines around them
				if (EditorGui::editorViewportActive())
				{
					Renderer::bindAndUpdateViewportForFramebuffer(editorFramebuffer);
					Renderer::clearFramebuffer(editorFramebuffer, "#3a3a39"_hex);

					Renderer::pushCamera2D(&EditorCameraController::getCamera(editorCamera));
					Renderer::pushCamera3D(&EditorCameraController::getCamera(editorCamera));

					{
						// Then render the rest of the stuff
						MP_PROFILE_EVENT("MainLoop_RenderToEditorViewport");
						editorFramebuffer.clearDepthStencil();

						// Collect draw calls
						AnimationManager::render(am, deltaFrame);

						Renderer::renderToFramebuffer(editorFramebuffer, "EditorVP_Main_Framebuffer_Pass");

						Renderer::clearDrawCalls();
					}

					{
						MP_PROFILE_EVENT("MainLoop_RenderActiveObjectOutlines");
						const std::vector<AnimObjId>& activeObjects = InspectorPanel::getAllActiveAnimObjects();
						Renderer::renderStencilOutlineToFramebuffer(editorFramebuffer, activeObjects);

						Renderer::clearDrawCalls();
					}

					{
						// Collect gizmo draw calls and render on top of outlined object
						MP_PROFILE_EVENT("MainLoop_RenderGizmos");
						editorFramebuffer.clearDepthStencil();
						GizmoManager::render(am);
						Renderer::renderToFramebuffer(editorFramebuffer, "Gizmos");
						Renderer::clearDrawCalls();

						// Draw the gizmo manager miscellaneous stuff
						GizmoManager::renderOrientationGizmo(EditorCameraController::getCamera(editorCamera));
						Renderer::clearDrawCalls();
					}

					Renderer::popCamera2D();
					Renderer::popCamera3D();
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
				EditorGui::update(mainFramebuffer, editorFramebuffer, am, deltaTime);
				ImGuiLayer::endFrame();
				GL::popDebugGroup();

				// End frame stuff
				AnimationManager::endFrame(am);
				EditorCameraController::endFrame(editorCamera);
				Renderer::endFrame();

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
			//       Like no hard coded image path here and hard coded number of components
			if (//AnimationManager::hasActive2DCamera(am) && 
				AnimationManager::hasActive3DCamera(am))
			{
				Renderer::pushCamera2D(&AnimationManager::getActiveCamera2D(am));
				Renderer::pushCamera3D(&AnimationManager::getActiveCamera3D(am));
				AnimationManager::render(am, 0);
				Renderer::popCamera2D();
				Renderer::popCamera3D();

				Renderer::bindAndUpdateViewportForFramebuffer(mainFramebuffer);
				Renderer::renderToFramebuffer(mainFramebuffer, am, "AppClosing_Screenshot");
			}
			else
			{
				// TODO: Add graphic warning no active camera here or something
			}

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
			EditorCameraController::free(editorCamera);

			onig_end();
			Highlighters::free();
			LaTexLayer::free();
			EditorSettings::free();
			LuauLayer::free();
			SceneManagementPanel::free();
			EditorGui::free(am);
			UndoSystem::free(undoSystem);
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
			Platform::free();
		}

		void saveProject()
		{
			nlohmann::json projectJson = {};
			SceneManagementPanel::serialize(projectJson["sceneManager"], sceneData);
			try
			{
				std::string projectFilepath = (currentProjectRoot / "project.json").string();
				std::ofstream jsonFile(projectFilepath);
				jsonFile << projectJson << std::endl;
			}
			catch (const std::exception& ex)
			{
				g_logger_error("Failed to save current scene with error: '{}'", ex.what());
			}

			saveCurrentScene();
		}

		void saveCurrentScene()
		{
			// Write data to json files
			nlohmann::json sceneJson = nlohmann::json();

			// This data should always be present regardless of file version
			// Container data layout
			sceneJson["Version"]["Major"] = SERIALIZER_VERSION_MAJOR;
			sceneJson["Version"]["Minor"] = SERIALIZER_VERSION_MINOR;
			sceneJson["Version"]["Full"] = std::to_string(SERIALIZER_VERSION_MAJOR) + "." + std::to_string(SERIALIZER_VERSION_MINOR);

			AnimationManager::serialize(am, sceneJson["AnimationManager"]);
			Timeline::serialize(EditorGui::getTimelineData(), sceneJson["TimelineData"]);
			sceneJson["EditorCameras"] = serializeCameras();

			try
			{
				// TODO: bson should be faster, but it does increase size a bit. Consider switching to
				// bson files and adding options to export projects as formatted JSON for debugging
				std::string jsonFilepath = (currentProjectSceneDir / sceneToFilename(sceneData.sceneNames[sceneData.currentScene], ".json")).string();
				std::ofstream jsonFile(jsonFilepath);
				jsonFile << sceneJson << std::endl;
			}
			catch (const std::exception& ex)
			{
				g_logger_error("Failed to save current scene with error: '{}'", ex.what());
			}
		}

		void loadProject(const std::filesystem::path& projectRoot)
		{
			std::string projectFilepath = (projectRoot / "project.json").string();
			if (!Platform::fileExists(projectFilepath.c_str()))
			{
				// If a legacy project exists load that instead
				std::string legacyProjectFilepath = (projectRoot / "project.bin").string();
				if (Platform::fileExists(legacyProjectFilepath.c_str()))
				{
					legacy_loadProject(projectRoot);
					return;
				}

				// Otherwise create an empty scene and initialize a new project
				// Save empty project now
				sceneData.sceneNames.push_back("New Scene");
				sceneData.currentScene = 0;
				// This should create a default scene since nothing exists
				loadScene(sceneData.sceneNames[sceneData.currentScene]);
				saveProject();
				return;
			}

			try
			{
				std::ifstream inputFile(projectFilepath);
				nlohmann::json projectJson;
				inputFile >> projectJson;

				if (projectJson.contains("sceneManager") && !projectJson["sceneManager"].is_null())
				{
					sceneData = SceneManagementPanel::deserialize(projectJson["sceneManager"]);
					loadScene(sceneData.sceneNames[sceneData.currentScene]);
				}
			}
			catch (const std::exception& ex)
			{
				g_logger_error("Failed to load project '{}' with error: '{}'", projectFilepath, ex.what());
			}
		}

		static void legacy_loadProject(const std::filesystem::path& projectRoot)
		{
			std::string projectFilepath = (projectRoot / "project.bin").string();
			if (!Platform::fileExists(projectFilepath.c_str()))
			{
				g_logger_error("LEGACY: Failed to upgrade legacy project.bin file to json.");
				return;
			}

			FILE* fp = fopen(projectFilepath.c_str(), "rb");
			if (!fp)
			{
				g_logger_warning("Could not load project '{}', error opening file: '{}'.", projectFilepath, strerror(errno));
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
				sceneData = SceneManagementPanel::legacy_deserialize(sceneDataMemory);
				loadScene(sceneData.sceneNames[sceneData.currentScene]);
			}

			sceneDataMemory.free();
		}

		void loadScene(const std::string& sceneName)
		{
			std::string filepath = (currentProjectSceneDir / sceneToFilename(sceneName, ".json")).string();
			if (!Platform::fileExists(filepath.c_str()))
			{
				// Check if a legacy project exists and try to load that, if loading fails
				// then load an empty project
				std::string legacyFilepath = (currentProjectRoot / sceneToFilename(sceneName, ".bin")).string();
				if (Platform::fileExists(legacyFilepath.c_str()))
				{
					legacy_loadScene(sceneName);
					return;
				}
				else
				{
					// Load default scene template
					filepath = "./assets/sceneTemplates/default.json";
				}
			}

			if (!Platform::fileExists(filepath.c_str()))
			{
				g_logger_error("Missing scene file '{}'. Cannot load scene.", filepath);
				resetToFrame(0);
				return;
			}

			try
			{
				std::ifstream inputFile(filepath);
				nlohmann::json sceneJson;
				inputFile >> sceneJson;

				// Read version
				uint32 versionMajor = 0;
				uint32 versionMinor = 0;
				if (sceneJson.contains("Version"))
				{
					if (sceneJson["Version"].contains("Major") && sceneJson["Version"].contains("Minor"))
					{
						versionMajor = sceneJson["Version"]["Major"];
						versionMinor = sceneJson["Version"]["Minor"];
					}
				}

				int loadedProjectCurrentFrame = 0;
				if (sceneJson.contains("TimelineData") && !sceneJson["TimelineData"].is_null())
				{
					TimelineData timeline = Timeline::deserialize(sceneJson["TimelineData"]);
					EditorGui::setTimelineData(timeline);
					loadedProjectCurrentFrame = timeline.currentFrame;
				}

				if (sceneJson.contains("AnimationManager") && !sceneJson["AnimationManager"].is_null())
				{
					AnimationManager::deserialize(am, sceneJson["AnimationManager"], loadedProjectCurrentFrame, versionMajor, versionMinor);
					// Flush any pending objects to be created for real
					AnimationManager::endFrame(am);
				}

				deserializeCameras(sceneJson["EditorCameras"], versionMajor);
			}
			catch (const std::exception& ex)
			{
				g_logger_error("Failed to load scene '{}' with error: '{}'", filepath, ex.what());
			}
		}

		static void legacy_loadScene(const std::string& sceneName)
		{
			std::string filepath = (currentProjectRoot / sceneToFilename(sceneName, ".bin")).string();
			if (!Platform::fileExists(filepath.c_str()))
			{
				g_logger_error("LEGACY: No legacy project, aborting legacy upgrade.");
				return;
			}

			FILE* fp = fopen(filepath.c_str(), "rb");
			if (!fp)
			{
				g_logger_warning("LEGACY: Could not load scene '{}', error opening file.", filepath);
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
				TimelineData timeline = Timeline::legacy_deserialize(timelineData);
				EditorGui::setTimelineData(timeline);
				loadedProjectCurrentFrame = timeline.currentFrame;
			}
			if (animationData.data)
			{
				AnimationManager::legacy_deserialize(am, animationData, loadedProjectCurrentFrame);
				// Flush any pending objects to be created for real
				AnimationManager::endFrame(am);

			}
			if (cameraData.data)
			{
				// NOTE: Legacy projects will have the editor camera automatically upgraded
				//       to the new camera type. That means that the data will be lost
				//       but it should be fine
			}

			animationData.free();
			timelineData.free();
			cameraData.free();
		}

		void deleteScene(const std::string& sceneName)
		{
			for (int i = 0; i < sceneData.sceneNames.size(); i++)
			{
				if (sceneData.sceneNames[i] == sceneName)
				{
					if (i <= sceneToChangeTo)
					{
						sceneToChangeTo--;
					}
					break;
				}
			}

			std::string filepath = (currentProjectSceneDir / sceneToFilename(sceneName, ".json")).string();
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

			g_logger_warning("Cannot change to unknown scene name '{}'", sceneName);
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

		const Camera* getEditorCamera()
		{
			return &EditorCameraController::getCamera(editorCamera);
		}

		SvgCache* getSvgCache()
		{
			return svgCache;
		}

		UndoSystemData* getUndoSystem()
		{
			return undoSystem;
		}

		GlobalThreadPool* threadPool()
		{
			return globalThreadPool;
		}

		static nlohmann::json serializeCameras()
		{
			nlohmann::json cameraData = nlohmann::json();

			EditorCameraController::serialize(editorCamera, cameraData["EditorCamera"]);

			return cameraData;
		}

		static void deserializeCameras(const nlohmann::json& cameraData, uint32 version)
		{
			switch (version)
			{
			case 3:
			{
				if (cameraData.contains("EditorCamera"))
				{
					EditorCameraController::free(editorCamera);
					editorCamera = EditorCameraController::deserialize(cameraData["EditorCamera"], version);
				}
			}
			break;
			case 2:
			{
				// Upgrading legacy projects will default to the orthographic camera
				Camera newCamera = editorCamera != nullptr
					? EditorCameraController::getCamera(editorCamera)
					: Camera::createDefault();
				if (cameraData.contains("EditorCamera2D"))
				{
					OrthoCamera oldCamera2D = OrthoCamera::deserialize(cameraData["EditorCamera2D"], version);
					newCamera.position = CMath::vector3From2(oldCamera2D.position);
					newCamera.position.z = -2;
					newCamera.aspectRatioFraction.x = (int)oldCamera2D.projectionSize.x;
					newCamera.aspectRatioFraction.y = (int)oldCamera2D.projectionSize.y;
				}
				else if (cameraData.contains("EditorCamera3D"))
				{
					PerspectiveCamera oldCamera3D = PerspectiveCamera::deserialize(cameraData["EditorCamera3D"], version);
					newCamera.position = CMath::convert(oldCamera3D.position);
					newCamera.fov = oldCamera3D.fov;
					newCamera.orientation = glm::quat(oldCamera3D.orientation);
				}

				newCamera.calculateMatrices(true);

				EditorCameraController::free(editorCamera);
				editorCamera = EditorCameraController::init(newCamera);
			}
			break;
			default:
			{
				g_logger_warning("Editor data serialized with unknown version: '{}'", version);
			}
			}
		}

		static std::string sceneToFilename(const std::string& stringName, const char* ext)
		{
			return "Scene_" + stringName + ext;
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
			UndoSystem::free(undoSystem);
			AnimationManager::free(am);
			EditorSettings::free();
		}

		static void initializeSceneSystems()
		{
			am = AnimationManager::create();
			undoSystem = UndoSystem::init(am, MAX_UNDO_HISTORY);
			EditorSettings::init();
		}
	}
}