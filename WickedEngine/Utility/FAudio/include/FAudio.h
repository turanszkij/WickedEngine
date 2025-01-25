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

#ifndef FAUDIO_H
#define FAUDIO_H

#ifdef _WIN32
#define FAUDIOAPI __declspec(dllexport)
#define FAUDIOCALL __cdecl
#else
#define FAUDIOAPI
#define FAUDIOCALL
#endif

#ifdef _MSC_VER
#define FAUDIODEPRECATED(msg) __declspec(deprecated(msg))
#else
#define FAUDIODEPRECATED(msg) __attribute__((deprecated(msg)))
#endif

/* -Wpedantic nameless union/struct silencing */
#ifndef FAUDIONAMELESS
#ifdef __GNUC__
#define FAUDIONAMELESS __extension__
#else
#define FAUDIONAMELESS
#endif /* __GNUC__ */
#endif /* FAUDIONAMELESS */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Type Declarations */

typedef struct FAudio FAudio;
typedef struct FAudioVoice FAudioVoice;
typedef FAudioVoice FAudioSourceVoice;
typedef FAudioVoice FAudioSubmixVoice;
typedef FAudioVoice FAudioMasteringVoice;
typedef struct FAudioEngineCallback FAudioEngineCallback;
typedef struct FAudioVoiceCallback FAudioVoiceCallback;

/* Enumerations */

typedef enum FAudioDeviceRole
{
	FAudioNotDefaultDevice =		0x0,
	FAudioDefaultConsoleDevice =		0x1,
	FAudioDefaultMultimediaDevice =		0x2,
	FAudioDefaultCommunicationsDevice =	0x4,
	FAudioDefaultGameDevice =		0x8,
	FAudioGlobalDefaultDevice =		0xF,
	FAudioInvalidDeviceRole = ~FAudioGlobalDefaultDevice
} FAudioDeviceRole;

typedef enum FAudioFilterType
{
	FAudioLowPassFilter,
	FAudioBandPassFilter,
	FAudioHighPassFilter,
	FAudioNotchFilter
} FAudioFilterType;

typedef enum FAudioStreamCategory
{
	FAudioStreamCategory_Other,
	FAudioStreamCategory_ForegroundOnlyMedia,
	FAudioStreamCategory_BackgroundCapableMedia,
	FAudioStreamCategory_Communications,
	FAudioStreamCategory_Alerts,
	FAudioStreamCategory_SoundEffects,
	FAudioStreamCategory_GameEffects,
	FAudioStreamCategory_GameMedia,
	FAudioStreamCategory_GameChat,
	FAudioStreamCategory_Speech,
	FAudioStreamCategory_Movie,
	FAudioStreamCategory_Media
} FAudioStreamCategory;

/* FIXME: The original enum violates ISO C and is platform specific anyway... */
typedef uint32_t FAudioProcessor;
#define FAUDIO_DEFAULT_PROCESSOR 0xFFFFFFFF

/* Structures */

#pragma pack(push, 1)

typedef struct FAudioGUID
{
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} FAudioGUID;

/* See MSDN:
 * https://msdn.microsoft.com/en-us/library/windows/desktop/dd390970%28v=vs.85%29.aspx
 */
typedef struct FAudioWaveFormatEx
{
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	uint16_t cbSize;
} FAudioWaveFormatEx;

/* See MSDN:
 * https://msdn.microsoft.com/en-us/library/windows/desktop/dd390971(v=vs.85).aspx
 */
typedef struct FAudioWaveFormatExtensible
{
	FAudioWaveFormatEx Format;
	union
	{
		uint16_t wValidBitsPerSample;
		uint16_t wSamplesPerBlock;
		uint16_t wReserved;
	} Samples;
	uint32_t dwChannelMask;
	FAudioGUID SubFormat;
} FAudioWaveFormatExtensible;

typedef struct FAudioADPCMCoefSet
{
	int16_t iCoef1;
	int16_t iCoef2;
} FAudioADPCMCoefSet;

typedef struct FAudioADPCMWaveFormat
{
	FAudioWaveFormatEx wfx;
	uint16_t wSamplesPerBlock;
	uint16_t wNumCoef;

	/* MSVC warns on empty arrays in structs */
	#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4200)
	#endif

	FAudioADPCMCoefSet aCoef[];
	/* MSADPCM has 7 coefficient pairs:
	 * {
	 *	{ 256,    0 },
	 *	{ 512, -256 },
	 *	{   0,    0 },
	 *	{ 192,   64 },
	 *	{ 240,    0 },
	 *	{ 460, -208 },
	 *	{ 392, -232 }
	 * }
	 */

	#ifdef _MSC_VER
	#pragma warning(pop)
	#endif
} FAudioADPCMWaveFormat;

typedef struct FAudioDeviceDetails
{
	int16_t DeviceID[256]; /* Win32 wchar_t */
	int16_t DisplayName[256]; /* Win32 wchar_t */
	FAudioDeviceRole Role;
	FAudioWaveFormatExtensible OutputFormat;
} FAudioDeviceDetails;

typedef struct FAudioVoiceDetails
{
	uint32_t CreationFlags;
	uint32_t ActiveFlags;
	uint32_t InputChannels;
	uint32_t InputSampleRate;
} FAudioVoiceDetails;

typedef struct FAudioSendDescriptor
{
	uint32_t Flags; /* 0 or FAUDIO_SEND_USEFILTER */
	FAudioVoice *pOutputVoice;
} FAudioSendDescriptor;

typedef struct FAudioVoiceSends
{
	uint32_t SendCount;
	FAudioSendDescriptor *pSends;
} FAudioVoiceSends;

#ifndef FAPO_DECL
#define FAPO_DECL
typedef struct FAPO FAPO;
#endif /* FAPO_DECL */

