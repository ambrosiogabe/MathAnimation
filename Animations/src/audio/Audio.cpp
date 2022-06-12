#include "audio/Audio.h"

#include <AL/al.h>
#include <AL/alext.h>

namespace MathAnim
{
	namespace Audio
	{
		static ALCdevice* device;
		static ALCcontext* context;

		void init()
		{
			device = alcOpenDevice(NULL);
			if (!device)
			{
				g_logger_error("Failed to open an audio device.");
				return;
			}

			context = alcCreateContext(device, NULL);
			alcMakeContextCurrent(context);

			// Check for EAX 2.0 support

			const ALCchar* devices = alcGetString(device, ALC_DEVICE_SPECIFIER);
			g_logger_info("%s", devices);
		}

		void free()
		{
			alcMakeContextCurrent(NULL);

			if (context)
			{
				alcDestroyContext(context);
				context = NULL;
			}

			if (device)
			{
				ALCboolean res = alcCloseDevice(device);
				device = NULL;
				if (!res)
				{
					g_logger_error("Failed to free openAL device.");
				}
			}
		}
	}
}