#include "wiAudio.h"
#include "wiBackLog.h"
#include "wiHelper.h"

#include <vector>

#include <xaudio2.h>
#include <x3daudio.h>
#pragma comment(lib,"xaudio2.lib")

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

namespace wiAudio
{
	IXAudio2* audioEngine = nullptr;
	IXAudio2MasteringVoice* masteringVoice = nullptr;
	XAUDIO2_VOICE_DETAILS masteringVoiceDetails = {};
	X3DAUDIO_HANDLE audio3D = {};
	IXAudio2SubmixVoice* submixVoices[SUBMIX_TYPE_COUNT] = {};

	Sound::~Sound()
	{
		Destroy(this);
	}
	SoundInstance::~SoundInstance()
	{
		Destroy(this);
	}
	struct SoundInternal
	{
		WAVEFORMATEX wfx = {};
		std::vector<uint8_t> audioData;
	};
	struct SoundInstanceInternal
	{
		IXAudio2SourceVoice* sourceVoice = nullptr;
		XAUDIO2_VOICE_DETAILS voiceDetails = {};
		std::vector<float> outputMatrix;
		XAUDIO2_BUFFER buffer = {};
		const SoundInternal* soundinternal = nullptr;
	};

	void Initialize()
	{
		HRESULT hr;
		hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		assert(SUCCEEDED(hr));

		hr = XAudio2Create(&audioEngine, 0, XAUDIO2_DEFAULT_PROCESSOR);
		assert(SUCCEEDED(hr));

#ifdef _DEBUG
		XAUDIO2_DEBUG_CONFIGURATION debugConfig = {};
		debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
		debugConfig.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
		audioEngine->SetDebugConfiguration(&debugConfig);
#endif // _DEBUG

		hr = audioEngine->CreateMasteringVoice(&masteringVoice);
		assert(SUCCEEDED(hr));

		masteringVoice->GetVoiceDetails(&masteringVoiceDetails);

		DWORD channelMask;
		masteringVoice->GetChannelMask(&channelMask);
		hr = X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, audio3D);
		assert(SUCCEEDED(hr));

		for (int i = 0; i < SUBMIX_TYPE_COUNT; ++i)
		{
			audioEngine->CreateSubmixVoice(
				&submixVoices[i], 
				masteringVoiceDetails.InputChannels,
				masteringVoiceDetails.InputSampleRate,
				0, 0, 0, 0);
		}

