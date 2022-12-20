#include "editor/AssetManagerPanel.h"
#include "editor/ImGuiExtended.h"
#include "utils/FontAwesome.h"
#include "platform/Platform.h"
#include "platform/FileSystemWatcher.h"
#include "scripting/LuauLayer.h"

namespace MathAnim
{
	namespace AssetManagerPanel
	{
		typedef void (*AddButtonCallbackFn)(const char* filename);

		// -------------- Internal Functions --------------
		static void iterateDirectory(const std::string& directory, AddButtonCallbackFn callback = nullptr, const char* defaultNewFilename = nullptr, const char* addButtonText = nullptr);
		static void onScriptChanged(const std::filesystem::path& scriptPath);
		static void onScriptDeleted(const std::filesystem::path& scriptPath);
		static void newScriptAddedCallback(const char* filename);

		// -------------- Internal Variables --------------
		static std::string assetsRoot;
		static std::string scriptsRoot;
		static FileSystemWatcher* scriptWatcher = nullptr;

		void init(const std::string& inAssetsRoot)
		{
			assetsRoot = inAssetsRoot;
			scriptsRoot = assetsRoot + "/scripts";

			// Initialize the script watcher
			scriptWatcher = (FileSystemWatcher*)g_memory_allocate(sizeof(FileSystemWatcher));
			new(scriptWatcher)FileSystemWatcher();
			scriptWatcher->path = scriptsRoot;
			scriptWatcher->onChanged = onScriptChanged;
			scriptWatcher->onRenamed = onScriptChanged;
			scriptWatcher->onCreated = onScriptChanged;
			scriptWatcher->onDeleted = onScriptDeleted;
			scriptWatcher->includeSubdirectories = true;
			scriptWatcher->notifyFilters |= NotifyFilters::FileName;
			scriptWatcher->notifyFilters |= NotifyFilters::LastWrite;
			scriptWatcher->notifyFilters |= NotifyFilters::Attributes;
			scriptWatcher->start();
		}

		void update()
		{
			scriptWatcher->poll();

			constexpr size_t stringBufferSize = 256;
			char stringBuffer[stringBufferSize];

			ImGui::Begin("Asset Manager");

			iterateDirectory(scriptsRoot, newScriptAddedCallback, "Script.luau", "Add Script");

			ImGui::End();
		}

		void free()
		{
			if (scriptWatcher)
			{
				scriptWatcher->~FileSystemWatcher();
				g_memory_free(scriptWatcher);
			}

			scriptWatcher = nullptr;
		}

		// -------------- Internal Functions --------------
		static void iterateDirectory(const std::string& directory, AddButtonCallbackFn callback, const char* defaultNewFilename, const char* addButtonText)
		{
			constexpr size_t stringBufferSize = _MAX_PATH;
			char stringBuffer[stringBufferSize];

			ImVec2 buttonSize = ImVec2(256, 0);
			static int lastSelectedFile = -1;
			int fileIndex = 0;
			for (auto file : std::filesystem::directory_iterator(directory))
			{
				const char* icon = ICON_FA_SCROLL;
				std::string filename = file.path().filename().string();

				// Shorten name length if it exceeds the buffer somehow
				if (filename.length() > stringBufferSize - 1)
				{
					filename = filename.substr(0, stringBufferSize - 1);
				}
				g_memory_copyMem(stringBuffer, (void*)filename.c_str(), filename.length());
				stringBuffer[filename.length()] = '\0';

				if (ImGuiExtended::RenamableIconSelectable(icon, stringBuffer, stringBufferSize, lastSelectedFile == fileIndex, 132.0f))
				{
					if (filename != stringBuffer)
					{
						std::filesystem::path oldFilepath = std::filesystem::path(directory) / filename;
						std::filesystem::path newFilepath = std::filesystem::path(directory) / stringBuffer;
						// File was renamed, so rename it on the filesystem to match it
						if (std::rename(oldFilepath.string().c_str(), newFilepath.string().c_str()))
						{
							g_logger_error("There was an error renaming file '%s' to '%s'.", filename.c_str(), stringBuffer);
						}
					}

					lastSelectedFile = fileIndex;
				}

				if (ImGui::BeginPopupContextItem(stringBuffer))
				{
					if (ImGui::MenuItem("Delete"))
					{
						// Delete the current file
						std::filesystem::path filepath = std::filesystem::path(directory) / filename;
						std::remove(filepath.string().c_str());
					}
					ImGui::EndPopup();
				}

				if (ImGui::GetContentRegionAvail().x > buttonSize.x)
				{
					ImGui::SameLine();
				}

				fileIndex++;
			}

			if (addButtonText && ImGuiExtended::VerticalIconButton(ICON_FA_PLUS, addButtonText, 132.0f))
			{
				int i = 1;
				std::filesystem::path newFilenamePath = directory / std::filesystem::path(defaultNewFilename);
				std::string newFilename = newFilenamePath.string();
				while (Platform::fileExists(newFilename.c_str()))
				{
					newFilename = newFilenamePath.stem().string() + "_" + std::to_string(i) + newFilenamePath.extension().string();
					newFilename = (newFilenamePath.parent_path() / std::filesystem::path(newFilename)).string();
					i++;
				}

				FILE* fp = fopen(newFilename.c_str(), "w");
				fclose(fp);

				if (callback)
				{
					callback(newFilename.c_str());
				}
			}
		}

		static void onScriptChanged(const std::filesystem::path& scriptPath)
		{
			LuauLayer::compile(scriptPath.filename().string());
			LuauLayer::execute(scriptPath.filename().string());
		}

		static void onScriptDeleted(const std::filesystem::path& scriptPath)
		{
			LuauLayer::remove(scriptPath.filename().string());
		}

		static void newScriptAddedCallback(const char* filename)
		{
			Platform::openFileWithVsCode(filename);
		}
	}
}
