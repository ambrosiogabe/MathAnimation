#ifndef MATH_ANIM_WAV_LOADER_H
#define MATH_ANIM_WAV_LOADER_H
#include "core.h"

namespace MathAnim
{
	enum class AudioChannelType : uint8
	{
		None,
		Dual,
		Mono,
		Length
	};

	struct WavData
	{
		uint32 sampleRate;
		uint32 bytesPerSec;
		uint16 blockAlignment;
		uint16 bitsPerSample;
		AudioChannelType audioChannelType;
		uint32 dataSize;
		uint8* audioData;
	};

	namespace WavLoader
	{
		WavData loadWavFile(const char* filename);

		void free(WavData& wav);
	}
}

#endif 