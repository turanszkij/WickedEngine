/* FAudio - XAudio Reimplementation for FNA
 *
 * Copyright (c) 2011-2024 Ethan Lee, Luigi Auriemma, and the MonoGame Team
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Ethan "flibitijibibo" Lee <flibitijibibo@flibitijibibo.com>
 *
 */

/* This file has no documentation since you are expected to already know how
 * XACT works if you are still using these APIs!
 */

#ifndef FACT_H
#define FACT_H

#include "FAudio.h"

#define FACTAPI FAUDIOAPI
#ifdef _WIN32
#define FACTCALL __stdcall
#else
#define FACTCALL
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Type Declarations */

typedef struct FACTAudioEngine FACTAudioEngine;
typedef struct FACTSoundBank FACTSoundBank;
typedef struct FACTWaveBank FACTWaveBank;
typedef struct FACTWave FACTWave;
typedef struct FACTCue FACTCue;
typedef struct FACTNotification FACTNotification;

typedef struct FACTRendererDetails
{
	int16_t rendererID[0xFF]; /* Win32 wchar_t */
	int16_t displayName[0xFF]; /* Win32 wchar_t */
	int32_t defaultDevice;
} FACTRendererDetails;

typedef struct FACTOverlapped
{
	void *Internal; /* ULONG_PTR */
	void *InternalHigh; /* ULONG_PTR */
	FAUDIONAMELESS union
	{
		FAUDIONAMELESS struct
		{
			uint32_t Offset;
			uint32_t OffsetHigh;
		};
		void *Pointer;
	};
	void *hEvent;
} FACTOverlapped;

typedef int32_t (FACTCALL * FACTReadFileCallback)(
	void *hFile,
	void *buffer,
	uint32_t nNumberOfBytesToRead,
	uint32_t *lpNumberOfBytesRead,
	FACTOverlapped *lpOverlapped
);

typedef int32_t (FACTCALL * FACTGetOverlappedResultCallback)(
	void *hFile,
	FACTOverlapped *lpOverlapped,
	uint32_t *lpNumberOfBytesTransferred,
	int32_t bWait
);

typedef struct FACTFileIOCallbacks
{
	FACTReadFileCallback readFileCallback;
	FACTGetOverlappedResultCallback getOverlappedResultCallback;
} FACTFileIOCallbacks;

typedef void (FACTCALL * FACTNotificationCallback)(
	const FACTNotification *pNotification
);

/* FIXME: ABI bug! This should be pack(1) explicitly. Do not memcpy this! */
typedef struct FACTRuntimeParameters
{
	uint32_t lookAheadTime;
	void *pGlobalSettingsBuffer;
	uint32_t globalSettingsBufferSize;
	uint32_t globalSettingsFlags;
	uint32_t globalSettingsAllocAttributes;
	FACTFileIOCallbacks fileIOCallbacks;
	FACTNotificationCallback fnNotificationCallback;
	int16_t *pRendererID; /* Win32 wchar_t* */
	FAudio *pXAudio2;
	FAudioMasteringVoice *pMasteringVoice;
} FACTRuntimeParameters;

typedef struct FACTStreamingParameters
{
	void *file;
	uint32_t offset;
	uint32_t flags;
	uint16_t packetSize; /* Measured in DVD sectors, or 2048 bytes */
} FACTStreamingParameters;

#define FACT_WAVEBANK_TYPE_BUFFER		0x00000000
#define FACT_WAVEBANK_TYPE_STREAMING		0x00000001
#define FACT_WAVEBANK_TYPE_MASK			0x00000001

#define FACT_WAVEBANK_FLAGS_ENTRYNAMES		0x00010000
#define FACT_WAVEBANK_FLAGS_COMPACT		0x00020000
#define FACT_WAVEBANK_FLAGS_SYNC_DISABLED	0x00040000
#define FACT_WAVEBANK_FLAGS_SEEKTABLES		0x00080000
#define FACT_WAVEBANK_FLAGS_MASK		0x000F0000

