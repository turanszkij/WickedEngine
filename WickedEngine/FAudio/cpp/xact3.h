#ifndef FACT_CPP_XACT3_H
#define FACT_CPP_XACT3_H

#include "xaudio2.h"
#include <FACT.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef XACT3_VERSION
#define XACT3_VERSION 7
#endif

typedef FACTRendererDetails XACT_RENDERER_DETAILS;
typedef FACTFileIOCallbacks XACT_FILEIO_CALLBACKS;
typedef FACTRuntimeParameters XACT_RUNTIME_PARAMETERS;
typedef FACTStreamingParameters XACT_STREAMING_PARAMETERS;
typedef FACTWaveBankRegion WAVEBANKREGION;
typedef FACTWaveBankSampleRegion WAVEBANKSAMPLEREGION;
typedef FACTWaveBankHeader WAVEBANKHEADER;
typedef FACTWaveBankMiniWaveFormat WAVEBANKMINIWAVEFORMAT;
typedef FACTWaveBankEntry WAVEBANKENTRY;
typedef FACTWaveBankEntryCompact WAVEBANKENTRYCOMPACT;
typedef FACTWaveBankData WAVEBANKDATA;
typedef FACTWaveProperties XACT_WAVE_PROPERTIES;
typedef FACTWaveInstanceProperties XACT_WAVE_INSTANCE_PROPERTIES;
typedef FACTCueProperties XACT_CUE_PROPERTIES;
typedef FACTTrackProperties XACT_TRACK_PROPERTIES;
typedef FACTVariationProperties XACT_VARIATION_PROPERTIES;
typedef FACTSoundProperties XACT_SOUND_PROPERTIES;
typedef FACTSoundVariationProperties XACT_SOUND_VARIATION_PROPERTIES;
typedef FACTCueInstanceProperties XACT_CUE_INSTANCE_PROPERTIES;

typedef FACTReadFileCallback XACT_READFILE_CALLBACK;
typedef FACTGetOverlappedResultCallback XACT_GETOVERLAPPEDRESULT_CALLBACK;

typedef FACTWaveBankSegIdx WAVEBANKSEGIDX;

class IXACT3Engine;
class IXACT3SoundBank;
class IXACT3WaveBank;
class IXACT3Wave;
class IXACT3Cue;

#pragma pack(push, 1)

typedef struct XACT_NOTIFICATION_DESCRIPTION
{
	uint8_t type;
	uint8_t flags;
	IXACT3SoundBank *pSoundBank;
	IXACT3WaveBank *pWaveBank;
	IXACT3Cue *pCue;
	IXACT3Wave *pWave;
	uint16_t cueIndex;
	uint16_t waveIndex;
	void* pvContext;
} XACT_NOTIFICATION_DESCRIPTION;

typedef struct XACT_NOTIFICATION_CUE
{
	uint16_t cueIndex;
	IXACT3SoundBank *pSoundBank;
	IXACT3Cue *pCue;
} XACT_NOTIFICATION_CUE;

typedef struct XACT_NOTIFICATION_MARKER
{
	uint16_t cueIndex;
	IXACT3SoundBank *pSoundBank;
	IXACT3Cue *pCue;
	uint32_t marker;
} XACT_NOTIFICATION_MARKER;

typedef struct XACT_NOTIFICATION_SOUNDBANK
{
	IXACT3SoundBank *pSoundBank;
} XACT_NOTIFICATION_SOUNDBANK;

typedef struct XACT_NOTIFICATION_WAVEBANK
{
	IXACT3WaveBank *pWaveBank;
} XACT_NOTIFICATION_WAVEBANK;

typedef struct XACT_NOTIFICATION_VARIABLE
{
	uint16_t cueIndex;
	IXACT3SoundBank *pSoundBank;
	IXACT3Cue *pCue;
	uint16_t variableIndex;
	float variableValue;
	uint8_t local;
} XACT_NOTIFICATION_VARIABLE;

typedef struct XACT_NOTIFICATION_GUI
{
	uint32_t reserved;
} XACT_NOTIFICATION_GUI;

typedef struct XACT_NOTIFICATION_WAVE
{
	IXACT3WaveBank *pWaveBank;
	uint16_t waveIndex;
	uint16_t cueIndex;
	IXACT3SoundBank *pSoundBank;
	IXACT3Cue *pCue;
	IXACT3Wave *pWave;
} XACT_NOTIFICATION_WAVE;

