#ifndef FACT_CPP_XAUDIO2_H
#define FACT_CPP_XAUDIO2_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef FAUDIOCPP_EXPORTS
#define FAUDIOCPP_API  HRESULT __stdcall
#else
#define FAUDIOCPP_API __declspec(dllimport) HRESULT __stdcall
#endif

#ifndef XAUDIO2_VERSION
#define XAUDIO2_VERSION 7
#endif

#include "com_utils.h"

typedef FAudioProcessor XAUDIO2_PROCESSOR;
typedef FAudioDeviceDetails XAUDIO2_DEVICE_DETAILS;
typedef FAudioWaveFormatEx WAVEFORMATEX;
typedef FAudioWaveFormatExtensible WAVEFORMATEXTENSIBLE;
typedef FAudioDebugConfiguration XAUDIO2_DEBUG_CONFIGURATION;
typedef FAudioFilterParameters XAUDIO2_FILTER_PARAMETERS;
typedef FAudioBuffer XAUDIO2_BUFFER;
typedef FAudioBufferWMA XAUDIO2_BUFFER_WMA;
typedef FAudioVoiceState XAUDIO2_VOICE_STATE;

class IXAudio2EngineCallback;
class IXAudio2Voice;
class IXAudio2SourceVoice;
class IXAudio2SubmixVoice;
class IXAudio2MasteringVoice;
class IXAudio2VoiceCallback;

#pragma pack(push, 1)

#if XAUDIO2_VERSION >= 4
typedef struct XAUDIO2_SEND_DESCRIPTOR {
	UINT32        Flags;
	IXAudio2Voice *pOutputVoice;
} XAUDIO2_SEND_DESCRIPTOR;

typedef struct XAUDIO2_VOICE_SENDS {
	UINT32                  SendCount;
	XAUDIO2_SEND_DESCRIPTOR *pSends;
} XAUDIO2_VOICE_SENDS;
#else
typedef struct XAUDIO2_VOICE_SENDS {
	UINT32 SendCount;
	IXAudio2Voice **pSends;
} XAUDIO2_VOICE_SENDS;
#endif

typedef struct XAUDIO2_EFFECT_DESCRIPTOR {
	IUnknown *pEffect;
	BOOL InitialState;
	UINT32  OutputChannels;
} XAUDIO2_EFFECT_DESCRIPTOR;

typedef struct XAUDIO2_EFFECT_CHAIN {
	UINT32 EffectCount;
	XAUDIO2_EFFECT_DESCRIPTOR *pEffectDescriptors;
} XAUDIO2_EFFECT_CHAIN;

#if XAUDIO2_VERSION >= 3
typedef FAudioPerformanceData XAUDIO2_PERFORMANCE_DATA;
#else
typedef struct XAUDIO2_PERFORMANCE_DATA
{
	uint64_t AudioCyclesSinceLastQuery;
	uint64_t TotalCyclesSinceLastQuery;
	uint32_t MinimumCyclesPerQuantum;
	uint32_t MaximumCyclesPerQuantum;
	uint32_t MemoryUsageInBytes;
	uint32_t CurrentLatencyInSamples;
	uint32_t GlitchesSinceEngineStarted;
	uint32_t ActiveSourceVoiceCount;
	uint32_t TotalSourceVoiceCount;
	uint32_t ActiveSubmixVoiceCount;
	uint32_t TotalSubmixVoiceCount;
	uint32_t ActiveXmaSourceVoices;
	uint32_t ActiveXmaStreams;
} XAUDIO2_PERFORMANCE_DATA;
#endif// XAUDIO2_VERSION >= 3

#if XAUDIO2_VERSION > 7
typedef FAudioVoiceDetails XAUDIO2_VOICE_DETAILS;
#else
typedef struct XAUDIO2_VOICE_DETAILS {
	uint32_t CreationFlags;
	uint32_t InputChannels;
	uint32_t InputSampleRate;
} XAUDIO2_VOICE_DETAILS;
#endif // XAUDIO2_VERSION > 7

#pragma pack(pop)

