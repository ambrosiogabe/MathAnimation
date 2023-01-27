#include "editor/ExportPanel.h"
#include "core.h"
#include "core/Application.h"

#include <nfd.h>

namespace MathAnim
{
	namespace ExportPanel
	{
		void init()
		{

		}

		void update()
		{
			ImGui::Begin("Export Video");

			constexpr int filenameBufferSize = MATH_ANIMATIONS_MAX_PATH;
			static char filenameBuffer[filenameBufferSize];
			ImGui::BeginDisabled();
			ImGui::InputText(": Filename", filenameBuffer, filenameBufferSize);
			ImGui::EndDisabled();

			ImGui::BeginDisabled(Application::isExportingVideo());
			if (ImGui::Button("Export"))
			{
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_SaveDialog("mp4", NULL, &outPath);

				if (result == NFD_OKAY)
				{
					size_t pathLength = std::strlen(outPath);
					if (pathLength < filenameBufferSize)
					{
						g_memory_copyMem(filenameBuffer, outPath, pathLength);
						filenameBuffer[pathLength] = '\0';
					}

					std::free(outPath);
				}
				else if (result == NFD_CANCEL)
				{
					filenameBuffer[0] = '\0';
				}
				else
				{
					filenameBuffer[0] = '\0';
					g_logger_error("Error opening file to save to for video export:\n\t%s", NFD_GetError());
				}

				std::string filename = std::string(filenameBuffer);
				if (filename != "")
				{
					std::filesystem::path filepath = filename;
					if (!filepath.has_extension())
					{
						filepath.replace_extension(".mp4");
					}
					g_logger_info("Exporting video to %s", filepath.string().c_str());
					Application::exportVideoTo(filepath.string());
				}
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::BeginDisabled(!Application::isExportingVideo());
			if (ImGui::Button("Stop Exporting"))
			{
				Application::endExport();
			}
			ImGui::EndDisabled();

			ImGui::End();
		}

		void free()
		{

		}
	}
}