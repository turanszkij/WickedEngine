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

/* This file has no documentation since the MSDN docs are still perfectly fine:
 * https://docs.microsoft.com/en-us/windows/desktop/api/xapobase/
 *
 * Of course, the APIs aren't exactly the same since XAPO is super dependent on
 * C++. Instead, we use a struct full of functions to mimic a vtable.
 *
 * To mimic the CXAPOParametersBase experience, initialize the object like this:
 *
 * extern FAPORegistrationProperties MyFAPOProperties;
 * extern int32_t producer;
 * typedef struct MyFAPOParams
 * {
 *	uint32_t something;
 * } MyFAPOParams;
 * typedef struct MyFAPO
 * {
 *	FAPOBase base;
 *	uint32_t somethingElse;
 * } MyFAPO;
 * void MyFAPO_Free(void* fapo)
 * {
 *	MyFAPO *mine = (MyFAPO*) fapo;
 *	mine->base.pFree(mine->base.m_pParameterBlocks);
 *	mine->base.pFree(fapo);
 * }
 *
 * MyFAPO *result = (MyFAPO*) SDL_malloc(sizeof(MyFAPO));
 * uint8_t *params = (uint8_t*) SDL_malloc(sizeof(MyFAPOParams) * 3);
 * CreateFAPOBase(
 *	&result->base,
 *	&MyFAPOProperties,
 *	params,
 *	sizeof(MyFAPOParams),
 *	producer
 * );
 * result->base.base.Initialize = (InitializeFunc) MyFAPO_Initialize;
 * result->base.base.Process = (ProcessFunc) MyFAPO_Process;
 * result->base.Destructor = MyFAPO_Free;
 */

#ifndef FAPOBASE_H
#define FAPOBASE_H

#include "FAPO.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Constants */

#define FAPOBASE_DEFAULT_FORMAT_TAG		FAUDIO_FORMAT_IEEE_FLOAT
#define FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS	FAPO_MIN_CHANNELS
#define FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS	FAPO_MAX_CHANNELS
#define FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE	FAPO_MIN_FRAMERATE
#define FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE	FAPO_MAX_FRAMERATE
#define FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE	32

#define FAPOBASE_DEFAULT_FLAG ( \
	FAPO_FLAG_CHANNELS_MUST_MATCH | \
	FAPO_FLAG_FRAMERATE_MUST_MATCH | \
	FAPO_FLAG_BITSPERSAMPLE_MUST_MATCH | \
	FAPO_FLAG_BUFFERCOUNT_MUST_MATCH | \
	FAPO_FLAG_INPLACE_SUPPORTED \
)

#define FAPOBASE_DEFAULT_BUFFER_COUNT 1

/* FAPOBase Interface */

typedef struct FAPOBase FAPOBase;

typedef void (FAPOCALL * OnSetParametersFunc)(
	FAPOBase *fapo,
	const void* parameters,
	uint32_t parametersSize
);

#pragma pack(push, 8)
struct FAPOBase
{
	/* Base Classes/Interfaces */
	FAPO base;
	void (FAPOCALL *Destructor)(void*);

	/* Public Virtual Functions */
	OnSetParametersFunc OnSetParameters;

	/* Private Variables */
	const FAPORegistrationProperties *m_pRegistrationProperties;
	void* m_pfnMatrixMixFunction;
	float *m_pfl32MatrixCoefficients;
	uint32_t m_nSrcFormatType;
	uint8_t m_fIsScalarMatrix;
	uint8_t m_fIsLocked;
	uint8_t *m_pParameterBlocks;
	uint8_t *m_pCurrentParameters;
	uint8_t *m_pCurrentParametersInternal;
	uint32_t m_uCurrentParametersIndex;
	uint32_t m_uParameterBlockByteSize;
	uint8_t m_fNewerResultsReady;
	uint8_t m_fProducer;

	/* Protected Variables */
	int32_t m_lReferenceCount; /* LONG */

	/* Allocator callbacks, NOT part of XAPOBase spec! */
	FAudioMallocFunc pMalloc;
	FAudioFreeFunc pFree;
	FAudioReallocFunc pRealloc;
};
#pragma pack(pop)

