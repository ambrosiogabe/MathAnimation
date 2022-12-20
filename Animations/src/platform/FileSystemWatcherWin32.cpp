#ifdef _WIN32
#include "platform/FileSystemWatcher.h"
#include "platform/Platform.h"

#include <Windows.h>

namespace MathAnim
{
	FileSystemWatcher::FileSystemWatcher()
	{

	}

	void FileSystemWatcher::start()
	{
		fileWatcherThread = std::thread(&FileSystemWatcher::startThread, this);
	}

	void FileSystemWatcher::startThread()
	{
		if (path.empty() || !Platform::dirExists(path.string().c_str()))
		{
			g_logger_error("Path empty or directory does not exist. Could not create FileSystemWatcher for '%s'", path.string().c_str());
			return;
		}

		HANDLE dirHandle = CreateFileA(path.string().c_str(), GENERIC_READ | FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL);
		if (dirHandle == INVALID_HANDLE_VALUE)
		{
			g_logger_error("Invalid file access. Could not create FileSystemWatcher for '%s'", path.string().c_str());
			return;
		}

		// Set up notification flags 
		int flags = 0;
		if (notifyFilters && NotifyFilters::FileName)
		{
			flags |= FILE_NOTIFY_CHANGE_FILE_NAME;
		}

		if (notifyFilters && NotifyFilters::DirectoryName)
		{
			flags |= FILE_NOTIFY_CHANGE_DIR_NAME;
		}

		if (notifyFilters && NotifyFilters::Attributes)
		{
			flags |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
		}

		if (notifyFilters && NotifyFilters::Size)
		{
			flags |= FILE_NOTIFY_CHANGE_SIZE;
		}

		if (notifyFilters && NotifyFilters::LastWrite)
		{
			flags |= FILE_NOTIFY_CHANGE_LAST_WRITE;
		}

		if (notifyFilters && NotifyFilters::LastAccess)
		{
			flags |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
		}

		if (notifyFilters && NotifyFilters::CreationTime)
		{
			flags |= FILE_NOTIFY_CHANGE_CREATION;
		}

		if (notifyFilters && NotifyFilters::Security)
		{
			flags |= FILE_NOTIFY_CHANGE_SECURITY;
		}

		char filename[MAX_PATH];
		char buffer[2048];
		DWORD bytesReturned;
		FILE_NOTIFY_INFORMATION* pNotify;
		int offset = 0;
		OVERLAPPED pollingOverlap;
		pollingOverlap.OffsetHigh = 0;
		pollingOverlap.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
		if (pollingOverlap.hEvent == NULL)
		{
			g_logger_error("Could not create event watcher for FileSystemWatcher '%s'", path.string().c_str());
			return;
		}

		bool result = true;
		HANDLE hEvents[2];
		hEvents[0] = pollingOverlap.hEvent;
		hEvents[1] = CreateEventA(NULL, TRUE, FALSE, NULL);
		stopEventHandle = hEvents[1];

		while (result && enableRaisingEvents)
		{
			result = ReadDirectoryChangesW(
				dirHandle,                   // handle to the directory to be watched
				&buffer,                     // pointer to the buffer to receive the read results
				sizeof(buffer),              // length of lpBuffer
				includeSubdirectories,       // flag for monitoring directory or directory tree
				flags,
				&bytesReturned,              // number of bytes returned
				&pollingOverlap,             // pointer to structure needed for overlapped I/O
				NULL
			);

			DWORD event = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
			offset = 0;
			int rename = 0;

			if (event == WAIT_OBJECT_0 + 1)
			{
				break;
			}

			do
			{
				std::lock_guard<std::mutex> queueLock(queueMtx);

				pNotify = (FILE_NOTIFY_INFORMATION*)((char*)buffer + offset);
				strcpy(filename, "");
				int filenamelen = WideCharToMultiByte(CP_ACP, 0, pNotify->FileName, pNotify->FileNameLength / 2, filename, sizeof(filename), NULL, NULL);
				filename[pNotify->FileNameLength / 2] = '\0';
				switch (pNotify->Action)
				{
				case FILE_ACTION_ADDED:
					createdQueue.push(filename);
					break;
				case FILE_ACTION_REMOVED:
					deletedQueue.push(filename);
					break;
				case FILE_ACTION_MODIFIED:
					changedQueue.push(filename);
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					// Log::Info("The file was renamed and this is the old name: [%s]", filename);
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					renamedQueue.push(filename);
					break;
				default:
					g_logger_error("Default error. Unknown file action '%d' for FileSystemWatcher '%s'", pNotify->Action, path.string().c_str());
					break;
				}

				offset += pNotify->NextEntryOffset;
			} while (pNotify->NextEntryOffset);


		}

		CloseHandle(dirHandle);
	}

	void FileSystemWatcher::stop()
	{
		if (enableRaisingEvents)
		{
			enableRaisingEvents = false;
			SetEvent(stopEventHandle);
			fileWatcherThread.join();
		}
	}

	void FileSystemWatcher::poll()
	{
		std::lock_guard<std::mutex> queueLock(queueMtx);

		while (!changedQueue.empty())
		{
			const std::filesystem::path& file = changedQueue.front();
			if (onChanged)
			{
				onChanged(file);
			}
			changedQueue.pop();
		}

		while (!renamedQueue.empty())
		{
			const std::filesystem::path& file = renamedQueue.front();
			if (onRenamed)
			{
				onRenamed(file);
			}
			renamedQueue.pop();
		}

		while (!deletedQueue.empty())
		{
			const std::filesystem::path& file = deletedQueue.front();
			if (onDeleted)
			{
				onDeleted(file);
			}
			deletedQueue.pop();
		}

		while (!createdQueue.empty())
		{
			const std::filesystem::path& file = createdQueue.front();
			if (onCreated)
			{
				onCreated(file);
			}
			createdQueue.pop();
		}
	}
}

#endif
