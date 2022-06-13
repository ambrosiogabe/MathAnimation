#ifndef MATH_ANIM_AUDIO_H
#define MATH_ANIM_AUDIO_H
#include "core.h"

namespace MathAnim
{
	struct WavData;
	struct AudioSource
	{
		uint32 bufferId;
		uint32 sourceId;
		bool isPlaying;
	};

	namespace Audio
	{
		void init();

		AudioSource loadWavFile(const char* filename);
		AudioSource loadWavFile(const WavData& wav);
		AudioSource defaultAudioSource();

		bool isNull(const AudioSource& wav);
		void play(AudioSource& source, float offsetInSeconds = 0.0f);
		void stop(AudioSource& source);

		void free(AudioSource& source);

		void free();
	}
}

#endif 