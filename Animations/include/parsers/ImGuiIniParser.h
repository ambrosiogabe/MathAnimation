#ifndef MATH_ANIM_IMGUI_INI_PARSER_H
#define MATH_ANIM_IMGUI_INI_PARSER_H
#include "core.h"

namespace MathAnim
{
	namespace ImGuiIniParser
	{
		void convertImGuiIniToJson(const char* iniFilepath, const char* outputFilepath, const Vec2& outputResolution);
		void convertJsonToImGuiIni(const char* jsonFilepath, const char* outputFilepath, const Vec2& targetResolution);
		
		inline void convertImGuiIniToJson(const std::string& iniFilepath, const std::string& outputFilepath, const Vec2& outputResolution)
		{
			convertImGuiIniToJson(iniFilepath.c_str(), outputFilepath.c_str(), outputResolution);
		}

		inline void convertJsonToImGuiIni(const std::string& jsonFilepath, const std::string& outputFilepath, const Vec2& targetResolution)
		{
			convertJsonToImGuiIni(jsonFilepath.c_str(), outputFilepath.c_str(), targetResolution);
		}
	}
}

#endif 