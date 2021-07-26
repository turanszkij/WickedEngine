#include "wiAudio.h"
#include "wiBackLog.h"
#include "wiHelper.h"

#include <vector>

#define STB_VORBIS_HEADER_ONLY
#include "Utility/stb_vorbis.c"

#ifdef _WIN32

#include <wrl/client.h> // ComPtr
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#pragma comment(lib,"xaudio2.lib")

//Little-Endian things:
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

namespace wiAudio
{
	static const XAUDIO2FX_REVERB_I3DL2_PARAMETERS reverbPresets[] =
	{
		XAUDIO2FX_I3DL2_PRESET_DEFAULT,
		XAUDIO2FX_I3DL2_PRESET_GENERIC,
		XAUDIO2FX_I3DL2_PRESET_FOREST,
		XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
		XAUDIO2FX_I3DL2_PRESET_ROOM,
		XAUDIO2FX_I3DL2_PRESET_BATHROOM,
		XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
		XAUDIO2FX_I3DL2_PRESET_STONEROOM,
		XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
		XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
		XAUDIO2FX_I3DL2_PRESET_CAVE,
		XAUDIO2FX_I3DL2_PRESET_ARENA,
		XAUDIO2FX_I3DL2_PRESET_HANGAR,
		XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
		XAUDIO2FX_I3DL2_PRESET_HALLWAY,
		XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
		XAUDIO2FX_I3DL2_PRESET_ALLEY,
		XAUDIO2FX_I3DL2_PRESET_CITY,
		XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
		XAUDIO2FX_I3DL2_PRESET_QUARRY,
		XAUDIO2FX_I3DL2_PRESET_PLAIN,
		XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
		XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
		XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
		XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
		XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
		XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
		XAUDIO2FX_I3DL2_PRESET_PLATE,
	};

	struct AudioInternal
	{
		bool success = false;
		Microsoft::WRL::ComPtr<IXAudio2> audioEngine;
		IXAudio2MasteringVoice* masteringVoice = nullptr;
		XAUDIO2_VOICE_DETAILS masteringVoiceDetails = {};
		IXAudio2SubmixVoice* submixVoices[SUBMIX_TYPE_COUNT] = {};
		X3DAUDIO_HANDLE audio3D = {};
		Microsoft::WRL::ComPtr<IUnknown> reverbEffect;
		IXAudio2SubmixVoice* reverbSubmix = nullptr;

		AudioInternal()
		{
			HRESULT hr;
			hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
			assert(SUCCEEDED(hr));

			hr = XAudio2Create(&audioEngine, 0, XAUDIO2_USE_DEFAULT_PROCESSOR);
			assert(SUCCEEDED(hr));

#ifdef _DEBUG
			XAUDIO2_DEBUG_CONFIGURATION debugConfig = {};
			debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
			debugConfig.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
			audioEngine->SetDebugConfiguration(&debugConfig);
#endif // _DEBUG

			hr = audioEngine->CreateMasteringVoice(&masteringVoice);
			assert(SUCCEEDED(hr));

			if (masteringVoice == nullptr)
			{
				wiBackLog::post("Failed to create XAudio2 mastering voice!");
				return;
			}

			masteringVoice->GetVoiceDetails(&masteringVoiceDetails);

			// Without clamping sample rate, it was crashing 32bit 192kHz audio devices
			if (masteringVoiceDetails.InputSampleRate > 48000)
				masteringVoiceDetails.InputSampleRate = 48000;

			for (int i = 0; i < SUBMIX_TYPE_COUNT; ++i)
			{
				hr = audioEngine->CreateSubmixVoice(
					&submixVoices[i],
					masteringVoiceDetails.InputChannels,
					masteringVoiceDetails.InputSampleRate,
					0, 0, 0, 0);
				assert(SUCCEEDED(hr));
			}

			DWORD channelMask;
			masteringVoice->GetChannelMask(&channelMask);
			hr = X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, audio3D);
			assert(SUCCEEDED(hr));

