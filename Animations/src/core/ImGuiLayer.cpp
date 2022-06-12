#include "core/ImGuiLayer.h"
#include "core/Window.h"
#include "utils/FontAwesome.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "ImGuizmo.h"

#include <GLFW/glfw3.h>

namespace MathAnim
{
	namespace ImGuiLayer
	{
		static ImFont* smallFont;
		static ImFont* bigFont;
		static ImFont* regularIconFont;
        
		void init(const Window& window)
		{
			// Set up dear imgui
			ImGui::CreateContext();
            
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
			//io.ConfigViewportsNoAutoMerge = true;
			//io.ConfigViewportsNoTaskBarIcon = true;
            
            // NOTE(voxel): This looks right for my machine (May have to go back and forth on the value 128.f
			float fontSize = window.width / 128.f;
			smallFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Arial.ttf", fontSize);
            
			static const ImWchar iconRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
			ImFontConfig config; 
			config.MergeMode = true;
			//config.GlyphMinAdvanceX = fontSize; // Use this if you want the icon to be monospaced
			// TODO: Optimize ram usage here...
			config.SizePixels = fontSize * 2.0f;
			config.PixelSnapH = true;
			io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.ttf", fontSize * 2.0f, &config, iconRanges);
			bigFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/Arial.ttf", fontSize * 1.5f);
            
			// Add this font separately
			config.MergeMode = false;
			config.SizePixels = fontSize * 1.5f;
			regularIconFont = io.Fonts->AddFontFromFileTTF("assets/fonts/fa-regular-400.ttf", fontSize * 1.5f, &config, iconRanges);
			
			// config.MergeMode = false;
			//const ImWchar iconRangesBrands[] = { ICON_MIN_FAB, ICON_MAX_FAB, 0 };
			//io.Fonts->AddFontFromFileTTF("assets/fonts/fa-brands-400.ttf", fontSize, &config, iconRanges);
            
			ImGui::StyleColorsDark();
            
			// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
			ImGuiStyle& style = ImGui::GetStyle();
			style.ScaleAllSizes(window.getContentScale());
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}
            
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
			ImGuizmo::BeginFrame();
            
            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		}
        
		void endFrame()
		{
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
			smallFont = nullptr;
			bigFont = nullptr;
			regularIconFont = nullptr;
            
			// Cleanup
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}
        
		ImFont* getDefaultFont()
		{
			return smallFont;
		}
        
		ImFont* getLargeFont()
		{
			return bigFont;
		}
        
		ImFont* getRegularIconFont()
		{
			return regularIconFont;
		}
	}
}