typedef struct FAudioEffectDescriptor
{
	FAPO *pEffect;
	int32_t InitialState; /* 1 - Enabled, 0 - Disabled */
	uint32_t OutputChannels;
} FAudioEffectDescriptor;

typedef struct FAudioEffectChain
{
	uint32_t EffectCount;
	FAudioEffectDescriptor *pEffectDescriptors;
} FAudioEffectChain;

typedef struct FAudioFilterParameters
{
	FAudioFilterType Type;
	float Frequency;	/* [0, FAUDIO_MAX_FILTER_FREQUENCY] */
	float OneOverQ;		/* [0, FAUDIO_MAX_FILTER_ONEOVERQ] */
} FAudioFilterParameters;

typedef struct FAudioFilterParametersEXT
{
	FAudioFilterType Type;
	float Frequency;	/* [0, FAUDIO_MAX_FILTER_FREQUENCY] */
	float OneOverQ;		/* [0, FAUDIO_MAX_FILTER_ONEOVERQ] */
	float WetDryMix;	/* [0, 1] */
} FAudioFilterParametersEXT;

typedef struct FAudioBuffer
{
	/* Either 0 or FAUDIO_END_OF_STREAM */
	uint32_t Flags;
	/* Pointer to wave data, memory block size.
	 * Note that pAudioData is not copied; FAudio reads directly from your
	 * pointer! This pointer must be valid until FAudio has finished using
	 * it, at which point an OnBufferEnd callback will be generated.
	 */
	uint32_t AudioBytes;
	const uint8_t *pAudioData;
	/* Play region, in sample frames. */
	uint32_t PlayBegin;
	uint32_t PlayLength;
	/* Loop region, in sample frames.
	 * This can be used to loop a subregion of the wave instead of looping
	 * the whole thing, i.e. if you have an intro/outro you can set these
	 * to loop the middle sections instead. If you don't need this, set both
	 * values to 0.
	 */
	uint32_t LoopBegin;
	uint32_t LoopLength;
	/* [0, FAUDIO_LOOP_INFINITE] */
	uint32_t LoopCount;
	/* This is sent to callbacks as pBufferContext */
	void *pContext;
} FAudioBuffer;

typedef struct FAudioBufferWMA
{
	const uint32_t *pDecodedPacketCumulativeBytes;
	uint32_t PacketCount;
} FAudioBufferWMA;

typedef struct FAudioVoiceState
{
	void *pCurrentBufferContext;
	uint32_t BuffersQueued;
	uint64_t SamplesPlayed;
} FAudioVoiceState;

typedef struct FAudioPerformanceData
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
	uint32_t ActiveResamplerCount;
	uint32_t ActiveMatrixMixCount;
	uint32_t ActiveXmaSourceVoices;
	uint32_t ActiveXmaStreams;
} FAudioPerformanceData;

typedef struct FAudioDebugConfiguration
{
	/* See FAUDIO_LOG_* */
	uint32_t TraceMask;
	uint32_t BreakMask;
	/* 0 or 1 */
	int32_t LogThreadID;
	int32_t LogFileline;
	int32_t LogFunctionName;
	int32_t LogTiming;
} FAudioDebugConfiguration;

#pragma pack(pop)

/* This ISN'T packed. Strictly speaking it wouldn't have mattered anyway but eh.
 * See https://github.com/microsoft/DirectXTK/issues/256
 */
typedef struct FAudioXMA2WaveFormatEx
{
	FAudioWaveFormatEx wfx;
	uint16_t wNumStreams;
	uint32_t dwChannelMask;
	uint32_t dwSamplesEncoded;
	uint32_t dwBytesPerBlock;
	uint32_t dwPlayBegin;
	uint32_t dwPlayLength;
	uint32_t dwLoopBegin;
	uint32_t dwLoopLength;
	uint8_t  bLoopCount;
	uint8_t  bEncoderVersion;
	uint16_t wBlockCount;
} FAudioXMA2WaveFormat;

/* Constants */

#define FAUDIO_E_OUT_OF_MEMORY		0x8007000e
#define FAUDIO_E_INVALID_ARG		0x80070057
#define FAUDIO_E_UNSUPPORTED_FORMAT	0x88890008
#define FAUDIO_E_INVALID_CALL		0x88960001
#define FAUDIO_E_DEVICE_INVALIDATED	0x88960004
#define FAPO_E_FORMAT_UNSUPPORTED	0x88970001

#define FAUDIO_MAX_BUFFER_BYTES		0x80000000
#define FAUDIO_MAX_QUEUED_BUFFERS	64
#define FAUDIO_MAX_AUDIO_CHANNELS	64
#define FAUDIO_MIN_SAMPLE_RATE		1000
#define FAUDIO_MAX_SAMPLE_RATE		200000
#define FAUDIO_MAX_VOLUME_LEVEL		16777216.0f
#define FAUDIO_MIN_FREQ_RATIO		(1.0f / 1024.0f)
#define FAUDIO_MAX_FREQ_RATIO		1024.0f
#define FAUDIO_DEFAULT_FREQ_RATIO	2.0f
#define FAUDIO_MAX_FILTER_ONEOVERQ	1.5f
#define FAUDIO_MAX_FILTER_FREQUENCY	1.0f
#define FAUDIO_MAX_LOOP_COUNT		254

#define FAUDIO_COMMIT_NOW		0
#define FAUDIO_COMMIT_ALL		0
#define FAUDIO_INVALID_OPSET		(uint32_t) (-1)
#define FAUDIO_NO_LOOP_REGION		0
#define FAUDIO_LOOP_INFINITE		255
#define FAUDIO_DEFAULT_CHANNELS		0
#define FAUDIO_DEFAULT_SAMPLERATE	0

