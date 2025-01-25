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

#include "FAudio.h"
#include "FAPOBase.h"
#include <stdarg.h>


#ifdef FAUDIO_WIN32_PLATFORM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define FAudio_malloc malloc
#define FAudio_realloc realloc
#define FAudio_free free
#define FAudio_alloca(x) alloca(x)
#define FAudio_dealloca(x) (void)(x)
#define FAudio_zero(ptr, size) memset(ptr, '\0', size)
#define FAudio_memset(ptr, val, size) memset(ptr, val, size)
#define FAudio_memcpy(dst, src, size) memcpy(dst, src, size)
#define FAudio_memmove(dst, src, size) memmove(dst, src, size)
#define FAudio_memcmp(ptr1, ptr2, size) memcmp(ptr1, ptr2, size)

#define FAudio_strlen(ptr) strlen(ptr)
#define FAudio_strcmp(str1, str2) strcmp(str1, str2)
#define FAudio_strncmp(str1, str2, size) strncmp(str1, str2, size)
#define FAudio_strlcpy(ptr1, ptr2, size) lstrcpynA(ptr1, ptr2, size)

#define FAudio_pow(x, y) pow(x, y)
#define FAudio_powf(x, y) powf(x, y)
#define FAudio_log(x) log(x)
#define FAudio_log10(x) log10(x)
#define FAudio_sin(x) sin(x)
#define FAudio_cos(x) cos(x)
#define FAudio_tan(x) tan(x)
#define FAudio_acos(x) acos(x)
#define FAudio_ceil(x) ceil(x)
#define FAudio_floor(x) floor(x)
#define FAudio_abs(x) abs(x)
#define FAudio_ldexp(v, e) ldexp(v, e)
#define FAudio_exp(x) exp(x)

#define FAudio_cosf(x) cosf(x)
#define FAudio_sinf(x) sinf(x)
#define FAudio_sqrtf(x) sqrtf(x)
#define FAudio_acosf(x) acosf(x)
#define FAudio_atan2f(y, x) atan2f(y, x)
#define FAudio_fabsf(x) fabsf(x)

#define FAudio_qsort qsort

#define FAudio_assert assert
#define FAudio_snprintf snprintf
#define FAudio_vsnprintf vsnprintf
#define FAudio_getenv getenv
#define FAudio_PRIu64 PRIu64
#define FAudio_PRIx64 PRIx64

extern void FAudio_Log(char const *msg);

/* FIXME: Assuming little-endian! */
#define FAudio_swap16LE(x) (x)
#define FAudio_swap16BE(x) \
	((x >> 8)	& 0x00FF) | \
	((x << 8)	& 0xFF00)
#define FAudio_swap32LE(x) (x)
#define FAudio_swap32BE(x) \
	((x >> 24)	& 0x000000FF) | \
	((x >> 8)	& 0x0000FF00) | \
	((x << 8)	& 0x00FF0000) | \
	((x << 24)	& 0xFF000000)
#define FAudio_swap64LE(x) (x)
#define FAudio_swap64BE(x) \
	((x >> 32)	& 0x00000000000000FF) | \
	((x >> 24)	& 0x000000000000FF00) | \
	((x >> 16)	& 0x0000000000FF0000) | \
	((x >> 8)	& 0x00000000FF000000) | \
	((x << 8)	& 0x000000FF00000000) | \
	((x << 16)	& 0x0000FF0000000000) | \
	((x << 24)	& 0x00FF000000000000) | \
	((x << 32)	& 0xFF00000000000000)
#else

#ifdef FAUDIO_SDL3_PLATFORM
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_endian.h>
#include <SDL3/SDL_log.h>

#define FAudio_swap16LE(x) SDL_Swap16LE(x)
#define FAudio_swap16BE(x) SDL_Swap16BE(x)
#define FAudio_swap32LE(x) SDL_Swap32LE(x)
#define FAudio_swap32BE(x) SDL_Swap32BE(x)
#define FAudio_swap64LE(x) SDL_Swap64LE(x)
#define FAudio_swap64BE(x) SDL_Swap64BE(x)

