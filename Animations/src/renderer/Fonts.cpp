#include "core.h"
#include "renderer/Fonts.h"

namespace MathAnim
{
	namespace Fonts
	{
		static bool initialized = false;
		static FT_Library library;

		void init()
		{
			int error = FT_Init_FreeType(&library);
			if (error)
			{
				Logger::Error("An error occurred during freetype initialization. Font rendering will not work.");
			}

			Logger::Info("Initialized freetype library.");
			initialized = true;
		}
	}
}