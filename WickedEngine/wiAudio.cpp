#include "wiAudio.h"
#include "wiBacklog.h"
#include "wiHelper.h"
#include "wiTimer.h"
#include "wiVector.h"

#define STB_VORBIS_HEADER_ONLY
#include "Utility/stb_vorbis.c"

template<typename T>
static constexpr T AlignTo(T value, T alignment)
{
	return ((value + alignment - T(1)) / alignment) * alignment;
}

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

namespace wi::audio
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
		uint32_t termination_data = 0;
		XAUDIO2_BUFFER termination_mark = {};

		AudioInternal()
		{
			wi::Timer timer;

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
				wi::backlog::post("Failed to create XAudio2 mastering voice!");
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

				if (reverbSubmix != nullptr)
				{
					XAUDIO2FX_REVERB_PARAMETERS native;
					ReverbConvertI3DL2ToNative(&reverbPresets[REVERB_PRESET_DEFAULT], &native);
					HRESULT hr = reverbSubmix->SetEffectParameters(0, &native, sizeof(native));
					assert(SUCCEEDED(hr));
				}
				else
				{
					wi::backlog::post("wi::audio [XAudio2] Reverb Submix was not created successfully!", wi::backlog::LogLevel::Warning);
				}
			}

			termination_mark.Flags = XAUDIO2_END_OF_STREAM;
			termination_mark.pAudioData = (const BYTE*)&termination_data;
			termination_mark.AudioBytes = sizeof(termination_data);

			if (SUCCEEDED(hr))
			{
				wi::backlog::post("wi::audio Initialized [XAudio2] (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
			}
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
	static std::shared_ptr<AudioInternal> audio_internal;

	void Initialize()
	{
		audio_internal = std::make_shared<AudioInternal>();
	}

	struct SoundInternal
	{
		std::shared_ptr<AudioInternal> audio;
		WAVEFORMATEX wfx = {};
		wi::vector<uint8_t> audioData;
	};
	struct SoundInstanceInternal : public IXAudio2VoiceCallback
	{
		std::shared_ptr<AudioInternal> audio;
		std::shared_ptr<SoundInternal> soundinternal;
		IXAudio2SourceVoice* sourceVoice = nullptr;
		XAUDIO2_VOICE_DETAILS voiceDetails = {};
		wi::vector<float> outputMatrix;
		wi::vector<float> channelAzimuths;
		XAUDIO2_BUFFER buffer = {};
		bool ended = true;

		~SoundInstanceInternal()
		{
			sourceVoice->Stop();
			sourceVoice->DestroyVoice();
		}

		// Called just before this voice's processing pass begins.
		STDMETHOD_(void, OnVoiceProcessingPassStart) (THIS_ UINT32 BytesRequired)
		{
		}

		// Called just after this voice's processing pass ends.
		STDMETHOD_(void, OnVoiceProcessingPassEnd) (THIS)
		{
		}

		// Called when this voice has just finished playing a buffer stream
		// (as marked with the XAUDIO2_END_OF_STREAM flag on the last buffer).
		STDMETHOD_(void, OnStreamEnd) (THIS)
		{
			ended = true;
		}

		// Called when this voice is about to start processing a new buffer.
		STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext)
		{
			ended = false;
		}

		// Called when this voice has just finished processing a buffer.
		// The buffer can now be reused or destroyed.
		STDMETHOD_(void, OnBufferEnd) (THIS_ void* pBufferContext)
		{
		}

		// Called when this voice has just reached the end position of a loop.
		STDMETHOD_(void, OnLoopEnd) (THIS_ void* pBufferContext)
		{
		}

		// Called in the event of a critical error during voice processing,
		// such as a failing xAPO or an error from the hardware XMA decoder.
		// The voice may have to be destroyed and re-created to recover from
		// the error.  The callback arguments report which buffer was being
		// processed when the error occurred, and its HRESULT code.
		STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error)
		{
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
		wi::vector<uint8_t> filedata;
		bool success = wi::helper::FileRead(filename, filedata);
		if (!success)
		{
			return false;
		}
		return CreateSound(filedata.data(), filedata.size(), sound);
	}
	bool CreateSound(const uint8_t* data, size_t size, Sound* sound)
	{
		std::shared_ptr<SoundInternal> soundinternal = std::make_shared<SoundInternal>();
		soundinternal->audio = audio_internal;
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

			size_t output_size = size_t(samples * channels) * sizeof(short);
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

		instanceinternal->audio = audio_internal;
		instanceinternal->soundinternal = soundinternal;

		XAUDIO2_SEND_DESCRIPTOR SFXSend[] = { 
			{ XAUDIO2_SEND_USEFILTER, instanceinternal->audio->submixVoices[instance->type] },
			{ XAUDIO2_SEND_USEFILTER, instanceinternal->audio->reverbSubmix }, // this should be last to enable/disable reverb simply
		};
		XAUDIO2_VOICE_SENDS SFXSendList = { 
			(instance->IsEnableReverb() && instanceinternal->audio->reverbSubmix != nullptr) ? (uint32_t)arraysize(SFXSend) : 1,
			SFXSend 
		};

		hr = instanceinternal->audio->audioEngine->CreateSourceVoice(&instanceinternal->sourceVoice, &soundinternal->wfx,
			0, XAUDIO2_DEFAULT_FREQ_RATIO, instanceinternal.get(), &SFXSendList, NULL);
		if (FAILED(hr))
		{
			assert(0);
			return false;
		}

		instanceinternal->sourceVoice->GetVoiceDetails(&instanceinternal->voiceDetails);
		
		instanceinternal->outputMatrix.resize(size_t(instanceinternal->voiceDetails.InputChannels) * size_t(instanceinternal->audio->masteringVoiceDetails.InputChannels));
		instanceinternal->channelAzimuths.resize(instanceinternal->voiceDetails.InputChannels);
		for (size_t i = 0; i < instanceinternal->channelAzimuths.size(); ++i)
		{
			instanceinternal->channelAzimuths[i] = X3DAUDIO_2PI * float(i) / float(instanceinternal->channelAzimuths.size());
		}

		const uint32_t bytes_per_second = soundinternal->wfx.nSamplesPerSec * soundinternal->wfx.nChannels * sizeof(short);
		instanceinternal->buffer.pAudioData = soundinternal->audioData.data();
		instanceinternal->buffer.AudioBytes = (uint32_t)soundinternal->audioData.size();
		if (instance->begin > 0)
		{
			const uint32_t bytes_from_beginning = AlignTo(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->begin * bytes_per_second)), 4u);
			instanceinternal->buffer.pAudioData += bytes_from_beginning;
			instanceinternal->buffer.AudioBytes -= bytes_from_beginning;
		}
		if (instance->length > 0)
		{
			instanceinternal->buffer.AudioBytes = AlignTo(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->length * bytes_per_second)), 4u);
		}

		uint32_t num_remaining_samples = instanceinternal->buffer.AudioBytes / (soundinternal->wfx.nChannels * sizeof(short));
		if (instance->loop_begin > 0)
		{
			instanceinternal->buffer.LoopBegin = AlignTo(std::min(num_remaining_samples, uint32_t(instance->loop_begin * soundinternal->wfx.nSamplesPerSec)), 4u);
			num_remaining_samples -= instanceinternal->buffer.LoopBegin;
		}
		instanceinternal->buffer.LoopLength = AlignTo(std::min(num_remaining_samples, uint32_t(instance->loop_length * soundinternal->wfx.nSamplesPerSec)), 4u);

		instanceinternal->buffer.Flags = XAUDIO2_END_OF_STREAM;
		instanceinternal->buffer.LoopCount = instance->IsLooped() ? XAUDIO2_LOOP_INFINITE : 0;

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
			if (!instanceinternal->ended) // if already ended, don't submit end again, it can cause high pitched jerky sound
			{
				hr = instanceinternal->sourceVoice->SubmitSourceBuffer(&audio_internal->termination_mark); // mark this as terminated, this resets XAUDIO2_VOICE_STATE::SamplesPlayed to zero
				assert(SUCCEEDED(hr));
			}
			hr = instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer); // resubmit
			assert(SUCCEEDED(hr));
		}
	}
	void SetVolume(float volume, SoundInstance* instance)
	{
		if (instance == nullptr || !instance->IsValid())
		{
			HRESULT hr = audio_internal->masteringVoice->SetVolume(volume);
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
			audio_internal->masteringVoice->GetVolume(&volume);
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
			if (instanceinternal->buffer.LoopCount == 0)
				return;
			HRESULT hr = instanceinternal->sourceVoice->ExitLoop();
			assert(SUCCEEDED(hr));
			if (instanceinternal->ended)
			{
				instanceinternal->buffer.LoopCount = 0;
				hr = instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer);
				assert(SUCCEEDED(hr));
			}
		}
	}
	bool IsEnded(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			return instanceinternal->ended;
		}
		return false;
	}

	SampleInfo GetSampleInfo(const Sound* sound)
	{
		SampleInfo info = {};
		if (sound != nullptr && sound->IsValid())
		{
			auto soundinternal = to_internal(sound);
			info.channel_count = soundinternal->wfx.nChannels;
			info.samples = (const short*)soundinternal->audioData.data();
			info.sample_count = soundinternal->audioData.size() / (info.channel_count * sizeof(short));
			info.sample_rate = soundinternal->wfx.nSamplesPerSec;
		}
		return info;
	}
	uint64_t GetTotalSamplesPlayed(const SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			XAUDIO2_VOICE_STATE state = {};
			instanceinternal->sourceVoice->GetState(&state, 0);
			return state.SamplesPlayed;
		}
		return 0ull;
	}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume)
	{
		HRESULT hr = audio_internal->submixVoices[type]->SetVolume(volume);
		assert(SUCCEEDED(hr));
	}
	float GetSubmixVolume(SUBMIX_TYPE type)
	{
		float volume;
		audio_internal->submixVoices[type]->GetVolume(&volume);
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
			settings.DstChannelCount = instanceinternal->audio->masteringVoiceDetails.InputChannels;
			settings.pMatrixCoefficients = instanceinternal->outputMatrix.data();

			X3DAudioCalculate(instanceinternal->audio->audio3D, &listener, &emitter, flags, &settings);

			HRESULT hr;

			hr = instanceinternal->sourceVoice->SetFrequencyRatio(settings.DopplerFactor);
			assert(SUCCEEDED(hr));

			hr = instanceinternal->sourceVoice->SetOutputMatrix(
				instanceinternal->audio->submixVoices[instance->type],
				settings.SrcChannelCount, 
				settings.DstChannelCount, 
				settings.pMatrixCoefficients
			);
			assert(SUCCEEDED(hr));
			
			XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFDirectCoefficient), 1.0f };
			hr = instanceinternal->sourceVoice->SetOutputFilterParameters(instanceinternal->audio->submixVoices[instance->type], &FilterParametersDirect);
			assert(SUCCEEDED(hr));

			if (instance->IsEnableReverb() && instanceinternal->audio->reverbSubmix != nullptr)
			{
				hr = instanceinternal->sourceVoice->SetOutputMatrix(instanceinternal->audio->reverbSubmix, settings.SrcChannelCount, 1, &settings.ReverbLevel);
				assert(SUCCEEDED(hr));
				XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFReverbCoefficient), 1.0f };
				hr = instanceinternal->sourceVoice->SetOutputFilterParameters(instanceinternal->audio->reverbSubmix, &FilterParametersReverb);
				assert(SUCCEEDED(hr));
			}
		}
	}

	void SetReverb(REVERB_PRESET preset)
	{
		XAUDIO2FX_REVERB_PARAMETERS native;
		ReverbConvertI3DL2ToNative(&reverbPresets[preset], &native);
		HRESULT hr = audio_internal->reverbSubmix->SetEffectParameters(0, &native, sizeof(native));
		assert(SUCCEEDED(hr));
	}
}

