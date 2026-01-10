#include "wiAudio.h"
#include "wiBacklog.h"
#include "wiHelper.h"
#include "wiTimer.h"
#include "wiVector.h"

#define STB_VORBIS_HEADER_ONLY
#include "Utility/stb_vorbis.c"

#include <sstream>

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

#define xaudio_assert(cond, fname) { wilog_assert(cond, "XAudio2 error: %s failed with %s (%s:%d)", fname, wi::helper::GetPlatformErrorString(hr).c_str(), relative_path(__FILE__), __LINE__); }
#define xaudio_check(call) [&]() { HRESULT hr = call; xaudio_assert(SUCCEEDED(hr), extract_function_name(#call).c_str()); return hr; }()

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
			hr = xaudio_check(CoInitializeEx(NULL, COINIT_MULTITHREADED));
			if (!SUCCEEDED(hr))
			{
				return;
			}

			hr = xaudio_check(XAudio2Create(&audioEngine, 0, XAUDIO2_USE_DEFAULT_PROCESSOR));
			if (!SUCCEEDED(hr))
			{
				return;
			}

#ifdef _DEBUG
			XAUDIO2_DEBUG_CONFIGURATION debugConfig = {};
			debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
			debugConfig.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
			audioEngine->SetDebugConfiguration(&debugConfig);
