#include <stdio.h>

#define GABE_CPP_UTILS_IMPL
#include "CppUtils/CppUtils.h"
using namespace CppUtils;
#undef GABE_CPP_UTILS_IMPL

#include "curl/curl.h"
#include "File.h"

#include "bitextractor.hpp"
#include <bit7z.hpp>
using namespace bit7z;

const char* zipFilename = "./Animations/vendor/tmp/tmp.7z";
const char* unzipDirName = "./Animations/vendor/tmp/unzipped";
const std::wstring lZipFilename = L"./Animations/vendor/tmp/tmp.7z";
const std::wstring lUnzipDirName = L"./Animations/vendor/tmp/unzipped";

const char* url = "https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-4.4-full_build-shared.7z";

#define kInputBufSize ((size_t)1 << 18)

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

bool downloadFfmpeg()
{
	bool ret = false;
	CURL* curl;
	FILE* fp;
	CURLcode res;

	Logger::Info("Downloading ffmpeg from '%s' into '%s'", url, zipFilename);
	if (!manim_create_dir_if_not_exists("./Animations/vendor/tmp"))
	{
		return false;
	}

	curl = curl_easy_init();
	if (curl)
	{
		fp = fopen(zipFilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
		curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			const char* msg = R"(
It looks like we couldn't download the ffmpeg binaries. Please go to https://www.gyan.dev/ffmpeg/builds/ and download
version 4.4 and place the binaries in the Animations/vendor/ffmpeg directory. If version 4.4 is not available then place
the newest version into the directory.
)";
			Logger::Error(msg);
			Logger::Error(curl_easy_strerror(res));
		}
		else
		{
			ret = true;
		}

		curl_easy_cleanup(curl);
		ret = true;
	}
	else
	{
		Logger::Error("Could not initialize curl properly.");
	}

	fclose(fp);
	if (!ret)
	{
		remove(zipFilename);
	}

	return ret;
}

void unzipFfmpeg()
{
	Logger::Info("Unzipping ffmpeg.");
	try
	{
		Bit7zLibrary lib{ L"7za.dll" };
		BitExtractor extractor{ lib, BitFormat::SevenZip };
		extractor.extract(lZipFilename, lUnzipDirName);
	}
	catch (const BitException& ex)
	{
		Logger::Error(ex.what());
	}
	
	Logger::Info("Ffmpeg successfully installed.");
	remove(zipFilename);

	const char* finalDir = "./Animations/vendor/ffmpeg";
	if (manim_is_dir(finalDir))
	{
		manim_remove_dir("./Animations/vendor/ffmpeg");
	}

	if (!manim_move_file("./Animations/vendor/tmp/unzipped/ffmpeg-4.4-full_build-shared", finalDir))
	{
		return;
	}

	Logger::Info("Removing tmp directory.");
	manim_remove_dir("./Animations/vendor/tmp");
}

int main()
{
	if (downloadFfmpeg())
	{
		unzipFfmpeg();
	}

	return 0;
}