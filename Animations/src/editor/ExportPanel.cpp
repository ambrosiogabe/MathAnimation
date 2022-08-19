#include "editor/ExportPanel.h"
#include "core.h"
#include "core/Application.h"

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

			constexpr int filenameBufferSize = 256;
			static char filenameBuffer[filenameBufferSize];
			ImGui::InputText(": Filename", filenameBuffer, filenameBufferSize);

			if (ImGui::Button("Export"))
			{
				std::string filename = std::string(filenameBuffer) + ".mp4";
				if (filename != "")
				{
					g_logger_info("Exporting video to %s", filename.c_str());
					Application::exportVideoTo(filename);
				}
				else
				{
					g_logger_error("Export filename cannot be empty.");
				}
			}

			ImGui::End();
		}

		void free()
		{

		}
	}
}