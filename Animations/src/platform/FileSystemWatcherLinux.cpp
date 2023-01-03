#ifdef __linux
#include "platform/FileSystemWatcher.h"
#include "platform/Platform.h"

#include <unistd.h>
#include <sys/inotify.h>

static int inotify_descriptor;
static int watch_descriptor;

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
    if (path.empty())
    {
      g_logger_error("Path empty. Could not create FileSystemWatcher for '%s'", path.string().c_str());
      return;
    }

    Platform::createDirIfNotExists(path.string().c_str());

    inotify_descriptor = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (inotify_descriptor == -1)
    {
      g_logger_error("Failed to create FileSystemWatcher for '%s': %s", path.string().c_str(), strerror(errno));
      return;
    }

    watch_descriptor = inotify_add_watch(inotify_descriptor, path.string().c_str(), IN_MODIFY | IN_DELETE | IN_ATTRIB | IN_MOVED_TO | IN_ACCESS | IN_CREATE);
    if (watch_descriptor == -1)
    {
      close(inotify_descriptor);
      inotify_descriptor = -1;
      g_logger_error("Failed to create FileSystemWatcher for '%s': %s", path.string().c_str(), strerror(errno));
      return;
    }
  }

  void FileSystemWatcher::stop()
  {
    if (enableRaisingEvents)
    {
      enableRaisingEvents = false;
      inotify_rm_watch(inotify_descriptor, watch_descriptor);
      close(watch_descriptor);
      close(inotify_descriptor);
      fileWatcherThread.join();
    }
  }

  void FileSystemWatcher::poll()
  {
    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    struct inotify_event const *event;
    ssize_t len;

    while (inotify_descriptor != -1 && watch_descriptor != -1)
    {
      len = read(inotify_descriptor, buf, sizeof(buf));
      if (len == -1 && errno != EAGAIN)
      {
        g_logger_error("Failed to poll from FileSystemWatcher for '%s': %s", path.string().c_str(), strerror(errno));
        return;
      }

      if (len <= 0)
      {
        // No more events
        return;
      }

      for (char *ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len)
      {
        event = (struct inotify_event const *)ptr;

        if (event->mask & (IN_ISDIR | IN_IGNORED))
        {
          continue;
        }

        if (onChanged && event->mask & (IN_MODIFY | IN_ACCESS | IN_ATTRIB))
        {
          onChanged(std::filesystem::path(event->name));
        }

        if (onRenamed && event->mask & IN_MOVED_TO)
        {
          onRenamed(std::filesystem::path(event->name));
        }

        if (onDeleted && event->mask & IN_DELETE)
        {
          onDeleted(std::filesystem::path(event->name));
        }

        if (onCreated && event->mask & IN_CREATE)
        {
          onCreated(std::filesystem::path(event->name));
        }
      }
    }
  }
}

#endif
