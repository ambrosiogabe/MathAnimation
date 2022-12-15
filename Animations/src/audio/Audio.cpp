#include "audio/Audio.h"
#include "audio/WavLoader.h"

#include <AL/al.h>
#include <AL/alext.h>

namespace MathAnim
{
	// TODO: Come up with an error handling macro or something for 
	// OpenAL calls
	namespace Audio
	{
		// -------------- Internal Variables --------------
		static ALCdevice* device;
		static ALCcontext* context;
		static const ALCchar* currentDeviceName;
		static std::vector<std::string> friendlyDeviceNames;
		static uint32 error;

		// -------------- Internal Functions --------------
		static void displayError(uint32 error);

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

			// Collect all the available audio playback device names and store them
			const ALCchar* devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
			currentDeviceName = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
			g_logger_info("Audio Output Device: '%s'", currentDeviceName);
			{
				std::string logMessage = "Available Output Devices:\n";
				const ALCchar* currentDevice = devices;
				// This should iterate until it encounters \0\0 which is the termination of 
				// the list of strings
				int deviceIndex = 0;
				while (*currentDevice != '\0')
				{
					std::string deviceName = std::string(currentDevice);
					// TODO: Provide a drop down in the audio preview so the user can select
					// which output device to use
					friendlyDeviceNames.push_back(deviceName);

					logMessage += std::string("\tDevice[") + std::to_string(deviceIndex) + "]: " + deviceName + "\n";
					while (*currentDevice != '\0')
					{
						currentDevice++;
					}
					deviceIndex++;
				}

				g_logger_info("%s", logMessage.c_str());
			}
		}

		AudioSource loadWavFile(const char* filename)
		{
			WavData data = WavLoader::loadWavFile(filename);
			AudioSource res = loadWavFile(data);
			WavLoader::free(data);

			return res;
		}

		AudioSource loadWavFile(const WavData& wav)
		{
			AudioSource res = defaultAudioSource();
			alGenBuffers(1, &res.bufferId);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				displayError(error);
				alDeleteBuffers(1, &res.bufferId);
				res.bufferId = UINT32_MAX;
				return res;
			}

			uint32 format = AL_INVALID_ENUM;
			switch (wav.audioChannelType)
			{
			case AudioChannelType::Mono:
				if (wav.bitsPerSample == 8)
				{
					format = AL_FORMAT_MONO8;
				}
				else if (wav.bitsPerSample == 16)
				{
					format = AL_FORMAT_MONO16;
				}
				break;
			case AudioChannelType::Dual:
				if (wav.bitsPerSample == 8)
				{
					format = AL_FORMAT_STEREO8;
				}
				else if (wav.bitsPerSample == 16)
				{
					format = AL_FORMAT_STEREO16;
				}
				break;
			case AudioChannelType::Length:
			case AudioChannelType::None:
				break;
			}

			if (format == AL_INVALID_ENUM)
			{
				g_logger_error("Unable to map WAV file with format %d channels and %d bits/sample to AL format.", (int)wav.audioChannelType, wav.bitsPerSample);
				alDeleteBuffers(1, &res.bufferId);
				res.bufferId = UINT32_MAX;
				return res;
			}