#endif // _DEBUG

			hr = xaudio_check(audioEngine->CreateMasteringVoice(&masteringVoice));
			if (!SUCCEEDED(hr))
			{
				return;
			}

			masteringVoice->GetVoiceDetails(&masteringVoiceDetails);

			// Without clamping sample rate, it was crashing 32bit 192kHz audio devices
			if (masteringVoiceDetails.InputSampleRate > 48000)
				masteringVoiceDetails.InputSampleRate = 48000;

			for (int i = 0; i < SUBMIX_TYPE_COUNT; ++i)
			{
				hr = xaudio_check(audioEngine->CreateSubmixVoice(
					&submixVoices[i],
					masteringVoiceDetails.InputChannels,
					masteringVoiceDetails.InputSampleRate,
					0,
					0,
					0,
					0
				));

				if (!SUCCEEDED(hr))
				{
					return;
				}
			}

			DWORD channelMask;
			masteringVoice->GetChannelMask(&channelMask);
			hr = xaudio_check(X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, audio3D));
			if (!SUCCEEDED(hr))
			{
				return;
			}

			// Reverb setup:
			{
				hr = xaudio_check(XAudio2CreateReverb(&reverbEffect));
				if (!SUCCEEDED(hr))
				{
					return;
				}

				XAUDIO2_EFFECT_DESCRIPTOR effects[] = { { reverbEffect.Get(), TRUE, 1 } };
				XAUDIO2_EFFECT_CHAIN effectChain = { arraysize(effects), effects };
				hr = xaudio_check(audioEngine->CreateSubmixVoice(
					&reverbSubmix,
					1, // reverb is mono
					masteringVoiceDetails.InputSampleRate,
					0,
					0,
					nullptr,
					&effectChain
				));
				if (!SUCCEEDED(hr))
				{
					return;
				}

				XAUDIO2FX_REVERB_PARAMETERS native;
				ReverbConvertI3DL2ToNative(&reverbPresets[REVERB_PRESET_DEFAULT], &native);
				HRESULT hr = xaudio_check(reverbSubmix->SetEffectParameters(0, &native, sizeof(native)));
				if (!SUCCEEDED(hr))
				{
					return;
				}
			}

			termination_mark.Flags = XAUDIO2_END_OF_STREAM;
			termination_mark.pAudioData = (const BYTE*)&termination_data;
			termination_mark.AudioBytes = sizeof(termination_data);

			success = true;
			wilog("wi::audio Initialized [XAudio2] (%d ms)", (int)std::round(timer.elapsed()));
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

		constexpr bool IsValid() const { return success; }
	};
	static wi::allocator::shared_ptr<AudioInternal> audio_internal;

	void Initialize()
	{
		audio_internal = wi::allocator::make_shared_single<AudioInternal>();
	}

	struct SoundInternal
	{
		wi::allocator::shared_ptr<AudioInternal> audio;
		WAVEFORMATEX wfx = {};
		wi::vector<uint8_t> audioData;
	};
	struct SoundInstanceInternal final : public IXAudio2VoiceCallback
	{
		wi::allocator::shared_ptr<AudioInternal> audio;
		wi::allocator::shared_ptr<SoundInternal> soundinternal;
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
		if (audio_internal == nullptr || !audio_internal->IsValid())
			return false;
		auto soundinternal = wi::allocator::make_shared<SoundInternal>();
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
		if (audio_internal == nullptr || !audio_internal->IsValid())
			return false;
		if (sound == nullptr || !sound->IsValid())
			return false;
		HRESULT hr;
		auto soundinternal = wi::allocator::shared_ptr<SoundInternal>(sound->internal_state);
		auto instanceinternal = wi::allocator::make_shared<SoundInstanceInternal>();
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

		hr = xaudio_check(instanceinternal->audio->audioEngine->CreateSourceVoice(&instanceinternal->sourceVoice, &soundinternal->wfx,
			0, XAUDIO2_DEFAULT_FREQ_RATIO, instanceinternal.get(), &SFXSendList, NULL));

		if (FAILED(hr))
		{
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
			const uint32_t bytes_from_beginning = align(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->begin * bytes_per_second)), 4u);
			instanceinternal->buffer.pAudioData += bytes_from_beginning;
			instanceinternal->buffer.AudioBytes -= bytes_from_beginning;
		}
		if (instance->length > 0)
		{
			instanceinternal->buffer.AudioBytes = align(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->length * bytes_per_second)), 4u);
		}

		uint32_t num_remaining_samples = instanceinternal->buffer.AudioBytes / (soundinternal->wfx.nChannels * sizeof(short));
		if (instance->loop_begin > 0)
		{
			instanceinternal->buffer.LoopBegin = align(std::min(num_remaining_samples, uint32_t(instance->loop_begin * soundinternal->wfx.nSamplesPerSec)), 4u);
			num_remaining_samples -= instanceinternal->buffer.LoopBegin;
		}
		instanceinternal->buffer.LoopLength = align(std::min(num_remaining_samples, uint32_t(instance->loop_length * soundinternal->wfx.nSamplesPerSec)), 4u);

		instanceinternal->buffer.Flags = XAUDIO2_END_OF_STREAM;
		instanceinternal->buffer.LoopCount = instance->IsLooped() ? XAUDIO2_LOOP_INFINITE : 0;

		hr = xaudio_check(instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer));

		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}
	void Play(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			xaudio_check(instanceinternal->sourceVoice->Start());

		}
	}
	void Pause(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			xaudio_check(instanceinternal->sourceVoice->Stop()); // preserves cursor position

		}
	}
	void Stop(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			xaudio_check(instanceinternal->sourceVoice->Stop()); // preserves cursor position

			xaudio_check(instanceinternal->sourceVoice->FlushSourceBuffers()); // reset submitted audio buffer

			if (!instanceinternal->ended) // if already ended, don't submit end again, it can cause high pitched jerky sound
			{
				xaudio_check(instanceinternal->sourceVoice->SubmitSourceBuffer(&audio_internal->termination_mark)); // mark this as terminated, this resets XAUDIO2_VOICE_STATE::SamplesPlayed to zero

			}
			xaudio_check(instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer)); // resubmit

		}
	}
	void SetVolume(float volume, SoundInstance* instance)
	{
		if (instance == nullptr || !instance->IsValid())
		{
			if (audio_internal->masteringVoice != nullptr)
				xaudio_check(audio_internal->masteringVoice->SetVolume(volume));
		}
		else
		{
			auto instanceinternal = to_internal(instance);
			if (instanceinternal->sourceVoice != nullptr)
				xaudio_check(instanceinternal->sourceVoice->SetVolume(volume));
		}
	}
	float GetVolume(const SoundInstance* instance)
	{
		float volume = 0;
		if (instance == nullptr || !instance->IsValid())
		{
			if(audio_internal->masteringVoice != nullptr)
				audio_internal->masteringVoice->GetVolume(&volume);
		}
		else
		{
			auto instanceinternal = to_internal(instance);
			if(instanceinternal->sourceVoice != nullptr)
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
			xaudio_check(instanceinternal->sourceVoice->ExitLoop());

			if (instanceinternal->ended)
			{
				instanceinternal->buffer.LoopCount = 0;
				xaudio_check(instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer));
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
			if (instanceinternal->sourceVoice != nullptr)
				instanceinternal->sourceVoice->GetState(&state, 0);
			return state.SamplesPlayed;
		}
		return 0ull;
	}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume)
	{
		if(audio_internal->submixVoices[type] != nullptr)
			xaudio_check(audio_internal->submixVoices[type]->SetVolume(volume));
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

			xaudio_check(instanceinternal->sourceVoice->SetFrequencyRatio(settings.DopplerFactor));

			xaudio_check(instanceinternal->sourceVoice->SetOutputMatrix(
				instanceinternal->audio->submixVoices[instance->type],
				settings.SrcChannelCount,
				settings.DstChannelCount,
				settings.pMatrixCoefficients
			));

			
			XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFDirectCoefficient), 1.0f };
			xaudio_check(instanceinternal->sourceVoice->SetOutputFilterParameters(instanceinternal->audio->submixVoices[instance->type], &FilterParametersDirect));


			if (instance->IsEnableReverb() && instanceinternal->audio->reverbSubmix != nullptr)
			{
				xaudio_check(instanceinternal->sourceVoice->SetOutputMatrix(instanceinternal->audio->reverbSubmix, settings.SrcChannelCount, 1, &settings.ReverbLevel));

				XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFReverbCoefficient), 1.0f };
				xaudio_check(instanceinternal->sourceVoice->SetOutputFilterParameters(instanceinternal->audio->reverbSubmix, &FilterParametersReverb));

			}
		}
	}

	void SetReverb(REVERB_PRESET preset)
	{
		XAUDIO2FX_REVERB_PARAMETERS native;
		ReverbConvertI3DL2ToNative(&reverbPresets[preset], &native);
		xaudio_check(audio_internal->reverbSubmix->SetEffectParameters(0, &native, sizeof(native)));

	}
}

