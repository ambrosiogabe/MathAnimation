// Implementations
#define GABE_CPP_UTILS_IMPL
#include <CppUtils/CppUtils.h>
#undef GABE_CPP_UTILS_IMPL
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_write.h>
#undef STB_IMAGE_WRITE_IMPLEMENTATION

#include <CppUtils/CppUtils.h>
#include "core/Application.h"

#include "renderer/VideoWriter.h"

using namespace MathAnim;
int main()
{
	VideoWriter::startEncodingFile("newTest.mp4", 3840, 2160, 60);

	Pixel* pixels = (Pixel*)AllocMem(sizeof(Pixel) * 3840 * 2160);
	for (int i = 0; i < 60 * 10; i++)
	{
		for (int y = 0; y < 2160; y++)
		{
			for (int x = 0; x < 3840; x++)
			{
				pixels[x + (y * 3840)].r = y < i ? 0x00 : 0xFF;
				pixels[x + (y * 3840)].g = 0x00;
				pixels[x + (y * 3840)].b = 0x00;
			}
		}

		VideoWriter::pushFrame(pixels, 3840 * 2160);
	}

	VideoWriter::finishEncodingFile();
	FreeMem(pixels);

	Application::run();
	return 0;
}

