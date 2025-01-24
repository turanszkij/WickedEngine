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

#include "FAPOBase.h"
#include "FAudio_internal.h"

/* FAPOBase Interface */

void CreateFAPOBase(
	FAPOBase *fapo,
	const FAPORegistrationProperties *pRegistrationProperties,
	uint8_t *pParameterBlocks,
	uint32_t uParameterBlockByteSize,
	uint8_t fProducer
) {
	CreateFAPOBaseWithCustomAllocatorEXT(
		fapo,
		pRegistrationProperties,
		pParameterBlocks,
		uParameterBlockByteSize,
		fProducer,
		FAudio_malloc,
		FAudio_free,
		FAudio_realloc
	);
}

void CreateFAPOBaseWithCustomAllocatorEXT(
	FAPOBase *fapo,
	const FAPORegistrationProperties *pRegistrationProperties,
	uint8_t *pParameterBlocks,
	uint32_t uParameterBlockByteSize,
	uint8_t fProducer,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
) {
	/* Base Classes/Interfaces */
	#define ASSIGN_VT(name) \
		fapo->base.name = (name##Func) FAPOBase_##name;
	ASSIGN_VT(AddRef)
	ASSIGN_VT(Release)
	ASSIGN_VT(GetRegistrationProperties)
	ASSIGN_VT(IsInputFormatSupported)
	ASSIGN_VT(IsOutputFormatSupported)
	ASSIGN_VT(Initialize)
	ASSIGN_VT(Reset)
	ASSIGN_VT(LockForProcess)
	ASSIGN_VT(UnlockForProcess)
	ASSIGN_VT(CalcInputFrames)
	ASSIGN_VT(CalcOutputFrames)
	ASSIGN_VT(SetParameters)
	ASSIGN_VT(GetParameters)
	#undef ASSIGN_VT

	/* Public Virtual Functions */
	fapo->OnSetParameters = (OnSetParametersFunc)
		FAPOBase_OnSetParameters;

	/* Private Variables */
	fapo->m_pRegistrationProperties = pRegistrationProperties; /* FIXME */
	fapo->m_pfnMatrixMixFunction = NULL; /* FIXME */
	fapo->m_pfl32MatrixCoefficients = NULL; /* FIXME */
	fapo->m_nSrcFormatType = 0; /* FIXME */
	fapo->m_fIsScalarMatrix = 0; /* FIXME: */
	fapo->m_fIsLocked = 0;
	fapo->m_pParameterBlocks = pParameterBlocks;
	fapo->m_pCurrentParameters = pParameterBlocks;
	fapo->m_pCurrentParametersInternal = pParameterBlocks;
	fapo->m_uCurrentParametersIndex = 0;
	fapo->m_uParameterBlockByteSize = uParameterBlockByteSize;
	fapo->m_fNewerResultsReady = 0;
	fapo->m_fProducer = fProducer;

	/* Allocator Callbacks */
	fapo->pMalloc = customMalloc;
	fapo->pFree = customFree;
	fapo->pRealloc = customRealloc;

	/* Protected Variables */
	fapo->m_lReferenceCount = 1;
}

int32_t FAPOBase_AddRef(FAPOBase *fapo)
{
	fapo->m_lReferenceCount += 1;
	return fapo->m_lReferenceCount;
}

int32_t FAPOBase_Release(FAPOBase *fapo)
{
	fapo->m_lReferenceCount -= 1;
	if (fapo->m_lReferenceCount == 0)
	{
		fapo->Destructor(fapo);
		return 0;
	}
	return fapo->m_lReferenceCount;
}

uint32_t FAPOBase_GetRegistrationProperties(
	FAPOBase *fapo,
	FAPORegistrationProperties **ppRegistrationProperties
) {
	*ppRegistrationProperties = (FAPORegistrationProperties*) fapo->pMalloc(
		sizeof(FAPORegistrationProperties)
	);
	FAudio_memcpy(
		*ppRegistrationProperties,
		fapo->m_pRegistrationProperties,
		sizeof(FAPORegistrationProperties)
	);
	return 0;
}

uint32_t FAPOBase_IsInputFormatSupported(
	FAPOBase *fapo,
	const FAudioWaveFormatEx *pOutputFormat,
	const FAudioWaveFormatEx *pRequestedInputFormat,
	FAudioWaveFormatEx **ppSupportedInputFormat
) {
	if (	pRequestedInputFormat->wFormatTag != FAPOBASE_DEFAULT_FORMAT_TAG ||
		pRequestedInputFormat->nChannels < FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS ||
		pRequestedInputFormat->nChannels > FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS ||
		pRequestedInputFormat->nSamplesPerSec < FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE ||
		pRequestedInputFormat->nSamplesPerSec > FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE ||
		pRequestedInputFormat->wBitsPerSample != FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE	)
	{
		if (ppSupportedInputFormat != NULL)
		{
			(*ppSupportedInputFormat)->wFormatTag =
				FAPOBASE_DEFAULT_FORMAT_TAG;
			(*ppSupportedInputFormat)->nChannels = FAudio_clamp(
				pRequestedInputFormat->nChannels,
				FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS,
				FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS
			);
			(*ppSupportedInputFormat)->nSamplesPerSec = FAudio_clamp(
				pRequestedInputFormat->nSamplesPerSec,
				FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE,
				FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE
			);
			(*ppSupportedInputFormat)->wBitsPerSample =
				FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE;
		}
		return FAPO_E_FORMAT_UNSUPPORTED;
	}
	return 0;
}

uint32_t FAPOBase_IsOutputFormatSupported(
	FAPOBase *fapo,
	const FAudioWaveFormatEx *pInputFormat,
	const FAudioWaveFormatEx *pRequestedOutputFormat,
	FAudioWaveFormatEx **ppSupportedOutputFormat
) {
	if (	pRequestedOutputFormat->wFormatTag != FAPOBASE_DEFAULT_FORMAT_TAG ||
		pRequestedOutputFormat->nChannels < FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS ||
		pRequestedOutputFormat->nChannels > FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS ||
		pRequestedOutputFormat->nSamplesPerSec < FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE ||
		pRequestedOutputFormat->nSamplesPerSec > FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE ||
		pRequestedOutputFormat->wBitsPerSample != FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE	)
	{
		if (ppSupportedOutputFormat != NULL)
		{
			(*ppSupportedOutputFormat)->wFormatTag =
				FAPOBASE_DEFAULT_FORMAT_TAG;
			(*ppSupportedOutputFormat)->nChannels = FAudio_clamp(
				pRequestedOutputFormat->nChannels,
				FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS,
				FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS
			);
			(*ppSupportedOutputFormat)->nSamplesPerSec = FAudio_clamp(
				pRequestedOutputFormat->nSamplesPerSec,
				FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE,
				FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE
			);
			(*ppSupportedOutputFormat)->wBitsPerSample =
				FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE;
		}
		return FAPO_E_FORMAT_UNSUPPORTED;
	}
	return 0;
}

uint32_t FAPOBase_Initialize(
	FAPOBase *fapo,
	const void* pData,
	uint32_t DataByteSize
) {
	return 0;
}

void FAPOBase_Reset(FAPOBase *fapo)
{
}

uint32_t FAPOBase_LockForProcess(
	FAPOBase *fapo,
	uint32_t InputLockedParameterCount,
	const FAPOLockForProcessBufferParameters *pInputLockedParameters,
	uint32_t OutputLockedParameterCount,
	const FAPOLockForProcessBufferParameters *pOutputLockedParameters
) {
	/* Verify parameter counts... */
	if (	InputLockedParameterCount < fapo->m_pRegistrationProperties->MinInputBufferCount ||
		InputLockedParameterCount > fapo->m_pRegistrationProperties->MaxInputBufferCount ||
		OutputLockedParameterCount < fapo->m_pRegistrationProperties->MinOutputBufferCount ||
		OutputLockedParameterCount > fapo->m_pRegistrationProperties->MaxOutputBufferCount	)
	{
		return FAUDIO_E_INVALID_ARG;
	}


	/* Validate input/output formats */
	#define VERIFY_FORMAT_FLAG(flag, prop) \
		if (	(fapo->m_pRegistrationProperties->Flags & flag) && \
			(pInputLockedParameters->pFormat->prop != pOutputLockedParameters->pFormat->prop)	) \
		{ \
			return FAUDIO_E_INVALID_ARG; \
		}
	VERIFY_FORMAT_FLAG(FAPO_FLAG_CHANNELS_MUST_MATCH, nChannels)
	VERIFY_FORMAT_FLAG(FAPO_FLAG_FRAMERATE_MUST_MATCH, nSamplesPerSec)
	VERIFY_FORMAT_FLAG(FAPO_FLAG_BITSPERSAMPLE_MUST_MATCH, wBitsPerSample)
	#undef VERIFY_FORMAT_FLAG
	if (	(fapo->m_pRegistrationProperties->Flags & FAPO_FLAG_BUFFERCOUNT_MUST_MATCH) &&
		(InputLockedParameterCount != OutputLockedParameterCount)	)
	{
		return FAUDIO_E_INVALID_ARG;
	}
	fapo->m_fIsLocked = 1;
	return 0;
}

void FAPOBase_UnlockForProcess(FAPOBase *fapo)
{
	fapo->m_fIsLocked = 0;
}

uint32_t FAPOBase_CalcInputFrames(FAPOBase *fapo, uint32_t OutputFrameCount)
{
	return OutputFrameCount;
}

uint32_t FAPOBase_CalcOutputFrames(FAPOBase *fapo, uint32_t InputFrameCount)
{
	return InputFrameCount;
}

uint32_t FAPOBase_ValidateFormatDefault(
	FAPOBase *fapo,
	FAudioWaveFormatEx *pFormat,
	uint8_t fOverwrite
) {
	if (	pFormat->wFormatTag != FAPOBASE_DEFAULT_FORMAT_TAG ||
		pFormat->nChannels < FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS ||
		pFormat->nChannels > FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS ||
		pFormat->nSamplesPerSec < FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE ||
		pFormat->nSamplesPerSec > FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE ||
		pFormat->wBitsPerSample != FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE	)
	{
		if (fOverwrite)
		{
			pFormat->wFormatTag =
				FAPOBASE_DEFAULT_FORMAT_TAG;
			pFormat->nChannels = FAudio_clamp(
				pFormat->nChannels,
				FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS,
				FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS
			);
			pFormat->nSamplesPerSec = FAudio_clamp(
				pFormat->nSamplesPerSec,
				FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE,
				FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE
			);
			pFormat->wBitsPerSample =
				FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE;
		}
		return FAPO_E_FORMAT_UNSUPPORTED;
	}
	return 0;
}

uint32_t FAPOBase_ValidateFormatPair(
	FAPOBase *fapo,
	const FAudioWaveFormatEx *pSupportedFormat,
	FAudioWaveFormatEx *pRequestedFormat,
	uint8_t fOverwrite
) {
	if (	pRequestedFormat->wFormatTag != FAPOBASE_DEFAULT_FORMAT_TAG ||
		pRequestedFormat->nChannels < FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS ||
		pRequestedFormat->nChannels > FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS ||
		pRequestedFormat->nSamplesPerSec < FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE ||
		pRequestedFormat->nSamplesPerSec > FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE ||
		pRequestedFormat->wBitsPerSample != FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE	)
	{
		if (fOverwrite)
		{
			pRequestedFormat->wFormatTag =
				FAPOBASE_DEFAULT_FORMAT_TAG;
			pRequestedFormat->nChannels = FAudio_clamp(
				pRequestedFormat->nChannels,
				FAPOBASE_DEFAULT_FORMAT_MIN_CHANNELS,
				FAPOBASE_DEFAULT_FORMAT_MAX_CHANNELS
			);
			pRequestedFormat->nSamplesPerSec = FAudio_clamp(
				pRequestedFormat->nSamplesPerSec,
				FAPOBASE_DEFAULT_FORMAT_MIN_FRAMERATE,
				FAPOBASE_DEFAULT_FORMAT_MAX_FRAMERATE
			);
			pRequestedFormat->wBitsPerSample =
				FAPOBASE_DEFAULT_FORMAT_BITSPERSAMPLE;
		}
		return FAPO_E_FORMAT_UNSUPPORTED;
	}
	return 0;
}

void FAPOBase_ProcessThru(
	FAPOBase *fapo,
	void* pInputBuffer,
	float *pOutputBuffer,
	uint32_t FrameCount,
	uint16_t InputChannelCount,
	uint16_t OutputChannelCount,
	uint8_t MixWithOutput
) {
	uint32_t i, co, ci;
	float *input = (float*) pInputBuffer;

	if (MixWithOutput)
	{
		/* TODO: SSE */
		for (i = 0; i < FrameCount; i += 1)
		for (co = 0; co < OutputChannelCount; co += 1)
		for (ci = 0; ci < InputChannelCount; ci += 1)
		{
			/* Add, don't overwrite! */
			pOutputBuffer[i * OutputChannelCount + co] +=
				input[i * InputChannelCount + ci];
		}
	}
	else
	{
		/* TODO: SSE */
		for (i = 0; i < FrameCount; i += 1)
		for (co = 0; co < OutputChannelCount; co += 1)
		for (ci = 0; ci < InputChannelCount; ci += 1)
		{
			/* Overwrite, don't add! */
			pOutputBuffer[i * OutputChannelCount + co] =
				input[i * InputChannelCount + ci];
		}
	}
}

void FAPOBase_SetParameters(
	FAPOBase *fapo,
	const void* pParameters,
	uint32_t ParameterByteSize
) {
	FAudio_assert(!fapo->m_fProducer);

	/* User callback for validation */
	fapo->OnSetParameters(
		fapo,
		pParameters,
		ParameterByteSize
	);

	/* Increment parameter block index... */
	fapo->m_uCurrentParametersIndex += 1;
	if (fapo->m_uCurrentParametersIndex == 3)
	{
		fapo->m_uCurrentParametersIndex = 0;
	}
	fapo->m_pCurrentParametersInternal = fapo->m_pParameterBlocks + (
		fapo->m_uParameterBlockByteSize *
		fapo->m_uCurrentParametersIndex
	);

	/* Copy to what will eventually be the next parameter update */
	FAudio_memcpy(
		fapo->m_pCurrentParametersInternal,
		pParameters,
		ParameterByteSize
	);
}

void FAPOBase_GetParameters(
	FAPOBase *fapo,
	void* pParameters,
	uint32_t ParameterByteSize
) {
	/* Copy what's current as of the last Process */
	FAudio_memcpy(
		pParameters,
		fapo->m_pCurrentParameters,
		ParameterByteSize
	);
}

void FAPOBase_OnSetParameters(
	FAPOBase *fapo,
	const void* parameters,
	uint32_t parametersSize
) {
}

uint8_t FAPOBase_ParametersChanged(FAPOBase *fapo)
{
	/* Internal will get updated when SetParameters is called */
	return fapo->m_pCurrentParametersInternal != fapo->m_pCurrentParameters;
}

uint8_t* FAPOBase_BeginProcess(FAPOBase *fapo)
{
	/* Set the latest block as "current", this is what Process will use now */
	fapo->m_pCurrentParameters = fapo->m_pCurrentParametersInternal;
	return fapo->m_pCurrentParameters;
}

void FAPOBase_EndProcess(FAPOBase *fapo)
{
	/* I'm 100% sure my parameter block increment is wrong... */
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
