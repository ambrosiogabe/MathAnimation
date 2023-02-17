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
			defaultLayouts.clear();
			customLayouts.clear();

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
					customLayouts.push_back(absPath);
				}
			}
		}

		void update()
		{

		}

		void addCustomLayout(const std::filesystem::path& layoutFilepath)
		{
			customLayouts.push_back(std::filesystem::absolute(layoutFilepath));
		}

		const std::vector<std::filesystem::path>& getDefaultLayouts()
		{
			return defaultLayouts;
		}

		const std::vector<std::filesystem::path>& getCustomLayouts()
		{
			return customLayouts;
		}

		const std::filesystem::path& getLayoutsRoot()
		{
			return layoutsRoot;
		}

		bool isReserved(const char* name)
		{
			for (const auto& layoutPath : defaultLayouts)
			{
				if (layoutPath.stem().string() == name)
				{
					return true;
				}
			}

			return false;
		}
	}
}