#elif SDL2

//FAudio implemetation
#include <FAudio.h>
#include <FAPO.h>
#include <FAudioFX.h>
#include <F3DAudio.h>

#define SPEED_OF_SOUND 343.5f
#define fourccRIFF 0x46464952
#define fourccWAVE 0x45564157
#define fourccFMT 0x20746d66
#define fourccDATA 0x61746164

namespace wi::audio
{
	static const FAudioFXReverbI3DL2Parameters reverbPresets[] = {
		FAUDIOFX_I3DL2_PRESET_DEFAULT,
		FAUDIOFX_I3DL2_PRESET_GENERIC,
		FAUDIOFX_I3DL2_PRESET_FOREST,
		FAUDIOFX_I3DL2_PRESET_PADDEDCELL,
		FAUDIOFX_I3DL2_PRESET_ROOM,
		FAUDIOFX_I3DL2_PRESET_BATHROOM,
		FAUDIOFX_I3DL2_PRESET_LIVINGROOM,
		FAUDIOFX_I3DL2_PRESET_STONEROOM,
		FAUDIOFX_I3DL2_PRESET_AUDITORIUM,
		FAUDIOFX_I3DL2_PRESET_CONCERTHALL,
		FAUDIOFX_I3DL2_PRESET_CAVE,
		FAUDIOFX_I3DL2_PRESET_ARENA,
		FAUDIOFX_I3DL2_PRESET_HANGAR,
		FAUDIOFX_I3DL2_PRESET_CARPETEDHALLWAY,
		FAUDIOFX_I3DL2_PRESET_HALLWAY,
		FAUDIOFX_I3DL2_PRESET_STONECORRIDOR,
		FAUDIOFX_I3DL2_PRESET_ALLEY,
		FAUDIOFX_I3DL2_PRESET_CITY,
		FAUDIOFX_I3DL2_PRESET_MOUNTAINS,
		FAUDIOFX_I3DL2_PRESET_QUARRY,
		FAUDIOFX_I3DL2_PRESET_PLAIN,
		FAUDIOFX_I3DL2_PRESET_PARKINGLOT,
		FAUDIOFX_I3DL2_PRESET_SEWERPIPE,
		FAUDIOFX_I3DL2_PRESET_UNDERWATER,
		FAUDIOFX_I3DL2_PRESET_SMALLROOM,
		FAUDIOFX_I3DL2_PRESET_MEDIUMROOM,
		FAUDIOFX_I3DL2_PRESET_LARGEROOM,
		FAUDIOFX_I3DL2_PRESET_MEDIUMHALL,
		FAUDIOFX_I3DL2_PRESET_LARGEHALL,
		FAUDIOFX_I3DL2_PRESET_PLATE,
	};