			// Reverb setup:
			{
				hr = XAudio2CreateReverb(&reverbEffect);
				assert(SUCCEEDED(hr));

				XAUDIO2_EFFECT_DESCRIPTOR effects[] = { { reverbEffect.Get(), TRUE, 1 } };
				XAUDIO2_EFFECT_CHAIN effectChain = { arraysize(effects), effects };
				hr = audioEngine->CreateSubmixVoice(
					&reverbSubmix,
					1, // reverb is mono
					masteringVoiceDetails.InputSampleRate,
					0, 0, nullptr, &effectChain);
				assert(SUCCEEDED(hr));

				XAUDIO2FX_REVERB_PARAMETERS native;
				ReverbConvertI3DL2ToNative(&reverbPresets[REVERB_PRESET_DEFAULT], &native);
				HRESULT hr = reverbSubmix->SetEffectParameters(0, &native, sizeof(native));
				assert(SUCCEEDED(hr));
			}

			success = SUCCEEDED(hr);
		}
		~AudioInternal()
		{

			if (reverbSubmix != nullptr)
				reverbSubmix->DestroyVoice();

			for (int i = 0; i < SUBMIX_TYPE_COUNT; ++i)
			{
				if (submixVoices[i] != nullptr)
					submixVoices[i]->DestroyVoice();
			}

			if (masteringVoice != nullptr)
				masteringVoice->DestroyVoice();

			audioEngine->StopEngine();

			CoUninitialize();
		}
	};
	std::shared_ptr<AudioInternal> audio;

	struct SoundInternal
	{
		std::shared_ptr<AudioInternal> audio;
		WAVEFORMATEX wfx = {};
		std::vector<uint8_t> audioData;
	};
	struct SoundInstanceInternal
	{
		std::shared_ptr<AudioInternal> audio;
		std::shared_ptr<SoundInternal> soundinternal;
		IXAudio2SourceVoice* sourceVoice = nullptr;
		XAUDIO2_VOICE_DETAILS voiceDetails = {};
		std::vector<float> outputMatrix;
		std::vector<float> channelAzimuths;
		XAUDIO2_BUFFER buffer = {};

		~SoundInstanceInternal()
		{
			sourceVoice->Stop();
			sourceVoice->DestroyVoice();
		}
	};
	SoundInternal* to_internal(const Sound* param)
	{
		return static_cast<SoundInternal*>(param->internal_state.get());
	}
	SoundInstanceInternal* to_internal(const SoundInstance* param)
	{
		return static_cast<SoundInstanceInternal*>(param->internal_state.get());
	}

	void Initialize()
	{
		audio = std::make_shared<AudioInternal>();

		if (audio->success)
		{
			wiBackLog::post("wiAudio Initialized");
		}
	}

	bool FindChunk(const uint8_t* data, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
	{
		size_t pos = 0;

		DWORD dwChunkType;
		DWORD dwChunkDataSize;
		DWORD dwRIFFDataSize = 0;
		DWORD dwFileType;
		DWORD bytesRead = 0;
		DWORD dwOffset = 0;

		while(true)
		{
			memcpy(&dwChunkType, data + pos, sizeof(DWORD));
			pos += sizeof(DWORD);
			memcpy(&dwChunkDataSize, data + pos, sizeof(DWORD));
			pos += sizeof(DWORD);

			switch (dwChunkType)
			{
			case fourccRIFF:
				dwRIFFDataSize = dwChunkDataSize;
				dwChunkDataSize = 4;
				memcpy(&dwFileType, data + pos, sizeof(DWORD));
				pos += sizeof(DWORD);
				break;

			default:
				pos += dwChunkDataSize;
			}

			dwOffset += sizeof(DWORD) * 2;

			if (dwChunkType == fourcc)
			{
				dwChunkSize = dwChunkDataSize;
				dwChunkDataPosition = dwOffset;
				return true;
			}

			dwOffset += dwChunkDataSize;

			if (bytesRead >= dwRIFFDataSize) return false;

		}

		return true;

	}

	bool CreateSound(const std::string& filename, Sound* sound)
	{
		std::vector<uint8_t> filedata;
		bool success = wiHelper::FileRead(filename, filedata);
		if (!success)
		{
			return false;
		}
		return CreateSound(filedata, sound);
	}
	bool CreateSound(const std::vector<uint8_t>& data, Sound* sound)
	{
		return CreateSound(data.data(), data.size(), sound);
	}
	bool CreateSound(const uint8_t* data, size_t size, Sound* sound)
	{
		std::shared_ptr<SoundInternal> soundinternal = std::make_shared<SoundInternal>();
		soundinternal->audio = audio;
		sound->internal_state = soundinternal;

		DWORD dwChunkSize;
		DWORD dwChunkPosition;

		bool success;

		success = FindChunk(data, fourccRIFF, dwChunkSize, dwChunkPosition);
		if (success)
		{
			// Wav decoder:
			DWORD filetype;
			memcpy(&filetype, data + dwChunkPosition, sizeof(DWORD));
			if (filetype != fourccWAVE)
			{
				assert(0);
				return false;
			}

			success = FindChunk(data, fourccFMT, dwChunkSize, dwChunkPosition);
			if (!success)
			{
				assert(0);
				return false;
			}
			memcpy(&soundinternal->wfx, data + dwChunkPosition, dwChunkSize);
			soundinternal->wfx.wFormatTag = WAVE_FORMAT_PCM;

			success = FindChunk(data, fourccDATA, dwChunkSize, dwChunkPosition);
			if (!success)
			{
				assert(0);
				return false;
			}

			soundinternal->audioData.resize(dwChunkSize);
			memcpy(soundinternal->audioData.data(), data + dwChunkPosition, dwChunkSize);
		}
		else
		{
			// Ogg decoder:
			int channels = 0;
			int sample_rate = 0;
			short* output = nullptr;
			int samples = stb_vorbis_decode_memory(data, (int)size, &channels, &sample_rate, &output);
			if (samples < 0)
			{
				assert(0);
				return false;
			}

			// WAVEFORMATEX: https://docs.microsoft.com/en-us/previous-versions/dd757713(v=vs.85)?redirectedfrom=MSDN
			soundinternal->wfx.wFormatTag = WAVE_FORMAT_PCM;
			soundinternal->wfx.nChannels = (WORD)channels;
			soundinternal->wfx.nSamplesPerSec = (DWORD)sample_rate;
			soundinternal->wfx.wBitsPerSample = sizeof(short) * 8;
			soundinternal->wfx.nBlockAlign = (WORD)channels * sizeof(short); // is this right?
			soundinternal->wfx.nAvgBytesPerSec = soundinternal->wfx.nSamplesPerSec * soundinternal->wfx.nBlockAlign;

			size_t output_size = (size_t)samples * sizeof(short);
			soundinternal->audioData.resize(output_size);
			memcpy(soundinternal->audioData.data(), output, output_size);

			free(output);
		}

		return true;
	}
	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance)
	{
		HRESULT hr;
		const auto& soundinternal = std::static_pointer_cast<SoundInternal>(sound->internal_state);
		std::shared_ptr<SoundInstanceInternal> instanceinternal = std::make_shared<SoundInstanceInternal>();
		instance->internal_state = instanceinternal;

		instanceinternal->audio = audio;
		instanceinternal->soundinternal = soundinternal;

		XAUDIO2_SEND_DESCRIPTOR SFXSend[] = { 
			{ XAUDIO2_SEND_USEFILTER, audio->submixVoices[instance->type] },
			{ XAUDIO2_SEND_USEFILTER, audio->reverbSubmix }, // this should be last to enable/disable reverb simply
		};
		XAUDIO2_VOICE_SENDS SFXSendList = { 
			instance->IsEnableReverb() ? arraysize(SFXSend) : 1, 
			SFXSend 
		};

		hr = audio->audioEngine->CreateSourceVoice(&instanceinternal->sourceVoice, &soundinternal->wfx, 
			0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, &SFXSendList, NULL);
		if (FAILED(hr))
		{
			assert(0);
			return false;
		}

		instanceinternal->sourceVoice->GetVoiceDetails(&instanceinternal->voiceDetails);
		
		instanceinternal->outputMatrix.resize(size_t(instanceinternal->voiceDetails.InputChannels) * size_t(audio->masteringVoiceDetails.InputChannels));
		instanceinternal->channelAzimuths.resize(instanceinternal->voiceDetails.InputChannels);
		for (size_t i = 0; i < instanceinternal->channelAzimuths.size(); ++i)
		{
			instanceinternal->channelAzimuths[i] = X3DAUDIO_2PI * float(i) / float(instanceinternal->channelAzimuths.size());
		}

		instanceinternal->buffer.AudioBytes = (UINT32)soundinternal->audioData.size();
		instanceinternal->buffer.pAudioData = soundinternal->audioData.data();
		instanceinternal->buffer.Flags = XAUDIO2_END_OF_STREAM;
		instanceinternal->buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		instanceinternal->buffer.LoopBegin = UINT32(instance->loop_begin * audio->masteringVoiceDetails.InputSampleRate);
		instanceinternal->buffer.LoopLength = UINT32(instance->loop_length * audio->masteringVoiceDetails.InputSampleRate);

		hr = instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer);
		if (FAILED(hr))
		{
			assert(0);
			return false;
		}

		return true;
	}
	void Play(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			HRESULT hr = instanceinternal->sourceVoice->Start();
			assert(SUCCEEDED(hr));
		}
	}
	void Pause(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			HRESULT hr = instanceinternal->sourceVoice->Stop(); // preserves cursor position
			assert(SUCCEEDED(hr));
		}
	}
	void Stop(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			HRESULT hr = instanceinternal->sourceVoice->Stop(); // preserves cursor position
			assert(SUCCEEDED(hr)); 
			hr = instanceinternal->sourceVoice->FlushSourceBuffers(); // reset submitted audio buffer
			assert(SUCCEEDED(hr)); 
			hr = instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer); // resubmit
			assert(SUCCEEDED(hr));
		}
	}
	void SetVolume(float volume, SoundInstance* instance)
	{
		if (instance == nullptr || !instance->IsValid())
		{
			HRESULT hr = audio->masteringVoice->SetVolume(volume);
			assert(SUCCEEDED(hr));
		}
		else
		{
			auto instanceinternal = to_internal(instance);
			HRESULT hr = instanceinternal->sourceVoice->SetVolume(volume);
			assert(SUCCEEDED(hr));
		}
	}
	float GetVolume(const SoundInstance* instance)
	{
		float volume = 0;
		if (instance == nullptr || !instance->IsValid())
		{
			audio->masteringVoice->GetVolume(&volume);
		}
		else
		{
			auto instanceinternal = to_internal(instance);
			instanceinternal->sourceVoice->GetVolume(&volume);
		}
		return volume;
	}
	void ExitLoop(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			HRESULT hr = instanceinternal->sourceVoice->ExitLoop();
			assert(SUCCEEDED(hr));
		}
	}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume)
	{
		HRESULT hr = audio->submixVoices[type]->SetVolume(volume);
		assert(SUCCEEDED(hr));
	}
	float GetSubmixVolume(SUBMIX_TYPE type)
	{
		float volume;
		audio->submixVoices[type]->GetVolume(&volume);
		return volume;
	}

	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);

			X3DAUDIO_LISTENER listener = {};
			listener.Position = instance3D.listenerPos;
			listener.OrientFront = instance3D.listenerFront;
			listener.OrientTop = instance3D.listenerUp;
			listener.Velocity = instance3D.listenerVelocity;

			X3DAUDIO_EMITTER emitter = {};
			emitter.Position = instance3D.emitterPos;
			emitter.OrientFront = instance3D.emitterFront;
			emitter.OrientTop = instance3D.emitterUp;
			emitter.Velocity = instance3D.emitterVelocity;
			emitter.InnerRadius = instance3D.emitterRadius;
			emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;
			emitter.ChannelCount = instanceinternal->voiceDetails.InputChannels;
			emitter.pChannelAzimuths = instanceinternal->channelAzimuths.data();
			emitter.ChannelRadius = 0.1f;
			emitter.CurveDistanceScaler = 1;
			emitter.DopplerScaler = 1;

			UINT32 flags = 0;
			flags |= X3DAUDIO_CALCULATE_MATRIX;
			flags |= X3DAUDIO_CALCULATE_LPF_DIRECT;
			flags |= X3DAUDIO_CALCULATE_REVERB;
			flags |= X3DAUDIO_CALCULATE_LPF_REVERB;
			flags |= X3DAUDIO_CALCULATE_DOPPLER;
			//flags |= X3DAUDIO_CALCULATE_DELAY;
			//flags |= X3DAUDIO_CALCULATE_EMITTER_ANGLE;
			//flags |= X3DAUDIO_CALCULATE_ZEROCENTER;
			//flags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;

			X3DAUDIO_DSP_SETTINGS settings = {};
			settings.SrcChannelCount = instanceinternal->voiceDetails.InputChannels;
			settings.DstChannelCount = audio->masteringVoiceDetails.InputChannels;
			settings.pMatrixCoefficients = instanceinternal->outputMatrix.data();

			X3DAudioCalculate(audio->audio3D, &listener, &emitter, flags, &settings);

			HRESULT hr;

			hr = instanceinternal->sourceVoice->SetFrequencyRatio(settings.DopplerFactor);
			assert(SUCCEEDED(hr));

			hr = instanceinternal->sourceVoice->SetOutputMatrix(
				audio->submixVoices[instance->type],
				settings.SrcChannelCount, 
				settings.DstChannelCount, 
				settings.pMatrixCoefficients
			);
			assert(SUCCEEDED(hr));
			
			XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFDirectCoefficient), 1.0f };
			hr = instanceinternal->sourceVoice->SetOutputFilterParameters(audio->submixVoices[instance->type], &FilterParametersDirect);
			assert(SUCCEEDED(hr));

			if (instance->IsEnableReverb())
			{
				hr = instanceinternal->sourceVoice->SetOutputMatrix(audio->reverbSubmix, settings.SrcChannelCount, 1, &settings.ReverbLevel);
				assert(SUCCEEDED(hr));
				XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFReverbCoefficient), 1.0f };
				hr = instanceinternal->sourceVoice->SetOutputFilterParameters(audio->reverbSubmix, &FilterParametersReverb);
				assert(SUCCEEDED(hr));
			}
		}
	}

	void SetReverb(REVERB_PRESET preset)
	{
		XAUDIO2FX_REVERB_PARAMETERS native;
		ReverbConvertI3DL2ToNative(&reverbPresets[preset], &native);
		HRESULT hr = audio->reverbSubmix->SetEffectParameters(0, &native, sizeof(native));
		assert(SUCCEEDED(hr));
	}
}

