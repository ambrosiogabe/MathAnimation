#ifdef _WIN32
#include "platform/Platform.h"
#include "core.h"

#include <io.h>
#include <Windows.h>
#include <shlobj_core.h>
#ifdef min
#undef min
#endif

namespace MathAnim
{
	struct MemMapUserData
	{
		HANDLE fileHandle;
		HANDLE fileMappingHandle;
	};

	namespace Platform
	{
		// --------------- Internal Functions ---------------
		static void wideToChar(const WCHAR* wide, char* buffer, size_t bufferLength);
		static bool stringsEqualIgnoreCase(const char* str1, const char* str2);
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

		// Adapted from https://stackoverflow.com/questions/2467429/c-check-installed-programms
		bool isProgramInstalled(const char* displayName)
		{
			constexpr int bufferMaxSize = 1024;

			// Open the "Uninstall" key.
			const WCHAR* uninstallRoot = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
			HKEY uninstallKey = NULL;
			if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, uninstallRoot, 0, KEY_READ, &uninstallKey) != ERROR_SUCCESS)
			{
				return false;
			}

			DWORD keyIndex = 0;
			long result = ERROR_SUCCESS;
			while (result == ERROR_SUCCESS)
			{
				//Enumerate all sub keys...
				WCHAR appKeyName[bufferMaxSize];
				DWORD appKeyNameBufferSize = sizeof(appKeyName);
				result = RegEnumKeyExW(
					uninstallKey,
					keyIndex,
					appKeyName,
					&appKeyNameBufferSize,
					NULL,
					NULL,
					NULL,
					NULL
				);

				if (result == ERROR_SUCCESS)
				{
					// Open the sub key.
					WCHAR subKey[bufferMaxSize];
					HKEY appKey = NULL;
					wsprintfW(subKey, L"%s\\%s", uninstallRoot, appKeyName);
					if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &appKey) != ERROR_SUCCESS)
					{
						// No more keys
						RegCloseKey(appKey);
						RegCloseKey(uninstallKey);
						return false;
					}

					// Get the display name value from the application's sub key.
					DWORD keyType = KEY_ALL_ACCESS;
					WCHAR currentDisplayNameWide[bufferMaxSize];
					DWORD currentDisplayNameBufferSize = sizeof(currentDisplayNameWide);
					long queryResult = RegQueryValueExW(
						appKey,
						L"DisplayName",
						NULL,
						&keyType,
						(unsigned char*)currentDisplayNameWide,
						&currentDisplayNameBufferSize
					);
					if (queryResult == ERROR_SUCCESS)
					{
						char currentDisplayName[bufferMaxSize];
						wideToChar(currentDisplayNameWide, currentDisplayName, bufferMaxSize);
						if (stringsEqualIgnoreCase((char*)currentDisplayName, displayName))
						{
							RegCloseKey(appKey);
							RegCloseKey(uninstallKey);
							return true;
						}
					}

