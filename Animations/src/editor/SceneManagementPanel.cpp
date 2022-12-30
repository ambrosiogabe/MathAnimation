#include "editor/SceneManagementPanel.h"
#include "editor/ImGuiExtended.h"
#include "core/Application.h"
#include "core/Colors.h"

#include "utils/FontAwesome.h"

namespace MathAnim
{
	namespace SceneManagementPanel
	{
		void init()
		{

		}

		void update(SceneData& sd)
		{
			constexpr size_t stringBufferSize = 256;
			char stringBuffer[stringBufferSize];

			ImGui::Begin("Scene Manager");

			float windowWidth = ImGui::GetWindowWidth();
			float cursorX = 0.0f;

			ImVec2 buttonSize = ImVec2(256, 0);
			for (int i = 0; i < sd.sceneNames.size(); i++)
			{
				const char* icon = ICON_FA_FILE;

				// Shorten name length if it exceeds the buffer somehow
				if (sd.sceneNames[i].length() >= stringBufferSize - 1)
				{
					sd.sceneNames[i] = sd.sceneNames[i].substr(0, stringBufferSize - 1);
				}
				g_memory_copyMem(stringBuffer, (void*)sd.sceneNames[i].c_str(), sd.sceneNames[i].length());
				stringBuffer[sd.sceneNames[i].length()] = '\0';

				if (i == sd.currentScene)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentGreen[1]);
				}

				if (ImGuiExtended::RenamableIconSelectable(icon, stringBuffer, stringBufferSize, i == sd.currentScene, 132.0f))
				{
					if (i != sd.currentScene)
					{
						Application::changeSceneTo(sd.sceneNames[i]);
					}
					else
					{
						// Scene was renamed, so rename it and save the result then delete the old scene
						std::string oldSceneName = sd.sceneNames[i];
						sd.sceneNames[i] = stringBuffer;
						Application::saveCurrentScene();
						Application::deleteScene(oldSceneName);
					}
				}

				if (i == sd.currentScene)
				{
					ImGui::PopStyleColor();
				}

				if (ImGui::BeginPopupContextItem(stringBuffer))
				{
					bool isDisabled = sd.sceneNames.size() == 1;
					ImGui::BeginDisabled(isDisabled);
					if (ImGui::MenuItem("Delete"))
					{
						// Delete the current scene
						Application::deleteScene(sd.sceneNames[i]);
						if (i == sd.currentScene)
						{
							// Change to the next scene if we deleted the current scene
							sd.currentScene = (sd.currentScene + 1) % sd.sceneNames.size();
							Application::changeSceneTo(sd.sceneNames[sd.currentScene], false);
						}
						sd.sceneNames.erase(sd.sceneNames.begin() + i);
						i--;
					}
					ImGui::EndDisabled();
					ImGui::EndPopup();
				}

				if (cursorX + (buttonSize.x * 2.0f) < windowWidth)
				{
					ImGui::SameLine();
				}
				else
				{
					cursorX = 0.0f;
				}
				cursorX += buttonSize.x;
			}

			if (ImGuiExtended::VerticalIconButton(ICON_FA_PLUS, "Add Scene", 132.0f))
			{
				std::string newSceneName = "New Scene " + std::to_string(sd.sceneNames.size());
				sd.sceneNames.push_back(newSceneName);
				Application::changeSceneTo(newSceneName);
			}

			ImGui::End();
		}

		void free()
		{

		}

		RawMemory serialize(const SceneData& data)
		{
			// numScenes      -> i32
			// sceneNames     -> strings[numScenes]
			//    strLength   -> i32
			//    string      -> u8[strLength]
			// currentScene   -> i32
			RawMemory res;
			res.init(sizeof(SceneData));

			int32 numScenes = (int32)data.sceneNames.size();
			res.write<int32>(&numScenes);
			for (int i = 0; i < numScenes; i++)
			{
				int32 strLength = (int32)data.sceneNames[i].length();
				res.write<int32>(&strLength);
				res.writeDangerous((uint8*)data.sceneNames[i].c_str(), strLength * sizeof(uint8));
			}

			res.write<int32>(&data.currentScene);

			res.shrinkToFit();
			return res;
		}

		SceneData deserialize(RawMemory& memory)
		{
			// numScenes      -> i32
			// sceneNames     -> strings[numScenes]
			//    strLength   -> i32
			//    string      -> u8[strLength]
			// currentScene   -> i32
			SceneData res;

			int32 numScenes;
			memory.read<int32>(&numScenes);
			for (int i = 0; i < numScenes; i++)
			{
				int32 strLength;
				memory.read<int32>(&strLength);
				uint8* string = (uint8*)g_memory_allocate(sizeof(uint8) * (strLength + 1));
				memory.readDangerous(string, sizeof(uint8) * strLength);
				string[strLength] = '\0';
				res.sceneNames.emplace_back(std::string((char*)string));
				g_memory_free(string);
			}

			memory.read<int32>(&res.currentScene);

			return res;
		}
	}
}
