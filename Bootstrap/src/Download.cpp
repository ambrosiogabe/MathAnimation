#include "Download.h"
#include "File.h"

#include <logger/logger.h>
#include <bitextractor.hpp>
#include <bit7z.hpp>
#include <curl/curl.h>

using namespace bit7z;

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

bool manim_download(const char* url, const char* outputDir, const char* outputFilename)
{
	bool ret = false;
	CURL* curl;
	FILE* fp;
	CURLcode res;

	g_logger_info("Downloading from '%s' into '%s'", url, outputFilename);
	if (!manim_create_dir_if_not_exists(outputDir))
	{
		g_logger_error("Could not create output directory '%s'. Cancelling curl installation.", outputDir);
		return false;
	}

	curl = curl_easy_init();
	if (curl)
	{
		fp = fopen(outputFilename, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
		curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			const char* msg = R"(
It looks like we couldn't download the ffmpeg binaries. Please go to https://www.gyan.dev/ffmpeg/builds/ and download
version 4.4 and place the binaries in the Animations/vendor/ffmpeg directory. If version 4.4 is not available then place
the newest version into the directory.
)";
			g_logger_error(msg);
			g_logger_error(curl_easy_strerror(res));
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
		g_logger_error("Could not initialize curl properly.");
	}

	fclose(fp);
	if (!ret)
	{
		remove(outputFilename);
	}

	return ret;
}

static std::wstring charPtrToWString(const char* string)
{
	size_t strLength = strlen(string);
	return std::wstring(&string[0], &string[strLength]);
}

bool manim_unzip(const char* fileToUnzip, const char* outputFile, zip_type type)
{
	g_logger_info("Unzipping '%s'.", fileToUnzip);
	try
	{
		std::wstring libStr = type == zip_type::_7Z ?
			L"7za.dll" : type == zip_type::_ZIP ?
			L"7z.dll" : L"";
		Bit7zLibrary lib{ libStr };
		const BitInOutFormat* format = type == zip_type::_7Z ?
			&BitFormat::SevenZip : type == zip_type::_ZIP ?
			&BitFormat::Zip : NULL;
		BitExtractor extractor{ lib, *format };

		std::wstring lFileToUnzip = charPtrToWString(fileToUnzip);
		std::wstring lOutputFile = charPtrToWString(outputFile);
		extractor.extract(lFileToUnzip, lOutputFile);
		
	}
	catch (const BitException& ex)
	{
		g_logger_error(ex.what());
		return false;
	}

	g_logger_info("'%s' successfully unzipped. Removing file '%s'", fileToUnzip, fileToUnzip);
	remove(fileToUnzip);
	return true;
}