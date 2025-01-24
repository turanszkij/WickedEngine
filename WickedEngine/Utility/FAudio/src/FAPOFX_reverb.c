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

#include "FAPOFX.h"
#include "FAudio_internal.h"

/* FXReverb FAPO Implementation */

const FAudioGUID FAPOFX_CLSID_FXReverb =
{
	0x7D9ACA56,
	0xCB68,
	0x4807,
	{
		0xB6,
		0x32,
		0xB1,
		0x37,
		0x35,
		0x2E,
		0x85,
		0x96
	}
};

static FAPORegistrationProperties FXReverbProperties =
{
	/* .clsid = */ {0},
	/* .FriendlyName = */
	{
		'F', 'X', 'R', 'e', 'v', 'e', 'r', 'b', '\0'
	},
	/*.CopyrightInfo = */
	{
		'C', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't', ' ', '(', 'c', ')',
		'E', 't', 'h', 'a', 'n', ' ', 'L', 'e', 'e', '\0'
	},
	/*.MajorVersion = */ 0,
	/*.MinorVersion = */ 0,
	/*.Flags = */(
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

const FAudioGUID FAPOFX_CLSID_FXReverb_LEGACY =
{
	0xA90BC001,
	0xE897,
	0xE897,
	{
		0x74,
		0x39,
		0x43,
		0x55,
		0x00,
		0x00,
		0x00,
		0x02
	}
};

static FAPORegistrationProperties FXReverbProperties_LEGACY =
{
	/* .clsid = */ {0},
	/* .FriendlyName = */
	{
		'F', 'X', 'R', 'e', 'v', 'e', 'r', 'b', '\0'
	},
	/*.CopyrightInfo = */
	{
		'C', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't', ' ', '(', 'c', ')',
		'E', 't', 'h', 'a', 'n', ' ', 'L', 'e', 'e', '\0'
	},
	/*.MajorVersion = */ 0,
	/*.MinorVersion = */ 0,
	/*.Flags = */(
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

typedef struct FAPOFXReverb
{
	FAPOBase base;

	/* TODO */
} FAPOFXReverb;

uint32_t FAPOFXReverb_Initialize(
	FAPOFXReverb *fapo,
	const void* pData,
	uint32_t DataByteSize
) {
	#define INITPARAMS(offset) \
		FAudio_memcpy( \
			fapo->base.m_pParameterBlocks + DataByteSize * offset, \
			pData, \
			DataByteSize \
		);
	INITPARAMS(0)
	INITPARAMS(1)
	INITPARAMS(2)
	#undef INITPARAMS
	return 0;
}

void FAPOFXReverb_Process(
	FAPOFXReverb *fapo,
	uint32_t InputProcessParameterCount,
	const FAPOProcessBufferParameters* pInputProcessParameters,
	uint32_t OutputProcessParameterCount,
	FAPOProcessBufferParameters* pOutputProcessParameters,
	int32_t IsEnabled
) {
	FAPOBase_BeginProcess(&fapo->base);

	/* TODO */

	FAPOBase_EndProcess(&fapo->base);
}

void FAPOFXReverb_Free(void* fapo)
{
	FAPOFXReverb *reverb = (FAPOFXReverb*) fapo;
	reverb->base.pFree(reverb->base.m_pParameterBlocks);
	reverb->base.pFree(fapo);
}

/* Public API */

uint32_t FAPOFXCreateReverb(
	FAPO **pEffect,
	const void *pInitData,
	uint32_t InitDataByteSize,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc,
	uint8_t legacy
) {
	const FAPOFXReverbParameters fxdefault =
	{
		FAPOFXREVERB_DEFAULT_DIFFUSION,
		FAPOFXREVERB_DEFAULT_ROOMSIZE,
	};

	/* Allocate... */
	FAPOFXReverb *result = (FAPOFXReverb*) customMalloc(
		sizeof(FAPOFXReverb)
	);
	uint8_t *params = (uint8_t*) customMalloc(
		sizeof(FAPOFXReverbParameters) * 3
	);
	if (pInitData == NULL)
	{
		FAudio_zero(params, sizeof(FAPOFXReverbParameters) * 3);
		#define INITPARAMS(offset) \
			FAudio_memcpy( \
				params + sizeof(FAPOFXReverbParameters) * offset, \
				&fxdefault, \
				sizeof(FAPOFXReverbParameters) \
			);
		INITPARAMS(0)
		INITPARAMS(1)
		INITPARAMS(2)
		#undef INITPARAMS
	}
	else
	{
		FAudio_assert(InitDataByteSize == sizeof(FAPOFXReverbParameters));
		FAudio_memcpy(params, pInitData, InitDataByteSize);
		FAudio_memcpy(params + InitDataByteSize, pInitData, InitDataByteSize);
		FAudio_memcpy(params + (InitDataByteSize * 2), pInitData, InitDataByteSize);
	}

	/* Initialize... */
	FAudio_memcpy(
		&FXReverbProperties_LEGACY.clsid,
		&FAPOFX_CLSID_FXReverb_LEGACY,
		sizeof(FAudioGUID)
	);
	FAudio_memcpy(
		&FXReverbProperties.clsid,
		&FAPOFX_CLSID_FXReverb,
		sizeof(FAudioGUID)
	);
	CreateFAPOBaseWithCustomAllocatorEXT(
		&result->base,
		legacy ? &FXReverbProperties_LEGACY : &FXReverbProperties,
		params,
		sizeof(FAPOFXReverbParameters),
		0,
		customMalloc,
		customFree,
		customRealloc
	);

	/* Function table... */
	result->base.base.Initialize = (InitializeFunc)
		FAPOFXReverb_Initialize;
	result->base.base.Process = (ProcessFunc)
		FAPOFXReverb_Process;
	result->base.Destructor = FAPOFXReverb_Free;

	/* Finally. */
	*pEffect = &result->base.base;
	return 0;
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
