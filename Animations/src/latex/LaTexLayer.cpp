#include "latex/LaTexLayer.h"
#include "core/Application.h"
#include "platform/Platform.h"
#include "multithreading/GlobalThreadPool.h"

static const char preamble[] = \
R"raw(\documentclass[preview]{standalone}

\usepackage[english]{babel}
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage{lmodern}
\usepackage{amsmath}
\usepackage{amssymb}
\usepackage{dsfont}
\usepackage{setspace}
\usepackage{tipa}
\usepackage{relsize}
\usepackage{textcomp}
\usepackage{mathrsfs}
\usepackage{calligra}
\usepackage{wasysym}
\usepackage{ragged2e}
\usepackage{physics}
\usepackage{xcolor}
\usepackage{microtype}
\DisableLigatures{encoding = *, family = * }
\linespread{1}

\begin{document}

)raw";
static constexpr size_t preambleLength = sizeof(preamble) - 1;

static const char beginAlign[] = "\n\\begin{align*}";
static constexpr size_t beginAlignLength = sizeof(beginAlign) - 1;

static const char endAlign[] = "\\end{align*}";
static constexpr size_t endAlignLength = sizeof(endAlign) - 1;

static const char postamble[] = "\n\\end{document}";
static constexpr size_t postambleLength = sizeof(postamble) - 1;

namespace MathAnim
{
	namespace LaTexLayer
	{
		struct LaTexTask
		{
			char* laTex;
			bool isMathTex;
		};

		// ----------- Internal variables ----------- 
		static bool latexIsInstalled;
		static char latexInstallLocation[MATH_ANIMATIONS_MAX_PATH];
		static char latexProgram[MATH_ANIMATIONS_MAX_PATH];
#ifdef _WIN32
		static const char* latexExeName = "/miktex/bin/x64/latex.exe";
#else
		static const char* latexExeName = "miktex";
#endif
		static char dvisvgmProgram[MATH_ANIMATIONS_MAX_PATH];
#ifdef _WIN32
		static const char* dvisvgmExeName = "/miktex/bin/x64/dvisvgm.exe";
#else
		static const char* dvisvgmExeName = "miktex-dvisvgm";
#endif

		static std::mutex latexQueueMutex;
		static std::unordered_map<std::string, std::string> latexCachedMd5;
		static std::queue<LaTexTask*> queuedLatex;
		static LaTexTask* latexInProgress;

		// ----------- Internal functions ----------- 
		static bool generateSvgFile(const std::string& filename, const std::string& latex, bool isMathTex);
		static void generateSvgTask(void* data, size_t dataSize);

		void init()
		{
			// Check if the user has latex.exe installed
			latexIsInstalled = Platform::getProgramInstallDir("miktex", latexInstallLocation, MATH_ANIMATIONS_MAX_PATH);
			if (!latexIsInstalled)
			{
				// TODO: Check alternative installation names
				// latexIsInstalled = Platform::getProgramInstallDir("latex", latexInstallLocation, _MAX_PATH);
				g_logger_warning("LaTeX was not found on the command line. TODO: Check the registry to see if it is installed but just not added to the %%path%%");
			}

			if (latexIsInstalled)
			{
				g_logger_info("LaTeX install location is '%s'.", latexInstallLocation);

				// Setup latex install location
				size_t latexInstallLocationLength = std::strlen(latexInstallLocation);
				size_t latexExeNameLength = std::strlen(latexExeName);
				if (latexInstallLocationLength + latexExeNameLength + 1 <= MATH_ANIMATIONS_MAX_PATH)
				{
					g_memory_copyMem(latexProgram, latexInstallLocation, sizeof(char) * latexInstallLocationLength);
					g_memory_copyMem(latexProgram + latexInstallLocationLength, (void*)latexExeName, sizeof(char) * latexExeNameLength);
					latexProgram[latexInstallLocationLength + latexExeNameLength] = '\0';

					if (Platform::fileExists(latexProgram))
					{
						g_logger_info("LaTeX program path: '%s'.", latexProgram);
					}
					else
					{
						g_logger_error("'%s' is not a valid program. You need to install LaTeX.", latexProgram);
						latexProgram[0] = '\0';
					}
				}
				else
				{
					g_logger_error("Not enough room for latex exe program path.");
					latexProgram[0] = '\0';
				}

				// Setup dvisvgm location
				size_t dvisvgmExeLength = std::strlen(dvisvgmExeName);
				if (latexInstallLocationLength + dvisvgmExeLength + 1 <= MATH_ANIMATIONS_MAX_PATH)
				{
					g_memory_copyMem(dvisvgmProgram, latexInstallLocation, sizeof(char) * latexInstallLocationLength);
					g_memory_copyMem(dvisvgmProgram + latexInstallLocationLength, (void*)dvisvgmExeName, sizeof(char) * dvisvgmExeLength);
					dvisvgmProgram[latexInstallLocationLength + dvisvgmExeLength] = '\0';

					if (Platform::fileExists(dvisvgmProgram))
					{
						g_logger_info("dvisvgm program path: '%s'.", dvisvgmProgram);
					}
					else
					{
						g_logger_error("'%s' is not a valid program. You need to install dvisvgm.", dvisvgmProgram);
						dvisvgmProgram[0] = '\0';
					}
				}
				else
				{
					g_logger_error("Not enough room for dvisvgm exe program path.");
					latexProgram[0] = '\0';
				}
			}

			Platform::createDirIfNotExists("latex");
		}