	struct AudioInternal{
		bool success = false;
		FAudio *audioEngine;
		FAudioMasteringVoice* masteringVoice = nullptr;
		FAudioVoiceDetails masteringVoiceDetails;
		FAudioSubmixVoice* submixVoices[SUBMIX_TYPE_COUNT] = {};
		F3DAUDIO_HANDLE audio3D = {};
		FAPO* reverbEffect;
		FAudioSubmixVoice* reverbSubmix = nullptr;
		uint32_t termination_data = 0;
		FAudioBuffer termination_mark = {};

		AudioInternal(){
			wi::Timer timer;

			uint32_t res;
			res = FAudioCreate(&audioEngine, 0, FAUDIO_DEFAULT_PROCESSOR);
			assert(res == 0);

			res = FAudio_CreateMasteringVoice(
				audioEngine, 
				&masteringVoice, 
				FAUDIO_DEFAULT_CHANNELS, 
				FAUDIO_DEFAULT_SAMPLERATE, 
				0, 0, NULL);
			assert(res == 0);
		
			FAudioVoice_GetVoiceDetails(masteringVoice, &masteringVoiceDetails);

			for (int i=0; i<SUBMIX_TYPE_COUNT; ++i){
				res = FAudio_CreateSubmixVoice(
					audioEngine, 
					&submixVoices[i], 
					masteringVoiceDetails.InputChannels, 
					masteringVoiceDetails.InputSampleRate, 
					0, 0, NULL, NULL);
				assert(res == 0);
			}

			uint32_t channelMask;
			FAudioMasteringVoice_GetChannelMask(masteringVoice, &channelMask);

			F3DAudioInitialize(channelMask, SPEED_OF_SOUND, audio3D);
			success = (res == 0);

			// Reverb setup
			{
				res = FAudioCreateReverb(&reverbEffect, 0);
				success = (res == 0);

				FAudioEffectDescriptor effects[] = { { reverbEffect, 1, 1 } };
				FAudioEffectChain effectChain = { arraysize(effects), effects };
				
				res = FAudio_CreateSubmixVoice(
					audioEngine, 
					&reverbSubmix, 
					1, 
					masteringVoiceDetails.InputSampleRate, 
					0, 
					0, 
					nullptr, 
					&effectChain);
			}

			termination_mark.Flags = FAUDIO_END_OF_STREAM;
			termination_mark.pAudioData = (const uint8_t*)&termination_data;
			termination_mark.AudioBytes = sizeof(termination_data);

			if (success)
			{
				wi::backlog::post("wi::audio Initialized [FAudio] (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
			}
		}
		~AudioInternal(){
			if(reverbSubmix != nullptr)
				FAudioVoice_DestroyVoice(reverbSubmix);

			for (int i = 0; i < SUBMIX_TYPE_COUNT; ++i){
				if(submixVoices[i] != nullptr)
					FAudioVoice_DestroyVoice(submixVoices[i]);
			}

			if(masteringVoice != nullptr)
				FAudioVoice_DestroyVoice(masteringVoice);

			FAudio_StopEngine(audioEngine);
		}
	};
	static std::shared_ptr<AudioInternal> audio_internal;