typedef enum FACTWaveBankSegIdx
{
	FACT_WAVEBANK_SEGIDX_BANKDATA = 0,
	FACT_WAVEBANK_SEGIDX_ENTRYMETADATA,
	FACT_WAVEBANK_SEGIDX_SEEKTABLES,
	FACT_WAVEBANK_SEGIDX_ENTRYNAMES,
	FACT_WAVEBANK_SEGIDX_ENTRYWAVEDATA,
	FACT_WAVEBANK_SEGIDX_COUNT
} FACTWaveBankSegIdx;

#pragma pack(push, 1)

typedef struct FACTWaveBankRegion
{
	uint32_t dwOffset;
	uint32_t dwLength;
} FACTWaveBankRegion;

typedef struct FACTWaveBankSampleRegion
{
	uint32_t dwStartSample;
	uint32_t dwTotalSamples;
} FACTWaveBankSampleRegion;

typedef struct FACTWaveBankHeader
{
	uint32_t dwSignature;
	uint32_t dwVersion;
	uint32_t dwHeaderVersion;
	FACTWaveBankRegion Segments[FACT_WAVEBANK_SEGIDX_COUNT];
} FACTWaveBankHeader;

typedef union FACTWaveBankMiniWaveFormat
{
	FAUDIONAMELESS struct
	{
		uint32_t wFormatTag : 2;
		uint32_t nChannels : 3;
		uint32_t nSamplesPerSec : 18;
		uint32_t wBlockAlign : 8;
		uint32_t wBitsPerSample : 1;
	};
	uint32_t dwValue;
} FACTWaveBankMiniWaveFormat;

typedef struct FACTWaveBankEntry
{
	FAUDIONAMELESS union
	{
		FAUDIONAMELESS struct
		{
			uint32_t dwFlags : 4;
			uint32_t Duration : 28;
		};
		uint32_t dwFlagsAndDuration;
	};
	FACTWaveBankMiniWaveFormat Format;
	FACTWaveBankRegion PlayRegion;
	FACTWaveBankSampleRegion LoopRegion;
} FACTWaveBankEntry;

typedef struct FACTWaveBankEntryCompact
{
	uint32_t dwOffset : 21;
	uint32_t dwLengthDeviation : 11;
} FACTWaveBankEntryCompact;

typedef struct FACTWaveBankData
{
	uint32_t dwFlags;
	uint32_t dwEntryCount;
	char szBankName[64];
	uint32_t dwEntryMetaDataElementSize;
	uint32_t dwEntryNameElementSize;
	uint32_t dwAlignment;
	FACTWaveBankMiniWaveFormat CompactFormat;
	uint64_t BuildTime;
} FACTWaveBankData;

#pragma pack(pop)

typedef struct FACTWaveProperties
{
	char friendlyName[64];
	FACTWaveBankMiniWaveFormat format;
	uint32_t durationInSamples;
	FACTWaveBankSampleRegion loopRegion;
	int32_t streaming;
} FACTWaveProperties;

typedef struct FACTWaveInstanceProperties
{
	FACTWaveProperties properties;
	int32_t backgroundMusic;
} FACTWaveInstanceProperties;

typedef struct FACTCueProperties
{
	char friendlyName[0xFF];
	int32_t interactive;
	uint16_t iaVariableIndex;
	uint16_t numVariations;
	uint8_t maxInstances;
	uint8_t currentInstances;
} FACTCueProperties;

typedef struct FACTTrackProperties
{
	uint32_t duration;
	uint16_t numVariations;
	uint8_t numChannels;
	uint16_t waveVariation;
	uint8_t loopCount;
} FACTTrackProperties;

typedef struct FACTVariationProperties
{
	uint16_t index;
	uint8_t weight;
	float iaVariableMin;
	float iaVariableMax;
	int32_t linger;
} FACTVariationProperties;

typedef struct FACTSoundProperties
{
	uint16_t category;
	uint8_t priority;
	int16_t pitch;
	float volume;
	uint16_t numTracks;
	FACTTrackProperties arrTrackProperties[1];
} FACTSoundProperties;

typedef struct FACTSoundVariationProperties
{
	FACTVariationProperties variationProperties;
	FACTSoundProperties soundProperties;
} FACTSoundVariationProperties;

typedef struct FACTCueInstanceProperties
{
	uint32_t allocAttributes;
	FACTCueProperties cueProperties;
	FACTSoundVariationProperties activeVariationProperties;
} FACTCueInstanceProperties;

#pragma pack(push, 1)

