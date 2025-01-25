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

#include "FAudioFX.h"
#include "FAudio_internal.h"

/* Volume Meter FAPO Implementation */

const FAudioGUID FAudioFX_CLSID_AudioVolumeMeter = /* 2.7 */
{
	0xCAC1105F,
	0x619B,
	0x4D04,
	{
		0x83,
		0x1A,
		0x44,
		0xE1,
		0xCB,
		0xF1,
		0x2D,
		0x57
	}
};

static FAPORegistrationProperties VolumeMeterProperties =
{
	/* .clsid = */ {0},
	/* .FriendlyName = */
	{
		'V', 'o', 'l', 'u', 'm', 'e', 'M', 'e', 't', 'e', 'r', '\0'
	},
	/*.CopyrightInfo = */
	{
		'C', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't', ' ', '(', 'c', ')',
		'E', 't', 'h', 'a', 'n', ' ', 'L', 'e', 'e', '\0'
	},
	/*.MajorVersion = */ 0,
	/*.MinorVersion = */ 0,
	/*.Flags = */(
		FAPO_FLAG_CHANNELS_MUST_MATCH |
		FAPO_FLAG_FRAMERATE_MUST_MATCH |
		FAPO_FLAG_BITSPERSAMPLE_MUST_MATCH |
		FAPO_FLAG_BUFFERCOUNT_MUST_MATCH |
		FAPO_FLAG_INPLACE_SUPPORTED |
		FAPO_FLAG_INPLACE_REQUIRED
	),
	/*.MinInputBufferCount = */ 1,
	/*.MaxInputBufferCount = */  1,
	/*.MinOutputBufferCount = */ 1,
	/*.MaxOutputBufferCount =*/ 1
};

typedef struct FAudioFXVolumeMeter
{
	FAPOBase base;
	uint16_t channels;
} FAudioFXVolumeMeter;

uint32_t FAudioFXVolumeMeter_LockForProcess(
	FAudioFXVolumeMeter *fapo,
	uint32_t InputLockedParameterCount,
	const FAPOLockForProcessBufferParameters *pInputLockedParameters,
	uint32_t OutputLockedParameterCount,
	const FAPOLockForProcessBufferParameters *pOutputLockedParameters
) {
	FAudioFXVolumeMeterLevels *levels = (FAudioFXVolumeMeterLevels*)
		fapo->base.m_pParameterBlocks;

	/* Verify parameter counts... */
	if (	InputLockedParameterCount < fapo->base.m_pRegistrationProperties->MinInputBufferCount ||
		InputLockedParameterCount > fapo->base.m_pRegistrationProperties->MaxInputBufferCount ||
		OutputLockedParameterCount < fapo->base.m_pRegistrationProperties->MinOutputBufferCount ||
		OutputLockedParameterCount > fapo->base.m_pRegistrationProperties->MaxOutputBufferCount	)
	{
		return FAUDIO_E_INVALID_ARG;
	}


	/* Validate input/output formats */
	#define VERIFY_FORMAT_FLAG(flag, prop) \
		if (	(fapo->base.m_pRegistrationProperties->Flags & flag) && \
			(pInputLockedParameters->pFormat->prop != pOutputLockedParameters->pFormat->prop)	) \
		{ \
			return FAUDIO_E_INVALID_ARG; \
		}
	VERIFY_FORMAT_FLAG(FAPO_FLAG_CHANNELS_MUST_MATCH, nChannels)
	VERIFY_FORMAT_FLAG(FAPO_FLAG_FRAMERATE_MUST_MATCH, nSamplesPerSec)
	VERIFY_FORMAT_FLAG(FAPO_FLAG_BITSPERSAMPLE_MUST_MATCH, wBitsPerSample)
	#undef VERIFY_FORMAT_FLAG
	if (	(fapo->base.m_pRegistrationProperties->Flags & FAPO_FLAG_BUFFERCOUNT_MUST_MATCH) &&
		(InputLockedParameterCount != OutputLockedParameterCount)	)
	{
		return FAUDIO_E_INVALID_ARG;
	}

	/* Allocate volume meter arrays */
	fapo->channels = pInputLockedParameters->pFormat->nChannels;
	levels[0].pPeakLevels = (float*) fapo->base.pMalloc(
		fapo->channels * sizeof(float) * 6
	);
	FAudio_zero(levels[0].pPeakLevels, fapo->channels * sizeof(float) * 6);
	levels[0].pRMSLevels = levels[0].pPeakLevels + fapo->channels;
	levels[1].pPeakLevels = levels[0].pPeakLevels + (fapo->channels * 2);
	levels[1].pRMSLevels = levels[0].pPeakLevels + (fapo->channels * 3);
	levels[2].pPeakLevels = levels[0].pPeakLevels + (fapo->channels * 4);
	levels[2].pRMSLevels = levels[0].pPeakLevels + (fapo->channels * 5);

	fapo->base.m_fIsLocked = 1;
	return 0;
}