/* SDL3 allows memcpy/memset for compiler optimization reasons */
#ifdef SDL_SLOW_MEMCPY
#define STB_MEMCPY_OVERRIDE
#endif
#ifdef SDL_SLOW_MEMSET
#define STB_MEMSET_OVERRIDE
#endif
#else
#include <SDL_stdinc.h>
#include <SDL_assert.h>
#include <SDL_endian.h>
#include <SDL_log.h>

#define FAudio_swap16LE(x) SDL_SwapLE16(x)
#define FAudio_swap16BE(x) SDL_SwapBE16(x)
#define FAudio_swap32LE(x) SDL_SwapLE32(x)
#define FAudio_swap32BE(x) SDL_SwapBE32(x)
#define FAudio_swap64LE(x) SDL_SwapLE64(x)
#define FAudio_swap64BE(x) SDL_SwapBE64(x)

#define STB_MEMCPY_OVERRIDE
#define STB_MEMSET_OVERRIDE
#endif

#define FAudio_malloc SDL_malloc
#define FAudio_realloc SDL_realloc
#define FAudio_free SDL_free
#define FAudio_alloca(x) SDL_stack_alloc(uint8_t, x)
#define FAudio_dealloca(x) SDL_stack_free(x)
#define FAudio_zero(ptr, size) SDL_memset(ptr, '\0', size)
#define FAudio_memset(ptr, val, size) SDL_memset(ptr, val, size)
#define FAudio_memcpy(dst, src, size) SDL_memcpy(dst, src, size)
#define FAudio_memmove(dst, src, size) SDL_memmove(dst, src, size)
#define FAudio_memcmp(ptr1, ptr2, size) SDL_memcmp(ptr1, ptr2, size)

#define FAudio_strlen(ptr) SDL_strlen(ptr)
#define FAudio_strcmp(str1, str2) SDL_strcmp(str1, str2)
#define FAudio_strncmp(str1, str2, size) SDL_strncmp(str1, str1, size)
#define FAudio_strlcpy(ptr1, ptr2, size) SDL_strlcpy(ptr1, ptr2, size)

#define FAudio_pow(x, y) SDL_pow(x, y)
#define FAudio_powf(x, y) SDL_powf(x, y)
#define FAudio_log(x) SDL_log(x)
#define FAudio_log10(x) SDL_log10(x)
#define FAudio_sin(x) SDL_sin(x)
#define FAudio_cos(x) SDL_cos(x)
#define FAudio_tan(x) SDL_tan(x)
#define FAudio_acos(x) SDL_acos(x)
#define FAudio_ceil(x) SDL_ceil(x)
#define FAudio_floor(x) SDL_floor(x)
#define FAudio_abs(x) SDL_abs(x)
#define FAudio_ldexp(v, e) SDL_scalbn(v, e)
#define FAudio_exp(x) SDL_exp(x)

#define FAudio_cosf(x) SDL_cosf(x)
#define FAudio_sinf(x) SDL_sinf(x)
#define FAudio_sqrtf(x) SDL_sqrtf(x)
#define FAudio_acosf(x) SDL_acosf(x)
#define FAudio_atan2f(y, x) SDL_atan2f(y, x)
#define FAudio_fabsf(x) SDL_fabsf(x)

#define FAudio_qsort SDL_qsort