typedef struct XACT_NOTIFICATION
{
	uint8_t type;
	int32_t timeStamp;
	void *pvContext;
	union
	{
		XACT_NOTIFICATION_CUE cue;
		XACT_NOTIFICATION_MARKER marker;
		XACT_NOTIFICATION_SOUNDBANK soundBank;
		XACT_NOTIFICATION_WAVEBANK waveBank;
		XACT_NOTIFICATION_VARIABLE variable;
		XACT_NOTIFICATION_GUI gui;
		XACT_NOTIFICATION_WAVE wave;
	};
} XACT_NOTIFICATION;

#pragma pack(pop)

class IXACT3Engine : public IUnknown
{
public:
	COM_METHOD(HRESULT) GetRendererCount(
		uint16_t *pnRendererCount
	) = 0;
	COM_METHOD(HRESULT) GetRendererDetails(
		uint16_t nRendererIndex,
		XACT_RENDERER_DETAILS *pRendererDetails
	) = 0;
	COM_METHOD(HRESULT) GetFinalMixFormat(
		WAVEFORMATEXTENSIBLE *pFinalMixFormat
	) = 0;
	COM_METHOD(HRESULT) Initialize(
		const XACT_RUNTIME_PARAMETERS *pParams
	) = 0;
	COM_METHOD(HRESULT) ShutDown() = 0;
	COM_METHOD(HRESULT) DoWork() = 0;
	COM_METHOD(HRESULT) CreateSoundBank(
		const void *pvBuffer,
		uint32_t dwSize,
		uint32_t dwFlags,
		uint32_t dwAllocAttributes,
		IXACT3SoundBank **ppSoundBank
	) = 0;
	COM_METHOD(HRESULT) CreateInMemoryWaveBank(
		const void *pvBuffer,
		uint32_t dwSize,
		uint32_t dwFlags,
		uint32_t dwAllocAttributes,
		IXACT3WaveBank **ppWaveBank
	) = 0;
	COM_METHOD(HRESULT) CreateStreamingWaveBank(
		const XACT_STREAMING_PARAMETERS *pParms,
		IXACT3WaveBank **ppWaveBank
	) = 0;
	COM_METHOD(HRESULT) PrepareWave(
		uint32_t dwFlags,
		const char *szWavePath,
		uint32_t wStreamingPacketSize,
		uint32_t dwAlignment,
		uint32_t dwPlayOffset,
		uint8_t nLoopCount,
		IXACT3Wave **ppWave
	) = 0;
	COM_METHOD(HRESULT) PrepareInMemoryWave(
		uint32_t dwFlags,
		WAVEBANKENTRY entry,
		uint32_t *pdwSeekTable, /* Optional! */
		uint8_t *pbWaveData,
		uint32_t dwPlayOffset,
		uint8_t nLoopCount,
		IXACT3Wave **ppWave
	) = 0;
	COM_METHOD(HRESULT) PrepareStreamingWave(
		uint32_t dwFlags,
		WAVEBANKENTRY entry,
		XACT_STREAMING_PARAMETERS streamingParams,
		uint32_t dwAlignment,
		uint32_t *pdwSeekTable, /* Optional! */
		uint8_t *pbWaveData,
		uint32_t dwPlayOffset,
		uint8_t nLoopCount,
		IXACT3Wave **ppWave
	) = 0;
	COM_METHOD(HRESULT) RegisterNotification(
		const XACT_NOTIFICATION_DESCRIPTION *pNotificationDescription
	) = 0;
	COM_METHOD(HRESULT) UnRegisterNotification(
		const XACT_NOTIFICATION_DESCRIPTION *pNotificationDescription
	) = 0;
	COM_METHOD(uint16_t) GetCategory(const char *szFriendlyName) = 0;
	COM_METHOD(HRESULT) Stop(uint16_t nCategory, uint32_t dwFlags) = 0;
	COM_METHOD(HRESULT) SetVolume(uint16_t nCategory, float volume) = 0;
	COM_METHOD(HRESULT) Pause(uint16_t nCategory, int32_t fPause) = 0;
	COM_METHOD(uint16_t) GetGlobalVariableIndex(
		const char *szFriendlyName
	) = 0;
	COM_METHOD(HRESULT) SetGlobalVariable(
		uint16_t nIndex,
		float nValue
	) = 0;
	COM_METHOD(HRESULT) GetGlobalVariable(
		uint16_t nIndex,
		float *pnValue
	) = 0;
};

