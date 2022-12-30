#ifndef MATH_ANIM_FILE_SYSTEM_WATCHER
#define MATH_ANIM_FILE_SYSTEM_WATCHER
#include "core.h"

namespace MathAnim
{
	enum NotifyFilters
	{
		FileName = 1,
		DirectoryName = 2,
		Attributes = 4,
		Size = 8,
		LastWrite = 16,
		LastAccess = 32,
		CreationTime = 64,
		Security = 256
	};

	class FileSystemWatcher
	{
	public:
		FileSystemWatcher();
		~FileSystemWatcher() { stop(); }
		void start();
		void stop();

		void poll();

	public:
		typedef void (*OnChanged)(const std::filesystem::path& file);
		typedef void (*OnRenamed)(const std::filesystem::path& file);
		typedef void (*OnDeleted)(const std::filesystem::path& file);
		typedef void (*OnCreated)(const std::filesystem::path& file);

		OnChanged onChanged = nullptr;
		OnRenamed onRenamed = nullptr;
		OnDeleted onDeleted = nullptr;
		OnCreated onCreated = nullptr;

		int notifyFilters = 0;
		bool includeSubdirectories = false;
		std::string filter = "";
		std::filesystem::path path = "";

	private:
		bool enableRaisingEvents = true;
		std::thread fileWatcherThread;
		void* stopEventHandle;

		std::set<std::filesystem::path> changedQueue;
		std::set<std::filesystem::path> renamedQueue;
		std::set<std::filesystem::path> deletedQueue;
		std::set<std::filesystem::path> createdQueue;

		std::set<std::filesystem::path> prevChangedQueue;
		std::set<std::filesystem::path> prevRenamedQueue;
		std::set<std::filesystem::path> prevDeletedQueue;
		std::set<std::filesystem::path> prevCreatedQueue;

		std::mutex queueMtx;

	private:
		void startThread();
	};
}

#endif