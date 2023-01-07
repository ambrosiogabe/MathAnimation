#include "editor/ProjectScreen.h"
#include "core/ProjectApp.h"
#include "core/Window.h"
#include "core/Colors.h"
#include "platform/Platform.h"
#include "renderer/Texture.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

namespace MathAnim
{
	namespace ProjectScreen
	{
		static ImFont *largeFont;
		static ImFont *defaultFont;
		static float titleBarHeight = 100.0f;
		static float buttonAreaHeight = 80.0f;
		static float dropShadowHeight = 20;

		static float createProjectTitleBarHeight = 45.0f;
		static ImVec2 createProjectWindowSize = ImVec2(535.0f, 195.0f);

		static ImVec2 projectAreaPadding = ImVec2(30, 0);
		static ImVec2 iconPadding = ImVec2(30, 45);
		static ImVec2 createProjectInputPadding = ImVec2(0, 30);
		static float iconBorderRounding = 3.0f;
		static float iconBorderThickness = 2.0f;

		static float buttonWidth = 150.0f;
		static float buttonPadding = 8.0f;
		static float buttonHeight = 30.0f;
		static float buttonRounding = 20.0f;
		static float borderSize = 2.0f;

		static std::vector<ProjectInfo> projects;
		static int selectedProjectIndex;

		static bool createNewProject;

		// --------------- Internal Functions ---------------
		static bool createNewProjectGui();
		static void deserializeMetadata();
		static void serializeMetadata();

		void init()
		{
			createNewProject = false;

			float dpiScale = ProjectApp::getWindow()->getContentScale();
			dropShadowHeight *= dpiScale;
			buttonWidth *= dpiScale;
			buttonHeight *= dpiScale;
			buttonPadding *= dpiScale;
			buttonRounding *= dpiScale;
			borderSize *= dpiScale;
			iconBorderRounding *= dpiScale;
			iconBorderThickness *= dpiScale;
			titleBarHeight = (float)titleBarHeight * dpiScale;
			buttonAreaHeight = (float)buttonAreaHeight * dpiScale;
			createProjectTitleBarHeight = (float)createProjectTitleBarHeight * dpiScale;
			createProjectWindowSize.x *= dpiScale;
			createProjectWindowSize.y *= dpiScale;
			createProjectInputPadding.x *= dpiScale;
			createProjectInputPadding.y *= dpiScale;
			projectAreaPadding.x *= dpiScale;
			projectAreaPadding.y *= dpiScale;
			iconPadding.x *= dpiScale;
			iconPadding.y *= dpiScale;

			ImGuiIO &io = ImGui::GetIO();
#if defined(_WIN32)
			largeFont = io.Fonts->AddFontFromFileTTF("C:/Windows/FONTS/SegoeUI.ttf", 36.0f * dpiScale);
			defaultFont = io.Fonts->AddFontFromFileTTF("C:/Windows/FONTS/SegoeUI.ttf", 20.0f * dpiScale);
#elif defined(__linux__)
			largeFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/TTF/DejaVuSans.ttf", 36.0f * dpiScale);
			defaultFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/TTF/DejaVuSans.ttf", 20.0f * dpiScale);
#endif

			selectedProjectIndex = -1;

			deserializeMetadata();

			for (int i = 0; i < projects.size(); i++)
			{
				if (Platform::fileExists(projects[i].previewImageFilepath.c_str()))
				{
					projects[i].texture = TextureBuilder()
																		.setFilepath(projects[i].previewImageFilepath.c_str())
																		.generate(true);
				}
				else
				{
					projects.erase(projects.begin() + i);
					i--;
				}
			}
		}

		bool update()
		{
			bool res = false;

			Window *window = ProjectApp::getWindow();
			ImVec2 size = {(float)window->width, (float)window->height};
			ImGui::SetNextWindowSize(size, ImGuiCond_Always);
			ImGuiViewport *viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->GetWorkCenter() - viewport->WorkSize / 2.0f, ImGuiCond_Always);