					RegCloseKey(appKey);
				}

				keyIndex++;
			}

			RegCloseKey(uninstallKey);

			return false;
		}

		bool getProgramInstallDir(const char* programDisplayName, char* buffer, size_t bufferLength)
		{
			if (bufferLength <= 0)
			{
				return false;
			}

			constexpr int bufferMaxSize = 1024;

			// Open the "Uninstall" key.
			const WCHAR* uninstallRoot = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
			HKEY uninstallKey = NULL;
			if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, uninstallRoot, 0, KEY_READ, &uninstallKey) != ERROR_SUCCESS)
			{
				return false;
			}

			DWORD keyIndex = 0;
			long result = ERROR_SUCCESS;
			while (result == ERROR_SUCCESS)
			{
				//Enumerate all sub keys...
				WCHAR appKeyName[bufferMaxSize];
				DWORD appKeyNameBufferSize = sizeof(appKeyName);
				result = RegEnumKeyExW(
					uninstallKey,
					keyIndex,
					appKeyName,
					&appKeyNameBufferSize,
					NULL,
					NULL,
					NULL,
					NULL
				);

				if (result == ERROR_SUCCESS)
				{
					// Open the sub key.
					WCHAR subKey[bufferMaxSize];
					HKEY appKey = NULL;
					wsprintfW(subKey, L"%s\\%s", uninstallRoot, appKeyName);
					if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &appKey) != ERROR_SUCCESS)
					{
						// No more keys
						RegCloseKey(appKey);
						RegCloseKey(uninstallKey);
						return false;
					}

					// Get the display name value from the application's sub key.
					DWORD keyType = KEY_ALL_ACCESS;
					WCHAR currentDisplayNameWide[bufferMaxSize];
					DWORD currentDisplayNameBufferSize = sizeof(currentDisplayNameWide);
					long queryResult = RegQueryValueExW(
						appKey,
						L"DisplayName",
						NULL,
						&keyType,
						(unsigned char*)currentDisplayNameWide,
						&currentDisplayNameBufferSize
					);
					if (queryResult == ERROR_SUCCESS)
					{
						char currentDisplayName[bufferMaxSize];
						wideToChar(currentDisplayNameWide, currentDisplayName, bufferMaxSize);
						if (stringsEqualIgnoreCase((char*)currentDisplayName, programDisplayName))
						{
							// Try to get install path
							WCHAR installPath[bufferMaxSize];
							DWORD installPathBufferSize = sizeof(installPath);
							RegQueryValueExW(
								appKey,
								L"InstallLocation",
								NULL,
								&keyType,
								(unsigned char*)installPath,
								&installPathBufferSize
							);
							if (queryResult == ERROR_SUCCESS)
							{
								if (installPathBufferSize <= bufferLength)
								{
									// Conver the wide path to the regular size
									wideToChar(installPath, buffer, bufferLength);
									RegCloseKey(appKey);
									RegCloseKey(uninstallKey);
									return true;
								}
								else
								{
									g_logger_warning("Buffer too small to contain install location for '%s'.", currentDisplayName);
									buffer[0] = '\0';
									RegCloseKey(appKey);
									RegCloseKey(uninstallKey);
									return false;
								}
							}

							g_logger_warning("Found app '%s' but failed to find the InstallLocation.", currentDisplayName);
							RegCloseKey(appKey);
							RegCloseKey(uninstallKey);
							return false;
						}
					}

					RegCloseKey(appKey);
				}

				keyIndex++;
			}

			RegCloseKey(uninstallKey);

			return false;
		}

		bool executeProgram(const char* programFilepath, const char* cmdLineArgs, const char* workingDirectory, const char* executionOutputFilename)
		{
			STARTUPINFOA si = { 0 };
			si.cb = sizeof(si);
			si.wShowWindow = false;

			PROCESS_INFORMATION pi = { 0 };
			HANDLE fileHandle = 0;

			std::string finalArgs = std::string("\"") + programFilepath + std::string("\" ") + cmdLineArgs;
			if (executionOutputFilename)
			{
				SECURITY_ATTRIBUTES sa = { 0 };
				sa.nLength = sizeof(sa);
				sa.lpSecurityDescriptor = NULL;
				sa.bInheritHandle = TRUE;

				std::string filepath = executionOutputFilename;
				if (workingDirectory)
				{
					filepath = std::string(workingDirectory) + std::string("/") + executionOutputFilename;
				}

				fileHandle = CreateFileA(
					filepath.c_str(),
					FILE_APPEND_DATA,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					&sa,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL
				);
				if (fileHandle != INVALID_HANDLE_VALUE)
				{
					si.dwFlags |= STARTF_USESTDHANDLES;
					si.hStdInput = NULL;
					si.hStdError = fileHandle;
					si.hStdOutput = fileHandle;
				}
				else
				{
					fileHandle = NULL;
				}
			}

			if (!CreateProcessA(
				NULL,
				(char*)finalArgs.c_str(),
				NULL,
				NULL,
				TRUE,
				CREATE_NO_WINDOW,
				NULL,
				workingDirectory,
				&si,
				&pi
			))
			{
				DWORD dwStatus = GetLastError();
				g_logger_error("Failed to launch process '%s': ", dwStatus);

				TerminateProcess(pi.hProcess, 0);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				if (fileHandle) CloseHandle(fileHandle);
				return false;
			}

			// Wait until child process exits.
			g_logger_log("Running program: '%s'", finalArgs.c_str());
			WaitForSingleObject(pi.hProcess, 25000);
			TerminateProcess(pi.hProcess, 0);
			if (fileHandle) CloseHandle(fileHandle);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return true;
		}

		bool openFileWithDefaultProgram(const char* filepath)
		{
			return (uint64)ShellExecuteA(NULL, "code", filepath, NULL, NULL, SW_SHOW) > 32;
		}

		bool openFileWithVsCode(const char* filepath, int lineNumber)
		{
			std::string command = lineNumber >= 0
				? std::string("/c code --goto \"") + filepath + ":" + std::to_string(lineNumber) + "\""
				: std::string("/c code --goto \"") + filepath + "\"";
			return (uint64)ShellExecuteA(NULL, "open", "cmd", command.c_str(), NULL, SW_HIDE);
		}

		bool fileExists(const char* filename)
		{
			DWORD dwAttrib = GetFileAttributesA(filename);
			return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
		}

		bool dirExists(const char* dirName)
		{
			DWORD dwAttrib = GetFileAttributesA(dirName);
			return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
		}

		bool deleteFile(const char* filename)
		{
			if (!DeleteFileA(filename))
			{
				g_logger_error("Delete file '%s' failed with: %d", filename, GetLastError());
				return false;
			}

			return true;
		}

		std::string tmpFilename(const std::string& directory)
		{
			char outPath[_MAX_PATH];
			uint32 res = GetTempFileNameA(
				directory.c_str(),
				"TMP",
				0,
				outPath
			);

			if (res == 0)
			{
				return std::string("");
			}

			return std::string(outPath);
		}

		std::string getSpecialAppDir()
		{
			CHAR szPath[MAX_PATH];

			if (SHGetFolderPathA(NULL,
				CSIDL_APPDATA,
				NULL,
				0,
				szPath) == ERROR_SUCCESS)
			{
				return std::string(szPath);
			}

			return "";
		}

		MemMappedFile* createTmpMemMappedFile(const std::string& directory, size_t size)
		{
			MemMappedFile* res = (MemMappedFile*)g_memory_allocate(sizeof(MemMappedFile));
			g_memory_zeroMem(res, sizeof(MemMappedFile));
			res->userData = (MemMapUserData*)g_memory_allocate(sizeof(MemMapUserData));
			res->userData->fileHandle = INVALID_HANDLE_VALUE;
			res->userData->fileMappingHandle = INVALID_HANDLE_VALUE;

			std::string filepath = tmpFilename(directory);
			res->userData->fileHandle = CreateFileA(
				filepath.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0, // Specify exclusive access for memmapped file
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
				NULL
			);

			if (res->userData->fileHandle == INVALID_HANDLE_VALUE)
			{
				g_logger_error("Failed to create file '%s' for memmapping. Last error: %d", filepath.c_str(), GetLastError());
				freeMemMappedFile(res);
				return nullptr;
			}

			DWORD sizeHigh = (size >> 32) & 0xFFFFFFFF;
			if constexpr (sizeof(size) < sizeof(uint64))
			{
				// Protect against overflow on machines with <64-bit architectures (even though this
				// isn't meant to be run on <64-bit architectures)
				sizeHigh = 0;
			}
			DWORD sizeLow = (size) & 0xFFFFFFFF;
			res->userData->fileMappingHandle = CreateFileMappingA(
				// TODO: Maybe this should be INVALID_HANDLE_VALUE so that the 
				// OS will automatically use disk only if RAM usage is too high???
				res->userData->fileHandle,
				NULL,
				PAGE_READWRITE,
				sizeHigh,
				sizeLow,
				NULL
			);
			if (res->userData->fileMappingHandle == NULL)
			{
				res->userData->fileMappingHandle = INVALID_HANDLE_VALUE;
				g_logger_error("Failed to memmap a temporary file. Last error: '%d'", GetLastError());
				freeMemMappedFile(res);
				return nullptr;
			}

			uint8* baseAddress = (uint8*)MapViewOfFile(
				res->userData->fileMappingHandle,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				size
			);
			if (baseAddress == NULL)
			{
				g_logger_error("Failed to create a mapped view of the memmap handle. Last Error: '%d'", GetLastError());
				freeMemMappedFile(res);
				return nullptr;
			}

			res->dataSize = size;

			// NOTE: This is kind of gross, but hopefully it communicates to the end user
			// that this pointer is not supposed to be changed, and if they do something 
			// similar bad stuff is gonna happen
#pragma warning( push )
#pragma warning( disable : 4213 )
			(uint8*)res->data = baseAddress;
#pragma warning( pop )
			return res;
		}

		void freeMemMappedFile(MemMappedFile* file)
		{
			if (!file)
			{
				return;
			}

			if (file->data)
			{
				BOOL res = UnmapViewOfFile(file->data);
				if (!res)
				{
					g_logger_error("Failed to unmap the file view for a memmapped file. Last error: '%d'", GetLastError());
				}
			}

			if (file->userData)
			{
				if (file->userData->fileMappingHandle != INVALID_HANDLE_VALUE)
				{
					BOOL res = CloseHandle(file->userData->fileMappingHandle);
					if (!res)
					{
						g_logger_error("Failed to close file mapping handle for a memmapped file. Last error: '%d'", GetLastError());
					}
				}

				if (file->userData->fileHandle != INVALID_HANDLE_VALUE)
				{
					BOOL res = CloseHandle(file->userData->fileHandle);
					if (!res)
					{
						g_logger_error("Failed to close file handle for a memmapped file. Last error: '%d'", GetLastError());
					}
				}

				g_memory_free(file->userData);
				file->userData = nullptr;
			}

			g_memory_zeroMem(file, sizeof(MemMappedFile));
			g_memory_free(file);
		}

		void createDirIfNotExists(const char* dirName)
		{
			CreateDirectoryA(dirName, NULL);
		}

		std::string md5FromString(const std::string& str, int md5Length)
		{
			return md5FromString(str.c_str(), str.length(), md5Length);
		}

		std::string md5FromString(const char* str, size_t length, int md5Length)
		{
			constexpr int maxMd5Length = 1024;
			g_logger_assert(md5Length < maxMd5Length, "Cannot generate md5 greater than %d characters.", maxMd5Length);

			// Get handle to the crypto provider
			HCRYPTPROV hProv = 0;
			if (!CryptAcquireContext(&hProv,
				NULL,
				NULL,
				PROV_RSA_FULL,
				CRYPT_VERIFYCONTEXT))
			{
				DWORD dwStatus = GetLastError();
				g_logger_error("CryptAcquireContext failed: %d", dwStatus);
				return "";
			}

			HCRYPTHASH hHash = 0;
			if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
			{
				DWORD dwStatus = GetLastError();
				g_logger_error("CryptAcquireContext failed: %d", dwStatus);
				CryptReleaseContext(hProv, 0);
				return "";
			}

			// Reinterpret the char's as unsigned char's
			// shouldn't make any difference since the data
			// doesn't matter, just the bits
			if (!CryptHashData(hHash, (BYTE*)str, (DWORD)length, 0))
			{
				DWORD dwStatus = GetLastError();
				g_logger_error("CryptHashData failed: %d", dwStatus);
				CryptReleaseContext(hProv, 0);
				CryptDestroyHash(hHash);
				return "";
			}

			BYTE rgbHash[maxMd5Length];
			CHAR hexDigits[] = "0123456789abcdef";
			DWORD cbHash = md5Length;
			std::string hashRes = "";
			if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
			{
				for (DWORD i = 0; i < cbHash; i++)
				{
					hashRes += hexDigits[rgbHash[i] >> 4];
					hashRes += hexDigits[rgbHash[i] & 0xf];
				}
			}
			else
			{
				DWORD dwStatus = GetLastError();
				g_logger_error("CryptGetHashParam failed: %d", dwStatus);
			}

			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);

			return hashRes;
		}

		// --------------- Internal Functions ---------------
		static void wideToChar(const WCHAR* wide, char* buffer, size_t bufferLength)
		{
			size_t wideLength = std::wcslen(wide);
			if (wideLength + 1 >= bufferLength)
			{
				buffer[0] = '\0';
				g_logger_warning("Could not convert wide string to char string. Wide string length '%d' buffer length '%d'.", wideLength, bufferLength);
				return;
			}

			for (size_t i = 0; i < wideLength; i++)
			{
				if (i < bufferLength)
				{
					// Truncate
					buffer[i] = (char)wide[i];
				}
			}

			buffer[wideLength] = '\0';
		}

		static bool stringsEqualIgnoreCase(const char* str1, const char* str2)
		{
			size_t lenStr1 = std::strlen(str1);
			size_t lenStr2 = std::strlen(str2);
			if (lenStr1 != lenStr2)
			{
				return false;
			}
			size_t minLength = glm::min(lenStr1, lenStr2);

			for (size_t i = 0; i < minLength; i++)
			{
				if (str1[i] != str2[i])
				{
					char str1Lower = str1[i] >= 'A' && str1[i] <= 'Z' ? str1[i] - 'A' + 'a' : str1[i];
					char str2Lower = str2[i] >= 'A' && str2[i] <= 'Z' ? str2[i] - 'A' + 'a' : str2[i];
					if (str1Lower != str2Lower)
					{
						return false;
					}
				}
			}

			return true;
		}
	}
}

#endif 