		void laTexToSvg(const char* latexRaw, bool isMathTex)
		{
			if (!latexIsInstalled)
			{
				g_logger_error("Cannot parse LaTeX. No LaTeX program found on the system.");
				return;
			}

			std::string latex = std::string(latexRaw);
			if (!laTexIsReady(latex))
			{
				LaTexTask* taskData = (LaTexTask*)g_memory_allocate(sizeof(LaTexTask));
				taskData->laTex = (char*)g_memory_allocate(sizeof(char) * latex.length() + 1);
				g_memory_copyMem(taskData->laTex, (void*)latex.c_str(), sizeof(char) * latex.length());
				taskData->laTex[latex.length()] = '\0';
				taskData->isMathTex = isMathTex;

				std::lock_guard<std::mutex> lock(latexQueueMutex);
				queuedLatex.push(taskData);
			}
		}

		bool laTexIsReady(const char* latexRaw, bool)
		{
			std::string latex = std::string(latexRaw);
			return laTexIsReady(latex);
		}

		bool laTexIsReady(const std::string& latex)
		{
			if (!latexIsInstalled)
			{
				g_logger_error("Cannot parse LaTeX. No LaTeX program found on the system.");
				return false;
			}

			// Md5 the latex to get a re-generatable filename
			std::string filename = Platform::md5FromString(latex);
			std::string svgFullpath = "latex/" + filename + ".svg";

			return Platform::fileExists(svgFullpath.c_str());
		}

		std::string getLaTexMd5(const char* latexRaw)
		{
			std::string latex = latexRaw;
			return getLaTexMd5(latex);
		}

		std::string getLaTexMd5(const std::string& latex)
		{
			auto iter = latexCachedMd5.find(latex);
			if (iter != latexCachedMd5.end())
			{
				return iter->second;
			}

			std::string md5 = Platform::md5FromString(latex);
			latexCachedMd5[latex] = md5;
			return md5;
		}

		void update()
		{
			std::lock_guard<std::mutex> lock(latexQueueMutex);

			// This loop ensures that only one LaTex file gets processed at a time so
			// that our application doesn't spawn a bunch of process asynchronously and
			// make everything super slow

			// First check if the last task that was being processed
			// has finished and free memory if it has
			if (queuedLatex.size() > 0)
			{
				if (queuedLatex.front() != nullptr && queuedLatex.front()  == latexInProgress)
				{
					// If the current item hasn't finished
					// processing, then wait until it's done
					// before processing more queued items
					if (!laTexIsReady(latexInProgress->laTex))
					{
						return;
					}

					// Free the task and pop it from our queue
					g_logger_info("Finished processing LaTex: '%s'", latexInProgress->laTex);
					g_memory_free(queuedLatex.front()->laTex);
					g_memory_free(queuedLatex.front());
					queuedLatex.pop();

					latexInProgress = nullptr;
				}

				// If there's any elements that need to be processed
				// process the next element
				if (queuedLatex.size() > 0)
				{
					LaTexTask* taskData = queuedLatex.front();
					// Launch this as a task on a new thread so that it doesn't block the main thread
					Application::threadPool()->queueTask(
						generateSvgTask,
						"Generate LaTeX Svg File",
						taskData,
						sizeof(LaTexTask)
					);

					latexInProgress = taskData;
				}
			}
		}