class IXAudio2Voice  {
public:
	COM_METHOD(void) GetVoiceDetails (XAUDIO2_VOICE_DETAILS* pVoiceDetails) = 0;
	COM_METHOD(HRESULT) SetOutputVoices (const XAUDIO2_VOICE_SENDS* pSendList) = 0;
	COM_METHOD(HRESULT) SetEffectChain (const XAUDIO2_EFFECT_CHAIN* pEffectChain) = 0;
	COM_METHOD(HRESULT) EnableEffect (
		UINT32 EffectIndex,
		UINT32 OperationSet = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(HRESULT) DisableEffect (
		UINT32 EffectIndex,
		UINT32 OperationSet  = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(void) GetEffectState (UINT32 EffectIndex, BOOL* pEnabled) = 0;
	COM_METHOD(HRESULT) SetEffectParameters (
		UINT32 EffectIndex,
		const void* pParameters,
		UINT32 ParametersByteSize,
		UINT32 OperationSet  = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(HRESULT) GetEffectParameters (
		UINT32 EffectIndex,
		void* pParameters,
		UINT32 ParametersByteSize) = 0;
	COM_METHOD(HRESULT) SetFilterParameters (
		const XAUDIO2_FILTER_PARAMETERS* pParameters,
		UINT32 OperationSet  = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(void) GetFilterParameters (XAUDIO2_FILTER_PARAMETERS* pParameters) = 0;
#if XAUDIO2_VERSION >= 4
	COM_METHOD(HRESULT) SetOutputFilterParameters (
		IXAudio2Voice* pDestinationVoice,
		const XAUDIO2_FILTER_PARAMETERS* pParameters,
		UINT32 OperationSet  = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(void) GetOutputFilterParameters (
		IXAudio2Voice* pDestinationVoice,
		XAUDIO2_FILTER_PARAMETERS* pParameters) = 0;
#endif // XAUDIO2_VERSION >= 4
	COM_METHOD(HRESULT) SetVolume (
		float Volume,
		UINT32 OperationSet = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(void) GetVolume (float* pVolume) = 0;
	COM_METHOD(HRESULT) SetChannelVolumes (
		UINT32 Channels, 
		const float* pVolumes,
		UINT32 OperationSet  = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(void) GetChannelVolumes (UINT32 Channels, float* pVolumes) = 0;
	COM_METHOD(HRESULT) SetOutputMatrix (
		IXAudio2Voice* pDestinationVoice,
		UINT32 SourceChannels, 
		UINT32 DestinationChannels,
		const float* pLevelMatrix,
		UINT32 OperationSet  = FAUDIO_COMMIT_NOW) = 0;
#if XAUDIO2_VERSION >= 1
	COM_METHOD(void) GetOutputMatrix (
#else
	COM_METHOD(HRESULT) GetOutputMatrix (
#endif // XAUDIO2_VERSION >= 1
		IXAudio2Voice* pDestinationVoice,
		UINT32 SourceChannels, 
		UINT32 DestinationChannels,
		float* pLevelMatrix) = 0;
	COM_METHOD(void) DestroyVoice() = 0;

public:
	// not the ideal solution but the cleanest way I known to get to the common FAudioVoice object
	// from a IXAudioVoice pointer to any derived class (without changing the vtable)
	FAudioVoice *faudio_voice;
};

class IXAudio2SourceVoice : public IXAudio2Voice {
public:
	COM_METHOD(HRESULT) Start (UINT32 Flags = 0, UINT32 OperationSet = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(HRESULT) Stop (UINT32 Flags = 0, UINT32 OperationSet = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(HRESULT) SubmitSourceBuffer (
		const XAUDIO2_BUFFER* pBuffer, 
		const XAUDIO2_BUFFER_WMA* pBufferWMA = NULL) = 0;
	COM_METHOD(HRESULT) FlushSourceBuffers () = 0;
	COM_METHOD(HRESULT) Discontinuity () = 0;
	COM_METHOD(HRESULT) ExitLoop (UINT32 OperationSet = FAUDIO_COMMIT_NOW) = 0;
#if (XAUDIO2_VERSION <= 7)
	COM_METHOD(void) GetState ( XAUDIO2_VOICE_STATE* pVoiceState) = 0;
#else
	COM_METHOD(void) GetState ( XAUDIO2_VOICE_STATE* pVoiceState, UINT32 Flags = 0) = 0;
#endif
	COM_METHOD(HRESULT) SetFrequencyRatio (
		float Ratio,
		UINT32 OperationSet = FAUDIO_COMMIT_NOW) = 0;
	COM_METHOD(void) GetFrequencyRatio (float* pRatio) = 0;
#if XAUDIO2_VERSION >= 4
	COM_METHOD(HRESULT) SetSourceSampleRate (UINT32 NewSourceSampleRate) = 0;
#endif // XAUDIO2_VERSION >= 4
};

class IXAudio2SubmixVoice : public IXAudio2Voice {

};

class IXAudio2MasteringVoice : public IXAudio2Voice {
public:
#if (XAUDIO2_VERSION >= 8)
	COM_METHOD(HRESULT) GetChannelMask (uint32_t *pChannelmask) = 0;
#endif
};

class IXAudio2VoiceCallback {
public:
#if XAUDIO2_VERSION >= 1
	COM_METHOD(void) OnVoiceProcessingPassStart (UINT32 BytesRequired) = 0;
#else
	COM_METHOD(void) OnVoiceProcessingPassStart () = 0;
#endif // XAUDIO2_VERSION >= 1
	COM_METHOD(void) OnVoiceProcessingPassEnd () = 0;
	COM_METHOD(void) OnStreamEnd () = 0;
	COM_METHOD(void) OnBufferStart (void* pBufferContext) = 0;
	COM_METHOD(void) OnBufferEnd (void* pBufferContext) = 0;
	COM_METHOD(void) OnLoopEnd (void* pBufferContext) = 0;
	COM_METHOD(void) OnVoiceError (void* pBufferContext, HRESULT Error) = 0;
};

class IXAudio2EngineCallback {
public:
	COM_METHOD(void) OnProcessingPassStart () = 0;
	COM_METHOD(void) OnProcessingPassEnd () = 0;
	COM_METHOD(void) OnCriticalError (HRESULT Error) = 0;
};

class IXAudio2 : public IUnknown {
public:
#if (XAUDIO2_VERSION <= 7)
	COM_METHOD(HRESULT) GetDeviceCount(UINT32 *pCount) = 0;
	COM_METHOD(HRESULT) GetDeviceDetails (UINT32 Index, XAUDIO2_DEVICE_DETAILS* pDeviceDetails) = 0;
	COM_METHOD(HRESULT) Initialize (
		UINT32 Flags = 0,
		XAUDIO2_PROCESSOR XAudio2Processor = FAUDIO_DEFAULT_PROCESSOR) = 0;
#endif // XAUDIO2_VERSION <= 7

	COM_METHOD(HRESULT) RegisterForCallbacks (IXAudio2EngineCallback* pCallback) = 0;
	COM_METHOD(void) UnregisterForCallbacks ( IXAudio2EngineCallback* pCallback) = 0;

	COM_METHOD(HRESULT) CreateSourceVoice (
		IXAudio2SourceVoice** ppSourceVoice,
		const WAVEFORMATEX* pSourceFormat,
		UINT32 Flags = 0,
		float MaxFrequencyRatio = FAUDIO_DEFAULT_FREQ_RATIO,
		IXAudio2VoiceCallback* pCallback = NULL,
		const XAUDIO2_VOICE_SENDS* pSendList = NULL,
		const XAUDIO2_EFFECT_CHAIN* pEffectChain = NULL) = 0;

	COM_METHOD(HRESULT) CreateSubmixVoice(
		IXAudio2SubmixVoice** ppSubmixVoice,
		UINT32 InputChannels, 
		UINT32 InputSampleRate,
		UINT32 Flags = 0,
		UINT32 ProcessingStage = 0,
		const XAUDIO2_VOICE_SENDS* pSendList = NULL,
		const XAUDIO2_EFFECT_CHAIN* pEffectChain = NULL) = 0;

#if XAUDIO2_VERSION <= 7
	COM_METHOD(HRESULT) CreateMasteringVoice(
		IXAudio2MasteringVoice** ppMasteringVoice,
		UINT32 InputChannels = FAUDIO_DEFAULT_CHANNELS,
		UINT32 InputSampleRate = FAUDIO_DEFAULT_SAMPLERATE,
		UINT32 Flags = 0,
		UINT32 DeviceIndex = 0,
		const XAUDIO2_EFFECT_CHAIN* pEffectChain = NULL) = 0;
#else
	COM_METHOD(HRESULT) CreateMasteringVoice (
		IXAudio2MasteringVoice** ppMasteringVoice,
		UINT32 InputChannels = FAUDIO_DEFAULT_CHANNELS,
		UINT32 InputSampleRate = FAUDIO_DEFAULT_SAMPLERATE,
		UINT32 Flags = 0,
		LPCWSTR szDeviceId = NULL,
		const XAUDIO2_EFFECT_CHAIN* pEffectChain = NULL,
		int StreamCategory = 6) = 0;	// FIXME: type was AUDIO_STREAM_CATEGORY (scoped enum so int for now)
#endif // XAUDIO2_VERSION <= 7

	COM_METHOD(HRESULT) StartEngine() = 0;
	COM_METHOD(void) StopEngine() = 0;

	COM_METHOD(HRESULT) CommitChanges(UINT32 OperationSet) = 0;

	COM_METHOD(void) GetPerformanceData(XAUDIO2_PERFORMANCE_DATA* pPerfData) = 0;

	COM_METHOD(void) SetDebugConfiguration(
		XAUDIO2_DEBUG_CONFIGURATION* pDebugConfiguration,
		void* pReserved = NULL) = 0;
};

#if XAUDIO2_VERSION >= 8

FAUDIOCPP_API XAudio2Create(
	IXAudio2          **ppXAudio2,
	UINT32            Flags,
	XAUDIO2_PROCESSOR XAudio2Processor
);

#endif // XAUDIO2_VERSION >= 8


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // FACT_CPP_XAUDIO2_H
