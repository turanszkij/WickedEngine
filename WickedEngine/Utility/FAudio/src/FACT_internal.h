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

#include "FACT.h"
#include "FACT3D.h"
#include "FAudio_internal.h"

/* Internal AudioEngine Types */

typedef struct FACTAudioCategory
{
	uint8_t instanceLimit;
	uint16_t fadeInMS;
	uint16_t fadeOutMS;
	uint8_t maxInstanceBehavior;
	int16_t parentCategory;
	float volume;
	uint8_t visibility;

	uint8_t instanceCount;
	float currentVolume;
} FACTAudioCategory;

typedef struct FACTVariable
{
	uint8_t accessibility;
	float initialValue;
	float minValue;
	float maxValue;
} FACTVariable;

typedef struct FACTRPCPoint
{
	float x;
	float y;
	uint8_t type;
} FACTRPCPoint;

typedef enum FACTRPCParameter
{
	RPC_PARAMETER_VOLUME,
	RPC_PARAMETER_PITCH,
	RPC_PARAMETER_REVERBSEND,
	RPC_PARAMETER_FILTERFREQUENCY,
	RPC_PARAMETER_FILTERQFACTOR,
	RPC_PARAMETER_COUNT /* If >=, DSP Parameter! */
} FACTRPCParameter;

typedef struct FACTRPC
{
	uint16_t variable;
	uint8_t pointCount;
	uint16_t parameter;
	FACTRPCPoint *points;
} FACTRPC;

typedef struct FACTDSPParameter
{
	uint8_t type;
	float value;
	float minVal;
	float maxVal;
	uint16_t unknown;
} FACTDSPParameter;

typedef struct FACTDSPPreset
{
	uint8_t accessibility;
	uint16_t parameterCount;
	FACTDSPParameter *parameters;
} FACTDSPPreset;

typedef enum FACTNoticationsFlags
{
	NOTIFY_CUEPREPARED           = 0x00000001,
	NOTIFY_CUEPLAY               = 0x00000002,
	NOTIFY_CUESTOP               = 0x00000004,
	NOTIFY_CUEDESTROY            = 0x00000008,
	NOTIFY_MARKER                = 0x00000010,
	NOTIFY_SOUNDBANKDESTROY      = 0x00000020,
	NOTIFY_WAVEBANKDESTROY       = 0x00000040,
	NOTIFY_LOCALVARIABLECHANGED  = 0x00000080,
	NOTIFY_GLOBALVARIABLECHANGED = 0x00000100,
	NOTIFY_GUICONNECTED          = 0x00000200,
	NOTIFY_GUIDISCONNECTED       = 0x00000400,
	NOTIFY_WAVEPREPARED          = 0x00000800,
	NOTIFY_WAVEPLAY              = 0x00001000,
	NOTIFY_WAVESTOP              = 0x00002000,
	NOTIFY_WAVELOOPED            = 0x00004000,
	NOTIFY_WAVEDESTROY           = 0x00008000,
	NOTIFY_WAVEBANKPREPARED      = 0x00010000,
	NOTIFY_WAVEBANKSTREAMING_INVALIDCONTENT = 0x00020000
} FACTNoticationsFlags;

/* Internal SoundBank Types */

typedef enum
{
	FACTEVENT_STOP =				0,
	FACTEVENT_PLAYWAVE =				1,
	FACTEVENT_PLAYWAVETRACKVARIATION =		3,
	FACTEVENT_PLAYWAVEEFFECTVARIATION =		4,
	FACTEVENT_PLAYWAVETRACKEFFECTVARIATION =	6,
	FACTEVENT_PITCH =				7,
	FACTEVENT_VOLUME =				8,
	FACTEVENT_MARKER =				9,
	FACTEVENT_PITCHREPEATING =			16,
	FACTEVENT_VOLUMEREPEATING =			17,
	FACTEVENT_MARKERREPEATING =			18
} FACTEventType;