#elif defined(SDL2)

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
			if (res != 0)
			{
				std::stringstream ss("");
				ss << "FAudioCreate returned error: " << res;
				wi::backlog::post(ss.str(), wi::backlog::LogLevel::Error);
				return;
			}

			res = FAudio_CreateMasteringVoice(
				audioEngine, 
				&masteringVoice, 
				FAUDIO_DEFAULT_CHANNELS, 
				FAUDIO_DEFAULT_SAMPLERATE, 
				0, 0, NULL);
			if (res != 0)
			{
				std::stringstream ss("");
				ss << "FAudio_CreateMasteringVoice returned error: " << res;
				wi::backlog::post(ss.str(), wi::backlog::LogLevel::Error);
				return;
			}
		
			FAudioVoice_GetVoiceDetails(masteringVoice, &masteringVoiceDetails);

			for (int i=0; i<SUBMIX_TYPE_COUNT; ++i){
				res = FAudio_CreateSubmixVoice(
					audioEngine, 
					&submixVoices[i], 
					masteringVoiceDetails.InputChannels, 
					masteringVoiceDetails.InputSampleRate, 
					0, 0, NULL, NULL);
				if (res != 0)
				{
					std::stringstream ss("");
					ss << "FAudio_CreateSubmixVoice returned error: " << res;
					wi::backlog::post(ss.str(), wi::backlog::LogLevel::Error);
					return;
				}
			}

			uint32_t channelMask;
			FAudioMasteringVoice_GetChannelMask(masteringVoice, &channelMask);

			F3DAudioInitialize(channelMask, SPEED_OF_SOUND, audio3D);
			if (res != 0)
			{
				std::stringstream ss("");
				ss << "F3DAudioInitialize returned error: " << res;
				wi::backlog::post(ss.str(), wi::backlog::LogLevel::Error);
				return;
			}

			// Reverb setup
			{
				res = FAudioCreateReverb(&reverbEffect, 0);
				if (res != 0)
				{
					std::stringstream ss("");
					ss << "FAudioCreateReverb returned error: " << res;
					wi::backlog::post(ss.str(), wi::backlog::LogLevel::Error);
					return;
				}

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
					&effectChain
				);
				if (res != 0)
				{
					std::stringstream ss("");
					ss << "FAudio_CreateSubmixVoice returned error: " << res;
					wi::backlog::post(ss.str(), wi::backlog::LogLevel::Error);
					return;
				}
			}

			termination_mark.Flags = FAUDIO_END_OF_STREAM;
			termination_mark.pAudioData = (const uint8_t*)&termination_data;
			termination_mark.AudioBytes = sizeof(termination_data);

			success = true;
			wilog("wi::audio Initialized [FAudio %d.%d.%d] (%d ms)",
				  FAUDIO_MAJOR_VERSION, FAUDIO_MINOR_VERSION, FAUDIO_PATCH_VERSION,
				  (int)std::round(timer.elapsed()));
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
		constexpr bool IsValid() const { return success; }
	};
	static wi::allocator::shared_ptr<AudioInternal> audio_internal;

	void Initialize()
	{
		audio_internal = wi::allocator::make_shared_single<AudioInternal>();
	}

	struct SoundInternal{
		wi::allocator::shared_ptr<AudioInternal> audio;
		FAudioWaveFormatEx wfx = {};
		wi::vector<uint8_t> audioData;
	};
	struct SoundInstanceInternal{
		wi::allocator::shared_ptr<AudioInternal> audio;
		wi::allocator::shared_ptr<SoundInternal> soundinternal;
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
	bool CreateSound(const uint8_t* data, size_t size, Sound* sound)
	{
		if (audio_internal == nullptr || !audio_internal->IsValid())
			return false;
		auto soundinternal = wi::allocator::make_shared<SoundInternal>();
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
	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance)
	{
		if (audio_internal == nullptr || !audio_internal->IsValid())
			return false;
		if (sound == nullptr || !sound->IsValid())
			return false;
		uint32_t res;
		auto soundinternal = wi::allocator::shared_ptr<SoundInternal>(sound->internal_state);
		auto instanceinternal = wi::allocator::make_shared<SoundInstanceInternal>();
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
			const uint32_t bytes_from_beginning = align(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->begin * bytes_per_second)), 4u);
			instanceinternal->buffer.pAudioData += bytes_from_beginning;
			instanceinternal->buffer.AudioBytes -= bytes_from_beginning;
		}
		if (instance->length > 0)
		{
			instanceinternal->buffer.AudioBytes = align(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->length * bytes_per_second)), 4u);
		}

		uint32_t num_remaining_samples = instanceinternal->buffer.AudioBytes / (soundinternal->wfx.nChannels * sizeof(short));
		if (instance->loop_begin > 0)
		{
			instanceinternal->buffer.LoopBegin = align(std::min(num_remaining_samples, uint32_t(instance->loop_begin * soundinternal->wfx.nSamplesPerSec)), 4u);
			num_remaining_samples -= instanceinternal->buffer.LoopBegin;
		}
		instanceinternal->buffer.LoopLength = align(std::min(num_remaining_samples, uint32_t(instance->loop_length * soundinternal->wfx.nSamplesPerSec)), 4u);

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