FAPOAPI void CreateFAPOBase(
	FAPOBase *fapo,
	const FAPORegistrationProperties *pRegistrationProperties,
	uint8_t *pParameterBlocks,
	uint32_t uParameterBlockByteSize,
	uint8_t fProducer
);

/* See "extensions/CustomAllocatorEXT.txt" for more information. */
FAPOAPI void CreateFAPOBaseWithCustomAllocatorEXT(
	FAPOBase *fapo,
	const FAPORegistrationProperties *pRegistrationProperties,
	uint8_t *pParameterBlocks,
	uint32_t uParameterBlockByteSize,
	uint8_t fProducer,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
);

FAPOAPI int32_t FAPOBase_AddRef(FAPOBase *fapo);

FAPOAPI int32_t FAPOBase_Release(FAPOBase *fapo);

FAPOAPI uint32_t FAPOBase_GetRegistrationProperties(
	FAPOBase *fapo,
	FAPORegistrationProperties **ppRegistrationProperties
);

FAPOAPI uint32_t FAPOBase_IsInputFormatSupported(
	FAPOBase *fapo,
	const FAudioWaveFormatEx *pOutputFormat,
	const FAudioWaveFormatEx *pRequestedInputFormat,
	FAudioWaveFormatEx **ppSupportedInputFormat
);

FAPOAPI uint32_t FAPOBase_IsOutputFormatSupported(
	FAPOBase *fapo,
	const FAudioWaveFormatEx *pInputFormat,
	const FAudioWaveFormatEx *pRequestedOutputFormat,
	FAudioWaveFormatEx **ppSupportedOutputFormat
);

FAPOAPI uint32_t FAPOBase_Initialize(
	FAPOBase *fapo,
	const void* pData,
	uint32_t DataByteSize
);

FAPOAPI void FAPOBase_Reset(FAPOBase *fapo);

FAPOAPI uint32_t FAPOBase_LockForProcess(
	FAPOBase *fapo,
	uint32_t InputLockedParameterCount,
	const FAPOLockForProcessBufferParameters *pInputLockedParameters,
	uint32_t OutputLockedParameterCount,
	const FAPOLockForProcessBufferParameters *pOutputLockedParameters
);

FAPOAPI void FAPOBase_UnlockForProcess(FAPOBase *fapo);

FAPOAPI uint32_t FAPOBase_CalcInputFrames(
	FAPOBase *fapo,
	uint32_t OutputFrameCount
);

FAPOAPI uint32_t FAPOBase_CalcOutputFrames(
	FAPOBase *fapo,
	uint32_t InputFrameCount
);

FAPOAPI uint32_t FAPOBase_ValidateFormatDefault(
	FAPOBase *fapo,
	FAudioWaveFormatEx *pFormat,
	uint8_t fOverwrite
);

FAPOAPI uint32_t FAPOBase_ValidateFormatPair(
	FAPOBase *fapo,
	const FAudioWaveFormatEx *pSupportedFormat,
	FAudioWaveFormatEx *pRequestedFormat,
	uint8_t fOverwrite
);

FAPOAPI void FAPOBase_ProcessThru(
	FAPOBase *fapo,
	void* pInputBuffer,
	float *pOutputBuffer,
	uint32_t FrameCount,
	uint16_t InputChannelCount,
	uint16_t OutputChannelCount,
	uint8_t MixWithOutput
);

FAPOAPI void FAPOBase_SetParameters(
	FAPOBase *fapo,
	const void* pParameters,
	uint32_t ParameterByteSize
);

FAPOAPI void FAPOBase_GetParameters(
	FAPOBase *fapo,
	void* pParameters,
	uint32_t ParameterByteSize
);

FAPOAPI void FAPOBase_OnSetParameters(
	FAPOBase *fapo,
	const void* parameters,
	uint32_t parametersSize
);

FAPOAPI uint8_t FAPOBase_ParametersChanged(FAPOBase *fapo);

FAPOAPI uint8_t* FAPOBase_BeginProcess(FAPOBase *fapo);

FAPOAPI void FAPOBase_EndProcess(FAPOBase *fapo);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FAPOBASE_H */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
