#ifndef MATH_ANIM_AUDIO_H
#define MATH_ANIM_AUDIO_H
#include "core.h"

namespace MathAnim
{
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
		AudioSource defaultAudioSource();

		void play(AudioSource& source);
		void stop(AudioSource& source);

		void free(AudioSource& source);

		void free();
	}
}

#endif 