#define FAUDIO_DEBUG_ENGINE		0x0001
#define FAUDIO_VOICE_NOPITCH		0x0002
#define FAUDIO_VOICE_NOSRC		0x0004
#define FAUDIO_VOICE_USEFILTER		0x0008
#define FAUDIO_VOICE_MUSIC		0x0010
#define FAUDIO_PLAY_TAILS		0x0020
#define FAUDIO_END_OF_STREAM		0x0040
#define FAUDIO_SEND_USEFILTER		0x0080
#define FAUDIO_VOICE_NOSAMPLESPLAYED	0x0100
#define FAUDIO_1024_QUANTUM		0x8000

#define FAUDIO_DEFAULT_FILTER_TYPE	FAudioLowPassFilter
#define FAUDIO_DEFAULT_FILTER_FREQUENCY	FAUDIO_MAX_FILTER_FREQUENCY
#define FAUDIO_DEFAULT_FILTER_ONEOVERQ	1.0f
#define FAUDIO_DEFAULT_FILTER_WETDRYMIX_EXT	1.0f

#define FAUDIO_LOG_ERRORS		0x0001
#define FAUDIO_LOG_WARNINGS		0x0002
#define FAUDIO_LOG_INFO			0x0004
#define FAUDIO_LOG_DETAIL		0x0008
#define FAUDIO_LOG_API_CALLS		0x0010
#define FAUDIO_LOG_FUNC_CALLS		0x0020
#define FAUDIO_LOG_TIMING		0x0040
#define FAUDIO_LOG_LOCKS		0x0080
#define FAUDIO_LOG_MEMORY		0x0100
#define FAUDIO_LOG_STREAMING		0x1000

#ifndef _SPEAKER_POSITIONS_
#define SPEAKER_FRONT_LEFT		0x00000001
#define SPEAKER_FRONT_RIGHT		0x00000002
#define SPEAKER_FRONT_CENTER		0x00000004
#define SPEAKER_LOW_FREQUENCY		0x00000008
#define SPEAKER_BACK_LEFT		0x00000010
#define SPEAKER_BACK_RIGHT		0x00000020
#define SPEAKER_FRONT_LEFT_OF_CENTER	0x00000040
#define SPEAKER_FRONT_RIGHT_OF_CENTER	0x00000080
#define SPEAKER_BACK_CENTER		0x00000100
#define SPEAKER_SIDE_LEFT		0x00000200
#define SPEAKER_SIDE_RIGHT		0x00000400
#define SPEAKER_TOP_CENTER		0x00000800
#define SPEAKER_TOP_FRONT_LEFT		0x00001000
#define SPEAKER_TOP_FRONT_CENTER	0x00002000
#define SPEAKER_TOP_FRONT_RIGHT		0x00004000
#define SPEAKER_TOP_BACK_LEFT		0x00008000
#define SPEAKER_TOP_BACK_CENTER		0x00010000
#define SPEAKER_TOP_BACK_RIGHT		0x00020000
#define _SPEAKER_POSITIONS_
#endif

#ifndef SPEAKER_MONO
#define SPEAKER_MONO	SPEAKER_FRONT_CENTER
#define SPEAKER_STEREO	(SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
#define SPEAKER_2POINT1 \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_LOW_FREQUENCY	)
#define SPEAKER_SURROUND \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_FRONT_CENTER	| \
		SPEAKER_BACK_CENTER	)
#define SPEAKER_QUAD \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_BACK_LEFT	| \
		SPEAKER_BACK_RIGHT	)
#define SPEAKER_4POINT1 \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_LOW_FREQUENCY	| \
		SPEAKER_BACK_LEFT	| \
		SPEAKER_BACK_RIGHT	)
#define SPEAKER_5POINT1 \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_FRONT_CENTER	| \
		SPEAKER_LOW_FREQUENCY	| \
		SPEAKER_BACK_LEFT	| \
		SPEAKER_BACK_RIGHT	)
#define SPEAKER_7POINT1 \
	(	SPEAKER_FRONT_LEFT		| \
		SPEAKER_FRONT_RIGHT		| \
		SPEAKER_FRONT_CENTER		| \
		SPEAKER_LOW_FREQUENCY		| \
		SPEAKER_BACK_LEFT		| \
		SPEAKER_BACK_RIGHT		| \
		SPEAKER_FRONT_LEFT_OF_CENTER	| \
		SPEAKER_FRONT_RIGHT_OF_CENTER	)
#define SPEAKER_5POINT1_SURROUND \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_FRONT_CENTER	| \
		SPEAKER_LOW_FREQUENCY	| \
		SPEAKER_SIDE_LEFT	| \
		SPEAKER_SIDE_RIGHT	)
#define SPEAKER_7POINT1_SURROUND \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_FRONT_CENTER	| \
		SPEAKER_LOW_FREQUENCY	| \
		SPEAKER_BACK_LEFT	| \
		SPEAKER_BACK_RIGHT	| \
		SPEAKER_SIDE_LEFT	| \
		SPEAKER_SIDE_RIGHT	)
#define SPEAKER_XBOX SPEAKER_5POINT1
#endif

#define FAUDIO_FORMAT_PCM		1
#define FAUDIO_FORMAT_MSADPCM		2
#define FAUDIO_FORMAT_IEEE_FLOAT	3
#define FAUDIO_FORMAT_WMAUDIO2		0x0161
#define FAUDIO_FORMAT_WMAUDIO3		0x0162
#define FAUDIO_FORMAT_WMAUDIO_LOSSLESS	0x0163
#define FAUDIO_FORMAT_XMAUDIO2		0x0166
#define FAUDIO_FORMAT_EXTENSIBLE	0xFFFE

