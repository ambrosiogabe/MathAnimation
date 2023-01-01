#include "core/ImGuiLayer.h"
#include "core/Window.h"
#include "core/Colors.h"
#include "utils/FontAwesome.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace MathAnim
{
	namespace ImGuiLayer
	{
		static ImFont* defaultFont;
		static ImFont* mediumFont;
		static ImFont* monoFont;

		static ImFont* largeSolidIconFont;
		static ImFont* mediumSolidIconFont;
		static ImFont* largeRegularIconFont;
		static ImFont* mediumRegularIconFont;

		// ---------- Internal Functions ----------
		static void loadCustomTheme();
        
		void init(const Window& window)
		{
			// Set up dear imgui
			ImGui::CreateContext();
            
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
			io.ConfigWindowsMoveFromTitleBarOnly = true;
            
            // NOTE(voxel): This looks right for my machine (May have to go back and forth on the value 128.f
			glm::ivec2 monitor_size = Window::getMonitorWorkingSize();
            float fontSize = monitor_size.x / (128.f + 16.0f);
#if defined(_WIN32)
			defaultFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Arial.ttf", fontSize);
#elif defined(__linux__)
			defaultFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/liberation/LiberationSans-Regular.ttf", fontSize);
#endif
            
			static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
			ImFontConfig config = {};
			config.MergeMode = true;
			// TODO: Optimize ram usage here...
			config.SizePixels = fontSize;
			config.PixelSnapH = true;
			io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.ttf", fontSize, &config, iconRanges);

			// Add the rest of the fonts separately
			config.MergeMode = false;
			config.SizePixels = fontSize * 1.5f;
#if defined(_WIN32)
			mediumFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Arial.ttf", fontSize * 1.5f);
#elif defined(__linux__)
			mediumFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/liberation/LiberationSerif-Regular.ttf", fontSize * 1.5f);
#endif

			config.MergeMode = false;
			config.SizePixels = fontSize;
#if defined(_WIN32)
			monoFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", fontSize);
#elif defined(__linux__)
			monoFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/liberation/LiberationMono-Regular.ttf", fontSize);
#endif
            
			config.SizePixels = fontSize * 1.5f;
			mediumSolidIconFont = io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.ttf", fontSize * 1.5f, &config, iconRanges);
			mediumRegularIconFont = io.Fonts->AddFontFromFileTTF("assets/fonts/fa-regular-400.ttf", fontSize * 1.5f, &config, iconRanges);

			config.SizePixels = fontSize * 2.0f;
			largeSolidIconFont = io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.ttf", fontSize * 2.0f, &config, iconRanges);
			largeRegularIconFont = io.Fonts->AddFontFromFileTTF("assets/fonts/fa-regular-400.ttf", fontSize * 2.0f, &config, iconRanges);
            
			ImGui::StyleColorsDark();
            
			// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
			ImGuiStyle& style = ImGui::GetStyle();
			style.ScaleAllSizes(window.getContentScale());
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}

			loadCustomTheme();
            
			// Setup Platform/Renderer backends
			ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window.windowPtr, true);
            
			const char* glsl_version = "#version 330";
			ImGui_ImplOpenGL3_Init(glsl_version);
		}
        
		void beginFrame()
		{
			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGui::CaptureKeyboardFromApp(true);
            
            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		}
        
		void endFrame()
		{
			if (ImGui::IsAnyItemFocused())
			{
				ImGui::CaptureKeyboardFromApp(false);
			}

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
			ImGuiIO& io = ImGui::GetIO();
            
			// Update and Render additional Platform Windows
			// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
			//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}
		}
        
		void keyEvent()
		{
			
		}
        
		void mouseEvent()
		{
            
		}
        
		void free()
		{
			defaultFont = nullptr;
			mediumFont = nullptr;
			
			largeSolidIconFont = nullptr;
			largeRegularIconFont = nullptr;
			mediumSolidIconFont = nullptr;
			mediumRegularIconFont = nullptr;
            
			// Cleanup
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}
        
		ImFont* getDefaultFont()
		{
			return defaultFont;
		}
        
		ImFont* getMediumFont()
		{
			return mediumFont;
		}

		ImFont* getMonoFont()
		{
			return monoFont;
		}
        
		ImFont* getLargeSolidIconFont()
		{
			return largeSolidIconFont;
		}

		ImFont* getMediumSolidIconFont()
		{
			return mediumSolidIconFont;
		}

		ImFont* getLargeRegularIconFont()
		{
			return largeRegularIconFont;
		}

		ImFont* getMediumRegularIconFont()
		{
			return mediumRegularIconFont;
		}

		// ---------- Internal Functions ----------
		static void loadCustomTheme()
		{
			ImGuiStyle* style = &ImGui::GetStyle();

			// Sizes
			style->ScrollbarRounding = 0.0f;
			style->FramePadding = Vec2{ 12.0f, 10.0f };

			// Colors
			ImVec4* colors = style->Colors;

			colors[ImGuiCol_Text] = Colors::Neutral[0];
			colors[ImGuiCol_TextDisabled] = Colors::Neutral[3];

			colors[ImGuiCol_WindowBg] = Colors::Neutral[8];
			colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
			colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_FrameBg] = Colors::Neutral[6];
			colors[ImGuiCol_FrameBgHovered] = Colors::Neutral[4];
			colors[ImGuiCol_FrameBgActive] = Colors::Neutral[5];

			colors[ImGuiCol_TitleBg] = Colors::Neutral[6];
			colors[ImGuiCol_TitleBgActive] = Colors::Primary[8];
			colors[ImGuiCol_TitleBgCollapsed] = Colors::Neutral[6];
			colors[ImGuiCol_MenuBarBg] = Colors::Neutral[8];

			colors[ImGuiCol_ScrollbarBg] = Colors::Neutral[6];
			colors[ImGuiCol_ScrollbarGrab] = Colors::Neutral[4];
			colors[ImGuiCol_ScrollbarGrabHovered] = Colors::Neutral[2];
			colors[ImGuiCol_ScrollbarGrabActive] = Colors::Neutral[3];
			colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
			colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);

			colors[ImGuiCol_Button] = Colors::Neutral[5];
			colors[ImGuiCol_ButtonHovered] = Colors::Neutral[4];
			colors[ImGuiCol_ButtonActive] = Colors::Neutral[6];

			colors[ImGuiCol_Header] = Colors::Neutral[6];
			colors[ImGuiCol_HeaderHovered] = Colors::Neutral[5];
			colors[ImGuiCol_HeaderActive] = Colors::Neutral[4];

			colors[ImGuiCol_Separator] = Colors::Neutral[5];
			colors[ImGuiCol_SeparatorHovered] = Colors::Neutral[3];
			colors[ImGuiCol_SeparatorActive] = Colors::Neutral[2];
			colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);

			colors[ImGuiCol_Tab] = Colors::Primary[8];
			colors[ImGuiCol_TabHovered] = Colors::Primary[5];
			colors[ImGuiCol_TabActive] = Colors::Primary[6];
			colors[ImGuiCol_TabUnfocused] = Colors::Neutral[6];
			colors[ImGuiCol_TabUnfocusedActive] = Colors::Neutral[8];
			colors[ImGuiCol_DockingPreview] = ImVec4(
				colors[ImGuiCol_Header].x,
				colors[ImGuiCol_Header].x,
				colors[ImGuiCol_Header].x,
				0.7f
			);

			colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
			colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colors[ImGuiCol_TableHeaderBg] = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
			colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);   // Prefer using Alpha=1.0 here
			colors[ImGuiCol_TableBorderLight] = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);   // Prefer using Alpha=1.0 here
			colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);

			colors[ImGuiCol_TextSelectedBg] = Colors::Primary[5];
			colors[ImGuiCol_DragDropTarget] = Colors::AccentGreen[2];

			colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
			colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
		}
	}
}