class IXACT3SoundBank
{
public:
	COM_METHOD(uint16_t) GetCueIndex(const char *szFriendlyName) = 0;
	COM_METHOD(HRESULT) GetNumCues(uint16_t *pnNumCues) = 0;
	COM_METHOD(HRESULT) GetCueProperties(
		uint16_t nCueIndex,
		XACT_CUE_PROPERTIES *pProperties
	) = 0;
	COM_METHOD(HRESULT) Prepare(
		uint16_t nCueIndex,
		uint32_t dwFlags,
		int32_t timeOffset,
		IXACT3Cue** ppCue
	) = 0;
	COM_METHOD(HRESULT) Play(
		uint16_t nCueIndex,
		uint32_t dwFlags,
		int32_t timeOffset,
		IXACT3Cue** ppCue /* Optional! */
	) = 0;
	COM_METHOD(HRESULT) Stop(uint16_t nCueIndex, uint32_t dwFlags) = 0;
	COM_METHOD(HRESULT) Destroy() = 0;
	COM_METHOD(HRESULT) GetState(uint32_t *pdwState) = 0;
};

class IXACT3WaveBank
{
public:
	COM_METHOD(HRESULT) Destroy() = 0;
	COM_METHOD(HRESULT) GetNumWaves(uint16_t *pnNumWaves) = 0;
	COM_METHOD(uint16_t) GetWaveIndex(const char *szFriendlyName) = 0;
	COM_METHOD(HRESULT) GetWaveProperties(
		uint16_t nWaveIndex,
		XACT_WAVE_PROPERTIES *pWaveProperties
	) = 0;
	COM_METHOD(HRESULT) Prepare(
		uint16_t nWaveIndex,
		uint32_t dwFlags,
		uint32_t dwPlayOffset,
		uint8_t nLoopCount,
		IXACT3Wave **ppWave
	) = 0;
	COM_METHOD(HRESULT) Play(
		uint16_t nWaveIndex,
		uint32_t dwFlags,
		uint32_t dwPlayOffset,
		uint8_t nLoopCount,
		IXACT3Wave **ppWave
	) = 0;
	COM_METHOD(HRESULT) Stop(
		uint16_t nWaveIndex,
		uint32_t dwFlags
	) = 0;
	COM_METHOD(HRESULT) GetState(uint32_t *pdwState) = 0;
};

class IXACT3Wave
{
public:
	COM_METHOD(HRESULT) Destroy() = 0;
	COM_METHOD(HRESULT) Play() = 0;
	COM_METHOD(HRESULT) Stop(uint32_t dwFlags) = 0;
	COM_METHOD(HRESULT) Pause(int32_t fPause) = 0;
	COM_METHOD(HRESULT) GetState(uint32_t *pdwState) = 0;
	COM_METHOD(HRESULT) SetPitch(int16_t pitch) = 0;
	COM_METHOD(HRESULT) SetVolume(float volume) = 0;
	COM_METHOD(HRESULT) SetMatrixCoefficients(
		uint32_t uSrcChannelCount,
		uint32_t uDstChannelCount,
		float *pMatrixCoefficients
	) = 0;
	COM_METHOD(HRESULT) GetProperties(
		XACT_WAVE_INSTANCE_PROPERTIES *pProperties
	) = 0;
};

class IXACT3Cue
{
public:
	COM_METHOD(HRESULT) Play() = 0;
	COM_METHOD(HRESULT) Stop(uint32_t dwFlags) = 0;
	COM_METHOD(HRESULT) GetState(uint32_t *pdwState) = 0;
	COM_METHOD(HRESULT) Destroy() = 0;
	COM_METHOD(HRESULT) SetMatrixCoefficients(
		uint32_t uSrcChannelCount,
		uint32_t uDstChannelCount,
		float *pMatrixCoefficients
	) = 0;
	COM_METHOD(uint16_t) GetVariableIndex(const char *szFriendlyName) = 0;
	COM_METHOD(HRESULT) SetVariable(uint16_t nIndex, float nValue) = 0;
	COM_METHOD(HRESULT) GetVariable(uint16_t nIndex, float *nValue) = 0;
	COM_METHOD(HRESULT) Pause(int32_t fPause) = 0;
	COM_METHOD(HRESULT) GetProperties(
		XACT_CUE_INSTANCE_PROPERTIES **ppProperties
	) = 0;
#if XACT3_VERSION >= 5
	COM_METHOD(HRESULT) SetOutputVoices(
		const XAUDIO2_VOICE_SENDS *pSendList /* Optional! */
	) = 0;
	COM_METHOD(HRESULT) SetOutputVoiceMatrix(
		const IXAudio2Voice *pDestinationVoice, /* Optional! */
		uint32_t SourceChannels,
		uint32_t DestinationChannels,
		const float *pLevelMatrix /* SourceChannels * DestinationChannels */
	) = 0;
#endif /* XACT_VERSION >= 5 */
};


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // FACT_CPP_XACT3_H