extern FAudioGUID DATAFORMAT_SUBTYPE_PCM;
extern FAudioGUID DATAFORMAT_SUBTYPE_IEEE_FLOAT;

/* FAudio Version API */

#define FAUDIO_TARGET_VERSION 8 /* Targeting compatibility with XAudio 2.8 */

#define FAUDIO_ABI_VERSION	 0
#define FAUDIO_MAJOR_VERSION	25
#define FAUDIO_MINOR_VERSION	 1
#define FAUDIO_PATCH_VERSION	 0

#define FAUDIO_COMPILED_VERSION ( \
	(FAUDIO_ABI_VERSION * 100 * 100 * 100) + \
	(FAUDIO_MAJOR_VERSION * 100 * 100) + \
	(FAUDIO_MINOR_VERSION * 100) + \
	(FAUDIO_PATCH_VERSION) \
)

FAUDIOAPI uint32_t FAudioLinkedVersion(void);

/* FAudio Interface */

/* This should be your first FAudio call.
 *
 * ppFAudio:		Filled with the FAudio core context.
 * Flags:		Can be 0 or FAUDIO_DEBUG_ENGINE.
 * XAudio2Processor:	Set this to FAUDIO_DEFAULT_PROCESSOR.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioCreate(
	FAudio **ppFAudio,
	uint32_t Flags,
	FAudioProcessor XAudio2Processor
);

/* See "extensions/COMConstructEXT.txt" for more details */
FAUDIOAPI uint32_t FAudioCOMConstructEXT(FAudio **ppFAudio, uint8_t version);

/* Increments a reference counter. When counter is 0, audio is freed.
 * Returns the reference count after incrementing.
 */
FAUDIOAPI uint32_t FAudio_AddRef(FAudio *audio);

/* Decrements a reference counter. When counter is 0, audio is freed.
 * Returns the reference count after decrementing.
 */
FAUDIOAPI uint32_t FAudio_Release(FAudio *audio);

/* Queries the number of sound devices available for use.
 *
 * pCount: Filled with the number of available sound devices.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudio_GetDeviceCount(FAudio *audio, uint32_t *pCount);

/* Gets basic information about a sound device.
 *
 * Index:		Can be between 0 and the result of GetDeviceCount.
 * pDeviceDetails:	Filled with the device information.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudio_GetDeviceDetails(
	FAudio *audio,
	uint32_t Index,
	FAudioDeviceDetails *pDeviceDetails
);

/* You don't actually have to call this, unless you're using the COM APIs.
 * See the FAudioCreate API for parameter information.
 */
FAUDIOAPI uint32_t FAudio_Initialize(
	FAudio *audio,
	uint32_t Flags,
	FAudioProcessor XAudio2Processor
);

/* Register a new set of engine callbacks.
 * There is no limit to the number of sets, but expect performance to degrade
 * if you have a whole bunch of these. You most likely only need one.
 *
 * pCallback: The completely-initialized FAudioEngineCallback structure.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudio_RegisterForCallbacks(
	FAudio *audio,
	FAudioEngineCallback *pCallback
);

/* Remove an active set of engine callbacks.
 * This checks the pointer value, NOT the callback values!
 *
 * pCallback: An FAudioEngineCallback structure previously sent to Register.
 *
 * Returns 0 on success.
 */
FAUDIOAPI void FAudio_UnregisterForCallbacks(
	FAudio *audio,
	FAudioEngineCallback *pCallback
);

/* Creates a "source" voice, used to play back wavedata.
 *
 * ppSourceVoice:	Filled with the source voice pointer.
 * pSourceFormat:	The input wavedata format, see the documentation for
 *			FAudioWaveFormatEx.
 * Flags:		Can be 0 or a mix of the following FAUDIO_VOICE_* flags:
 *			NOPITCH/NOSRC:	Resampling is disabled. If you set this,
 *					the source format sample rate MUST match
 *					the output voices' input sample rates.
 *					Also, SetFrequencyRatio will fail.
 *			USEFILTER:	Enables the use of SetFilterParameters.
 *			MUSIC:		Unsupported.
 * MaxFrequencyRatio:	AKA your max pitch. This allows us to optimize the size
 *			of the decode/resample cache sizes. For example, if you
 *			only expect to raise pitch by a single octave, you can
 *			set this value to 2.0f. 2.0f is the default value.
 *			Bounds: [FAUDIO_MIN_FREQ_RATIO, FAUDIO_MAX_FREQ_RATIO].
 * pCallback:		Voice callbacks, see FAudioVoiceCallback documentation.
 * pSendList:		List of output voices. If NULL, defaults to master.
 *			All output voices must have the same sample rate!
 * pEffectChain:	List of FAPO effects. This value can be NULL.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudio_CreateSourceVoice(
	FAudio *audio,
	FAudioSourceVoice **ppSourceVoice,
	const FAudioWaveFormatEx *pSourceFormat,
	uint32_t Flags,
	float MaxFrequencyRatio,
	FAudioVoiceCallback *pCallback,
	const FAudioVoiceSends *pSendList,
	const FAudioEffectChain *pEffectChain
);

/* Creates a "submix" voice, used to mix/process input voices.
 * The typical use case for this is to perform CPU-intensive tasks on large
 * groups of voices all at once. Examples include resampling and FAPO effects.
 *
 * ppSubmixVoice:	Filled with the submix voice pointer.
 * InputChannels:	Input voices will convert to this channel count.
 * InputSampleRate:	Input voices will convert to this sample rate.
 * Flags:		Can be 0 or FAUDIO_VOICE_USEFILTER.
 * ProcessingStage:	If you have multiple submixes that depend on a specific
 *			order of processing, you can sort them by setting this
 *			value to prioritize them. For example, submixes with
 *			stage 0 will process first, then stage 1, 2, and so on.
 * pSendList:		List of output voices. If NULL, defaults to master.
 *			All output voices must have the same sample rate!
 * pEffectChain:	List of FAPO effects. This value can be NULL.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudio_CreateSubmixVoice(
	FAudio *audio,
	FAudioSubmixVoice **ppSubmixVoice,
	uint32_t InputChannels,
	uint32_t InputSampleRate,
	uint32_t Flags,
	uint32_t ProcessingStage,
	const FAudioVoiceSends *pSendList,
	const FAudioEffectChain *pEffectChain
);

/* This should be your second FAudio call, unless you care about which device
 * you want to use. In that case, see GetDeviceDetails.
 *
 * ppMasteringVoice:	Filled with the mastering voice pointer.
 * InputChannels:	Device channel count. Can be FAUDIO_DEFAULT_CHANNELS.
 * InputSampleRate:	Device sample rate. Can be FAUDIO_DEFAULT_SAMPLERATE.
 * Flags:		This value must be 0.
 * DeviceIndex:		0 for the default device. See GetDeviceCount.
 * pEffectChain:	List of FAPO effects. This value can be NULL.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudio_CreateMasteringVoice(
	FAudio *audio,
	FAudioMasteringVoice **ppMasteringVoice,
	uint32_t InputChannels,
	uint32_t InputSampleRate,
	uint32_t Flags,
	uint32_t DeviceIndex,
	const FAudioEffectChain *pEffectChain
);

/* This is the XAudio 2.8+ version of CreateMasteringVoice.
 * Right now this doesn't do anything. Don't use this function.
 */