#elif SDL2
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_stdinc.h>
#include <cstdint>

//WAVE audio data tags
//Little-Endian things in hex:
#define fourccRIFF 0x46464952 //'FFIR'
#define fourccDATA 0x61746164 //'atad'
#define fourccFMT  0x20746d66 //' tmf'
#define fourccWAVE 0x45564157 //'EVAW'
#define fourccXWMA 0x414d5758 //'AMWX'
#define fourccDPDS 0x73647064 //'sdpd'

//Format tags
//Source: https://github.com/libsdl-org/SDL/blob/main/src/audio/SDL_wave.h
#define WAV_UNKNOWN    0x0000
#define WAV_PCM        0x0001
#define WAV_ADPCM_MS   0x0002
#define WAV_IEEE       0x0003
#define WAV_ALAW       0x0006
#define WAV_MULAW      0x0007
#define WAV_ADPCM_IMA  0x0011
#define WAV_MPEG       0x0050
#define WAV_MPEGLAYER3 0x0055
#define WAV_EXTENSIBLE 0xFFFE

namespace wiAudio
{
    struct AudioInternal;

	static inline void ProcessAudioFeed(void* userdata, uint8_t* stream, int len);
	inline bool OpenAudioDevice(SDL_AudioDeviceID& device, AudioInternal* audio_internal);

