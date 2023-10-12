#include "editor/panels/AssetManagerPanel.h"
#include "editor/panels/CodeEditorPanelManager.h"
#include "editor/imgui/ImGuiExtended.h"
#include "utils/FontAwesome.h"
#include "platform/Platform.h"
#include "platform/FileSystemWatcher.h"
#include "scripting/LuauLayer.h"

namespace MathAnim
{
	namespace AssetManagerPanel
	{
		typedef void (*AddButtonCallbackFn)(const char* filename);
		typedef void (*FileRenamedCallbackFn)(const char* oldFilename, const char* newFilename);
		typedef void (*FileSelectedFn)(const char* filename);
		typedef void (*FileDeletedFn)(const char* filename);

		// -------------- Internal Functions --------------
		static void iterateDirectory(
			const std::filesystem::path& directory,
			AddButtonCallbackFn callback = nullptr,
			FileRenamedCallbackFn renameCallback = nullptr,
			FileSelectedFn fileSelectedCallback = nullptr,
			FileDeletedFn fileDeletedCallback = nullptr,
			const char* defaultNewFilename = nullptr,
			const char* addButtonText = nullptr
		);
		static void onScriptChanged(const std::filesystem::path& scriptPath);
		static void onScriptRenamed(const std::filesystem::path& scriptPath);
		static void onScriptCreated(const std::filesystem::path& scriptPath);
		static void onScriptDeleted(const std::filesystem::path& scriptPath);
		static void scriptRenamedCallback(const char* oldFilename, const char* newFilename);
		static void newScriptAddedCallback(const char* filename);
		static void scriptSelectedCallback(const char* filename);
		static void scriptDeletedCallback(const char* filename);

		// -------------- Internal Variables --------------
		static std::filesystem::path assetsRoot;
		static std::filesystem::path scriptsRoot;
		static FileSystemWatcher* scriptWatcher = nullptr;

		void init(const std::filesystem::path& projectRoot)
		{
			assetsRoot = projectRoot;
			scriptsRoot = assetsRoot/"scripts";

			// Initialize the script watcher
			scriptWatcher = g_memory_new FileSystemWatcher();
			scriptWatcher->path = scriptsRoot;
			scriptWatcher->onChanged = onScriptChanged;
			scriptWatcher->onRenamed = onScriptRenamed;
			scriptWatcher->onCreated = onScriptCreated;
			scriptWatcher->onDeleted = onScriptDeleted;
			scriptWatcher->includeSubdirectories = true;
			scriptWatcher->notifyFilters = NotifyFilters::FileName;
			scriptWatcher->notifyFilters |= NotifyFilters::Attributes;
			scriptWatcher->start();
		}

		void update()
		{
			scriptWatcher->poll();

			ImGui::Begin("Asset Manager");

			iterateDirectory(
				scriptsRoot, 
				newScriptAddedCallback,
				scriptRenamedCallback,
				scriptSelectedCallback,
				scriptDeletedCallback,
				"Script.luau", 
				"Add Script"
			);

			ImGui::End();
		}

		void free()
		{
			if (scriptWatcher)
			{
				g_memory_delete(scriptWatcher);
			}

			scriptWatcher = nullptr;
		}

		// -------------- Internal Functions --------------
		static void iterateDirectory(
			const std::filesystem::path& directory,
			AddButtonCallbackFn callback,
			FileRenamedCallbackFn fileRenamedCallback,
			FileSelectedFn fileSelectedCallback,
			FileDeletedFn fileDeletedCallback,
			const char* defaultNewFilename,
			const char* addButtonText
		)
		{
			constexpr size_t stringBufferSize = MATH_ANIMATIONS_MAX_PATH;
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
				g_memory_copyMem(stringBuffer, stringBufferSize,(void*)filename.c_str(), filename.length());
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
							g_logger_error("There was an error renaming file '{}' to '{}'.", filename, stringBuffer);
						}
						else if (fileRenamedCallback)
						{
							// If file renaming succeeded and callback was provided then call the callback
							fileRenamedCallback(oldFilepath.string().c_str(), newFilepath.string().c_str());
						}
					}
					else if (fileSelectedCallback)
					{
						// If file wasn't renamed but was selected then open it
						std::filesystem::path filepath = directory / std::filesystem::path(filename);
						fileSelectedCallback(filepath.string().c_str());
					}

					lastSelectedFile = fileIndex;
				}

				// We do drag drop right after the element we want it to effect, and this one will be a source
				if (ImGui::BeginDragDropSource())
				{
					// Set payload to carry AnimObjectPayload
					static constexpr size_t bufferSize = 512;
					static char buffer[bufferSize];
					static FilePayload payload = {};
					std::string filepath = file.path().string();
					if (bufferSize > filepath.length() + 1)
					{
						payload.filepathLength = filepath.length();
						payload.filepath = buffer;
						g_memory_copyMem((void*)buffer, bufferSize, (void*)filepath.c_str(), sizeof(char) * filepath.length());
						buffer[payload.filepathLength] = '\0';
						ImGui::Text("%s", buffer);
					}
					ImGui::SetDragDropPayload(ImGuiExtended::getFileDragDropPayloadId(), &payload, sizeof(FilePayload), ImGuiCond_Once);
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginPopupContextItem(stringBuffer))
				{
					if (ImGui::MenuItem("Delete"))
					{
						// Delete the current file
						std::filesystem::path filepath = std::filesystem::path(directory) / filename;
						std::remove(filepath.string().c_str());
						if (fileDeletedCallback)
						{
							fileDeletedCallback(filepath.string().c_str());
						}
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
			CodeEditorPanelManager::closeFile(scriptPath.string());
		}

		static void onScriptCreated(const std::filesystem::path& scriptPath)
		{
			LuauLayer::remove(scriptPath.filename().string());
			CodeEditorPanelManager::openFile(scriptPath.string());
		}

		static void onScriptRenamed(const std::filesystem::path& scriptPath)
		{
			LuauLayer::remove(scriptPath.filename().string());
		}

		static void scriptRenamedCallback(const char* oldFilename, const char* newFilename)
		{
			CodeEditorPanelManager::closeFile(oldFilename);
			CodeEditorPanelManager::openFile(newFilename);
		}

		static void newScriptAddedCallback(const char* filename)
		{
			// TODO: Add custom options if people want to use something else as their
			//       editor. An example would be something like VSCode
			// Platform::openFileWithVsCode(filename);
			CodeEditorPanelManager::openFile(filename);
		}

		static void scriptSelectedCallback(const char* filename)
		{
			// TODO: Add custom options if people want to use something else as their
			//       editor. An example would be something like VSCode
			// Platform::openFileWithVsCode(filename);
			CodeEditorPanelManager::openFile(filename);
		}

		static void scriptDeletedCallback(const char* filename)
		{
			CodeEditorPanelManager::closeFile(filename);
		}
	}
}