typedef struct FACTNotificationDescription
{
	uint8_t type;
	uint8_t flags;
	FACTSoundBank *pSoundBank;
	FACTWaveBank *pWaveBank;
	FACTCue *pCue;
	FACTWave *pWave;
	uint16_t cueIndex;
	uint16_t waveIndex;
	void* pvContext;
} FACTNotificationDescription;

typedef struct FACTNotificationCue
{
	uint16_t cueIndex;
	FACTSoundBank *pSoundBank;
	FACTCue *pCue;
} FACTNotificationCue;

typedef struct FACTNotificationMarker
{
	uint16_t cueIndex;
	FACTSoundBank *pSoundBank;
	FACTCue *pCue;
	uint32_t marker;
} FACTNotificationMarker;

typedef struct FACTNotificationSoundBank
{
	FACTSoundBank *pSoundBank;
} FACTNotificationSoundBank;

typedef struct FACTNotificationWaveBank
{
	FACTWaveBank *pWaveBank;
} FACTNotificationWaveBank;

typedef struct FACTNotificationVariable
{
	uint16_t cueIndex;
	FACTSoundBank *pSoundBank;
	FACTCue *pCue;
	uint16_t variableIndex;
	float variableValue;
	int32_t local;
} FACTNotificationVariable;

typedef struct FACTNotificationGUI
{
	uint32_t reserved;
} FACTNotificationGUI;

typedef struct FACTNotificationWave
{
	FACTWaveBank *pWaveBank;
	uint16_t waveIndex;
	uint16_t cueIndex;
	FACTSoundBank *pSoundBank;
	FACTCue *pCue;
	FACTWave *pWave;
} FACTNotificationWave;

struct FACTNotification
{
	uint8_t type;
	int32_t timeStamp;
	void *pvContext;
	FAUDIONAMELESS union
	{
		FACTNotificationCue cue;
		FACTNotificationMarker marker;
		FACTNotificationSoundBank soundBank;
		FACTNotificationWaveBank waveBank;
		FACTNotificationVariable variable;
		FACTNotificationGUI gui;
		FACTNotificationWave wave;
	};
};

#pragma pack(pop)

/* Constants */

#define FACT_CONTENT_VERSION 46

static const uint32_t FACT_FLAG_MANAGEDATA =		0x00000001;

static const uint32_t FACT_FLAG_STOP_RELEASE =		0x00000000;
static const uint32_t FACT_FLAG_STOP_IMMEDIATE =	0x00000001;

static const uint32_t FACT_FLAG_BACKGROUND_MUSIC =	0x00000002;
static const uint32_t FACT_FLAG_UNITS_MS =		0x00000004;
static const uint32_t FACT_FLAG_UNITS_SAMPLES =		0x00000008;

static const uint32_t FACT_STATE_CREATED =		0x00000001;
static const uint32_t FACT_STATE_PREPARING =		0x00000002;
static const uint32_t FACT_STATE_PREPARED =		0x00000004;
static const uint32_t FACT_STATE_PLAYING =		0x00000008;
static const uint32_t FACT_STATE_STOPPING =		0x00000010;
static const uint32_t FACT_STATE_STOPPED =		0x00000020;
static const uint32_t FACT_STATE_PAUSED =		0x00000040;
static const uint32_t FACT_STATE_INUSE =		0x00000080;
static const uint32_t FACT_STATE_PREPAREFAILED =	0x80000000;

static const int16_t FACTPITCH_MIN =		-1200;
static const int16_t FACTPITCH_MAX =		 1200;
static const int16_t FACTPITCH_MIN_TOTAL =	-2400;
static const int16_t FACTPITCH_MAX_TOTAL =	 2400;

static const float FACTVOLUME_MIN = 0.0f;
static const float FACTVOLUME_MAX = 16777216.0f;

static const uint16_t FACTINDEX_INVALID =		0xFFFF;
static const uint16_t FACTVARIABLEINDEX_INVALID =	0xFFFF;
static const uint16_t FACTCATEGORY_INVALID =		0xFFFF;