FAUDIOAPI uint32_t FAudio_CreateMasteringVoice8(
	FAudio *audio,
	FAudioMasteringVoice **ppMasteringVoice,
	uint32_t InputChannels,
	uint32_t InputSampleRate,
	uint32_t Flags,
	uint16_t *szDeviceId,
	const FAudioEffectChain *pEffectChain,
	FAudioStreamCategory StreamCategory
);

/* Starts the engine, begins processing the audio graph.
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudio_StartEngine(FAudio *audio);

/* Stops the engine and halts all processing.
 * The audio device will continue to run, but will produce silence.
 * The graph will be frozen until you call StartEngine, where it will then
 * resume all processing exactly as it would have had this never been called.
 */
FAUDIOAPI void FAudio_StopEngine(FAudio *audio);

/* Flushes a batch of FAudio calls compiled with a given "OperationSet" tag.
 * This function is based on IXAudio2::CommitChanges from the XAudio2 spec.
 * This is useful for pushing calls that need to be done perfectly in sync. For
 * example, if you want to play two separate sources at the exact same time, you
 * can call FAudioSourceVoice_Start with an OperationSet value of your choice,
 * then call CommitChanges with that same value to start the sources together.
 *
 * OperationSet: Either a value known by you or FAUDIO_COMMIT_ALL
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudio_CommitOperationSet(
	FAudio *audio,
	uint32_t OperationSet
);

/* DO NOT USE THIS FUNCTION OR I SWEAR TO GOD */
FAUDIODEPRECATED("This function will break your program! Use FAudio_CommitOperationSet instead!")
FAUDIOAPI uint32_t FAudio_CommitChanges(FAudio *audio);

/* Requests various bits of performance information from the engine.
 *
 * pPerfData: Filled with the data. See FAudioPerformanceData for details.
 */
FAUDIOAPI void FAudio_GetPerformanceData(
	FAudio *audio,
	FAudioPerformanceData *pPerfData
);

/* When using a Debug binary, this lets you configure what information gets
 * logged to output. Be careful, this can spit out a LOT of text.
 *
 * pDebugConfiguration:	See FAudioDebugConfiguration for details.
 * pReserved:		Set this to NULL.
 */
FAUDIOAPI void FAudio_SetDebugConfiguration(
	FAudio *audio,
	FAudioDebugConfiguration *pDebugConfiguration,
	void* pReserved
);

/* Requests the values that determine's the engine's update size.
 * For example, a 48KHz engine with a 1024-sample update period would return
 * 1024 for the numerator and 48000 for the denominator. With this information,
 * you can determine the precise update size in milliseconds.
 *
 * quantumNumerator - The engine's update size, in sample frames.
 * quantumDenominator - The engine's sample rate, in Hz
 */
FAUDIOAPI void FAudio_GetProcessingQuantum(
	FAudio *audio,
	uint32_t *quantumNumerator,
	uint32_t *quantumDenominator
);

/* FAudioVoice Interface */

/* Requests basic information about a voice.
 *
 * pVoiceDetails: See FAudioVoiceDetails for details.
 */
FAUDIOAPI void FAudioVoice_GetVoiceDetails(
	FAudioVoice *voice,
	FAudioVoiceDetails *pVoiceDetails
);

/* Change the output voices for this voice.
 * This function is invalid for mastering voices.
 *
 * pSendList:	List of output voices. If NULL, defaults to master.
 *		All output voices must have the same sample rate!
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetOutputVoices(
	FAudioVoice *voice,
	const FAudioVoiceSends *pSendList
);

/* Change/Remove the effect chain for this voice.
 *
 * pEffectChain:	List of FAPO effects. This value can be NULL.
 *			Note that the final channel counts for this chain MUST
 *			match the input/output channel count that was
 *			determined at voice creation time!
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetEffectChain(
	FAudioVoice *voice,
	const FAudioEffectChain *pEffectChain
);

/* Enables an effect in the effect chain.
 *
 * EffectIndex:		The index of the effect (based on the chain order).
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_EnableEffect(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	uint32_t OperationSet
);

/* Disables an effect in the effect chain.
 *
 * EffectIndex:		The index of the effect (based on the chain order).
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_DisableEffect(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	uint32_t OperationSet
);

/* Queries the enabled/disabled state of an effect in the effect chain.
 *
 * EffectIndex:	The index of the effect (based on the chain order).
 * pEnabled:	Filled with either 1 (Enabled) or 0 (Disabled).
 *
 * Returns 0 on success.
 */