	void Initialize()
	{
		audio_internal = std::make_shared<AudioInternal>();
	}

	struct SoundInternal{
		std::shared_ptr<AudioInternal> audio;
		FAudioWaveFormatEx wfx = {};
		wi::vector<uint8_t> audioData;
	};
	struct SoundInstanceInternal{
		std::shared_ptr<AudioInternal> audio;
		std::shared_ptr<SoundInternal> soundinternal;
		FAudioSourceVoice* sourceVoice = nullptr;
		FAudioVoiceDetails voiceDetails = {};
		wi::vector<float> outputMatrix;
		wi::vector<float> channelAzimuths;
		FAudioBuffer buffer = {};
		bool ended = true;

		~SoundInstanceInternal(){
			FAudioSourceVoice_Stop(sourceVoice, 0, FAUDIO_COMMIT_NOW);
			FAudioVoice_DestroyVoice(sourceVoice);
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

	bool FindChunk(const uint8_t* data, uint32_t fourcc, uint32_t& dwChunkSize, uint32_t& dwChunkDataPosition)
	{
		size_t pos = 0;

		uint32_t dwChunkType;
		uint32_t dwChunkDataSize;
		uint32_t dwRIFFDataSize = 0;
		uint32_t dwFileType;
		uint32_t bytesRead = 0;
		uint32_t dwOffset = 0;

		while(true)
		{
			memcpy(&dwChunkType, data + pos, sizeof(uint32_t));
			pos += sizeof(uint32_t);
			memcpy(&dwChunkDataSize, data + pos, sizeof(uint32_t));
			pos += sizeof(uint32_t);

			switch (dwChunkType)
			{
			case fourccRIFF: //TODO
				dwRIFFDataSize = dwChunkDataSize;
				dwChunkDataSize = 4;
				memcpy(&dwFileType, data + pos, sizeof(uint32_t));
				pos += sizeof(uint32_t);
				break;

			default:
				pos += dwChunkDataSize;
			}

			dwOffset += sizeof(uint32_t) * 2;

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

	bool CreateSound(const std::string& filename, Sound* sound) { 
		wi::vector<uint8_t> filedata;
		bool success = wi::helper::FileRead(filename, filedata);
		if (!success)
		{
			return false;
		}
		return CreateSound(filedata.data(), filedata.size(), sound);
	}
	bool CreateSound(const uint8_t* data, size_t size, Sound* sound) {
		std::shared_ptr<SoundInternal> soundinternal = std::make_shared<SoundInternal>();
		soundinternal->audio = audio_internal;
		sound->internal_state = soundinternal;

		uint32_t dwChunkSize;
		uint32_t dwChunkPosition;

		bool success;

		success = FindChunk(data, fourccRIFF, dwChunkSize, dwChunkPosition);
		if (success)
		{
			// Wav decoder:
			uint32_t filetype;
			memcpy(&filetype, data + dwChunkPosition, sizeof(uint32_t));
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
			soundinternal->wfx.wFormatTag = FAUDIO_FORMAT_PCM;

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
			soundinternal->wfx.wFormatTag = FAUDIO_FORMAT_PCM;
			soundinternal->wfx.nChannels = (uint16_t)channels;
			soundinternal->wfx.nSamplesPerSec = (uint32_t)sample_rate;
			soundinternal->wfx.wBitsPerSample = sizeof(short) * 8;
			soundinternal->wfx.nBlockAlign = (uint16_t)channels * sizeof(short); // is this right?
			soundinternal->wfx.nAvgBytesPerSec = soundinternal->wfx.nSamplesPerSec * soundinternal->wfx.nBlockAlign;

			size_t output_size = size_t(samples * channels) * sizeof(short);
			soundinternal->audioData.resize(output_size);
			memcpy(soundinternal->audioData.data(), output, output_size);

			free(output);
		}

		return true;
	}
	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance) { 
		uint32_t res;
		const auto& soundinternal = std::static_pointer_cast<SoundInternal>(sound->internal_state);
		std::shared_ptr<SoundInstanceInternal> instanceinternal = std::make_shared<SoundInstanceInternal>();
		instance->internal_state = instanceinternal;

		instanceinternal->audio = audio_internal;
		instanceinternal->soundinternal = soundinternal;

		FAudioSendDescriptor SFXSend[] = {
			{ FAUDIO_SEND_USEFILTER, instanceinternal->audio->submixVoices[instance->type] },
			{ FAUDIO_SEND_USEFILTER, instanceinternal->audio->reverbSubmix }, // this should be last to enable/disable reverb simply
		};

		FAudioVoiceSends SFXSendList = {
			instance->IsEnableReverb() ? (uint32_t)arraysize(SFXSend) : 1,
			SFXSend
		};
		
		res = FAudio_CreateSourceVoice(instanceinternal->audio->audioEngine, &instanceinternal->sourceVoice, &soundinternal->wfx,
			0, FAUDIO_DEFAULT_FREQ_RATIO, NULL, &SFXSendList, NULL);
		if(res != 0){
			assert(0);
			return false;
		}

		FAudioVoice_GetVoiceDetails(instanceinternal->sourceVoice, &instanceinternal->voiceDetails);
		instanceinternal->outputMatrix.resize(size_t(instanceinternal->voiceDetails.InputChannels) * size_t(instanceinternal->audio->masteringVoiceDetails.InputChannels));
		instanceinternal->channelAzimuths.resize(instanceinternal->voiceDetails.InputChannels);
		for (size_t i = 0; i < instanceinternal->channelAzimuths.size(); ++i)
		{
			instanceinternal->channelAzimuths[i] = F3DAUDIO_2PI * float(i) / float(instanceinternal->channelAzimuths.size());
		}

		const uint32_t bytes_per_second = soundinternal->wfx.nSamplesPerSec * soundinternal->wfx.nChannels * sizeof(short);
		instanceinternal->buffer.pAudioData = soundinternal->audioData.data();
		instanceinternal->buffer.AudioBytes = (uint32_t)soundinternal->audioData.size();
		if (instance->begin > 0)
		{
			const uint32_t bytes_from_beginning = AlignTo(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->begin * bytes_per_second)), 4u);
			instanceinternal->buffer.pAudioData += bytes_from_beginning;
			instanceinternal->buffer.AudioBytes -= bytes_from_beginning;
		}
		if (instance->length > 0)
		{
			instanceinternal->buffer.AudioBytes = AlignTo(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->length * bytes_per_second)), 4u);
		}