#ifdef FAUDIO_LOG_ASSERTIONS
#define FAudio_assert(condition) \
	{ \
		static uint8_t logged = 0; \
		if (!(condition) && !logged) \
		{ \
			SDL_Log("Assertion failed: %s", #condition); \
			logged = 1; \
		} \
	}
#else
#define FAudio_assert SDL_assert
#endif
#define FAudio_snprintf SDL_snprintf
#define FAudio_vsnprintf SDL_vsnprintf
#define FAudio_Log(msg) SDL_Log("%s", msg)
#define FAudio_getenv SDL_getenv
#define FAudio_PRIu64 SDL_PRIu64
#define FAudio_PRIx64 SDL_PRIx64
#endif

/* Easy Macros */
#define FAudio_min(val1, val2) \
	(val1 < val2 ? val1 : val2)
#define FAudio_max(val1, val2) \
	(val1 > val2 ? val1 : val2)
#define FAudio_clamp(val, min, max) \
	(val > max ? max : (val < min ? min : val))

/* Windows/Visual Studio cruft */
#ifdef _WIN32
	#ifdef __cplusplus
		/* C++ should have `inline`, but not `restrict` */
		#define restrict
	#else
		#define inline __inline
		#if defined(_MSC_VER)
			#if (_MSC_VER >= 1700) /* VS2012+ */
				#define restrict __restrict
			#else /* VS2010- */
				#define restrict
			#endif
		#endif
	#endif
#endif

/* C++ does not have restrict (though VS2012+ does have __restrict) */
#if defined(__cplusplus) && !defined(restrict)
#define restrict
#endif

/* Alignment macro for gcc/clang/msvc */
#if defined(__clang__) || defined(__GNUC__)
#define ALIGN(type, boundary) type __attribute__((aligned(boundary)))
#elif defined(_MSC_VER)
#define ALIGN(type, boundary) __declspec(align(boundary)) type
#else
#define ALIGN(type, boundary) type
#endif

/* Threading Types */

typedef void* FAudioThread;
typedef void* FAudioMutex;
typedef int32_t (FAUDIOCALL * FAudioThreadFunc)(void* data);
typedef enum FAudioThreadPriority
{
	FAUDIO_THREAD_PRIORITY_LOW,
	FAUDIO_THREAD_PRIORITY_NORMAL,
	FAUDIO_THREAD_PRIORITY_HIGH,
} FAudioThreadPriority;

/* Linked Lists */

typedef struct LinkedList LinkedList;
struct LinkedList
{
	void* entry;
	LinkedList *next;
};
void LinkedList_AddEntry(
	LinkedList **start,
	void* toAdd,
	FAudioMutex lock,
	FAudioMallocFunc pMalloc
);
void LinkedList_PrependEntry(
	LinkedList **start,
	void* toAdd,
	FAudioMutex lock,
	FAudioMallocFunc pMalloc
);
void LinkedList_RemoveEntry(
	LinkedList **start,
	void* toRemove,
	FAudioMutex lock,
	FAudioFreeFunc pFree
);

/* Internal FAudio Types */

typedef enum FAudioVoiceType
{
	FAUDIO_VOICE_SOURCE,
	FAUDIO_VOICE_SUBMIX,
	FAUDIO_VOICE_MASTER
} FAudioVoiceType;

typedef struct FAudioBufferEntry FAudioBufferEntry;
struct FAudioBufferEntry
{
	FAudioBuffer buffer;
	FAudioBufferWMA bufferWMA;
	FAudioBufferEntry *next;
};

typedef void (FAUDIOCALL * FAudioDecodeCallback)(
	FAudioVoice *voice,
	FAudioBuffer *buffer,	/* Buffer to decode */
	float *decodeCache,	/* Decode into here */
	uint32_t samples	/* Samples to decode */
);

typedef void (FAUDIOCALL * FAudioResampleCallback)(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t channels
);

typedef void (FAUDIOCALL * FAudioMixCallback)(
	uint32_t toMix,
	uint32_t srcChans,
	uint32_t dstChans,
	float *restrict srcData,
	float *restrict dstData,
	float *restrict coefficients
);

typedef float FAudioFilterState[4];

/* Operation Sets, original implementation by Tyler Glaiel */

typedef struct FAudio_OPERATIONSET_Operation FAudio_OPERATIONSET_Operation;

void FAudio_OPERATIONSET_Commit(FAudio *audio, uint32_t OperationSet);
void FAudio_OPERATIONSET_CommitAll(FAudio *audio);
void FAudio_OPERATIONSET_Execute(FAudio *audio);

void FAudio_OPERATIONSET_ClearAll(FAudio *audio);
void FAudio_OPERATIONSET_ClearAllForVoice(FAudioVoice *voice);

void FAudio_OPERATIONSET_QueueEnableEffect(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueDisableEffect(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueSetEffectParameters(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	const void *pParameters,
	uint32_t ParametersByteSize,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueSetFilterParameters(
	FAudioVoice *voice,
	const FAudioFilterParametersEXT *pParameters,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueSetOutputFilterParameters(
	FAudioVoice *voice,
	FAudioVoice *pDestinationVoice,
	const FAudioFilterParametersEXT *pParameters,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueSetVolume(
	FAudioVoice *voice,
	float Volume,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueSetChannelVolumes(
	FAudioVoice *voice,
	uint32_t Channels,
	const float *pVolumes,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueSetOutputMatrix(
	FAudioVoice *voice,
	FAudioVoice *pDestinationVoice,
	uint32_t SourceChannels,
	uint32_t DestinationChannels,
	const float *pLevelMatrix,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueStart(
	FAudioSourceVoice *voice,
	uint32_t Flags,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueStop(
	FAudioSourceVoice *voice,
	uint32_t Flags,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueExitLoop(
	FAudioSourceVoice *voice,
	uint32_t OperationSet
);
void FAudio_OPERATIONSET_QueueSetFrequencyRatio(
	FAudioSourceVoice *voice,
	float Ratio,
	uint32_t OperationSet
);

/* Public FAudio Types */

struct FAudio
{
	uint8_t version;
	uint8_t active;
	uint32_t refcount;
	uint32_t initFlags;
	uint32_t updateSize;
	FAudioMasteringVoice *master;
	LinkedList *sources;
	LinkedList *submixes;
	LinkedList *callbacks;
	FAudioMutex sourceLock;
	FAudioMutex submixLock;
	FAudioMutex callbackLock;
	FAudioMutex operationLock;
	FAudioWaveFormatExtensible mixFormat;

	FAudio_OPERATIONSET_Operation *queuedOperations;
	FAudio_OPERATIONSET_Operation *committedOperations;

	/* Used to prevent destroying an active voice */
	FAudioSourceVoice *processingSource;

	/* Temp storage for processing, interleaved PCM32F */
	#define EXTRA_DECODE_PADDING 2
	uint32_t decodeSamples;
	uint32_t resampleSamples;
	uint32_t effectChainSamples;
	float *decodeCache;
	float *resampleCache;
	float *effectChainCache;

	/* Allocator callbacks */
	FAudioMallocFunc pMalloc;
	FAudioFreeFunc pFree;
	FAudioReallocFunc pRealloc;

	/* EngineProcedureEXT */
	void *clientEngineUser;
	FAudioEngineProcedureEXT pClientEngineProc;

#ifndef FAUDIO_DISABLE_DEBUGCONFIGURATION
	/* Debug Information */
	FAudioDebugConfiguration debug;
#endif /* FAUDIO_DISABLE_DEBUGCONFIGURATION */

	/* Platform opaque pointer */
	void *platform;
};

struct FAudioVoice
{
	FAudio *audio;
	uint32_t flags;
	FAudioVoiceType type;

	FAudioVoiceSends sends;
	float **sendCoefficients;
	float **mixCoefficients;
	FAudioMixCallback *sendMix;
	FAudioFilterParametersEXT *sendFilter;
	FAudioFilterState **sendFilterState;
	struct
	{
		FAPOBufferFlags state;
		uint32_t count;
		FAudioEffectDescriptor *desc;
		void **parameters;
		uint32_t *parameterSizes;
		uint8_t *parameterUpdates;
		uint8_t *inPlaceProcessing;
	} effects;
	FAudioFilterParametersEXT filter;
	FAudioFilterState *filterState;
	FAudioMutex sendLock;
	FAudioMutex effectLock;
	FAudioMutex filterLock;

	float volume;
	float *channelVolume;
	uint32_t outputChannels;
	FAudioMutex volumeLock;

	FAUDIONAMELESS union
	{
		struct
		{
			/* Sample storage */
			uint32_t decodeSamples;
			uint32_t resampleSamples;

			/* Resampler */
			float resampleFreq;
			uint64_t resampleStep;
			uint64_t resampleOffset;
			uint64_t curBufferOffsetDec;
			uint32_t curBufferOffset;

			/* WMA decoding */
#ifdef HAVE_WMADEC
			struct FAudioWMADEC *wmadec;
#endif /* HAVE_WMADEC*/

			/* Read-only */
			float maxFreqRatio;
			FAudioWaveFormatEx *format;
			FAudioDecodeCallback decode;
			FAudioResampleCallback resample;
			FAudioVoiceCallback *callback;

			/* Dynamic */
			uint8_t active;
			float freqRatio;
			uint8_t newBuffer;
			uint64_t totalSamples;
			FAudioBufferEntry *bufferList;
			FAudioBufferEntry *flushList;
			FAudioMutex bufferLock;
		} src;
		struct
		{
			/* Sample storage */
			uint32_t inputSamples;
			uint32_t outputSamples;
			float *inputCache;
			uint64_t resampleStep;
			FAudioResampleCallback resample;

			/* Read-only */
			uint32_t inputChannels;
			uint32_t inputSampleRate;
			uint32_t processingStage;
		} mix;
		struct
		{
			/* Output stream, allocated by Platform */
			float *output;

			/* Needed when inputChannels != outputChannels */
			float *effectCache;

			/* Read-only */
			uint32_t inputChannels;
			uint32_t inputSampleRate;
		} master;
	};
};

/* Internal Functions */
void FAudio_INTERNAL_InsertSubmixSorted(
	LinkedList **start,
	FAudioSubmixVoice *toAdd,
	FAudioMutex lock,
	FAudioMallocFunc pMalloc
);
void FAudio_INTERNAL_UpdateEngine(FAudio *audio, float *output);
void FAudio_INTERNAL_ResizeDecodeCache(FAudio *audio, uint32_t size);
void FAudio_INTERNAL_AllocEffectChain(
	FAudioVoice *voice,
	const FAudioEffectChain *pEffectChain
);
void FAudio_INTERNAL_FreeEffectChain(FAudioVoice *voice);
uint32_t FAudio_INTERNAL_VoiceOutputFrequency(
	FAudioVoice *voice,
	const FAudioVoiceSends *pSendList
);
extern const float FAUDIO_INTERNAL_MATRIX_DEFAULTS[8][8][64];

/* Debug */

#ifdef FAUDIO_DISABLE_DEBUGCONFIGURATION

#define LOG_ERROR(engine, fmt, ...)
#define LOG_WARNING(engine, fmt, ...)
#define LOG_INFO(engine, fmt, ...)
#define LOG_DETAIL(engine, fmt, ...)
#define LOG_API_ENTER(engine)
#define LOG_API_EXIT(engine)
#define LOG_FUNC_ENTER(engine)
#define LOG_FUNC_EXIT(engine)
/* TODO: LOG_TIMING */
#define LOG_MUTEX_CREATE(engine, mutex)
#define LOG_MUTEX_DESTROY(engine, mutex)
#define LOG_MUTEX_LOCK(engine, mutex)
#define LOG_MUTEX_UNLOCK(engine, mutex)
/* TODO: LOG_MEMORY */
/* TODO: LOG_STREAMING */

#define LOG_FORMAT(engine, waveFormat)

#else

#if defined(_MSC_VER)
/* VC doesn't support __attribute__ at all, and there's no replacement for format. */
void FAudio_INTERNAL_debug(
	FAudio *audio,
	const char *file,
	uint32_t line,
	const char *func,
	const char *fmt,
	...
);
#if _MSC_VER <= 1700 /* <=2012 also doesn't support __func__ */
#define __func__ __FUNCTION__
#endif
#else
void FAudio_INTERNAL_debug(
	FAudio *audio,
	const char *file,
	uint32_t line,
	const char *func,
	const char *fmt,
	...
) __attribute__((format(printf,5,6)));
#endif
void FAudio_INTERNAL_debug_fmt(
	FAudio *audio,
	const char *file,
	uint32_t line,
	const char *func,
	const FAudioWaveFormatEx *fmt
);

#define PRINT_DEBUG(engine, cond, type, fmt, ...) \
	if (engine->debug.TraceMask & FAUDIO_LOG_##cond) \
	{ \
		FAudio_INTERNAL_debug( \
			engine, \
			__FILE__, \
			__LINE__, \
			__func__, \
			type ": " fmt, \
			__VA_ARGS__ \
		); \
	}

#define LOG_ERROR(engine, fmt, ...) PRINT_DEBUG(engine, ERRORS, "ERROR", fmt, __VA_ARGS__)
#define LOG_WARNING(engine, fmt, ...) PRINT_DEBUG(engine, WARNINGS, "WARNING", fmt, __VA_ARGS__)
#define LOG_INFO(engine, fmt, ...) PRINT_DEBUG(engine, INFO, "INFO", fmt, __VA_ARGS__)
#define LOG_DETAIL(engine, fmt, ...) PRINT_DEBUG(engine, DETAIL, "DETAIL", fmt, __VA_ARGS__)
#define LOG_API_ENTER(engine) PRINT_DEBUG(engine, API_CALLS, "API Enter", "%s", __func__)
#define LOG_API_EXIT(engine) PRINT_DEBUG(engine, API_CALLS, "API Exit", "%s", __func__)
#define LOG_FUNC_ENTER(engine) PRINT_DEBUG(engine, FUNC_CALLS, "FUNC Enter", "%s", __func__)
#define LOG_FUNC_EXIT(engine) PRINT_DEBUG(engine, FUNC_CALLS, "FUNC Exit", "%s", __func__)
/* TODO: LOG_TIMING */
#define LOG_MUTEX_CREATE(engine, mutex) PRINT_DEBUG(engine, LOCKS, "Mutex Create", "%p (%s)", mutex, #mutex)
#define LOG_MUTEX_DESTROY(engine, mutex) PRINT_DEBUG(engine, LOCKS, "Mutex Destroy", "%p (%s)", mutex, #mutex)
#define LOG_MUTEX_LOCK(engine, mutex) PRINT_DEBUG(engine, LOCKS, "Mutex Lock", "%p (%s)", mutex, #mutex)
#define LOG_MUTEX_UNLOCK(engine, mutex) PRINT_DEBUG(engine, LOCKS, "Mutex Unlock", "%p (%s)", mutex, #mutex)
/* TODO: LOG_MEMORY */
/* TODO: LOG_STREAMING */

#define LOG_FORMAT(engine, waveFormat) \
	if (engine->debug.TraceMask & FAUDIO_LOG_INFO) \
	{ \
		FAudio_INTERNAL_debug_fmt( \
			engine, \
			__FILE__, \
			__LINE__, \
			__func__, \
			waveFormat \
		); \
	}

#endif /* FAUDIO_DISABLE_DEBUGCONFIGURATION */

/* FAPOFX Creators */

#define CREATE_FAPOFX_FUNC(effect) \
	extern uint32_t FAPOFXCreate##effect( \
		FAPO **pEffect, \
		const void *pInitData, \
		uint32_t InitDataByteSize, \
		FAudioMallocFunc customMalloc, \
		FAudioFreeFunc customFree, \
		FAudioReallocFunc customRealloc, \
		uint8_t legacy \
	);
CREATE_FAPOFX_FUNC(EQ)
CREATE_FAPOFX_FUNC(MasteringLimiter)
CREATE_FAPOFX_FUNC(Reverb)
CREATE_FAPOFX_FUNC(Echo)
#undef CREATE_FAPOFX_FUNC

/* SIMD Stuff */

/* Callbacks declared as functions (rather than function pointers) are
 * scalar-only, for now. SIMD versions should be possible for these.
 */

extern void (*FAudio_INTERNAL_Convert_U8_To_F32)(
	const uint8_t *restrict src,
	float *restrict dst,
	uint32_t len
);
extern void (*FAudio_INTERNAL_Convert_S16_To_F32)(
	const int16_t *restrict src,
	float *restrict dst,
	uint32_t len
);
extern void (*FAudio_INTERNAL_Convert_S32_To_F32)(
	const int32_t *restrict src,
	float *restrict dst,
	uint32_t len
);

extern FAudioResampleCallback FAudio_INTERNAL_ResampleMono;
extern FAudioResampleCallback FAudio_INTERNAL_ResampleStereo;
extern void FAudio_INTERNAL_ResampleGeneric(
	float *restrict dCache,
	float *restrict resampleCache,
	uint64_t *resampleOffset,
	uint64_t resampleStep,
	uint64_t toResample,
	uint8_t channels
);

extern void (*FAudio_INTERNAL_Amplify)(
	float *output,
	uint32_t totalSamples,
	float volume
);

extern FAudioMixCallback FAudio_INTERNAL_Mix_Generic;

#define MIX_FUNC(type) \
	extern void FAudio_INTERNAL_Mix_##type##_Scalar( \
		uint32_t toMix, \
		uint32_t srcChans, \
		uint32_t dstChans, \
		float *restrict srcData, \
		float *restrict dstData, \
		float *restrict coefficients \
	);
MIX_FUNC(Generic)
MIX_FUNC(1in_1out)
MIX_FUNC(1in_2out)
MIX_FUNC(1in_6out)
MIX_FUNC(1in_8out)
MIX_FUNC(2in_1out)
MIX_FUNC(2in_2out)
MIX_FUNC(2in_6out)
MIX_FUNC(2in_8out)
#undef MIX_FUNC

void FAudio_INTERNAL_InitSIMDFunctions(uint8_t hasSSE2, uint8_t hasNEON);

/* Decoders */

#define DECODE_FUNC(type) \
	extern void FAudio_INTERNAL_Decode##type( \
		FAudioVoice *voice, \
		FAudioBuffer *buffer, \
		float *decodeCache, \
		uint32_t samples \
	);
DECODE_FUNC(PCM8)
DECODE_FUNC(PCM16)
DECODE_FUNC(PCM24)
DECODE_FUNC(PCM32)
DECODE_FUNC(PCM32F)
DECODE_FUNC(MonoMSADPCM)
DECODE_FUNC(StereoMSADPCM)
DECODE_FUNC(WMAERROR)
#undef DECODE_FUNC

/* WMA decoding */

#ifdef HAVE_WMADEC
uint32_t FAudio_WMADEC_init(FAudioSourceVoice *pSourceVoice, uint32_t type);
void FAudio_WMADEC_free(FAudioSourceVoice *voice);
void FAudio_WMADEC_end_buffer(FAudioSourceVoice *voice);
#endif /* HAVE_WMADEC */

/* Platform Functions */

void FAudio_PlatformAddRef(void);
void FAudio_PlatformRelease(void);
void FAudio_PlatformInit(
	FAudio *audio,
	uint32_t flags,
	uint32_t deviceIndex,
	FAudioWaveFormatExtensible *mixFormat,
	uint32_t *updateSize,
	void** platformDevice
);
void FAudio_PlatformQuit(void* platformDevice);

uint32_t FAudio_PlatformGetDeviceCount(void);
uint32_t FAudio_PlatformGetDeviceDetails(
	uint32_t index,
	FAudioDeviceDetails *details
);

/* Threading */

FAudioThread FAudio_PlatformCreateThread(
	FAudioThreadFunc func,
	const char *name,
	void* data
);
void FAudio_PlatformWaitThread(FAudioThread thread, int32_t *retval);
void FAudio_PlatformThreadPriority(FAudioThreadPriority priority);
uint64_t FAudio_PlatformGetThreadID(void);
FAudioMutex FAudio_PlatformCreateMutex(void);
void FAudio_PlatformDestroyMutex(FAudioMutex mutex);
void FAudio_PlatformLockMutex(FAudioMutex mutex);
void FAudio_PlatformUnlockMutex(FAudioMutex mutex);
void FAudio_sleep(uint32_t ms);

/* Time */

uint32_t FAudio_timems(void);

/* WaveFormatExtensible Helpers */

static inline uint32_t GetMask(uint16_t channels)
{
	if (channels == 1) return SPEAKER_MONO;
	if (channels == 2) return SPEAKER_STEREO;
	if (channels == 3) return SPEAKER_2POINT1;
	if (channels == 4) return SPEAKER_QUAD;
	if (channels == 5) return SPEAKER_4POINT1;
	if (channels == 6) return SPEAKER_5POINT1;
	if (channels == 8) return SPEAKER_7POINT1;
	FAudio_assert(0 && "Unrecognized speaker layout!");
	return 0;
}

static inline void WriteWaveFormatExtensible(
	FAudioWaveFormatExtensible *fmt,
	int channels,
	int samplerate,
	const FAudioGUID *subformat
) {
	FAudio_assert(fmt != NULL);
	fmt->Format.wBitsPerSample = 32;
	fmt->Format.wFormatTag = FAUDIO_FORMAT_EXTENSIBLE;
	fmt->Format.nChannels = channels;
	fmt->Format.nSamplesPerSec = samplerate;
	fmt->Format.nBlockAlign = (
		fmt->Format.nChannels *
		(fmt->Format.wBitsPerSample / 8)
	);
	fmt->Format.nAvgBytesPerSec = (
		fmt->Format.nSamplesPerSec *
		fmt->Format.nBlockAlign
	);
	fmt->Format.cbSize = sizeof(FAudioWaveFormatExtensible) - sizeof(FAudioWaveFormatEx);
	fmt->Samples.wValidBitsPerSample = 32;
	fmt->dwChannelMask = GetMask(fmt->Format.nChannels);
	FAudio_memcpy(&fmt->SubFormat, subformat, sizeof(FAudioGUID));
}

/* Resampling */

/* Okay, so here's what all this fixed-point goo is for:
 *
 * Inevitably you're going to run into weird sample rates,
 * both from WaveBank data and from pitch shifting changes.
 *
 * How we deal with this is by calculating a fixed "step"
 * value that steps from sample to sample at the speed needed
 * to get the correct output sample rate, and the offset
 * is stored as separate integer and fraction values.
 *
 * This allows us to do weird fractional steps between samples,
 * while at the same time not letting it drift off into death
 * thanks to floating point madness.
 *
 * Steps are stored in fixed-point with 32 bits for the fraction:
 *
 * 00000000000000000000000000000000 00000000000000000000000000000000
 * ^ Integer block (32)             ^ Fraction block (32)
 *
 * For example, to get 1.5:
 * 00000000000000000000000000000001 10000000000000000000000000000000
 *
 * The Integer block works exactly like you'd expect.
 * The Fraction block is divided by the Integer's "One" value.
 * So, the above Fraction represented visually...
 *   1 << 31
 *   -------
 *   1 << 32
 * ... which, simplified, is...
 *   1 << 0
 *   ------
 *   1 << 1
 * ... in other words, 1 / 2, or 0.5.
 */
#define FIXED_PRECISION		32
#define FIXED_ONE		(1LL << FIXED_PRECISION)

/* Quick way to drop parts */
#define FIXED_FRACTION_MASK	(FIXED_ONE - 1)
#define FIXED_INTEGER_MASK	~FIXED_FRACTION_MASK

/* Helper macros to convert fixed to float */
#define DOUBLE_TO_FIXED(dbl) \
	((uint64_t) (dbl * FIXED_ONE + 0.5))
#define FIXED_TO_DOUBLE(fxd) ( \
	(double) (fxd >> FIXED_PRECISION) + /* Integer part */ \
	((fxd & FIXED_FRACTION_MASK) * (1.0 / FIXED_ONE)) /* Fraction part */ \
)
#define FIXED_TO_FLOAT(fxd) ( \
	(float) (fxd >> FIXED_PRECISION) + /* Integer part */ \
	((fxd & FIXED_FRACTION_MASK) * (1.0f / FIXED_ONE)) /* Fraction part */ \
)

#ifdef FAUDIO_DUMP_VOICES
/* File writing structure */
typedef size_t (FAUDIOCALL * FAudio_writefunc)(
	void *data,
	const void *src,
	size_t size,
	size_t count
);
typedef size_t (FAUDIOCALL * FAudio_sizefunc)(
	void *data
);
typedef struct FAudioIOStreamOut
{
	void *data;
	FAudio_readfunc read;
	FAudio_writefunc write;
	FAudio_seekfunc seek;
	FAudio_sizefunc size;
	FAudio_closefunc close;
	void *lock;
} FAudioIOStreamOut;

FAudioIOStreamOut* FAudio_fopen_out(const char *path, const char *mode);
void FAudio_close_out(FAudioIOStreamOut *io);
#endif /* FAUDIO_DUMP_VOICES */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