		if (SUCCEEDED(hr))
		{
			wiBackLog::post("wiAudio Initialized");
		}
	}
	void CleanUp()
	{
		for (int i = 0; i < SUBMIX_TYPE_COUNT; ++i)
		{
			if (submixVoices[i] != nullptr) submixVoices[i]->DestroyVoice();
		}
		if (masteringVoice != nullptr) masteringVoice->DestroyVoice();
		SAFE_RELEASE(audioEngine);
	}

	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
	{
		HRESULT hr = S_OK;
		if (INVALID_SET_FILE_POINTER == SetFilePointerEx(hFile, LARGE_INTEGER(), NULL, FILE_BEGIN))
			return HRESULT_FROM_WIN32(GetLastError());

		DWORD dwChunkType;
		DWORD dwChunkDataSize;
		DWORD dwRIFFDataSize = 0;
		DWORD dwFileType;
		DWORD bytesRead = 0;
		DWORD dwOffset = 0;

		while (hr == S_OK)
		{
			DWORD dwRead;
			if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());

			if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());

			switch (dwChunkType)
			{
			case fourccRIFF:
				dwRIFFDataSize = dwChunkDataSize;
				dwChunkDataSize = 4;
				if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
					hr = HRESULT_FROM_WIN32(GetLastError());
				break;

			default:
				LARGE_INTEGER li = LARGE_INTEGER();
				li.QuadPart = dwChunkDataSize;
				if (INVALID_SET_FILE_POINTER == SetFilePointerEx(hFile, li, NULL, FILE_CURRENT))
					return HRESULT_FROM_WIN32(GetLastError());
			}

			dwOffset += sizeof(DWORD) * 2;

			if (dwChunkType == fourcc)
			{
				dwChunkSize = dwChunkDataSize;
				dwChunkDataPosition = dwOffset;
				return S_OK;
			}

			dwOffset += dwChunkDataSize;

			if (bytesRead >= dwRIFFDataSize) return S_FALSE;

		}

		return S_OK;

	}
	HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
	{
		HRESULT hr = S_OK;

		LARGE_INTEGER li = LARGE_INTEGER();
		li.QuadPart = bufferoffset;
		if (INVALID_SET_FILE_POINTER == SetFilePointerEx(hFile, li, NULL, FILE_BEGIN))
			return HRESULT_FROM_WIN32(GetLastError());
		DWORD dwRead;
		if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	HRESULT CreateSound(const std::string& filename, Sound* sound)
	{
		Destroy(sound);

		std::wstring wfilename;
		wiHelper::StringConvert(filename, wfilename);

		HANDLE hFile = CreateFile2(
			wfilename.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			OPEN_EXISTING,
			nullptr
		);

		HRESULT hr;
		if (INVALID_HANDLE_VALUE == hFile)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
		}

		if (INVALID_SET_FILE_POINTER == SetFilePointerEx(hFile, LARGE_INTEGER(), NULL, FILE_BEGIN))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
		}

		DWORD dwChunkSize;
		DWORD dwChunkPosition;

		FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
		DWORD filetype;
		hr = ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
		assert(SUCCEEDED(hr));
		assert(filetype == fourccWAVE);

		SoundInternal* soundinternal = new SoundInternal;

		hr = FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
		assert(SUCCEEDED(hr));
		hr = ReadChunkData(hFile, &soundinternal->wfx, dwChunkSize, dwChunkPosition);
		assert(SUCCEEDED(hr));
		soundinternal->wfx.wFormatTag = WAVE_FORMAT_PCM;

		hr = FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
		assert(SUCCEEDED(hr));

		soundinternal->audioData.resize(dwChunkSize);
		hr = ReadChunkData(hFile, soundinternal->audioData.data(), dwChunkSize, dwChunkPosition);
		assert(SUCCEEDED(hr));

		sound->handle = (wiCPUHandle)soundinternal;

		return S_OK;
	}
	HRESULT CreateSoundInstance(const Sound* sound, SoundInstance* instance)
	{
		Destroy(instance);

		HRESULT hr;
		const SoundInternal* soundinternal = (const SoundInternal*)sound->handle;
		SoundInstanceInternal* instanceinternal = new SoundInstanceInternal;
		instanceinternal->soundinternal = soundinternal;

		XAUDIO2_SEND_DESCRIPTOR SFXSend = { 0, submixVoices[instance->type] };
		XAUDIO2_VOICE_SENDS SFXSendList = { 1, &SFXSend };

		hr = audioEngine->CreateSourceVoice(&instanceinternal->sourceVoice, &soundinternal->wfx, 
			XAUDIO2_VOICE_USEFILTER, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, &SFXSendList, NULL);
		if (FAILED(hr))
		{
			assert(0);
			return hr;
		}

		instanceinternal->sourceVoice->GetVoiceDetails(&instanceinternal->voiceDetails);
		
		instanceinternal->outputMatrix.resize(masteringVoiceDetails.InputChannels);

		instanceinternal->buffer.AudioBytes = (UINT32)soundinternal->audioData.size();
		instanceinternal->buffer.pAudioData = soundinternal->audioData.data();
		instanceinternal->buffer.Flags = XAUDIO2_END_OF_STREAM;
		instanceinternal->buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

		hr = instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer);
		if (FAILED(hr))
		{
			assert(0);
			return hr;
		}

		instance->handle = (wiCPUHandle)instanceinternal;

		return S_OK;
	}
	void Destroy(Sound* sound)
	{
		if (sound != nullptr && sound->handle != WI_NULL_HANDLE)
		{
			SoundInternal* soundinternal = (SoundInternal*)sound->handle;
			delete soundinternal;
			sound->handle = WI_NULL_HANDLE;
		}
	}
	void Destroy(SoundInstance* instance)
	{
		if (instance != nullptr && instance->handle != WI_NULL_HANDLE)
		{
			SoundInstanceInternal* instanceinternal = (SoundInstanceInternal*)instance->handle;
			instanceinternal->sourceVoice->Stop();
			instanceinternal->sourceVoice->DestroyVoice();
			delete instanceinternal;
			instance->handle = WI_NULL_HANDLE;
		}
	}
	void Play(SoundInstance* instance)
	{
		if (instance != nullptr && instance->handle != WI_NULL_HANDLE)
		{
			SoundInstanceInternal* instanceinternal = (SoundInstanceInternal*)instance->handle;
			HRESULT hr = instanceinternal->sourceVoice->Start();
			assert(SUCCEEDED(hr));
		}
	}
	void Pause(SoundInstance* instance)
	{
		if (instance != nullptr && instance->handle != WI_NULL_HANDLE)
		{
			SoundInstanceInternal* instanceinternal = (SoundInstanceInternal*)instance->handle;
			HRESULT hr = instanceinternal->sourceVoice->Stop(); // preserves cursor position
			assert(SUCCEEDED(hr));
		}
	}
	void Stop(SoundInstance* instance)
	{
		if (instance != nullptr && instance->handle != WI_NULL_HANDLE)
		{
			SoundInstanceInternal* instanceinternal = (SoundInstanceInternal*)instance->handle;
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
		if (instance == nullptr || instance->handle == WI_NULL_HANDLE)
		{
			HRESULT hr = masteringVoice->SetVolume(volume);
			assert(SUCCEEDED(hr));
		}
		else
		{
			SoundInstanceInternal* instanceinternal = (SoundInstanceInternal*)instance->handle;
			HRESULT hr = instanceinternal->sourceVoice->SetVolume(volume);
			assert(SUCCEEDED(hr));
		}
	}
	float GetVolume(const SoundInstance* instance)
	{
		float volume = 0;
		if (instance == nullptr || instance->handle == WI_NULL_HANDLE)
		{
			masteringVoice->GetVolume(&volume);
		}
		else
		{
			const SoundInstanceInternal* instanceinternal = (const SoundInstanceInternal*)instance->handle;
			instanceinternal->sourceVoice->GetVolume(&volume);
		}
		return volume;
	}
	void ExitLoop(SoundInstance* instance)
	{
		if (instance != nullptr && instance->handle != WI_NULL_HANDLE)
		{
			SoundInstanceInternal* instanceinternal = (SoundInstanceInternal*)instance->handle;
			HRESULT hr = instanceinternal->sourceVoice->ExitLoop();
			assert(SUCCEEDED(hr));
		}
	}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume)
	{
		submixVoices[type]->SetVolume(volume);
	}
	float GetSubmixVolume(SUBMIX_TYPE type)
	{
		float volume;
		submixVoices[type]->GetVolume(&volume);
		return volume;
	}

	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D)
	{
		if (instance != nullptr && instance->handle != WI_NULL_HANDLE)
		{
			SoundInstanceInternal* instanceinternal = (SoundInstanceInternal*)instance->handle;

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
			emitter.CurveDistanceScaler = 1;
			emitter.ChannelCount = 1;
			emitter.InnerRadius = instance3D.emitterRadius;

			UINT32 flags = 0;
			flags |= X3DAUDIO_CALCULATE_MATRIX;
			flags |= X3DAUDIO_CALCULATE_LPF_DIRECT;
			flags |= X3DAUDIO_CALCULATE_REVERB;
			flags |= X3DAUDIO_CALCULATE_DOPPLER;
			//flags |= X3DAUDIO_CALCULATE_DELAY;
			//flags |= X3DAUDIO_CALCULATE_LPF_REVERB;
			//flags |= X3DAUDIO_CALCULATE_EMITTER_ANGLE;
			//flags |= X3DAUDIO_CALCULATE_ZEROCENTER;
			//flags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;

			X3DAUDIO_DSP_SETTINGS settings = {};
			settings.SrcChannelCount = instanceinternal->voiceDetails.InputChannels;
			settings.DstChannelCount = masteringVoiceDetails.InputChannels;
			settings.pMatrixCoefficients = instanceinternal->outputMatrix.data();

			X3DAudioCalculate(audio3D, &listener, &emitter, flags, &settings);

			HRESULT hr;
			hr = instanceinternal->sourceVoice->SetOutputMatrix(
				submixVoices[instance->type],
				settings.SrcChannelCount, 
				settings.DstChannelCount, 
				settings.pMatrixCoefficients
			);
			assert(SUCCEEDED(hr));

			hr = instanceinternal->sourceVoice->SetFrequencyRatio(settings.DopplerFactor);
			assert(SUCCEEDED(hr));

			XAUDIO2_FILTER_PARAMETERS filterParams =
			{ 
				LowPassFilter, 
				2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFDirectCoefficient), 
				1.0f 
			};
			hr = instanceinternal->sourceVoice->SetFilterParameters(&filterParams);
			assert(SUCCEEDED(hr));
		}
	}
}