		void free()
		{
			std::lock_guard<std::mutex> lock(latexQueueMutex);
			while (queuedLatex.size() > 0)
			{
				g_memory_free(queuedLatex.front()->laTex);
				g_memory_free(queuedLatex.front());
				queuedLatex.pop();
			}
		}

		// ----------- Internal functions ----------- 
		static void generateSvgTask(void* data, size_t dataSize)
		{
			g_logger_assert(dataSize == sizeof(LaTexTask), "Invalid latex task.");
			LaTexTask* task = (LaTexTask*)data;

			std::string filename = getLaTexMd5(task->laTex);
			if (!generateSvgFile(filename, task->laTex, task->isMathTex))
			{
				std::lock_guard<std::mutex> lock(latexQueueMutex);
				g_memory_free(queuedLatex.front()->laTex);
				g_memory_free(queuedLatex.front());
				queuedLatex.pop();
			}
		}

		static bool generateSvgFile(const std::string& filename, const std::string& latex, bool isMathTex)
		{
			if (isMathTex)
			{
				// Make sure there are no new lines
				for (int i = 0; i < latex.length(); i++)
				{
					if (latex[i] == '\n')
					{
						if (i == 0)
						{
							// If the newline is at the beginning return a custom error message
							g_logger_error("No new lines allowed in MathTex. Newline was found at the beginning of your math equation:\n\t'%s'", latex.c_str());
						}
						else
						{
							std::string errorMessage = "No new lines allowed in MathTex. New line found:\n\t'";
							errorMessage += latex.substr(0, i) + std::string(" '\n\t");
							errorMessage += std::string(i + 1, ' ');
							errorMessage += std::string("^--- Here");
							g_logger_error("%s", errorMessage.c_str());
						}

						return false;
					}
				}
			}

			// First write the latex to a file to be processed
			std::string latexFilename = filename + ".tex";
			std::string latexFullpath = "latex/" + latexFilename;
			{
				FILE* fp = fopen(latexFullpath.c_str(), "wb");
				if (!fp)
				{
					g_logger_error("Failed to create file: '%s'", latexFullpath.c_str());
					return false;
				}

				fwrite(preamble, preambleLength, 1, fp);
				if (isMathTex)
				{
					fwrite(beginAlign, beginAlignLength, 1, fp);
				}
				fwrite(latex.c_str(), latex.length(), 1, fp);
				if (isMathTex)
				{
					fwrite(endAlign, endAlignLength, 1, fp);
				}
				fwrite(postamble, postambleLength, 1, fp);
				fclose(fp);
			}

			// Next process the files using the latex program and dvisgm on the system
			std::string workingDirectory = "./latex/";
			std::string cmdArgs = latexFilename + " -halt-on-error";
			std::string logFilename = filename + ".tex.log.txt";
			Platform::executeProgram(latexProgram, cmdArgs.c_str(), workingDirectory.c_str(), logFilename.c_str());

			std::string dviFilename = filename + ".dvi";
			std::string dviFullpath = "latex/" + dviFilename;
			if (Platform::fileExists(dviFullpath.c_str()))
			{
				std::string dviLogFilename = filename + ".dvi.log.txt";
				std::string cmdArgs2 = dviFilename + " -n";
				Platform::executeProgram(dvisvgmProgram, cmdArgs2.c_str(), workingDirectory.c_str(), dviLogFilename.c_str());

				// Sleep for 1 second before deleting files to make sure they are not locked
				std::this_thread::sleep_for(std::chrono::seconds(1));
				// If everything succeeded, clear out all the unnecessary files
				Platform::deleteFile(dviFullpath.c_str());
				Platform::deleteFile(latexFullpath.c_str());
				Platform::deleteFile(("latex/" + dviLogFilename).c_str());
				Platform::deleteFile(("latex/" + logFilename).c_str());
				Platform::deleteFile(("latex/" + filename + ".aux").c_str());
				Platform::deleteFile(("latex/" + filename + ".log").c_str());
			}
			else
			{
				g_logger_error("There was an error processing the latex file.");
				return false;
			}

			return true;
		}
	}
}
