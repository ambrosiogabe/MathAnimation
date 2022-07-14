#include "editor/ProjectScreen.h"
#include "core/ProjectApp.h"
#include "core/Window.h"

#include <imgui.h>

namespace MathAnim
{
	namespace ProjectScreen
	{
		static ImFont* largeFont;
		static ImFont* defaultFont;

		void init()
		{
			ImGuiIO& io = ImGui::GetIO();
			largeFont = io.Fonts->AddFontFromFileTTF("C:/Windows/FONTS/SegoeUI.ttf", 20.0f);
			defaultFont = io.Fonts->AddFontFromFileTTF("C:/Windows/FONTS/SegoeUI.ttf", 12.0f);
		}

		void update()
		{
			Window* window = ProjectApp::getWindow();
			ImVec2 size = { (float)window->width, (float)window->height };
			ImGui::SetNextWindowSize(size, ImGuiCond_Always);
			ImGui::PushFont(defaultFont);
			ImGui::Begin("Project Selector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

			ImGui::Text("Welcome to the project selector!");

			ImGui::End();
			ImGui::PopFont();
		}

		void free()
		{
			ImGuiIO io = ImGui::GetIO();
			io.Fonts->ClearFonts();
		}
	}
}