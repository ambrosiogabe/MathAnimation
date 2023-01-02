#include "audio/WavLoader.h"

namespace MathAnim
{
	struct WavDataHeader
	{
		uint32 size;
		uint16 wavTypeFormat;
		uint16 audioChannelType;
		uint32 sampleRate;
		uint32 bytesPerSec;
		uint16 blockAlignment;
		uint16 bitsPerSample;
		uint8 magicData[4];
		uint32 dataSize;
	};

	struct WavFileHeader
	{
		uint8 magicRiff[4];
		uint32 size;
		uint8 magicWave[4];
		uint8 magicFmt[4];
		WavDataHeader dataHeader;
	};

	namespace WavLoader
	{
		WavData loadWavFile(const char* filename)
		{
			FILE* fp = fopen(filename, "rb");
			if (!fp)
			{
				return {};
			}

			WavFileHeader header;
			fread((void*)&header, sizeof(WavFileHeader), 1, fp);
			g_logger_assert(
				header.magicRiff[0] == 'R' &&
				header.magicRiff[1] == 'I' &&
				header.magicRiff[2] == 'F' &&
				header.magicRiff[3] == 'F', "Invalid header. Did not get 'RIFF' magic number. Instead got '%s'", header.magicRiff);
			g_logger_assert(
				header.magicWave[0] == 'W' &&
				header.magicWave[1] == 'A' &&
				header.magicWave[2] == 'V' &&
				header.magicWave[3] == 'E', "Invalid header. Did not get 'WAVE' magic number. Instead got '%s'", header.magicWave);
			g_logger_assert(
				header.magicFmt[0] == 'f' &&
				header.magicFmt[1] == 'm' &&
				header.magicFmt[2] == 't' &&
				header.magicFmt[3] == ' ', "Invalid header. Did not get 'fmt ' magic number. Instead got '%s'", header.magicFmt);
			g_logger_assert(
				header.dataHeader.magicData[0] == 'd' &&
				header.dataHeader.magicData[1] == 'a' &&
				header.dataHeader.magicData[2] == 't' &&
				header.dataHeader.magicData[3] == 'a', "Invalid header. Did not get 'data' magic number. Instead got '%s'", header.dataHeader.magicData);


			WavData res;
			res.audioChannelType = header.dataHeader.audioChannelType == 2
				? AudioChannelType::Dual
				: header.dataHeader.audioChannelType == 1
				? AudioChannelType::Mono
				: AudioChannelType::None;
			res.bitsPerSample = header.dataHeader.bitsPerSample;
			res.blockAlignment = header.dataHeader.blockAlignment;
			res.bytesPerSec = header.dataHeader.bytesPerSec;
			res.dataSize = header.dataHeader.dataSize;
			res.sampleRate = header.dataHeader.sampleRate;

			uint8* audioData = (uint8*)g_memory_allocate(sizeof(uint8) * header.dataHeader.dataSize);
			size_t amtRead = fread(audioData, sizeof(uint8) * header.dataHeader.dataSize, 1, fp);
			g_logger_assert(amtRead == 1, "Failed to read all the audio data.");
			fclose(fp);
			res.audioData = audioData;

			g_logger_assert(res.audioChannelType != AudioChannelType::None, "Unknown audio channel format '%d'. Should be 1 or 2 for mono or dual audio channels.", header.dataHeader.audioChannelType);

			return res;
		}

		void free(WavData& wav)
		{
			if (wav.audioData)
			{
				g_memory_free(wav.audioData);
				wav.audioData = nullptr;
			}
			g_memory_zeroMem(&wav, sizeof(WavData));
		}
	}
}