#include "editor/EditorLayout.h"
#include "platform/Platform.h"

#include <imgui.h>

namespace MathAnim
{
	namespace EditorLayout
	{
		static std::filesystem::path layoutsRoot = "";
		static std::vector<std::filesystem::path> defaultLayouts = {};
		static std::vector<std::filesystem::path> customLayouts = {};

		void init(const std::filesystem::path& projectRoot)
		{
			std::filesystem::path appRoot = projectRoot.parent_path();
			// Copy default layouts to assets directory if it does not exist
			layoutsRoot = appRoot / "editorLayouts";
			Platform::createDirIfNotExists(layoutsRoot.string().c_str());

			std::filesystem::path builtinLayoutsRoot = "./assets/layouts";
			for (const auto& layoutPath : std::filesystem::directory_iterator(builtinLayoutsRoot))
			{
				std::filesystem::path filename = layoutPath.path().filename();
				std::filesystem::path outputPath = (layoutsRoot / filename);
				if (!Platform::fileExists(outputPath.string().c_str()))
				{
					std::filesystem::copy_file(layoutPath, outputPath);
				}

				if (outputPath.extension().string() == ".json")
				{
					defaultLayouts.push_back(std::filesystem::absolute(outputPath));
				}
			}

			for (const auto& layoutPath : std::filesystem::directory_iterator(layoutsRoot))
			{
				std::filesystem::path absPath = std::filesystem::absolute(layoutPath);
				const auto& iter = std::find(defaultLayouts.begin(), defaultLayouts.end(), absPath);
				if (iter == defaultLayouts.end() && absPath.extension().string() == ".json")
				{
					g_logger_info("Custom layout: %s", absPath.string().c_str());
					customLayouts.push_back(absPath);
				}
			}
		}

		void update()
		{

		}

		const std::vector<std::filesystem::path>& getDefaultLayouts()
		{
			return defaultLayouts;
		}

		const std::vector<std::filesystem::path>& getCustomLayouts()
		{
			return customLayouts;
		}
	}
}