#elif defined(__APPLE__)
#define MA_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_DR_FLAC_NO_NEON
#define MA_API static

#include "Utility/miniaudio.h"

namespace wi::audio
{
	struct WrappedEngine : ma_engine
	{
		WrappedEngine()
		{
			ma_result res = ma_engine_init(nullptr, this);
			wilog_assert(res == MA_SUCCESS, "MiniAudio engine failed to initialize");
		}

		~WrappedEngine()
		{
			ma_engine_uninit(this);
		}
	};

	struct WrappedSound : ma_sound
	{
		ma_audio_buffer* buffer = nullptr;
		bool playing = false;

		WrappedSound() {}

		// Just for testing
		WrappedSound(WrappedSound& other) = delete;

		ma_result Create(ma_engine* engine, const ma_audio_buffer_config& config)
		{
			buffer = new ma_audio_buffer();
			ma_audio_buffer_init(&config, buffer);
			ma_result res = ma_sound_init_from_data_source(engine, buffer, 0, nullptr, this);
			return res;
		}

		~WrappedSound()
		{
			ma_sound_uninit(this);
			if (buffer != nullptr) delete buffer;
		}
	};

	struct WrappedSampleInfo : SampleInfo
	{
		~WrappedSampleInfo()
		{
			delete samples;
		}
	};

