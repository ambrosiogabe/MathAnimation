#ifndef MATH_ANIM_PLATFORM_H
#define MATH_ANIM_PLATFORM_H
#include "core.h"

namespace MathAnim
{
	namespace Platform
	{
		const std::vector<std::string>& getAvailableFonts();

		bool isProgramInstalled(const char* displayName);

		bool getProgramInstallDir(const char* programDisplayName, char* buffer, size_t bufferLength);

		bool executeProgram(const char* programFilepath, const char* cmdLineArgs = nullptr, const char* workingDirectory = nullptr, const char* executionOutputFilename = nullptr);

		bool openFileWithDefaultProgram(const char* filepath);

		bool openFileWithVsCode(const char* filepath, int lineNumber = -1);

		bool fileExists(const char* filename);

		bool dirExists(const char* dirName);

		bool deleteFile(const char* filename);

		std::string tmpFilename();

		std::string getSpecialAppDir();

		void createDirIfNotExists(const char* dirName);

		std::string md5FromString(const char* str, size_t length, int md5Length = 16);

		std::string md5FromString(const std::string& str, int md5Length = 16);
	}
}

#endif 