static const uint8_t FACTNOTIFICATIONTYPE_CUEPREPARED =				1;
static const uint8_t FACTNOTIFICATIONTYPE_CUEPLAY =				2;
static const uint8_t FACTNOTIFICATIONTYPE_CUESTOP =				3;
static const uint8_t FACTNOTIFICATIONTYPE_CUEDESTROYED =			4;
static const uint8_t FACTNOTIFICATIONTYPE_MARKER =				5;
static const uint8_t FACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED =			6;
static const uint8_t FACTNOTIFICATIONTYPE_WAVEBANKDESTROYED =			7;
static const uint8_t FACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED =		8;
static const uint8_t FACTNOTIFICATIONTYPE_GLOBALVARIABLECHANGED =		9;
static const uint8_t FACTNOTIFICATIONTYPE_GUICONNECTED =			10;
static const uint8_t FACTNOTIFICATIONTYPE_GUIDISCONNECTED =			11;
static const uint8_t FACTNOTIFICATIONTYPE_WAVEPREPARED =			12;
static const uint8_t FACTNOTIFICATIONTYPE_WAVEPLAY =				13;
static const uint8_t FACTNOTIFICATIONTYPE_WAVESTOP =				14;
static const uint8_t FACTNOTIFICATIONTYPE_WAVELOOPED =				15;
static const uint8_t FACTNOTIFICATIONTYPE_WAVEDESTROYED =			16;
static const uint8_t FACTNOTIFICATIONTYPE_WAVEBANKPREPARED =			17;
static const uint8_t FACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT =	18;

static const uint8_t FACT_FLAG_NOTIFICATION_PERSIST = 0x01;

#define FACT_ENGINE_LOOKAHEAD_DEFAULT 250

#define FACT_MAX_WMA_AVG_BYTES_PER_SEC_ENTRIES 7
static const uint32_t aWMAAvgBytesPerSec[] =
{
	12000,
	24000,
	4000,
	6000,
	8000,
	20000,
	2500
};

#define FACT_MAX_WMA_BLOCK_ALIGN_ENTRIES 17
static const uint32_t aWMABlockAlign[] =
{
	929,
	1487,
	1280,
	2230,
	8917,
	8192,
	4459,
	5945,
	2304,
	1536,
	1485,
	1008,
	2731,
	4096,
	6827,
	5462,
	1280
};

/* AudioEngine Interface */

FACTAPI uint32_t FACTCreateEngine(
	uint32_t dwCreationFlags,
	FACTAudioEngine **ppEngine
);

/* See "extensions/CustomAllocatorEXT.txt" for more details. */
FACTAPI uint32_t FACTCreateEngineWithCustomAllocatorEXT(
	uint32_t dwCreationFlags,
	FACTAudioEngine **ppEngine,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
);

FACTAPI uint32_t FACTAudioEngine_AddRef(FACTAudioEngine *pEngine);

FACTAPI uint32_t FACTAudioEngine_Release(FACTAudioEngine *pEngine);

/* FIXME: QueryInterface? Or just ignore COM garbage... -flibit */

FACTAPI uint32_t FACTAudioEngine_GetRendererCount(
	FACTAudioEngine *pEngine,
	uint16_t *pnRendererCount
);

FACTAPI uint32_t FACTAudioEngine_GetRendererDetails(
	FACTAudioEngine *pEngine,
	uint16_t nRendererIndex,
	FACTRendererDetails *pRendererDetails
);

FACTAPI uint32_t FACTAudioEngine_GetFinalMixFormat(
	FACTAudioEngine *pEngine,
	FAudioWaveFormatExtensible *pFinalMixFormat
);

FACTAPI uint32_t FACTAudioEngine_Initialize(
	FACTAudioEngine *pEngine,
	const FACTRuntimeParameters *pParams
);

FACTAPI uint32_t FACTAudioEngine_ShutDown(FACTAudioEngine *pEngine);

FACTAPI uint32_t FACTAudioEngine_DoWork(FACTAudioEngine *pEngine);

FACTAPI uint32_t FACTAudioEngine_CreateSoundBank(
	FACTAudioEngine *pEngine,
	const void *pvBuffer,
	uint32_t dwSize,
	uint32_t dwFlags,
	uint32_t dwAllocAttributes,
	FACTSoundBank **ppSoundBank
);