	static wi::allocator::shared_ptr<WrappedEngine> engine;

	static WrappedSound* get_ma_sound(const SoundInstance* instance)
	{
		return instance == nullptr ? nullptr : (WrappedSound*)instance->internal_state.get();
	}

	void Initialize()
	{
		wi::Timer timer;
		engine = wi::allocator::make_shared_single<WrappedEngine>();
		wilog("wi::audio Initialized [miniaudio] (%d ms)", (int)std::round(timer.elapsed()));
	}

	static bool CreateSoundInternal(std::function<ma_result (ma_decoder_config* config, ma_uint64* frameCount, void** pcmFrames)> decoder, Sound* sound)
	{
		auto info = wi::allocator::make_shared<WrappedSampleInfo>();
		ma_decoder_config config{};
		info->channel_count = config.channels = 1;
		info->sample_rate = config.sampleRate = ma_engine_get_sample_rate(engine.get());
		config.format = ma_format_s16;
		ma_uint64 count;
		ma_result res = decoder(&config, &count, (void**)(&info->samples));
		info->sample_count = count;
		if (res == MA_SUCCESS)
		{
			sound->internal_state = info;
			info->sample_count *= info->channel_count;
		}
		assert(res == MA_SUCCESS);
		return res == MA_SUCCESS;

	}

	bool CreateSound(const std::string& filename, Sound* sound)
	{
		return CreateSoundInternal(
			[filename](auto config, auto frameCount, auto frames) {
				return ma_decode_file(filename.c_str(), config, frameCount, frames);
			},
			sound
		);
	}

