#include "File.h"

#include "CppUtils/CppUtils.h"
using namespace CppUtils;

#include <string>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>

bool manim_remove_dir(const char* directoryName)
{
	static char buffer[FILENAME_MAX];
	strcpy(buffer, directoryName);
	buffer[strlen(directoryName) + 1] = '\0';

	SHFILEOPSTRUCTA op = {
		NULL,
		FO_DELETE,
		buffer,
		NULL,
		FOF_SILENT | FOF_NOCONFIRMATION,
		NULL,
		NULL,
		NULL
	};
	if (SHFileOperationA(&op) != 0)
	{
		Logger::Error("Removing directory '%s' failed with '%d'", directoryName, GetLastError());
		return false;
	}

	return true;
}

bool manim_is_dir(const char* directoryName)
{
	DWORD fileAttrib = GetFileAttributesA(directoryName);
	return (fileAttrib != INVALID_FILE_ATTRIBUTES && (fileAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool manim_is_file(const char* directoryName)
{
	DWORD fileAttrib = GetFileAttributesA(directoryName);
	return (fileAttrib != INVALID_FILE_ATTRIBUTES && !(fileAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool manim_move_file(const char* from, const char* to)
{
	if (!MoveFileExA(from, to, MOVEFILE_WRITE_THROUGH))
	{
		Logger::Error("Move file failed with %d", GetLastError());
		return false;
	}

	return true;
}

bool manim_create_dir_if_not_exists(const char* directoryName)
{
	DWORD fileAttrib = GetFileAttributesA(directoryName);
	if (fileAttrib == INVALID_FILE_ATTRIBUTES)
	{
		if (!CreateDirectoryA(directoryName, NULL))
		{
			Logger::Error("Creating tmp directory failed with error code '%d'", GetLastError());
			return false;
		}
	}
	
	return true;
}

#endif