FACTAPI uint32_t FACTAudioEngine_CreateInMemoryWaveBank(
	FACTAudioEngine *pEngine,
	const void *pvBuffer,
	uint32_t dwSize,
	uint32_t dwFlags,
	uint32_t dwAllocAttributes,
	FACTWaveBank **ppWaveBank
);

FACTAPI uint32_t FACTAudioEngine_CreateStreamingWaveBank(
	FACTAudioEngine *pEngine,
	const FACTStreamingParameters *pParms,
	FACTWaveBank **ppWaveBank
);

FACTAPI uint32_t FACTAudioEngine_PrepareWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	const char *szWavePath,
	uint32_t wStreamingPacketSize,
	uint32_t dwAlignment,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
);

FACTAPI uint32_t FACTAudioEngine_PrepareInMemoryWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	FACTWaveBankEntry entry,
	uint32_t *pdwSeekTable, /* Optional! */
	uint8_t *pbWaveData,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
);

FACTAPI uint32_t FACTAudioEngine_PrepareStreamingWave(
	FACTAudioEngine *pEngine,
	uint32_t dwFlags,
	FACTWaveBankEntry entry,
	FACTStreamingParameters streamingParams,
	uint32_t dwAlignment,
	uint32_t *pdwSeekTable, /* Optional! */
	uint8_t *pbWaveData, /* ABI bug, do not use! */
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
);

FACTAPI uint32_t FACTAudioEngine_RegisterNotification(
	FACTAudioEngine *pEngine,
	const FACTNotificationDescription *pNotificationDescription
);

FACTAPI uint32_t FACTAudioEngine_UnRegisterNotification(
	FACTAudioEngine *pEngine,
	const FACTNotificationDescription *pNotificationDescription
);

FACTAPI uint16_t FACTAudioEngine_GetCategory(
	FACTAudioEngine *pEngine,
	const char *szFriendlyName
);

FACTAPI uint32_t FACTAudioEngine_Stop(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	uint32_t dwFlags
);

FACTAPI uint32_t FACTAudioEngine_SetVolume(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	float volume
);

FACTAPI uint32_t FACTAudioEngine_Pause(
	FACTAudioEngine *pEngine,
	uint16_t nCategory,
	int32_t fPause
);

FACTAPI uint16_t FACTAudioEngine_GetGlobalVariableIndex(
	FACTAudioEngine *pEngine,
	const char *szFriendlyName
);

FACTAPI uint32_t FACTAudioEngine_SetGlobalVariable(
	FACTAudioEngine *pEngine,
	uint16_t nIndex,
	float nValue
);

FACTAPI uint32_t FACTAudioEngine_GetGlobalVariable(
	FACTAudioEngine *pEngine,
	uint16_t nIndex,
	float *pnValue
);

/* SoundBank Interface */

FACTAPI uint16_t FACTSoundBank_GetCueIndex(
	FACTSoundBank *pSoundBank,
	const char *szFriendlyName
);

FACTAPI uint32_t FACTSoundBank_GetNumCues(
	FACTSoundBank *pSoundBank,
	uint16_t *pnNumCues
);

FACTAPI uint32_t FACTSoundBank_GetCueProperties(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	FACTCueProperties *pProperties
);

FACTAPI uint32_t FACTSoundBank_Prepare(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	uint32_t dwFlags,
	int32_t timeOffset,
	FACTCue** ppCue
);

FACTAPI uint32_t FACTSoundBank_Play(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	uint32_t dwFlags,
	int32_t timeOffset,
	FACTCue** ppCue /* Optional! */
);

#ifndef F3DAUDIO_DSP_SETTINGS_DECL
#define F3DAUDIO_DSP_SETTINGS_DECL
typedef struct F3DAUDIO_DSP_SETTINGS F3DAUDIO_DSP_SETTINGS;
#endif /* F3DAUDIO_DSP_SETTINGS_DECL */

FACTAPI uint32_t FACTSoundBank_Play3D(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	uint32_t dwFlags,
	int32_t timeOffset,
	F3DAUDIO_DSP_SETTINGS *pDSPSettings,
	FACTCue** ppCue /* Optional! */
);

FACTAPI uint32_t FACTSoundBank_Stop(
	FACTSoundBank *pSoundBank,
	uint16_t nCueIndex,
	uint32_t dwFlags
);

FACTAPI uint32_t FACTSoundBank_Destroy(FACTSoundBank *pSoundBank);

