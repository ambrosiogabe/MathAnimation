#ifndef BOOTSTRAP_DOWNLOAD_H
#define BOOTSTRAP_DOWNLOAD_H

enum zip_type
{
	NONE = 0,
	_7Z,
	_ZIP
};

bool manim_download(const char* url, const char* outputDir, const char* outputFilename);

bool manim_unzip(const char* fileToUnzip, const char* outputFile, zip_type type);

#endif
