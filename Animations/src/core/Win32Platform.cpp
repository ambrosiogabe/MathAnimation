#ifdef _WIN32
#include "core/Platform.h"

#include <shlobj_core.h>
#include <Shlwapi.h>

namespace MathAnim
{
	namespace Platform
	{
		// -------------- Internal Variables --------------
		static std::vector<std::string> availableFonts = {};
		static bool availableFontsCached = false;

		const std::vector<std::string>& getAvailableFonts()
		{
			if (!availableFontsCached)
			{
				char szPath[MAX_PATH];

				if (SHGetFolderPathA(NULL,
					CSIDL_FONTS,
					NULL,
					0,
					szPath) == S_OK)
				{
					for (auto file : std::filesystem::directory_iterator(szPath))
					{
						if (file.path().extension() == ".ttf" || file.path().extension() == ".TTF")
						{
							availableFonts.push_back(file.path().string());
						}
					}
				}

				availableFontsCached = true;
			}

			return availableFonts;
		}
	}
}

#endif 
