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

	inline std::ostream& operator<<(std::ostream& ostream, AudioChannelType channel)
	{
		switch (channel)
		{
		case AudioChannelType::None: ostream << "AudioChannelType::None"; break;
		case AudioChannelType::Dual: ostream << "AudioChannelType::Dual"; break;
		case AudioChannelType::Mono: ostream << "AudioChannelType::Mono"; break;
		case AudioChannelType::Length: ostream << "AudioChannelType::Length"; break;
		default: ostream << "AudioChannelType::UNDEFINED"; break;
		}
		return ostream;
	}

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