			ImGui::PushFont(defaultFont);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::Neutral[7]);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, Colors::Neutral[7]);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Colors::Neutral[4]);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, Colors::Neutral[2]);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, Colors::Neutral[2]);
			ImGui::Begin("Project Selector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking);
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			ImVec2 availableSpace = ImGui::GetContentRegionAvail();

			{
				ImGui::PushFont(largeFont);
				std::string appTitle = "Math Anim";
				ImVec2 textSize = ImGui::CalcTextSize(appTitle.c_str());
				drawList->AddRectFilled(cursorPos, cursorPos + ImVec2(availableSpace.x, titleBarHeight), ImColor(Colors::Neutral[8]));

                ImGui::SetCursorScreenPos(cursorPos + ImVec2(
                        availableSpace.x / 2.0f - textSize.x / 2.0f,
                        titleBarHeight / 2.0f - textSize.y / 2.0f
                ));
                ImGui::Text("%s", appTitle.c_str());

                ImGui::PopFont();

				ImGui::PopFont();
			}

			ImGui::SetCursorScreenPos(cursorPos + ImVec2(0.0f, titleBarHeight));
			ImGui::BeginChild("##ProjectArea", ImVec2(availableSpace.x, availableSpace.y - (titleBarHeight + buttonAreaHeight)));

			float iconWidth = (availableSpace.x - projectAreaPadding.x * 2.0f);
			iconWidth = (iconWidth / 3.0f) - (iconPadding.x * 2.0f);
			float iconHeight = (1080.0f / 1920.0f) * iconWidth;
			ImVec2 projectAreaBegin = ImGui::GetCursorScreenPos();
			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + projectAreaPadding);
			float absProjectAreaX = ImGui::GetCursorScreenPos().x;

			float childScrollY = ImGui::GetScrollY();
			ImVec2 buttonAreaBegin = projectAreaBegin + ImVec2(0.0f, availableSpace.y - (titleBarHeight + buttonAreaHeight) + childScrollY);

			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(0, 0, 0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, iconBorderRounding);
			for (int i = 0; i < projects.size(); i++)
			{
				ProjectInfo &pi = projects[i];
				ImVec2 iconStart = ImGui::GetCursorPos();
				ImVec2 absIconStart = ImGui::GetCursorScreenPos();
				ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + iconPadding);

				std::string uid = "##" + projects[i].projectFilepath;
				bool selected = i == selectedProjectIndex;
				if (ImGui::Selectable(uid.c_str(), &selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(iconWidth, iconHeight)))
				{
					selectedProjectIndex = i;
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					res = true;
				}

				// Draw Project Preview
				ImVec2 rectMin = ImGui::GetItemRectMin();
				ImVec2 rectMax = ImGui::GetItemRectMax();
				drawList->PushClipRect(ImGui::GetCurrentWindow()->ClipRect.Min, ImGui::GetCurrentWindow()->ClipRect.Max);
				drawList->AddImageRounded(
                        (ImTextureID)(uint64)projects[i].texture.graphicsId,
                        rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1),
                        IM_COL32(255, 255, 255, 255), iconBorderRounding);

				// Draw highlight border
				const ImVec4 &borderColor =
						i == selectedProjectIndex
								? Colors::AccentGreen[2]
						: ImGui::IsItemHovered()
								? Colors::Neutral[2]
								: Colors::Neutral[9];
				drawList->AddRect(rectMin, rectMax, ImColor(borderColor), iconBorderRounding, 0, iconBorderThickness);
				drawList->PopClipRect();

				// Draw our icon
				ImVec2 textSize = ImGui::CalcTextSize(pi.projectName.c_str());
				float offsetX = (iconWidth / 2.0f - textSize.x / 2.0f) + iconPadding.x;
				ImGui::PushStyleColor(ImGuiCol_Text, i == selectedProjectIndex ? Colors::Neutral[0] : Colors::Neutral[2]);
				ImGui::SetCursorPosX(iconStart.x + offsetX);
				ImGui::Text("%s", pi.projectName.c_str());
				ImGui::PopStyleColor();

				// Increment to next icon position
				ImGui::SetCursorScreenPos(absIconStart + ImVec2(iconWidth + iconPadding.x * 2.0f, 0.0f));

				if (((i + 1) % 3) == 0)
				{
					// Increment to new line
					ImGui::SetCursorScreenPos(ImVec2(absProjectAreaX, absIconStart.y + iconHeight + textSize.y + iconPadding.y));
				}
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);

			drawList->AddRectFilledMultiColor(
					buttonAreaBegin + ImVec2(0.0f, -dropShadowHeight),
					buttonAreaBegin + ImVec2(availableSpace.x, 0.0f),
					IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
					ImColor(Colors::Neutral[8]), ImColor(Colors::Neutral[8]));
			ImGui::EndChild();

			float buttonOffsetX = (availableSpace.x - buttonWidth * 2.0f - buttonPadding * 2.0f);
			float buttonOffsetY = (buttonAreaHeight / 2.0f - buttonHeight / 2.0f);
			ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(buttonOffsetX, buttonOffsetY));

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, buttonRounding);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, borderSize);
			ImGui::PushStyleColor(ImGuiCol_Button, Colors::Neutral[7]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::Neutral[7]);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::Neutral[7]);

			static bool newProjectFocused = false;
			ImGui::PushStyleColor(ImGuiCol_Border, newProjectFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			ImGui::PushStyleColor(ImGuiCol_Text, newProjectFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			if (ImGui::Button("New Project", ImVec2(buttonWidth, buttonHeight)))
			{
				// Create new project screen
				createNewProject = true;
			}
			newProjectFocused = ImGui::IsItemHovered();
			ImGui::PopStyleColor(2);
			ImGui::SameLine();

			static bool openFocused = false;
			ImGui::PushStyleColor(ImGuiCol_Border, openFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			ImGui::PushStyleColor(ImGuiCol_Text, openFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			if (ImGui::Button("Open", ImVec2(buttonWidth, buttonHeight)))
			{
				res = true;
			}
			openFocused = ImGui::IsItemHovered();
			ImGui::PopStyleColor(2);

			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);

			ImGui::End();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(5);
			ImGui::PopFont();

			if (createNewProject)
			{
				res = createNewProjectGui();
			}

			return res;
		}

		void free()
		{
			serializeMetadata();
			for (int i = 0; i < projects.size(); i++)
			{
				projects[i].texture.destroy();
			}

			ImGuiIO io = ImGui::GetIO();
			io.Fonts->ClearFonts();
		}

		const ProjectInfo &getSelectedProject()
		{
			if (selectedProjectIndex >= 0 && selectedProjectIndex < projects.size())
			{
				return projects[selectedProjectIndex];
			}

			static ProjectInfo dummy;
			dummy.projectFilepath = "";
			dummy.previewImageFilepath = "";
			dummy.projectName = "";
			return dummy;
		}

		// --------------- Internal Functions ---------------
		static bool createNewProjectGui()
		{
			bool res = false;

			ImGui::SetNextWindowSize(createProjectWindowSize, ImGuiCond_Always);

			ImGui::PushFont(defaultFont);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::Neutral[7]);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, Colors::Neutral[7]);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Colors::Neutral[4]);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, Colors::Neutral[2]);
			ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, Colors::Neutral[2]);
			ImGui::PushStyleColor(ImGuiCol_Border, Colors::Neutral[9]);
			ImGui::Begin("Create New Project", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking);
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			ImDrawList *drawList = ImGui::GetWindowDrawList();

			ImGui::PushFont(defaultFont);
			std::string createProjectTitle = "Create New Project";
			ImVec2 textSize = ImGui::CalcTextSize(createProjectTitle.c_str());
			ImVec2 availableSpace = ImGui::GetContentRegionAvail();
			drawList->AddRectFilled(cursorPos, cursorPos + ImVec2(availableSpace.x, createProjectTitleBarHeight), ImColor(Colors::Neutral[8]));

            ImGui::SetCursorScreenPos(cursorPos + ImVec2(
                    projectAreaPadding.x,
                    createProjectTitleBarHeight / 2.0f - textSize.y / 2.0f
            ));
            ImGui::Text("%s", createProjectTitle.c_str());

			ImGui::PopFont();

			float inputWidth = availableSpace.x - projectAreaPadding.x * 2.0f - createProjectInputPadding.x * 2.0f;
			ImGui::SetNextItemWidth(inputWidth);
			static bool inputFocused = false;
			ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::Neutral[8]);
			ImGui::PushStyleColor(ImGuiCol_Text, inputFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, Colors::Neutral[5]);
			ImGui::PushStyleColor(ImGuiCol_Border, Colors::Neutral[9]);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::SetCursorScreenPos(cursorPos + ImVec2(0.0f, createProjectTitleBarHeight) + projectAreaPadding + createProjectInputPadding);

			constexpr int maxBufferSize = 256;
			static char buffer[maxBufferSize] = "Untitled Project";
			ImGui::InputText("##createProjectTitle", buffer, maxBufferSize);
			inputFocused = ImGui::IsItemHovered() || ImGui::IsItemActive();
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar();

			float buttonOffsetX = (availableSpace.x - buttonWidth * 2.0f - buttonPadding * 2.0f) - projectAreaPadding.x;
			float buttonOffsetY = availableSpace.y - createProjectInputPadding.y - buttonHeight;
			ImGui::SetCursorScreenPos(cursorPos + ImVec2(buttonOffsetX, buttonOffsetY));

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, buttonRounding);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, borderSize);
			ImGui::PushStyleColor(ImGuiCol_Button, Colors::Neutral[7]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::Neutral[7]);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::Neutral[7]);

			static bool cancelButtonFocused = false;
			ImGui::PushStyleColor(ImGuiCol_Border, cancelButtonFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			ImGui::PushStyleColor(ImGuiCol_Text, cancelButtonFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			if (ImGui::Button("Cancel", ImVec2(buttonWidth, buttonHeight)))
			{
				createNewProject = false;
			}
			cancelButtonFocused = ImGui::IsItemHovered();
			ImGui::PopStyleColor(2);
			ImGui::SameLine();

			static bool createButtonFocused = false;
			ImGui::PushStyleColor(ImGuiCol_Border, createButtonFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			ImGui::PushStyleColor(ImGuiCol_Text, createButtonFocused ? Colors::Neutral[0] : Colors::Neutral[2]);
			if (ImGui::Button("Create", ImVec2(buttonWidth, buttonHeight)))
			{
				ProjectInfo pi;
				pi.projectName = std::string(buffer);
				std::string cleanProjectName = pi.projectName;
				// Replace any non-alpha characters with underscores to normalize the filepath
				for (int i = 0; i < cleanProjectName.length(); i++)
				{
					if (!(cleanProjectName[i] >= 'a' && cleanProjectName[i] <= 'z' ||
								cleanProjectName[i] >= 'A' && cleanProjectName[i] <= 'Z' ||
								cleanProjectName[i] >= '0' && cleanProjectName[i] <= '9'))
					{
						cleanProjectName[i] = '_';
					}
				}

				int num = 0;
				std::string uniquePrjName = cleanProjectName;
				while (Platform::dirExists((ProjectApp::getAppRoot() / uniquePrjName).string().c_str()))
				{
					// Append a number if this filename already exists
					uniquePrjName = cleanProjectName + std::to_string(num);
					num++;
				}
				cleanProjectName = uniquePrjName;

				pi.projectFilepath = (ProjectApp::getAppRoot() / cleanProjectName / "project.bin").string();
				pi.previewImageFilepath = (ProjectApp::getAppRoot() / cleanProjectName / "projectPreview.png").string();
				Platform::createDirIfNotExists((ProjectApp::getAppRoot() / cleanProjectName).string().c_str());
				projects.push_back(pi);
				selectedProjectIndex = (int32)projects.size() - 1;
				res = true;
			}
			createButtonFocused = ImGui::IsItemHovered();
			ImGui::PopStyleColor(2);

			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);

			ImGui::End();

			ImGui::PopStyleColor(6);
			ImGui::PopStyleVar();
			ImGui::PopFont();

			return res;
		}

		static void deserializeMetadata()
		{
			std::string metadataFilepath = (ProjectApp::getAppRoot() / "projects.bin").string();
			FILE *fp = fopen(metadataFilepath.c_str(), "rb");
			if (!fp)
			{
				return;
			}

			fseek(fp, 0, SEEK_END);
			size_t fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			RawMemory memory;
			memory.init(fileSize);
			fread(memory.data, memory.size, 1, fp);
			fclose(fp);
			fp = nullptr;

			// magicNumber         -> uint64 0xDEADBEEF
			// selectedProject     -> int32
			// numProjects         -> uint32
			uint64 magicNumber;
			memory.read<uint64>(&magicNumber);
			if (magicNumber != 0xDEADBEEF)
			{
				g_logger_error("Project file corrupted. Magic number not 0xDEADBEEF, instead got: 0x%8x", magicNumber);
				goto cleanup;
			}
			memory.read<int32>(&selectedProjectIndex);

			uint32 numProjects;
			memory.read<uint32>(&numProjects);
			for (uint32 i = 0; i < numProjects; i++)
			{
				// prjFilepathLength   -> uint32
				// prjFilepath         -> uint8[prjFilepathLength]
				// imageFilepathLength -> uint32
				// imageFilepath       -> uint8[imageFilepathLength]
				// prjNameLength       -> uint32
				// prjName             -> uint8[prjNameLength]
				uint32 prjFilepathLength;
				memory.read<uint32>(&prjFilepathLength);
				uint8 *scratchMemory = (uint8 *)g_memory_allocate(sizeof(uint8) * prjFilepathLength);
				memory.readDangerous(scratchMemory, prjFilepathLength);

				ProjectInfo projectInfo;
				projectInfo.projectFilepath = std::string((char *)scratchMemory);

				uint32 imageFilepathLength;
				memory.read<uint32>(&imageFilepathLength);
				scratchMemory = (uint8 *)g_memory_realloc(scratchMemory, sizeof(uint8) * imageFilepathLength);
				memory.readDangerous(scratchMemory, imageFilepathLength);

				projectInfo.previewImageFilepath = std::string((char *)scratchMemory);

				uint32 prjNameLength;
				memory.read<uint32>(&prjNameLength);
				scratchMemory = (uint8 *)g_memory_realloc(scratchMemory, sizeof(uint8) * prjNameLength);
				memory.readDangerous(scratchMemory, prjNameLength);

				projectInfo.projectName = std::string((char *)scratchMemory);
				g_memory_free(scratchMemory);

				projects.push_back(projectInfo);
			}

		cleanup:
			memory.free();
			return;
		}

		static void serializeMetadata()
		{
			RawMemory memory;
			memory.init(sizeof(uint64));

			// magicNumber         -> uint64 0xDEADBEEF
			// selectedProject    -> int32
			// numProjects         -> uint32
			uint64 magicNumber = 0xDEADBEEF;
			memory.write<uint64>(&magicNumber);
			memory.write<int32>(&selectedProjectIndex);

			uint32 numProjects = (uint32)projects.size();
			memory.write<uint32>(&numProjects);
			for (uint32 i = 0; i < numProjects; i++)
			{
				// prjFilepathLength   -> uint32
				// prjFilepath         -> uint8[prjFilepathLength]
				// imageFilepathLength -> uint32
				// imageFilepath       -> uint8[imageFilepathLength]
				// prjNameLength       -> uint32
				// prjName             -> uint8[prjNameLength]
				uint32 prjFilepathLength = (uint32)projects[i].projectFilepath.length() + 1;
				memory.write<uint32>(&prjFilepathLength);
				memory.writeDangerous((const uint8 *)projects[i].projectFilepath.c_str(), prjFilepathLength);

				uint32 imageFilepathLength = (uint32)projects[i].previewImageFilepath.length() + 1;
				memory.write<uint32>(&imageFilepathLength);
				memory.writeDangerous((const uint8 *)projects[i].previewImageFilepath.c_str(), imageFilepathLength);

				uint32 prjNameLength = (uint32)projects[i].projectName.length() + 1;
				memory.write<uint32>(&prjNameLength);
				memory.writeDangerous((const uint8 *)projects[i].projectName.c_str(), prjNameLength);
			}

			std::string metadataFilepath = (ProjectApp::getAppRoot() / "projects.bin").string();
			FILE *fp = fopen(metadataFilepath.c_str(), "wb");
			if (!fp)
			{
				g_logger_error("Failed to open file for writing.");
				memory.free();
				return;
			}

			fwrite(memory.data, memory.size, 1, fp);
			fclose(fp);

			memory.free();
			return;
		}
	}
}