typedef struct FACTEvent
{
	uint16_t type;
	uint16_t timestamp;
	uint16_t randomOffset;
	FAUDIONAMELESS union
	{
		/* Play Wave Event */
		struct
		{
			uint8_t flags;
			uint8_t loopCount;
			uint16_t position;
			uint16_t angle;

			/* Track Variation */
			uint8_t isComplex;
			FAUDIONAMELESS union
			{
				struct
				{
					uint16_t track;
					uint8_t wavebank;
				} simple;
				struct
				{
					uint16_t variation;
					uint16_t trackCount;
					uint16_t *tracks;
					uint8_t *wavebanks;
					uint8_t *weights;
				} complex;
			};

			/* Effect Variation */
			int16_t minPitch;
			int16_t maxPitch;
			float minVolume;
			float maxVolume;
			float minFrequency;
			float maxFrequency;
			float minQFactor;
			float maxQFactor;
			uint16_t variationFlags;
		} wave;
		/* Set Pitch/Volume Event */
		struct
		{
			uint8_t settings;
			uint16_t repeats;
			uint16_t frequency;
			FAUDIONAMELESS union
			{
				struct
				{
					float initialValue;
					float initialSlope;
					float slopeDelta;
					uint16_t duration;
				} ramp;
				struct
				{
					uint8_t flags;
					float value1;
					float value2;
				} equation;
			};
		} value;
		/* Stop Event */
		struct
		{
			uint8_t flags;
		} stop;
		/* Marker Event */
		struct
		{
			uint32_t marker;
			uint16_t repeats;
			uint16_t frequency;
		} marker;
	};
} FACTEvent;

typedef struct FACTTrack
{
	uint32_t code;

	float volume;
	uint8_t filter;
	uint8_t qfactor;
	uint16_t frequency;

	uint8_t rpcCodeCount;
	uint32_t *rpcCodes;

	uint8_t eventCount;
	FACTEvent *events;
} FACTTrack;

typedef struct FACTSound
{
	uint8_t flags;
	uint16_t category;
	float volume;
	int16_t pitch;
	uint8_t priority;

	uint8_t trackCount;
	uint8_t rpcCodeCount;
	uint8_t dspCodeCount;

	FACTTrack *tracks;
	uint32_t *rpcCodes;
	uint32_t *dspCodes;
} FACTSound;

typedef struct FACTCueData
{
	uint8_t flags;
	uint32_t sbCode;
	uint32_t transitionOffset;
	uint8_t instanceLimit;
	uint16_t fadeInMS;
	uint16_t fadeOutMS;
	uint8_t maxInstanceBehavior;
	uint8_t instanceCount;
} FACTCueData;

typedef struct FACTVariation
{
	FAUDIONAMELESS union
	{
		struct
		{
			uint16_t track;
			uint8_t wavebank;
		} simple;
		uint32_t soundCode;
	};
	float minWeight;
	float maxWeight;
	uint32_t linger;
} FACTVariation;

typedef struct FACTVariationTable
{
	uint8_t flags;
	int16_t variable;
	uint8_t isComplex;

	uint16_t entryCount;
	FACTVariation *entries;
} FACTVariationTable;

typedef struct FACTTransition
{
	int32_t soundCode;
	uint32_t srcMarkerMin;
	uint32_t srcMarkerMax;
	uint32_t dstMarkerMin;
	uint32_t dstMarkerMax;
	uint16_t fadeIn;
	uint16_t fadeOut;
	uint16_t flags;
} FACTTransition;

typedef struct FACTTransitionTable
{
	uint32_t entryCount;
	FACTTransition *entries;
} FACTTransitionTable;

/* Internal WaveBank Types */

typedef struct FACTSeekTable
{
	uint32_t entryCount;
	uint32_t *entries;
} FACTSeekTable;

/* Internal Cue Types */

typedef struct FACTInstanceRPCData
{
	float rpcVolume;
	float rpcPitch;
	float rpcReverbSend;
	float rpcFilterFreq;
	float rpcFilterQFactor;
} FACTInstanceRPCData;

typedef struct FACTEventInstance
{
	uint32_t timestamp;
	uint16_t loopCount;
	uint8_t finished;
	FAUDIONAMELESS union
	{
		float value;
		uint32_t valuei;
	};
} FACTEventInstance;