FAUDIOAPI void FAudioVoice_GetEffectState(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	int32_t *pEnabled
);

/* Submits a block of memory to be sent to FAPO::SetParameters.
 *
 * EffectIndex:		The index of the effect (based on the chain order).
 * pParameters:		The values to be copied and submitted to the FAPO.
 * ParametersByteSize:	This should match what the FAPO expects!
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetEffectParameters(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	const void *pParameters,
	uint32_t ParametersByteSize,
	uint32_t OperationSet
);

/* Requests the latest parameters from FAPO::GetParameters.
 *
 * EffectIndex:		The index of the effect (based on the chain order).
 * pParameters:		Filled with the latest parameter values from the FAPO.
 * ParametersByteSize:	This should match what the FAPO expects!
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_GetEffectParameters(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	void *pParameters,
	uint32_t ParametersByteSize
);

/* Sets the filter variables for a voice.
 * This is only valid on voices with the USEFILTER flag.
 *
 * pParameters:		See FAudioFilterParameters for details.
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetFilterParameters(
	FAudioVoice *voice,
	const FAudioFilterParameters *pParameters,
	uint32_t OperationSet
);

/* Requests the filter variables for a voice.
 * This is only valid on voices with the USEFILTER flag.
 *
 * pParameters: See FAudioFilterParameters for details.
 */
FAUDIOAPI void FAudioVoice_GetFilterParameters(
	FAudioVoice *voice,
	FAudioFilterParameters *pParameters
);

/* Sets the filter variables for a voice's output voice.
 * This is only valid on sends with the USEFILTER flag.
 *
 * pDestinationVoice:	An output voice from the voice's send list.
 * pParameters:		See FAudioFilterParameters for details.
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetOutputFilterParameters(
	FAudioVoice *voice,
	FAudioVoice *pDestinationVoice,
	const FAudioFilterParameters *pParameters,
	uint32_t OperationSet
);

/* Requests the filter variables for a voice's output voice.
 * This is only valid on sends with the USEFILTER flag.
 *
 * pDestinationVoice:	An output voice from the voice's send list.
 * pParameters:		See FAudioFilterParameters for details.
 */
FAUDIOAPI void FAudioVoice_GetOutputFilterParameters(
	FAudioVoice *voice,
	FAudioVoice *pDestinationVoice,
	FAudioFilterParameters *pParameters
);

/* Sets the filter variables for a voice.
 * This is only valid on voices with the USEFILTER flag.
 *
 * pParameters:		See FAudioFilterParametersEXT for details.
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetFilterParametersEXT(
	FAudioVoice* voice,
	const FAudioFilterParametersEXT* pParameters,
	uint32_t OperationSet
);

/* Requests the filter variables for a voice.
 * This is only valid on voices with the USEFILTER flag.
 *
 * pParameters: See FAudioFilterParametersEXT for details.
 */
FAUDIOAPI void FAudioVoice_GetFilterParametersEXT(
	FAudioVoice* voice,
	FAudioFilterParametersEXT* pParameters
);

/* Sets the filter variables for a voice's output voice.
 * This is only valid on sends with the USEFILTER flag.
 *
 * pDestinationVoice:	An output voice from the voice's send list.
 * pParameters:		See FAudioFilterParametersEXT for details.
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetOutputFilterParametersEXT(
	FAudioVoice* voice,
	FAudioVoice* pDestinationVoice,
	const FAudioFilterParametersEXT* pParameters,
	uint32_t OperationSet
);

/* Requests the filter variables for a voice's output voice.
 * This is only valid on sends with the USEFILTER flag.
 *
 * pDestinationVoice:	An output voice from the voice's send list.
 * pParameters:		See FAudioFilterParametersEXT for details.
 */
FAUDIOAPI void FAudioVoice_GetOutputFilterParametersEXT(
	FAudioVoice* voice,
	FAudioVoice* pDestinationVoice,
	FAudioFilterParametersEXT* pParameters
);

/* Sets the global volume of a voice.
 *
 * Volume:		Amplitude ratio. 1.0f is default, 0.0f is silence.
 *			Note that you can actually set volume < 0.0f!
 *			Bounds: [-FAUDIO_MAX_VOLUME_LEVEL, FAUDIO_MAX_VOLUME_LEVEL]
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetVolume(
	FAudioVoice *voice,
	float Volume,
	uint32_t OperationSet
);

/* Requests the global volume of a voice.
 *
 * pVolume: Filled with the current voice amplitude ratio.
 */
FAUDIOAPI void FAudioVoice_GetVolume(
	FAudioVoice *voice,
	float *pVolume
);

/* Sets the per-channel volumes of a voice.
 *
 * Channels:		Must match the channel count of this voice!
 * pVolumes:		Amplitude ratios for each channel. Same as SetVolume.
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetChannelVolumes(
	FAudioVoice *voice,
	uint32_t Channels,
	const float *pVolumes,
	uint32_t OperationSet
);

/* Requests the per-channel volumes of a voice.
 *
 * Channels:	Must match the channel count of this voice!
 * pVolumes:	Filled with the current channel amplitude ratios.
 */
FAUDIOAPI void FAudioVoice_GetChannelVolumes(
	FAudioVoice *voice,
	uint32_t Channels,
	float *pVolumes
);

