#include <stdio.h>

#define GABE_CPP_UTILS_IMPL
#include "CppUtils/CppUtils.h"
using namespace CppUtils;
#undef GABE_CPP_UTILS_IMPL

#include "File.h"
#include "Download.h"

const char* tmpDir = "./Animations/vendor/tmp";

const char* ffmpegZipFile = "./Animations/vendor/tmp/ffmpegTmp.7z";
const char* ffmpegUnzipDir = "./Animations/vendor/tmp/ffmpegUnzipped";
const char* ffmpegVendorDir = "./Animations/vendor/ffmpeg";
const char* ffmpegUrl = "https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-4.4-full_build-shared.7z";

const char* freetypeZipFile = "./Animations/vendor/tmp/freetypeTmp.zip";
const char* freetypeUnzipDir = "./Animations/vendor/tmp/freetypeUnzipped";
const char* freetypeVendorDir = "./Animations/vendor/freetype";
const char* freetypeUrl = "https://github.com/ubawurinna/freetype-windows-binaries/archive/refs/tags/v2.11.0.zip";

void install(const char* url, const char* zipFile, const char* unzipDir, const char* vendorDir, const char* unzippedFilename, zip_type zipType)
{
	if (manim_download(url, tmpDir, zipFile))
	{
		if (!manim_unzip(zipFile, unzipDir, zipType))
		{
			Logger::Error("Failed to unzip '%s'. Please install '%s' binaries manually. TODO: Write detailed instructions", zipFile, zipFile);
			return;
		}
	}

	if (manim_is_dir(vendorDir))
	{
		if (!manim_remove_dir(vendorDir))
		{
			Logger::Warning("Failed to remove directory for '%s'. Installation may fail.", vendorDir);
		}
	}

	if (!manim_move_file(unzippedFilename, vendorDir))
	{
		Logger::Error("Failed to move unzipped directory '%s' into '%s'", unzippedFilename, vendorDir);
	}
}

int main()
{
	install(ffmpegUrl, ffmpegZipFile, ffmpegUnzipDir, ffmpegVendorDir, "./Animations/vendor/tmp/ffmpegUnzipped/ffmpeg-4.4-full_build-shared", zip_type::_7Z);
	install(freetypeUrl, freetypeZipFile, freetypeUnzipDir, freetypeVendorDir, "./Animations/vendor/tmp/freetypeUnzipped/freetype-windows-binaries-2.11.0", zip_type::_ZIP);

	Logger::Info("Removing tmp directory.");
	manim_remove_dir(tmpDir);

	return 0;
}