typedef struct FACTTrackInstance
{
	/* Tracks which events have fired */
	FACTEventInstance *events;

	/* RPC instance data */
	FACTInstanceRPCData rpcData;

	/* SetPitch/SetVolume data */
	float evtPitch;
	float evtVolume;

	/* Wave playback */
	struct
	{
		FACTWave *wave;
		float baseVolume;
		int16_t basePitch;
		float baseQFactor;
		float baseFrequency;
	} activeWave, upcomingWave;
	FACTEvent *waveEvt;
	FACTEventInstance *waveEvtInst;
} FACTTrackInstance;

typedef struct FACTSoundInstance
{
	/* Base Sound reference */
	FACTSound *sound;

	/* Per-instance track information */
	FACTTrackInstance *tracks;

	/* RPC instance data */
	FACTInstanceRPCData rpcData;

	/* Fade data */
	uint32_t fadeStart;
	uint16_t fadeTarget;
	uint8_t fadeType; /* In (1), Out (2), Release RPC (3) */

	/* Engine references */
	FACTCue *parentCue;
} FACTSoundInstance;

/* Internal Wave Types */

typedef struct FACTWaveCallback
{
	FAudioVoiceCallback callback;
	FACTWave *wave;
} FACTWaveCallback;

/* Public XACT Types */

struct FACTAudioEngine
{
	uint32_t refcount;
	FACTNotificationCallback notificationCallback;
	FACTReadFileCallback pReadFile;
	FACTGetOverlappedResultCallback pGetOverlappedResult;

	uint16_t categoryCount;
	uint16_t variableCount;
	uint16_t rpcCount;
	uint16_t dspPresetCount;
	uint16_t dspParameterCount;

	char **categoryNames;
	char **variableNames;
	uint32_t *rpcCodes;
	uint32_t *dspPresetCodes;

	FACTAudioCategory *categories;
	FACTVariable *variables;
	FACTRPC *rpcs;
	FACTDSPPreset *dspPresets;

	/* Engine references */
	LinkedList *sbList;
	LinkedList *wbList;
	FAudioMutex sbLock;
	FAudioMutex wbLock;
	float *globalVariableValues;

	/* FAudio references */
	FAudio *audio;
	FAudioMasteringVoice *master;
	FAudioSubmixVoice *reverbVoice;

	/* Engine thread */
	FAudioThread apiThread;
	FAudioMutex apiLock;
	uint8_t initialized;

	/* Allocator callbacks */
	FAudioMallocFunc pMalloc;
	FAudioFreeFunc pFree;
	FAudioReallocFunc pRealloc;

	/* Peristent Notifications */
	FACTNoticationsFlags notifications;
	void *cue_context;
	void *sb_context;
	void *wb_context;
	void *wave_context;
	LinkedList *wb_notifications_list;

	/* Settings handle */
	void *settings;
};

struct FACTSoundBank
{
	/* Engine references */
	FACTAudioEngine *parentEngine;
	FACTCue *cueList;
	uint8_t notifyOnDestroy;
	void *usercontext;

	/* Array sizes */
	uint16_t cueCount;
	uint8_t wavebankCount;
	uint16_t soundCount;
	uint16_t variationCount;
	uint16_t transitionCount;

	/* Strings, strings everywhere! */
	char **wavebankNames;
	char **cueNames;

	/* Actual SoundBank information */
	char *name;
	FACTCueData *cues;
	FACTSound *sounds;
	uint32_t *soundCodes;
	FACTVariationTable *variations;
	uint32_t *variationCodes;
	FACTTransitionTable *transitions;
	uint32_t *transitionCodes;
};

struct FACTWaveBank
{
	/* Engine references */
	FACTAudioEngine *parentEngine;
	LinkedList *waveList;
	FAudioMutex waveLock;
	uint8_t notifyOnDestroy;
	void *usercontext;

	/* Actual WaveBank information */
	char *name;
	uint32_t entryCount;
	FACTWaveBankEntry *entries;
	uint32_t *entryRefs;
	FACTSeekTable *seekTables;
	char *waveBankNames;

	/* I/O information */
	uint32_t packetSize;
	uint16_t streaming;
	uint8_t *packetBuffer;
	uint32_t packetBufferLen;
	void* io;
};

struct FACTWave
{
	/* Engine references */
	FACTWaveBank *parentBank;
	FACTCue *parentCue;
	uint16_t index;
	uint8_t notifyOnDestroy;
	void *usercontext;