/* Sets the volumes of a send's output channels. The matrix is based on the
 * voice's input channels. For example, the default matrix for a 2-channel
 * source and a 2-channel output voice is as follows:
 * [0] = 1.0f; <- Left input, left output
 * [1] = 0.0f; <- Right input, left output
 * [2] = 0.0f; <- Left input, right output
 * [3] = 1.0f; <- Right input, right output
 * This is typically only used for panning or 3D sound (via F3DAudio).
 *
 * pDestinationVoice:	An output voice from the voice's send list.
 * SourceChannels:	Must match the voice's input channel count!
 * DestinationChannels:	Must match the destination's input channel count!
 * pLevelMatrix:	A float[SourceChannels * DestinationChannels].
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioVoice_SetOutputMatrix(
	FAudioVoice *voice,
	FAudioVoice *pDestinationVoice,
	uint32_t SourceChannels,
	uint32_t DestinationChannels,
	const float *pLevelMatrix,
	uint32_t OperationSet
);

/* Gets the volumes of a send's output channels. See SetOutputMatrix.
 *
 * pDestinationVoice:	An output voice from the voice's send list.
 * SourceChannels:	Must match the voice's input channel count!
 * DestinationChannels:	Must match the voice's output channel count!
 * pLevelMatrix:	A float[SourceChannels * DestinationChannels].
 */
FAUDIOAPI void FAudioVoice_GetOutputMatrix(
	FAudioVoice *voice,
	FAudioVoice *pDestinationVoice,
	uint32_t SourceChannels,
	uint32_t DestinationChannels,
	float *pLevelMatrix
);

/* Removes this voice from the audio graph and frees memory. */
FAUDIOAPI void FAudioVoice_DestroyVoice(FAudioVoice *voice);

/*
 * Returns S_OK on success and E_FAIL if voice could not be destroyed (e. g., because it is in use).
 */
FAUDIOAPI uint32_t FAudioVoice_DestroyVoiceSafeEXT(FAudioVoice *voice);

/* FAudioSourceVoice Interface */

/* Starts processing for a source voice.
 *
 * Flags:		Must be 0.
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioSourceVoice_Start(
	FAudioSourceVoice *voice,
	uint32_t Flags,
	uint32_t OperationSet
);

/* Pauses processing for a source voice. Yes, I said pausing.
 * If you want to _actually_ stop, call FlushSourceBuffers next.
 *
 * Flags:		Can be 0 or FAUDIO_PLAY_TAILS, which allows effects to
 *			keep emitting output even after processing has stopped.
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioSourceVoice_Stop(
	FAudioSourceVoice *voice,
	uint32_t Flags,
	uint32_t OperationSet
);

/* Submits a block of wavedata for the source to process.
 *
 * pBuffer:	See FAudioBuffer for details.
 * pBufferWMA:	See FAudioBufferWMA for details. (Also, don't use WMA.)
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioSourceVoice_SubmitSourceBuffer(
	FAudioSourceVoice *voice,
	const FAudioBuffer *pBuffer,
	const FAudioBufferWMA *pBufferWMA
);

/* Removes all buffers from a source, with a minor exception.
 * If the voice is still playing, the active buffer is left alone.
 * All buffers that are removed will spawn an OnBufferEnd callback.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioSourceVoice_FlushSourceBuffers(
	FAudioSourceVoice *voice
);

/* Takes the last buffer currently queued and sets the END_OF_STREAM flag.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioSourceVoice_Discontinuity(
	FAudioSourceVoice *voice
);

/* Sets the loop count of the active buffer to 0.
 *
 * OperationSet: See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioSourceVoice_ExitLoop(
	FAudioSourceVoice *voice,
	uint32_t OperationSet
);

/* Requests the state and some basic statistics for this source.
 *
 * pVoiceState:	See FAudioVoiceState for details.
 * Flags:	Can be 0 or FAUDIO_VOICE_NOSAMPLESPLAYED.
 */
FAUDIOAPI void FAudioSourceVoice_GetState(
	FAudioSourceVoice *voice,
	FAudioVoiceState *pVoiceState,
	uint32_t Flags
);

/* Sets the frequency ratio (fancy phrase for pitch) of this source.
 *
 * Ratio:		The frequency ratio, must be <= MaxFrequencyRatio.
 * OperationSet:	See CommitChanges. Default is FAUDIO_COMMIT_NOW.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioSourceVoice_SetFrequencyRatio(
	FAudioSourceVoice *voice,
	float Ratio,
	uint32_t OperationSet
);

/* Requests the frequency ratio (fancy phrase for pitch) of this source.
 *
 * pRatio: Filled with the frequency ratio.
 */
FAUDIOAPI void FAudioSourceVoice_GetFrequencyRatio(
	FAudioSourceVoice *voice,
	float *pRatio
);

/* Resets the core sample rate of this source.
 * You probably don't want this, it's more likely you want SetFrequencyRatio.
 * This is used to recycle voices without having to constantly reallocate them.
 * For example, if you have wavedata that's all float32 mono, but the sample
 * rates are different, you can take a source that was being used for a 48KHz
 * wave and call this so it can be used for a 44.1KHz wave.
 *
 * NewSourceSampleRate: The new sample rate for this source.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioSourceVoice_SetSourceSampleRate(
	FAudioSourceVoice *voice,
	uint32_t NewSourceSampleRate
);

/* FAudioMasteringVoice Interface */

/* Requests the channel mask for the mastering voice.
 * This is typically used with F3DAudioInitialize, but you may find it
 * interesting if you want to see the user's basic speaker layout.
 *
 * pChannelMask: Filled with the channel mask.
 *
 * Returns 0 on success.
 */
FAUDIOAPI uint32_t FAudioMasteringVoice_GetChannelMask(
	FAudioMasteringVoice *voice,
	uint32_t *pChannelMask
);

/* FAudioEngineCallback Interface */

