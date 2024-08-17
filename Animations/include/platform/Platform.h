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

	enum class FileCopyOptions : uint8
	{
		/**
		 * @brief Copy happens with no custom behavior
		*/
		None = 1,
		/**
		 * @brief If the file at `dst` exists, then the copy fails
		*/
		FailIfDstExists = None << 1
	};

	namespace Platform
	{
		void free();

		const std::vector<std::string>& getAvailableFonts();

		bool isProgramInstalled(const char* displayName);

		bool getProgramInstallDir(const char* programDisplayName, char* buffer, size_t bufferLength);

		bool executeProgram(const char* programFilepath, const char* cmdLineArgs = nullptr, const char* workingDirectory = nullptr, const char* executionOutputFilename = nullptr);

		bool openFileWithDefaultProgram(const char* filepath);

		bool openFileWithVsCode(const char* filepath, int lineNumber = -1);

		/**
		 * @brief Copys file from `srcFile` to the destination specified by `dst`
		 * 
		 * @param srcFile The filepath of the file to copy
		 * @param dst The location of the file to be copied to
		 * @param options A list of options to customize what happens on copy
		 * 
		 * @return True if the copy succeeds. False if the `srcFile` does not exist,
		 *         or one of the options fails.
		*/
		bool copyFile(const char* srcFile, const char* dst, FileCopyOptions options = FileCopyOptions::None);

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