		uint32_t num_remaining_samples = instanceinternal->buffer.AudioBytes / (soundinternal->wfx.nChannels * sizeof(short));
		if (instance->loop_begin > 0)
		{
			instanceinternal->buffer.LoopBegin = AlignTo(std::min(num_remaining_samples, uint32_t(instance->loop_begin * soundinternal->wfx.nSamplesPerSec)), 4u);
			num_remaining_samples -= instanceinternal->buffer.LoopBegin;
		}
		instanceinternal->buffer.LoopLength = AlignTo(std::min(num_remaining_samples, uint32_t(instance->loop_length * soundinternal->wfx.nSamplesPerSec)), 4u);

		instanceinternal->buffer.Flags = FAUDIO_END_OF_STREAM;
		instanceinternal->buffer.LoopCount = instance->IsLooped() ? FAUDIO_LOOP_INFINITE : 0;

		res = FAudioSourceVoice_SubmitSourceBuffer(instanceinternal->sourceVoice, &(instanceinternal->buffer), nullptr);
		if(res != 0){
			assert(0);
			return false;
		}

		return true;
	}

	void Play(SoundInstance* instance) {
		if (instance != nullptr && instance->IsValid()){
			auto instanceinternal = to_internal(instance);
			uint32_t res = FAudioSourceVoice_Start(instanceinternal->sourceVoice, 0, FAUDIO_COMMIT_NOW);
			assert(res == 0);
		}
	}
	void Pause(SoundInstance* instance) {
		if (instance != nullptr && instance->IsValid()){
			auto instanceinternal = to_internal(instance);
			uint32_t res = FAudioSourceVoice_Stop(instanceinternal->sourceVoice, 0, FAUDIO_COMMIT_NOW); // preserves cursor position
			assert(res == 0);
		}
	}
	void Stop(SoundInstance* instance) {
		if (instance != nullptr && instance->IsValid()){
			auto instanceinternal = to_internal(instance);
			uint32_t res = FAudioSourceVoice_Stop(instanceinternal->sourceVoice, 0, FAUDIO_COMMIT_NOW); // preserves cursor position
			assert(res == 0);
			res = FAudioSourceVoice_FlushSourceBuffers(instanceinternal->sourceVoice); // reset submitted audio buffer
			assert(res == 0);
			res = FAudioSourceVoice_SubmitSourceBuffer(instanceinternal->sourceVoice, &audio_internal->termination_mark, nullptr); // mark this as terminated, this resets XAUDIO2_VOICE_STATE::SamplesPlayed to zero
			assert(res == 0);
			res = FAudioSourceVoice_SubmitSourceBuffer(instanceinternal->sourceVoice, &(instanceinternal->buffer), nullptr);
			assert(res == 0);
		}
	}
	void SetVolume(float volume, SoundInstance* instance) {
		if (instance == nullptr || !instance->IsValid()){
			uint32_t res = FAudioVoice_SetVolume(audio_internal->masteringVoice, volume, FAUDIO_COMMIT_NOW);
			assert(res == 0);
		}
		else {
			auto instanceinternal = to_internal(instance);
			uint32_t res = FAudioVoice_SetVolume(instanceinternal->sourceVoice, volume, FAUDIO_COMMIT_NOW);
			assert(res == 0);
		}
	}
	float GetVolume(const SoundInstance* instance) {
		float volume = 0;
		if (instance == nullptr || !instance->IsValid()){
			FAudioVoice_GetVolume(audio_internal->masteringVoice, &volume);
		}
		else {
			auto instanceinternal = to_internal(instance);
			FAudioVoice_GetVolume(instanceinternal->sourceVoice, &volume);
		}
		return volume;
	}
	void ExitLoop(SoundInstance* instance) {
		if (instance != nullptr && instance->IsValid()){
			auto instanceinternal = to_internal(instance);
			if (instanceinternal->buffer.LoopCount == 0)
				return;
			uint32_t res = FAudioSourceVoice_ExitLoop(instanceinternal->sourceVoice, FAUDIO_COMMIT_NOW);
			assert(res == 0);
			if (instanceinternal->ended)
			{
				instanceinternal->buffer.LoopCount = 0;
				res = FAudioSourceVoice_SubmitSourceBuffer(instanceinternal->sourceVoice, &(instanceinternal->buffer), nullptr);
				assert(res == 0);
			}
		}
	}
	bool IsEnded(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			return instanceinternal->ended;
		}
		return false;
	}

	SampleInfo GetSampleInfo(const Sound* sound)
	{
		SampleInfo info = {};
		if (sound != nullptr && sound->IsValid())
		{
			auto soundinternal = to_internal(sound);
			info.samples = (const short*)soundinternal->audioData.data();
			info.sample_count = soundinternal->audioData.size() / sizeof(short);
			info.sample_rate = soundinternal->wfx.nSamplesPerSec;
			info.channel_count = soundinternal->wfx.nChannels;
		}
		return info;
	}
	uint64_t GetTotalSamplesPlayed(const SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			FAudioVoiceState state = {};
			FAudioSourceVoice_GetState(instanceinternal->sourceVoice, &state, 0);
			return state.SamplesPlayed;
		}
		return 0ull;
	}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume) {
		uint32_t res = FAudioVoice_SetVolume(audio_internal->submixVoices[type], volume, FAUDIO_COMMIT_NOW);
		assert(res == 0);
	}
	float GetSubmixVolume(SUBMIX_TYPE type) { 
		float volume;
		FAudioVoice_GetVolume(audio_internal->submixVoices[type], &volume);
		return volume; 
	}

	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D) {
		if (instance != nullptr && instance->IsValid()){
			auto instanceinternal = to_internal(instance);
			F3DAUDIO_LISTENER listener = {};
			listener.Position = (F3DAUDIO_VECTOR){ instance3D.listenerPos.x, instance3D.listenerPos.y, instance3D.listenerPos.z };
			listener.OrientFront = (F3DAUDIO_VECTOR){ instance3D.listenerFront.x, instance3D.listenerFront.y, instance3D.listenerFront.z };
			listener.OrientTop = (F3DAUDIO_VECTOR){ instance3D.listenerUp.x, instance3D.listenerUp.y, instance3D.listenerUp.z };
			listener.Velocity = (F3DAUDIO_VECTOR){ instance3D.listenerVelocity.x, instance3D.listenerVelocity.y, instance3D.listenerVelocity.z }; 

			F3DAUDIO_EMITTER emitter = {};
			emitter.Position = (F3DAUDIO_VECTOR){ instance3D.emitterPos.x, instance3D.emitterPos.y, instance3D.emitterPos.z };
			emitter.OrientFront = (F3DAUDIO_VECTOR){ instance3D.emitterFront.x, instance3D.emitterFront.y, instance3D.emitterFront.z };
			emitter.OrientTop = (F3DAUDIO_VECTOR){ instance3D.emitterUp.x, instance3D.emitterUp.y, instance3D.emitterUp.z };
			emitter.Velocity = (F3DAUDIO_VECTOR){ instance3D.emitterVelocity.x, instance3D.emitterVelocity.y, instance3D.emitterVelocity.z }; 
			emitter.InnerRadius = instance3D.emitterRadius;
			emitter.InnerRadiusAngle = F3DAUDIO_PI / 4.0f;
			emitter.ChannelCount = instanceinternal->voiceDetails.InputChannels;
			emitter.pChannelAzimuths = instanceinternal->channelAzimuths.data();
			emitter.ChannelRadius = 0.1f;
			emitter.CurveDistanceScaler = 1;
			emitter.DopplerScaler = 1;

			uint32_t flags = 0;
			flags |= F3DAUDIO_CALCULATE_MATRIX;
			flags |= F3DAUDIO_CALCULATE_LPF_DIRECT;
			flags |= F3DAUDIO_CALCULATE_REVERB;
			flags |= F3DAUDIO_CALCULATE_LPF_REVERB;
			flags |= F3DAUDIO_CALCULATE_DOPPLER;
			// flags |= F3DAUDIO_CALCULATE_DELAY;
			// flags |= F3DAUDIO_CALCULATE_EMITTER_ANGLE;
			// flags |= F3DAUDIO_CALCULATE_ZEROCENTER;
			// flags |= F3DAUDIO_CALCULATE_REDIRECT_TO_LFE;

			F3DAUDIO_DSP_SETTINGS settings = {};
			settings.SrcChannelCount = instanceinternal->voiceDetails.InputChannels;
			settings.DstChannelCount = instanceinternal->audio->masteringVoiceDetails.InputChannels;
			settings.pMatrixCoefficients = instanceinternal->outputMatrix.data();

			F3DAudioCalculate(instanceinternal->audio->audio3D, &listener, &emitter, flags, &settings);

			uint32_t res;

			res = FAudioSourceVoice_SetFrequencyRatio(instanceinternal->sourceVoice, settings.DopplerFactor, FAUDIO_COMMIT_NOW);
			assert(res == 0);

			res = FAudioVoice_SetOutputMatrix(
				instanceinternal->sourceVoice, 
				instanceinternal->audio->submixVoices[instance->type],
				settings.SrcChannelCount, 
				settings.DstChannelCount, 
				settings.pMatrixCoefficients, 
				FAUDIO_COMMIT_NOW);
			assert(res == 0);

			FAudioFilterParameters FilterParametersDirect = { FAudioLowPassFilter, 2.0f * sinf(F3DAUDIO_PI / 6.0f * settings.LPFDirectCoefficient), 1.0f };
			res = FAudioVoice_SetOutputFilterParameters(instanceinternal->sourceVoice, instanceinternal->audio->submixVoices[instance->type], &FilterParametersDirect, FAUDIO_COMMIT_NOW);
			assert(res == 0);

			if(instance->IsEnableReverb()){
				res = FAudioVoice_SetOutputMatrix(instanceinternal->sourceVoice, instanceinternal->audio->reverbSubmix, settings.SrcChannelCount, 1, &settings.ReverbLevel, FAUDIO_COMMIT_NOW);
				assert(res == 0);
				FAudioFilterParameters FilterParametersReverb = { FAudioLowPassFilter, 2.0f * sinf(F3DAUDIO_PI / 6.0f * settings.LPFReverbCoefficient), 1.0f };
				res = FAudioVoice_SetOutputFilterParameters(instanceinternal->sourceVoice, instanceinternal->audio->reverbSubmix, &FilterParametersReverb, FAUDIO_COMMIT_NOW);
				assert(res == 0);
			}
		}
	}

	void SetReverb(REVERB_PRESET preset) {
		FAudioFXReverbParameters native;
		ReverbConvertI3DL2ToNative(&reverbPresets[preset], &native);
		uint32_t res = FAudioVoice_SetEffectParameters(audio_internal->reverbSubmix, 0, &native, sizeof(native), FAUDIO_COMMIT_NOW);
		assert(res == 0);
	}
}

#elif __SCE__
// PS5 audio implementation in wiAudio_PS5.cpp extension file
#else

namespace wi::audio
{
	void Initialize() {}

	bool CreateSound(const std::string& filename, Sound* sound) { return false; }
	bool CreateSound(const uint8_t* data, size_t size, Sound* sound) { return false; }
	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance) { return false; }

	void Play(SoundInstance* instance) {}
	void Pause(SoundInstance* instance) {}
	void Stop(SoundInstance* instance) {}
	void SetVolume(float volume, SoundInstance* instance) {}
	float GetVolume(const SoundInstance* instance) { return 0; }
	void ExitLoop(SoundInstance* instance) {}
	bool IsEnded(SoundInstance* instance) { return true; }

	SampleInfo GetSampleInfo(const Sound* sound) { return {}; }
	uint64_t GetTotalSamplesPlayed(const SoundInstance* instance) { return 0; }

	void SetSubmixVolume(SUBMIX_TYPE type, float volume) {}
	float GetSubmixVolume(SUBMIX_TYPE type) { return 0; }

	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D) {}

	void SetReverb(REVERB_PRESET preset) {}
}

#endif // _WIN32
