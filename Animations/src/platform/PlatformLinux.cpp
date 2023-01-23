#ifdef __linux
#include "platform/Platform.h"
#include "core.h"

#include <filesystem>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include <openssl/evp.h>

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

      size_t start = 0U;
      size_t end = path.find(delim);  

      std::error_code dir_error_code;

      while (true)
      {
        std::string const folder = path.substr(start, end - start);

        std::filesystem::directory_iterator directory_iterator(folder, dir_error_code);
        if (dir_error_code)
        {
          g_logger_warning("Failed to create directory_iterator for %s: %s", folder.c_str(), dir_error_code.message().c_str());

          dir_error_code.clear();
        } else {
          for (std::filesystem::directory_entry const &directory_entry : directory_iterator)
          {
            std::filesystem::path const file = directory_entry.path();
            // Check if filename matches, and if file is executable.
            if (strcmp(file.filename().c_str(), programDisplayName) == 0 && access(file.c_str(), X_OK) == 0)
            {
              strncpy(buffer, file.parent_path().c_str(), bufferLength - 1);
              // Append / to the end of the path
              size_t buffer_strlen = strlen(buffer);
              buffer[buffer_strlen + 1] = '\0';
              buffer[buffer_strlen] = '/';
              return true;
            }
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

    bool executeProgram(const char *programFilePath, const char *cmdLineArgs, const char *workingDirectory, const char *executionOutputFilename)
    {
      g_logger_assert(executionOutputFilename == nullptr, "Executing program with executionOutputFilename (%s) not supported on *nix yet", executionOutputFilename);

      pid_t pid = fork();
      if (pid == -1)
      {
        // TODO: Cry
        return false;
      }
      else if (pid == 0)
      {
        // TODO: Handle executionOutputFilename
        chdir(workingDirectory);
        std::stringstream args_string(cmdLineArgs);
        std::string buf;

        std::vector<char *> args;
        while(std::getline(args_string, buf, ' '))
        {
          // +1 for null byte
          char *const arg = (char *const) calloc(buf.length() + 1, sizeof(char));
          strncpy(arg, buf.c_str(), buf.length() + 1);
          args.push_back(arg);
        }
        args.push_back(NULL);
        char *const *const argv = args.data();

        g_logger_assert(execvp(programFilePath, argv) != -1, "Failed to start program (%s %s) - %s", programFilePath, cmdLineArgs, strerror(errno));
        g_logger_assert(false, "Unreachable state. Process should've been replaced");
        return false;
      }
      else
      {
        return true;
      }
    }

    bool openFileWithDefaultProgram(const char *filepath)
    {
      return executeProgram("code", filepath);
    }

    bool openFileWithVsCode(const char *filepath, int lineNumber)
    {
      std::string arg = lineNumber >= 0
                            ? std::string("--goto ") + filepath + ":" + std::to_string(lineNumber)
                            : std::string("--goto ") + filepath;
      return executeProgram("code", arg.c_str());
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

    std::string md5FromString(char const *const str, size_t length, int md5Length) {
      unsigned int md5_digest_len = EVP_MD_size(EVP_md5());
			g_logger_assert(length == md5_digest_len, "Cannot generate md5 of size %d. Must be %d", md5Length, md5_digest_len);
      
      // MD5_Init
      EVP_MD_CTX * mdctx = EVP_MD_CTX_new();
      EVP_DigestInit_ex(mdctx, EVP_md5(), NULL); 

      // MD5_Update
      EVP_DigestUpdate(mdctx, str, length);

      // MD5_Final
      unsigned char* md5_digest = (unsigned char *)OPENSSL_malloc(md5_digest_len);
      EVP_DigestFinal_ex(mdctx, md5_digest, &md5_digest_len);
      EVP_MD_CTX_free(mdctx);
      // TODO: Is this how strings work? I dunno
      return std::string((char*)md5_digest);
    }
  }
}

#endif