			alBufferData(res.bufferId, format, wav.audioData, wav.dataSize, wav.sampleRate);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				displayError(error);
				alDeleteBuffers(1, &res.bufferId);
				res.bufferId = UINT32_MAX;
				return res;
			}

			alGenSources(1, &res.sourceId);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				alDeleteBuffers(1, &res.bufferId);
				alDeleteSources(1, &res.sourceId);
				displayError(error);
				res.sourceId = UINT32_MAX;
				res.bufferId = UINT32_MAX;
				return res;
			}

			alSourcef(res.sourceId, AL_PITCH, 1.0f);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				alDeleteBuffers(1, &res.bufferId);
				alDeleteSources(1, &res.sourceId);
				displayError(error);
				res.sourceId = UINT32_MAX;
				res.bufferId = UINT32_MAX;
				return res;
			}

			alSourcef(res.sourceId, AL_GAIN, 1.0f);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				alDeleteBuffers(1, &res.bufferId);
				alDeleteSources(1, &res.sourceId);
				displayError(error);
				res.sourceId = UINT32_MAX;
				res.bufferId = UINT32_MAX;
				return res;
			}

			alSource3f(res.sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				alDeleteBuffers(1, &res.bufferId);
				alDeleteSources(1, &res.sourceId);
				displayError(error);
				res.sourceId = UINT32_MAX;
				res.bufferId = UINT32_MAX;
				return res;
			}

			alSource3f(res.sourceId, AL_VELOCITY, 0, 0, 0);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				alDeleteBuffers(1, &res.bufferId);
				alDeleteSources(1, &res.sourceId);
				displayError(error);
				res.sourceId = UINT32_MAX;
				res.bufferId = UINT32_MAX;
				return res;
			}

			alSourcei(res.sourceId, AL_LOOPING, AL_FALSE);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				alDeleteBuffers(1, &res.bufferId);
				alDeleteSources(1, &res.sourceId);
				displayError(error);
				res.sourceId = UINT32_MAX;
				res.bufferId = UINT32_MAX;
				return res;
			}

			alSourcei(res.sourceId, AL_BUFFER, res.bufferId);
			if ((error = alGetError()) != AL_NO_ERROR)
			{
				alDeleteBuffers(1, &res.bufferId);
				alDeleteSources(1, &res.sourceId);
				displayError(error);
				res.sourceId = UINT32_MAX;
				res.bufferId = UINT32_MAX;
				return res;
			}

			return res;
		}

		AudioSource defaultAudioSource()
		{
			AudioSource res;
			res.bufferId = UINT32_MAX;
			res.sourceId = UINT32_MAX;
			res.isPlaying = false;
			return res;
		}

		bool isNull(const AudioSource& source)
		{
			return source.bufferId == UINT32_MAX && source.sourceId == UINT32_MAX;
		}

		void play(AudioSource& source, float offsetInSeconds)
		{
			g_logger_assert(source.sourceId != UINT32_MAX, "Tried to play null audio source.");
			if (offsetInSeconds != 0.0f)
			{
				alSourcef(source.sourceId, AL_SEC_OFFSET, offsetInSeconds);
			}
			alSourcePlay(source.sourceId);
			source.isPlaying = true;
		}

		void stop(AudioSource& source)
		{
			g_logger_assert(source.sourceId != UINT32_MAX, "Tried to stop null audio source.");
			alSourceStop(source.sourceId);
			source.isPlaying = false;
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

		void free(AudioSource& source)
		{
			if (source.sourceId != UINT32_MAX)
			{
				alDeleteSources(1, &source.sourceId);
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					displayError(error);
				}
				source.sourceId = UINT32_MAX;
			}

			if (source.bufferId != UINT32_MAX)
			{
				alDeleteBuffers(1, &source.bufferId);
				if ((error = alGetError()) != AL_NO_ERROR)
				{
					displayError(error);
				}
				source.bufferId = UINT32_MAX;
			}

			source.isPlaying = false;
		}

		// -------------- Internal Functions --------------
		static void displayError(uint32 inError)
		{
			if (inError != ALC_NO_ERROR)
			{
				switch (inError)
				{
				case ALC_INVALID_VALUE:
					g_logger_error("ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function");
					break;
				case ALC_INVALID_DEVICE:
					g_logger_error("ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function");
					break;
				case ALC_INVALID_CONTEXT:
					g_logger_error("ALC_INVALID_CONTEXT: a bad context was passed to an OpenAL function");
					break;
				case ALC_INVALID_ENUM:
					g_logger_error("ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function");
					break;
				case ALC_OUT_OF_MEMORY:
					g_logger_error("ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function");
					break;
				default:
					g_logger_error("UNKNOWN ALC ERROR: %u", inError);
				}
			}
		}
	}
}