	/* Playback */
	uint32_t state;
	float volume;
	int16_t pitch;
	uint8_t loopCount;

	/* Stream data */
	uint32_t streamSize;
	uint32_t streamOffset;
	uint8_t *streamCache;

	/* FAudio references */
	uint16_t srcChannels;
	FAudioSourceVoice *voice;
	FACTWaveCallback callback;
};

struct FACTCue
{
	/* Engine references */
	FACTSoundBank *parentBank;
	FACTCue *next;
	uint8_t managed;
	uint16_t index;
	uint8_t notifyOnDestroy;
	void *usercontext;

	/* Sound data */
	FACTCueData *data;
	FAUDIONAMELESS union
	{
		FACTVariationTable *variation;

		/* This is only used in scenarios where there is only one
		 * Sound; XACT does not generate variation tables for
		 * Cues with only one Sound.
		 */
		FACTSound *sound;
	};

	/* Instance data */
	float *variableValues;
	float interactive;

	/* Playback */
	uint32_t state;
	FACTWave *simpleWave;
	FACTSoundInstance *playingSound;
	FACTVariation *playingVariation;
	uint32_t maxRpcReleaseTime;

	/* 3D Data */
	uint8_t active3D;
	uint32_t srcChannels;
	uint32_t dstChannels;
	float matrixCoefficients[2 * 8]; /* Stereo input, 7.1 output */

	/* Timer */
	uint32_t start;
	uint32_t elapsed;
};

/* Internal functions */

void FACT_INTERNAL_GetNextWave(
	FACTCue *cue,
	FACTSound *sound,
	FACTTrack *track,
	FACTTrackInstance *trackInst,
	FACTEvent *evt,
	FACTEventInstance *evtInst
);
uint8_t FACT_INTERNAL_CreateSound(FACTCue *cue, uint16_t fadeInMS);
void FACT_INTERNAL_DestroySound(FACTSoundInstance *sound);
void FACT_INTERNAL_BeginFadeOut(FACTSoundInstance *sound, uint16_t fadeOutMS);
void FACT_INTERNAL_BeginReleaseRPC(FACTSoundInstance *sound, uint16_t releaseMS);

void FACT_INTERNAL_SendCueNotification(FACTCue *cue, FACTNoticationsFlags flag, uint8_t type);

/* RPC Helper Functions */

FACTRPC* FACT_INTERNAL_GetRPC(FACTAudioEngine *engine, uint32_t code);

/* FACT Thread */

int32_t FAUDIOCALL FACT_INTERNAL_APIThread(void* enginePtr);

/* FAudio callbacks */

void FACT_INTERNAL_OnBufferEnd(FAudioVoiceCallback *callback, void* pContext);
void FACT_INTERNAL_OnStreamEnd(FAudioVoiceCallback *callback);

/* FAudioIOStream functions */

int32_t FACTCALL FACT_INTERNAL_DefaultReadFile(
	void *hFile,
	void *buffer,
	uint32_t nNumberOfBytesToRead,
	uint32_t *lpNumberOfBytesRead,
	FACTOverlapped *lpOverlapped
);

int32_t FACTCALL FACT_INTERNAL_DefaultGetOverlappedResult(
	void *hFile,
	FACTOverlapped *lpOverlapped,
	uint32_t *lpNumberOfBytesTransferred,
	int32_t bWait
);

/* Parsing functions */

uint32_t FACT_INTERNAL_ParseAudioEngine(
	FACTAudioEngine *pEngine,
	const FACTRuntimeParameters *pParams
);
uint32_t FACT_INTERNAL_ParseSoundBank(
	FACTAudioEngine *pEngine,
	const void *pvBuffer,
	uint32_t dwSize,
	FACTSoundBank **ppSoundBank
);
uint32_t FACT_INTERNAL_ParseWaveBank(
	FACTAudioEngine *pEngine,
	void* io,
	uint32_t offset,
	uint32_t packetSize,
	FACTReadFileCallback pRead,
	FACTGetOverlappedResultCallback pOverlap,
	uint16_t isStreaming,
	FACTWaveBank **ppWaveBank
);

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