	struct SoundInstanceInternal;

	struct AudioInternal
	{
		bool success = false;
		SDL_AudioDeviceID device;

		std::vector<std::shared_ptr<SoundInstanceInternal>> instances;

		AudioInternal()
		    : device(-1)
        {
			if(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)){
				return;
			}

			if(OpenAudioDevice(device, this)){
				success = true;
                set_mute(false);
			}
		}

		void set_mute(bool mute) {
            if (mute) {
                SDL_PauseAudioDevice(device, 1);
            } else {
                SDL_PauseAudioDevice(device, 0);
            }

        }

		~AudioInternal(){
		    if (device > 0) {
                set_mute(true);
                SDL_CloseAudioDevice(device);
            }
		}
	};
	std::shared_ptr<AudioInternal> audio;

	inline bool OpenAudioDevice(SDL_AudioDeviceID& device, AudioInternal *audio_internal){
		SDL_AudioSpec desired;
		desired.format = AUDIO_S16LSB;
		desired.freq = 48000;
		desired.channels = 2;
		desired.samples = 4096;
		desired.callback = ProcessAudioFeed;
		desired.userdata = audio_internal;

		return ((device = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE)) > 0);
	}

	struct SoundInternal {
		SDL_AudioSpec info {};
		std::vector<uint8_t> audioData;
	};
	struct SoundInstanceInternal {
		std::shared_ptr<AudioInternal> audio = nullptr;
		std::shared_ptr<SoundInternal> soundinternal = nullptr;

		enum FLAGS{
			EMPTY = 0,
			PLAY = 1 << 0,
			PAUSE = 1 << 1,
			LOOP = 1 << 2,
		};
		uint32_t _flags = EMPTY;

		size_t marker = 0;
		float volume = 0;

		size_t loop_begin = 0;
		size_t loop_end = 0;
	};
	SoundInternal* to_internal(const Sound* param)
	{
		return static_cast<SoundInternal*>(param->internal_state.get());
	}
	SoundInstanceInternal* to_internal(const SoundInstance* param)
	{
		return static_cast<SoundInstanceInternal*>(param->internal_state.get());
	}

	//Primary processing spot for audio processing
	static inline void ProcessAudioFeed(void* userdata, uint8_t* stream, int len){
		AudioInternal *_this = static_cast<AudioInternal *>(userdata);
        assert(_this);

        //Nullify master sound to receive new signal batch, see SDL_MixAudioFormat code snippet
        SDL_memset(stream, 0, len);

        for (std::shared_ptr<SoundInstanceInternal> &instance : _this->instances)
        {
            if (!(instance->_flags & instance->PLAY) || (instance->_flags & instance->PAUSE)) {
                continue;
            }

            bool loop_sound = instance->_flags & instance->LOOP;
            size_t source_buffer_size = instance->loop_end - instance->loop_begin;
            size_t marker_begin = instance->marker;
            size_t marker_end = marker_begin + len;
            size_t marker_end_loop = marker_end % source_buffer_size;
            bool loopback_necessary = loop_sound && (marker_end > instance->loop_end);
            if (!loop_sound && marker_end < instance->loop_end) {
                len = instance->loop_end - marker_begin;
            }
            marker_end = std::min(marker_end, instance->loop_end);

            assert(marker_begin != marker_end_loop);
            assert(marker_begin >= instance->loop_begin);
            assert(marker_end <= instance->loop_end);

            // buffer structure should be 2 bytes per channel alternating. E.g. LLRRLLRRLLRRLLRRLLRR..
            // Channel layout reference : https://github.com/libsdl-org/SDL/blob/main/include/SDL_audio.h

            //TODO remove Sine wave test
//            for (int j = 0; j < buffer.size(); j += 4) {
//                float amplitude = 200;
//                float frequency = 32;
//                buffer[j + 2] = amplitude * std::sin(2.0 * M_PI * frequency * (float(marker + j) / 48000.0));
//                buffer[j + 3] = amplitude * std::sin(2.0 * M_PI * frequency * (float(marker + j + 1) / 48000.0));
//            }

            const auto &audio_data = instance->soundinternal->audioData.data();
            const SDL_AudioFormat &format = instance->soundinternal->info.format;
            // SDL_MixAudioFormat wants the volume between 0 and 128.
            int volume = static_cast<int>(std::clamp(instance->volume, 0.0f, 1.0f) * 128.0f);

            //Mix sound instances buffer into master sound
            Uint32 sdl_len = marker_end - marker_begin;
            SDL_MixAudioFormat(stream, audio_data + marker_begin, format, sdl_len, volume);

            if (loopback_necessary) {
                Uint32 old_len = sdl_len;
                sdl_len = marker_end_loop - instance->loop_begin;
                SDL_MixAudioFormat(stream + old_len, audio_data + instance->loop_begin, format, sdl_len, volume);
            }

            //Wave chunk positioning behavior
            if ((instance->marker + len) < instance->loop_end) {
                instance->marker += len;
            } else if (loop_sound) {
                instance->marker = instance->loop_begin;
            } else {
                instance->marker = 0;
                instance->_flags &= ~(uint32_t) instance->PLAY;
            }
        }
	}

	void Initialize()
	{
		audio = std::make_shared<AudioInternal>();

		if (audio->success)
		{
			wiBackLog::post("wiAudio Initialized");
		}
	}

	bool CreateSound(const std::string& filename, Sound* sound)
	{
        SDL_RWops *filedata = SDL_RWFromFile(filename.c_str(), "rb");
		return CreateSound(filedata, sound);
	}
	bool CreateSound(const std::vector<uint8_t>& data, Sound* sound)
	{
		return CreateSound(data.data(), data.size(), sound);
	}
    bool CreateSound(const uint8_t* buffer, size_t size, Sound* sound)
    {
        SDL_RWops *data = SDL_RWFromMem((void *) buffer, size);
        return CreateSound(data, sound);
    }
    bool CreateSound(SDL_RWops* data, Sound* sound)
	{
        std::shared_ptr<SoundInternal> soundinternal = std::make_shared<SoundInternal>();

        Uint8 *audio_buffer;
        Uint32 audio_len;

		Uint32 header[2];
		SDL_RWread(data,header,4,2);
        
		if(header[0]==fourccRIFF)
		{
			SDL_RWseek(data,0,RW_SEEK_SET);
			SDL_AudioSpec *ret = SDL_LoadWAV_RW(data, false, &soundinternal->info, &audio_buffer, &audio_len);
        	if (ret == nullptr) {
            	return false;
        	}
        	soundinternal->audioData.resize(audio_len);
        	std::copy(audio_buffer, audio_buffer+audio_len, soundinternal->audioData.begin());
        	SDL_FreeWAV(audio_buffer);
		}
		else
		{
			SDL_RWseek(data,0,RW_SEEK_SET);

			Sint64 size = SDL_RWsize(data);

			std::vector<uint8_t> temp;
			temp.resize(size);

			SDL_RWread(data, temp.data(), 1, size);

			int channels = 0;
			int sample_rate = 0;
			short* output = nullptr;
			int samples = stb_vorbis_decode_memory(temp.data(), (int)temp.size(), &channels, &sample_rate, &output);
			if (samples < 0)
			{
				assert(0);
				return false;
			}

			soundinternal->info.format = AUDIO_S16LSB;
			soundinternal->info.samples = 4096;
			soundinternal->info.channels = (Uint8)channels;
			soundinternal->info.freq = sample_rate;

			size_t output_size = (size_t)samples * sizeof(short);
			soundinternal->audioData.resize(output_size);
			memcpy(soundinternal->audioData.data(), output, output_size);

			free(output);
		}
        
		sound->internal_state = soundinternal;

		return true;
	}
	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance)
	{
		const auto& soundinternal = std::static_pointer_cast<SoundInternal>(sound->internal_state);
		std::shared_ptr<SoundInstanceInternal> instanceinternal = std::make_shared<SoundInstanceInternal>();
		instance->internal_state = instanceinternal;

		instanceinternal->audio = audio;
		instanceinternal->soundinternal = soundinternal;

		instanceinternal->marker = instance->loop_begin;
		instanceinternal->loop_begin = instance->loop_begin;
		if (instance->loop_length > 0) {
            instanceinternal->loop_end = instance->loop_begin + instance->loop_length;
        } else {
		   // If length is zero, use full buffer
           instanceinternal->loop_end = soundinternal->audioData.size();
        }
		instanceinternal->volume = 1.0;
		instanceinternal->_flags |= instanceinternal->LOOP;

		audio->instances.push_back(instanceinternal);

		return true;
	}

	void Play(SoundInstance* instance)
	{
		SoundInstanceInternal* instanceinternal = to_internal(instance);
		instanceinternal->_flags |= instanceinternal->PLAY;
		instanceinternal->_flags &= ~(uint32_t)instanceinternal->PAUSE;
	}
	void Pause(SoundInstance* instance)
	{
		SoundInstanceInternal* instanceinternal = to_internal(instance);
		instanceinternal->_flags |= instanceinternal->PAUSE;
	}
	void Stop(SoundInstance* instance){
		SoundInstanceInternal* instanceinternal = to_internal(instance);
		instanceinternal->_flags &= ~(uint32_t)instanceinternal->PLAY;
		instanceinternal->_flags &= ~(uint32_t)instanceinternal->PAUSE;
	}
	void SetVolume(float volume, SoundInstance* instance)
	{
		SoundInstanceInternal* instanceinternal = to_internal(instance);
		instanceinternal->volume = volume;
	}
	float GetVolume(const SoundInstance* instance) { return to_internal(instance)->volume; }
	void ExitLoop(SoundInstance* instance)
	{
		SoundInstanceInternal* instanceinternal = to_internal(instance);
		instanceinternal->_flags &= ~(uint32_t)instanceinternal->LOOP;
	}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume) {}
	float GetSubmixVolume(SUBMIX_TYPE type) { return 0; }

	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D) {}

	void SetReverb(REVERB_PRESET preset) {}
}

#else

namespace wiAudio
{
	void Initialize() {}

	bool CreateSound(const std::string& filename, Sound* sound) { return false; }
	bool CreateSound(const std::vector<uint8_t>& data, Sound* sound) { return false; }
	bool CreateSound(const uint8_t* data, size_t size, Sound* sound) { return false; }
	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance) { return false; }

	void Play(SoundInstance* instance) {}
	void Pause(SoundInstance* instance) {}
	void Stop(SoundInstance* instance) {}
	void SetVolume(float volume, SoundInstance* instance) {}
	float GetVolume(const SoundInstance* instance) { return 0; }
	void ExitLoop(SoundInstance* instance) {}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume) {}
	float GetSubmixVolume(SUBMIX_TYPE type) { return 0; }

	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D) {}

	void SetReverb(REVERB_PRESET preset) {}
}

#endif // _WIN32