void FAudioFXVolumeMeter_UnlockForProcess(FAudioFXVolumeMeter *fapo)
{
	FAudioFXVolumeMeterLevels *levels = (FAudioFXVolumeMeterLevels*)
		fapo->base.m_pParameterBlocks;
	fapo->base.pFree(levels[0].pPeakLevels);
	fapo->base.m_fIsLocked = 0;
}

void FAudioFXVolumeMeter_Process(
	FAudioFXVolumeMeter *fapo,
	uint32_t InputProcessParameterCount,
	const FAPOProcessBufferParameters* pInputProcessParameters,
	uint32_t OutputProcessParameterCount,
	FAPOProcessBufferParameters* pOutputProcessParameters,
	int32_t IsEnabled
) {
	float peak;
	float total;
	float *buffer;
	uint32_t i, j;
	FAudioFXVolumeMeterLevels *levels = (FAudioFXVolumeMeterLevels*)
		FAPOBase_BeginProcess(&fapo->base);

	/* TODO: This could probably be SIMD-ified... */
	for (i = 0; i < fapo->channels; i += 1)
	{
		peak = 0.0f;
		total = 0.0f;
		buffer = ((float*) pInputProcessParameters->pBuffer) + i;
		for (j = 0; j < pInputProcessParameters->ValidFrameCount; j += 1, buffer += fapo->channels)
		{
			const float sampleAbs = FAudio_fabsf(*buffer);
			if (sampleAbs > peak)
			{
				peak = sampleAbs;
			}
			total += (*buffer) * (*buffer);
		}
		levels->pPeakLevels[i] = peak;
		levels->pRMSLevels[i] = FAudio_sqrtf(
			total / pInputProcessParameters->ValidFrameCount
		);
	}

	FAPOBase_EndProcess(&fapo->base);
}

void FAudioFXVolumeMeter_GetParameters(
	FAudioFXVolumeMeter *fapo,
	FAudioFXVolumeMeterLevels *pParameters,
	uint32_t ParameterByteSize
) {
	FAudioFXVolumeMeterLevels *levels = (FAudioFXVolumeMeterLevels*)
		fapo->base.m_pCurrentParameters;
	FAudio_assert(ParameterByteSize == sizeof(FAudioFXVolumeMeterLevels));
	FAudio_assert(pParameters->ChannelCount == fapo->channels);

	/* Copy what's current as of the last Process */
	if (pParameters->pPeakLevels != NULL)
	{
		FAudio_memcpy(
			pParameters->pPeakLevels,
			levels->pPeakLevels,
			fapo->channels * sizeof(float)
		);
	}
	if (pParameters->pRMSLevels != NULL)
	{
		FAudio_memcpy(
			pParameters->pRMSLevels,
			levels->pRMSLevels,
			fapo->channels * sizeof(float)
		);
	}
}

void FAudioFXVolumeMeter_Free(void* fapo)
{
	FAudioFXVolumeMeter *volumemeter = (FAudioFXVolumeMeter*) fapo;
	volumemeter->base.pFree(volumemeter->base.m_pParameterBlocks);
	volumemeter->base.pFree(fapo);
}

/* Public API */

uint32_t FAudioCreateVolumeMeter(FAPO** ppApo, uint32_t Flags)
{
	return FAudioCreateVolumeMeterWithCustomAllocatorEXT(
		ppApo,
		Flags,
		FAudio_malloc,
		FAudio_free,
		FAudio_realloc
	);
}

uint32_t FAudioCreateVolumeMeterWithCustomAllocatorEXT(
	FAPO** ppApo,
	uint32_t Flags,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
) {
	/* Allocate... */
	FAudioFXVolumeMeter *result = (FAudioFXVolumeMeter*) customMalloc(
		sizeof(FAudioFXVolumeMeter)
	);
	uint8_t *params = (uint8_t*) customMalloc(
		sizeof(FAudioFXVolumeMeterLevels) * 3
	);
	FAudio_zero(params, sizeof(FAudioFXVolumeMeterLevels) * 3);

	/* Initialize... */
	FAudio_memcpy(
		&VolumeMeterProperties.clsid,
		&FAudioFX_CLSID_AudioVolumeMeter,
		sizeof(FAudioGUID)
	);
	CreateFAPOBaseWithCustomAllocatorEXT(
		&result->base,
		&VolumeMeterProperties,
		params,
		sizeof(FAudioFXVolumeMeterLevels),
		1,
		customMalloc,
		customFree,
		customRealloc
	);

	/* Function table... */
	result->base.base.LockForProcess = (LockForProcessFunc)
		FAudioFXVolumeMeter_LockForProcess;
	result->base.base.UnlockForProcess = (UnlockForProcessFunc)
		FAudioFXVolumeMeter_UnlockForProcess;
	result->base.base.Process = (ProcessFunc)
		FAudioFXVolumeMeter_Process;
	result->base.base.GetParameters = (GetParametersFunc)
		FAudioFXVolumeMeter_GetParameters;
	result->base.Destructor = FAudioFXVolumeMeter_Free;

	/* Finally. */
	*ppApo = &result->base.base;
	return 0;
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