/* If something horrible happens, this will be called.
 *
 * Error: The error code that spawned this callback.
 */
typedef void (FAUDIOCALL * OnCriticalErrorFunc)(
	FAudioEngineCallback *callback,
	uint32_t Error
);

/* This is called at the end of a processing update. */
typedef void (FAUDIOCALL * OnProcessingPassEndFunc)(
	FAudioEngineCallback *callback
);

/* This is called at the beginning of a processing update. */
typedef void (FAUDIOCALL * OnProcessingPassStartFunc)(
	FAudioEngineCallback *callback
);

struct FAudioEngineCallback
{
	OnCriticalErrorFunc OnCriticalError;
	OnProcessingPassEndFunc OnProcessingPassEnd;
	OnProcessingPassStartFunc OnProcessingPassStart;
};

/* FAudioVoiceCallback Interface */

/* When a buffer is no longer in use, this is called.
 *
 * pBufferContext: The pContext for the FAudioBuffer in question.
 */
typedef void (FAUDIOCALL * OnBufferEndFunc)(
	FAudioVoiceCallback *callback,
	void *pBufferContext
);

/* When a buffer is now being used, this is called.
 *
 * pBufferContext: The pContext for the FAudioBuffer in question.
 */
typedef void (FAUDIOCALL * OnBufferStartFunc)(
	FAudioVoiceCallback *callback,
	void *pBufferContext
);

/* When a buffer completes a loop, this is called.
 *
 * pBufferContext: The pContext for the FAudioBuffer in question.
 */
typedef void (FAUDIOCALL * OnLoopEndFunc)(
	FAudioVoiceCallback *callback,
	void *pBufferContext
);

/* When a buffer that has the END_OF_STREAM flag is finished, this is called. */
typedef void (FAUDIOCALL * OnStreamEndFunc)(
	FAudioVoiceCallback *callback
);

/* If something horrible happens to a voice, this is called.
 *
 * pBufferContext:	The pContext for the FAudioBuffer in question.
 * Error:		The error code that spawned this callback.
 */
typedef void (FAUDIOCALL * OnVoiceErrorFunc)(
	FAudioVoiceCallback *callback,
	void *pBufferContext,
	uint32_t Error
);

/* When this voice is done being processed, this is called. */
typedef void (FAUDIOCALL * OnVoiceProcessingPassEndFunc)(
	FAudioVoiceCallback *callback
);

/* When a voice is about to start being processed, this is called.
 *
 * BytesRequested:	The number of bytes needed from the application to
 *			complete a full update. For example, if we need 512
 *			frames for a whole update, and the voice is a float32
 *			stereo source, BytesRequired will be 4096.
 */
typedef void (FAUDIOCALL * OnVoiceProcessingPassStartFunc)(
	FAudioVoiceCallback *callback,
	uint32_t BytesRequired
);

struct FAudioVoiceCallback
{
	OnBufferEndFunc OnBufferEnd;
	OnBufferStartFunc OnBufferStart;
	OnLoopEndFunc OnLoopEnd;
	OnStreamEndFunc OnStreamEnd;
	OnVoiceErrorFunc OnVoiceError;
	OnVoiceProcessingPassEndFunc OnVoiceProcessingPassEnd;
	OnVoiceProcessingPassStartFunc OnVoiceProcessingPassStart;
};

/* FAudio Custom Allocator API
 * See "extensions/CustomAllocatorEXT.txt" for more information.
 */

typedef void* (FAUDIOCALL * FAudioMallocFunc)(size_t size);
typedef void (FAUDIOCALL * FAudioFreeFunc)(void* ptr);
typedef void* (FAUDIOCALL * FAudioReallocFunc)(void* ptr, size_t size);

FAUDIOAPI uint32_t FAudioCreateWithCustomAllocatorEXT(
	FAudio **ppFAudio,
	uint32_t Flags,
	FAudioProcessor XAudio2Processor,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
);
FAUDIOAPI uint32_t FAudioCOMConstructWithCustomAllocatorEXT(
	FAudio **ppFAudio,
	uint8_t version,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
);

/* FAudio Engine Procedure API
 * See "extensions/EngineProcedureEXT.txt" for more information.
 */
typedef void (FAUDIOCALL *FAudioEngineCallEXT)(FAudio *audio, float *output);
typedef void (FAUDIOCALL *FAudioEngineProcedureEXT)(FAudioEngineCallEXT defaultEngineProc, FAudio *audio, float *output, void *user);

FAUDIOAPI void FAudio_SetEngineProcedureEXT(
	FAudio *audio,
	FAudioEngineProcedureEXT clientEngineProc,
	void *user
);


/* FAudio I/O API */

#define FAUDIO_SEEK_SET 0
#define FAUDIO_SEEK_CUR 1
#define FAUDIO_SEEK_END 2
#define FAUDIO_EOF -1

typedef size_t (FAUDIOCALL * FAudio_readfunc)(
	void *data,
	void *dst,
	size_t size,
	size_t count
);
typedef int64_t (FAUDIOCALL * FAudio_seekfunc)(
	void *data,
	int64_t offset,
	int whence
);
typedef int (FAUDIOCALL * FAudio_closefunc)(
	void *data
);

typedef struct FAudioIOStream
{
	void *data;
	FAudio_readfunc read;
	FAudio_seekfunc seek;
	FAudio_closefunc close;
	void *lock;
} FAudioIOStream;

FAUDIOAPI FAudioIOStream* FAudio_fopen(const char *path);
FAUDIOAPI FAudioIOStream* FAudio_memopen(void *mem, int len);
FAUDIOAPI uint8_t* FAudio_memptr(FAudioIOStream *io, size_t offset);
FAUDIOAPI void FAudio_close(FAudioIOStream *io);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FAUDIO_H */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
