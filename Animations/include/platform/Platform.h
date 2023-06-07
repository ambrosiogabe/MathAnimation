#ifndef MATH_ANIM_PLATFORM_H
#define MATH_ANIM_PLATFORM_H
#include "core.h"

namespace MathAnim
{
	struct MemMapUserData;
	struct Window;

	struct MemMappedFile
	{
		uint8* const data;
		size_t dataSize;
		// Implementation defined, this will contain mappings on windows
		// and different information on Linux as needed
		MemMapUserData* userData;
	};

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

		std::string tmpFilename(const std::string& directory);

		std::string getSpecialAppDir();

		MemMappedFile* createTmpMemMappedFile(const std::string& directory, size_t fileSize);

		void freeMemMappedFile(MemMappedFile* file);

		void createDirIfNotExists(const char* dirName);

		std::string md5FromString(const char* str, size_t length, int md5Length = 16);

		std::string md5FromString(const std::string& str, int md5Length = 16);

		bool setCursorPos(const Window& window, const Vec2i& cursorPos);
	}
}

#endif