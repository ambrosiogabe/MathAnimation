#include "latex/LaTexLayer.h"
#include "core/Application.h"
#include "platform/Platform.h"

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
		static bool latexIsInstalled;
		static char latexInstallLocation[_MAX_PATH];
		static char latexProgram[_MAX_PATH];
		static const char* latexExeName = "/miktex/bin/x64/latex.exe";
		static char dvisvgmProgram[_MAX_PATH];
		static const char* dvisvgmExeName = "/miktex/bin/x64/dvisvgm.exe";

		static std::unordered_map<std::string, std::string> latexCachedFiles;

		// ----------- Internal functions ----------- 
		static bool generateSvgFile(const std::string& filename, const std::string& latex, bool isMathTex);

		void init()
		{
			// Check if the user has latex.exe installed
			latexIsInstalled = Platform::getProgramInstallDir("miktex", latexInstallLocation, _MAX_PATH);
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
				if (latexInstallLocationLength + latexExeNameLength + 1 <= _MAX_PATH)
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
				if (latexInstallLocationLength + dvisvgmExeLength + 1 <= _MAX_PATH)
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

		void parseLaTeX(const char* latexRaw, bool isMathTex)
		{
			if (!latexIsInstalled)
			{
				g_logger_error("Cannot parse LaTeX. No LaTeX found on the system.");
				return;
			}


			std::string latex = std::string(latexRaw);
			// Md5 the latex to get a re-generatable filename
			std::string filename = Platform::md5FromString(latex);

			std::string svgFilename = filename + ".svg";
			std::string svgFullpath = "latex/" + svgFilename;

			// Try to generate the SVG file if needed
			if (!Platform::fileExists(svgFullpath.c_str()))
			{
				if (!generateSvgFile(filename, latex, isMathTex))
				{
					return;
				}
			}

			// Finally process the resulting SVG file
		}

		void update()
		{

		}

		void free()
		{
		}

		// ----------- Internal functions ----------- 
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