FACTAPI uint32_t FACTSoundBank_GetState(
	FACTSoundBank *pSoundBank,
	uint32_t *pdwState
);

/* WaveBank Interface */

FACTAPI uint32_t FACTWaveBank_Destroy(FACTWaveBank *pWaveBank);

FACTAPI uint32_t FACTWaveBank_GetState(
	FACTWaveBank *pWaveBank,
	uint32_t *pdwState
);

FACTAPI uint32_t FACTWaveBank_GetNumWaves(
	FACTWaveBank *pWaveBank,
	uint16_t *pnNumWaves
);

FACTAPI uint16_t FACTWaveBank_GetWaveIndex(
	FACTWaveBank *pWaveBank,
	const char *szFriendlyName
);

FACTAPI uint32_t FACTWaveBank_GetWaveProperties(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	FACTWaveProperties *pWaveProperties
);

FACTAPI uint32_t FACTWaveBank_Prepare(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
);

FACTAPI uint32_t FACTWaveBank_Play(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags,
	uint32_t dwPlayOffset,
	uint8_t nLoopCount,
	FACTWave **ppWave
);

FACTAPI uint32_t FACTWaveBank_Stop(
	FACTWaveBank *pWaveBank,
	uint16_t nWaveIndex,
	uint32_t dwFlags
);

/* Wave Interface */

FACTAPI uint32_t FACTWave_Destroy(FACTWave *pWave);

FACTAPI uint32_t FACTWave_Play(FACTWave *pWave);

FACTAPI uint32_t FACTWave_Stop(FACTWave *pWave, uint32_t dwFlags);

FACTAPI uint32_t FACTWave_Pause(FACTWave *pWave, int32_t fPause);

FACTAPI uint32_t FACTWave_GetState(FACTWave *pWave, uint32_t *pdwState);

FACTAPI uint32_t FACTWave_SetPitch(FACTWave *pWave, int16_t pitch);

FACTAPI uint32_t FACTWave_SetVolume(FACTWave *pWave, float volume);

FACTAPI uint32_t FACTWave_SetMatrixCoefficients(
	FACTWave *pWave,
	uint32_t uSrcChannelCount,
	uint32_t uDstChannelCount,
	float *pMatrixCoefficients
);

FACTAPI uint32_t FACTWave_GetProperties(
	FACTWave *pWave,
	FACTWaveInstanceProperties *pProperties
);

/* Cue Interface */

FACTAPI uint32_t FACTCue_Destroy(FACTCue *pCue);

FACTAPI uint32_t FACTCue_Play(FACTCue *pCue);

FACTAPI uint32_t FACTCue_Stop(FACTCue *pCue, uint32_t dwFlags);

FACTAPI uint32_t FACTCue_GetState(FACTCue *pCue, uint32_t *pdwState);

FACTAPI uint32_t FACTCue_SetMatrixCoefficients(
	FACTCue *pCue,
	uint32_t uSrcChannelCount,
	uint32_t uDstChannelCount,
	float *pMatrixCoefficients
);

FACTAPI uint16_t FACTCue_GetVariableIndex(
	FACTCue *pCue,
	const char *szFriendlyName
);

FACTAPI uint32_t FACTCue_SetVariable(
	FACTCue *pCue,
	uint16_t nIndex,
	float nValue
);

FACTAPI uint32_t FACTCue_GetVariable(
	FACTCue *pCue,
	uint16_t nIndex,
	float *nValue
);

FACTAPI uint32_t FACTCue_Pause(FACTCue *pCue, int32_t fPause);

FACTAPI uint32_t FACTCue_GetProperties(
	FACTCue *pCue,
	FACTCueInstanceProperties **ppProperties
);

FACTAPI uint32_t FACTCue_SetOutputVoices(
	FACTCue *pCue,
	const FAudioVoiceSends *pSendList /* Optional! */
);

FACTAPI uint32_t FACTCue_SetOutputVoiceMatrix(
	FACTCue *pCue,
	const FAudioVoice *pDestinationVoice, /* Optional! */
	uint32_t SourceChannels,
	uint32_t DestinationChannels,
	const float *pLevelMatrix /* SourceChannels * DestinationChannels */
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FACT_H */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