	bool CreateSound(const uint8_t* data, size_t size, Sound* sound)
	{
		return CreateSoundInternal(
			[data, size](auto config, auto frameCount, auto frames) {
				return ma_decode_memory(data, size, config, frameCount, frames);
			},
			sound
		);
	}

	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance)
	{
		auto info = (WrappedSampleInfo*)sound->internal_state.get();
		auto wrapped_sound = wi::allocator::make_shared<WrappedSound>();

		ma_audio_buffer_config config{};
		config.channels = info->channel_count;
		config.format = ma_format_s16;
		config.sampleRate = info->sample_rate;
		config.sizeInFrames = info->sample_count / info->channel_count;
		config.pData = info->samples;
		ma_result res = wrapped_sound->Create(engine.get(), config);
		if (res == MA_SUCCESS)
		{
			instance->internal_state = wrapped_sound;
		}
		assert(res == MA_SUCCESS);
		return res == MA_SUCCESS;
	}

	void Play(SoundInstance* instance)
	{
		auto sound = get_ma_sound(instance);
		
		if (sound->playing) return;
		sound->playing = true;
		ma_sound_set_looping(sound, instance->IsLooped());
		if (instance->begin > 0.f)
		{
			ma_sound_seek_to_second(sound, instance->begin);
		}
		if (instance->length > 0.f)
		{
			ma_sound_set_stop_time_in_milliseconds(sound, ma_engine_get_time_in_milliseconds(engine.get()) + ma_uint64(instance->length * 1000.f));
		}
		else
		{
			ma_sound_set_stop_time_in_milliseconds(sound, UINT64_MAX);
		}

		ma_sound_start(sound);
	}

	void Pause(SoundInstance* instance)
	{
		ma_sound_stop(get_ma_sound(instance));
		get_ma_sound(instance)->playing = false;
	}

	void Stop(SoundInstance* instance)
	{
		ma_sound_stop(get_ma_sound(instance));
		ma_sound_seek_to_pcm_frame(get_ma_sound(instance), 0);
		get_ma_sound(instance)->playing = false;
	}

	void SetVolume(float volume, SoundInstance* instance)
	{
		// FIXME: temporary hack because Island level has background sounds set at volume 10
		// and XAudio doesn't seem to support amplifiying sounds
		ma_sound_set_volume(get_ma_sound(instance), std::min(1.f, volume));
	}

	float GetVolume(const SoundInstance* instance)
	{
		return ma_sound_get_volume(get_ma_sound(instance));
	}

	void ExitLoop(SoundInstance* instance)
	{

	}
	bool IsEnded(SoundInstance* instance)
	{
		// FIXME: Dirty hack for character_controller demo, this would be true for paused sounds as well,
		// which is incorrect.
		return !get_ma_sound(instance)->playing || ma_sound_at_end(get_ma_sound(instance));
	}

	SampleInfo GetSampleInfo(const Sound* sound)
	{
		return *(SampleInfo*)sound->internal_state.get();
	}

	uint64_t GetTotalSamplesPlayed(const SoundInstance* instance)
	{
		// FIXME: multiply by channels
		return ma_sound_get_time_in_pcm_frames(get_ma_sound(instance));
	}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume)
	{

	}

	float GetSubmixVolume(SUBMIX_TYPE type)
	{
		return 0.;
	}

	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D)
	{
		ma_sound_set_position(
			get_ma_sound(instance),
			instance3D.emitterPos.x,
			instance3D.emitterPos.y,
			-instance3D.emitterPos.z
		);
		ma_sound_set_direction(
			get_ma_sound(instance),
			instance3D.emitterFront.x,
			instance3D.emitterFront.y,
			-instance3D.emitterFront.z
		);
		// TODO: emitterUp
		ma_sound_set_velocity(
			get_ma_sound(instance),
			instance3D.emitterVelocity.x,
			instance3D.emitterVelocity.y,
			-instance3D.emitterVelocity.z
		);
		ma_sound_set_max_distance(get_ma_sound(instance), instance3D.emitterRadius);

		ma_engine_listener_set_position(
			engine.get(),
			0,
			instance3D.listenerPos.x,
			instance3D.listenerPos.y,
			-instance3D.listenerPos.z
		);
		ma_engine_listener_set_velocity(
			engine.get(),
			0,
			instance3D.listenerVelocity.x,
			instance3D.listenerVelocity.y,
			-instance3D.listenerVelocity.z
		);
		ma_engine_listener_set_world_up(
			engine.get(),
			0,
			instance3D.listenerUp.x,
			instance3D.listenerUp.y,
			-instance3D.listenerUp.z
		);
		ma_engine_listener_set_direction(
			engine.get(),
			0,
			instance3D.listenerFront.x,
			instance3D.listenerFront.y,
			-instance3D.listenerFront.z
		);

	}

	void SetReverb(REVERB_PRESET preset)
	{

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
