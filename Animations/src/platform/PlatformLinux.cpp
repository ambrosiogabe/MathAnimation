#ifdef __linux
#include "platform/Platform.h"
#include "core.h"

#include <filesystem>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include <openssl/md5.h>

#ifdef min
#undef min
#endif

/* Make a directory; already existing dir okay */
static int maybe_mkdir(const char *path, mode_t mode)
{
  struct stat st;
  errno = 0;

  /* Try to make the directory */
  if (mkdir(path, mode) == 0)
    return 0;

  /* If it fails for any reason but EEXIST, fail */
  if (errno != EEXIST)
    return -1;

  /* Check if the existing path is a directory */
  if (stat(path, &st) != 0)
    return -1;

  /* If not, fail with ENOTDIR */
  if (!S_ISDIR(st.st_mode))
  {
    errno = ENOTDIR;
    return -1;
  }

  errno = 0;
  return 0;
}

static int mkdir_p(const char *path, mode_t mode)
{
  /* Adapted from http://stackoverflow.com/a/2336245/119527 */
  char *_path = NULL;
  char *p;
  int result = -1;

  errno = 0;

  /* Copy string so it's mutable */
  _path = strdup(path);
  if (_path == NULL)
    goto out;

  /* Iterate the string */
  for (p = _path + 1; *p; p++)
  {
    if (*p == '/')
    {
      /* Temporarily truncate */
      *p = '\0';

      if (maybe_mkdir(_path, mode) != 0)
        goto out;

      *p = '/';
    }
  }

  if (maybe_mkdir(_path, mode) != 0)
    goto out;

  result = 0;

out:
  free(_path);
  return result;
}

namespace MathAnim
{
  namespace Platform
  {
    // --------------- Internal Functions ---------------
    // static void wideToChar(const WCHAR *wide, char *buffer, size_t bufferLength);
    // static bool stringsEqualIgnoreCase(const char *str1, const char *str2);
    static std::vector<std::string> availableFonts = {
        "JetBrains Mono"};
    static bool availableFontsCached = false;
    static std::string homeDirectory = std::string();

    const std::vector<std::string> &getAvailableFonts()
    {
      if (!availableFontsCached)
      {
        // TODO: Placeholder cuz it's 01:39
        availableFontsCached = true;
      }

      return availableFonts;
    }

    // Adapted from https://stackoverflow.com/questions/2467429/c-check-installed-programms
    bool isProgramInstalled(const char *displayName)
    {
      // TODO: Placeholder, but also this function seems unused?
      return false;
    }

    bool getProgramInstallDir(const char *programDisplayName, char *buffer, size_t bufferLength)
    {
      // TODO: I'm used to Rust or managed languages and string handling in C/++ scares me

      std::string const path = std::getenv("PATH");
      char const delim = ':';

      auto start = 0U;
      auto end = path.find(delim);
      while (true)
      {
        auto const folder = path.substr(start, end - start);

        int i = 1;

        for (auto const &directory_entry : std::filesystem::directory_iterator(folder))
        {
          auto const file = directory_entry.path();
          if (strcmp(file.filename().c_str(), programDisplayName) == 0 && access(file.c_str(), X_OK) == 0)
          {
            strncpy(buffer, file.parent_path().c_str(), bufferLength - 1);
            // Append / to the end of the path
            buffer[strlen(buffer) + 1] = '\0';
            buffer[strlen(buffer)] = '/';
            return true;
          }
        }
        if (end == std::string::npos)
        {
          break;
        }
        start = end + 1;
        end = path.find(delim, start);
      }

      return false;
    }

    bool executeProgram(const char *programFilepath, const char *cmdLineArgs, const char *workingDirectory, const char *executionOutputFilename)
    {
      exit(1029);
      std::string finalArgs = std::string("\"") + programFilepath + std::string("\" ") + cmdLineArgs;
      // if (executionOutputFilename)
      // {
      //   SECURITY_ATTRIBUTES sa = {0};
      //   sa.nLength = sizeof(sa);
      //   sa.lpSecurityDescriptor = NULL;
      //   sa.bInheritHandle = TRUE;

      //   std::string filepath = executionOutputFilename;
      //   if (workingDirectory)
      //   {
      //     filepath = std::string(workingDirectory) + std::string("/") + executionOutputFilename;
      //   }

      //   fileHandle = CreateFileA(
      //       filepath.c_str(),
      //       FILE_APPEND_DATA,
      //       FILE_SHARE_READ | FILE_SHARE_WRITE,
      //       &sa,
      //       CREATE_ALWAYS,
      //       FILE_ATTRIBUTE_NORMAL,
      //       NULL);
      //   if (fileHandle != INVALID_HANDLE_VALUE)
      //   {
      //     si.dwFlags |= STARTF_USESTDHANDLES;
      //     si.hStdInput = NULL;
      //     si.hStdError = fileHandle;
      //     si.hStdOutput = fileHandle;
      //   }
      //   else
      //   {
      //     fileHandle = NULL;
      //   }
      // }

      // TODO: set $CWD to `workingDirectory`?
      // returns -1 on error
      // otherwise return status of the program

      auto pid = fork();
      if (pid == -1)
      {
        // TODO: Cry
        return false;
      }
      else if (pid == 0)
      {
        // TODO: Run finalArgs, ig
        char *arg;
        strcpy(arg, finalArgs.c_str());
        char *args[] = {
            arg,
            NULL,
        };
        execvp(programFilepath, args);
      }
      else
      {
        // TODO: Handle errors ig

        waitpid(pid, NULL, 0);
      }

      return true;
    }

    bool openFileWithDefaultProgram(const char *filepath)
    {
      executeProgram("code", filepath);
    }

    bool openFileWithVsCode(const char *filepath, int lineNumber)
    {
      std::string arg = lineNumber >= 0
                            ? std::string("--goto \"") + filepath + ":" + std::to_string(lineNumber) + "\""
                            : std::string("--goto \"") + filepath + "\"";
      executeProgram("code", arg.c_str());
    }

    bool fileExists(const char *filename)
    {
      struct stat file_stat;
      return (stat(filename, &file_stat) == 0) && S_ISREG(file_stat.st_mode);
    }

    bool dirExists(const char *dirName)
    {
      struct stat file_stat;
      return (stat(dirName, &file_stat) == 0) && S_ISDIR(file_stat.st_mode);
    }

    bool deleteFile(const char *filename)
    {
      return std::remove(filename) == 0;
    }

    std::string tmpfilename()
    {
      char buffer[] = "fnXXXXXX\0";
      if (mkstemp(buffer) == 0)
      {
        return std::string(buffer);
      }

      return std::string("");
    }

    std::string getSpecialAppDir()
    {
      if (homeDirectory.empty())
      {
        const char *homeVar = getenv("HOME");
        if (homeVar == NULL)
        {
          homeDirectory = std::string(getpwuid(getuid())->pw_dir);
        }
        else
        {
          homeDirectory = std::string(homeVar);
        }
      }

      return homeDirectory + "/.mathanimation";
    }

    void createDirIfNotExists(const char *dirName)
    {
      mkdir_p(dirName, 0755);
    }

    std::string md5FromString(const std::string &str, int md5Length)
    {
      return md5FromString(str.c_str(), str.length(), md5Length);
    }

    std::string md5FromString(char const *const str, size_t length, int md5Length)
    {
      unsigned char hashRes[MD5_DIGEST_LENGTH];
      MD5((unsigned char const *)str, length, hashRes);
      return (char *)hashRes;
    }
  }
}

#endif