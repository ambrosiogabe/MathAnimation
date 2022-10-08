#include "editor/SceneManagementPanel.h"

namespace MathAnim
{
	namespace SceneManagementPanel
	{
		void init()
		{

		}

		void update(SceneData& sd)
		{
			ImGui::Begin("Scene Manager");



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
				int32 strLength = data.sceneNames[i].length();
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
