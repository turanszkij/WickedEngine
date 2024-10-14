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

/* FXEQ FAPO Implementation */

const FAudioGUID FAPOFX_CLSID_FXEQ =
{
	0xF5E01117,
	0xD6C4,
	0x485A,
	{
		0xA3,
		0xF5,
		0x69,
		0x51,
		0x96,
		0xF3,
		0xDB,
		0xFA
	}
};

static FAPORegistrationProperties FXEQProperties =
{
	/* .clsid = */ {0},
	/* .FriendlyName = */
	{
		'F', 'X', 'E', 'Q', '\0'
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

const FAudioGUID FAPOFX_CLSID_FXEQ_LEGACY =
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
		0x00
	}
};

static FAPORegistrationProperties FXEQProperties_LEGACY =
{
	/* .clsid = */ {0},
	/* .FriendlyName = */
	{
		'F', 'X', 'E', 'Q', '\0'
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

typedef struct FAPOFXEQ
{
	FAPOBase base;

	/* TODO */
} FAPOFXEQ;

uint32_t FAPOFXEQ_Initialize(
	FAPOFXEQ *fapo,
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

void FAPOFXEQ_Process(
	FAPOFXEQ *fapo,
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

void FAPOFXEQ_Free(void* fapo)
{
	FAPOFXEQ *eq = (FAPOFXEQ*) fapo;
	eq->base.pFree(eq->base.m_pParameterBlocks);
	eq->base.pFree(fapo);
}

/* Public API */

uint32_t FAPOFXCreateEQ(
	FAPO **pEffect,
	const void *pInitData,
	uint32_t InitDataByteSize,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc,
	uint8_t legacy
) {
	const FAPOFXEQParameters fxdefault =
	{
		FAPOFXEQ_DEFAULT_FREQUENCY_CENTER_0,
		FAPOFXEQ_DEFAULT_GAIN,
		FAPOFXEQ_DEFAULT_BANDWIDTH,
		FAPOFXEQ_DEFAULT_FREQUENCY_CENTER_1,
		FAPOFXEQ_DEFAULT_GAIN,
		FAPOFXEQ_DEFAULT_BANDWIDTH,
		FAPOFXEQ_DEFAULT_FREQUENCY_CENTER_2,
		FAPOFXEQ_DEFAULT_GAIN,
		FAPOFXEQ_DEFAULT_BANDWIDTH,
		FAPOFXEQ_DEFAULT_FREQUENCY_CENTER_3,
		FAPOFXEQ_DEFAULT_GAIN,
		FAPOFXEQ_DEFAULT_BANDWIDTH
	};

	/* Allocate... */
	FAPOFXEQ *result = (FAPOFXEQ*) customMalloc(
		sizeof(FAPOFXEQ)
	);
	uint8_t *params = (uint8_t*) customMalloc(
		sizeof(FAPOFXEQParameters) * 3
	);
	if (pInitData == NULL)
	{
		FAudio_zero(params, sizeof(FAPOFXEQParameters) * 3);
		#define INITPARAMS(offset) \
			FAudio_memcpy( \
				params + sizeof(FAPOFXEQParameters) * offset, \
				&fxdefault, \
				sizeof(FAPOFXEQParameters) \
			);
		INITPARAMS(0)
		INITPARAMS(1)
		INITPARAMS(2)
		#undef INITPARAMS
	}
	else
	{
		FAudio_assert(InitDataByteSize == sizeof(FAPOFXEQParameters));
		FAudio_memcpy(params, pInitData, InitDataByteSize);
		FAudio_memcpy(params + InitDataByteSize, pInitData, InitDataByteSize);
		FAudio_memcpy(params + (InitDataByteSize * 2), pInitData, InitDataByteSize);
	}

	/* Initialize... */
	FAudio_memcpy(
		&FXEQProperties_LEGACY.clsid,
		&FAPOFX_CLSID_FXEQ_LEGACY,
		sizeof(FAudioGUID)
	);
	FAudio_memcpy(
		&FXEQProperties.clsid,
		&FAPOFX_CLSID_FXEQ,
		sizeof(FAudioGUID)
	);
	CreateFAPOBaseWithCustomAllocatorEXT(
		&result->base,
		legacy ? &FXEQProperties_LEGACY : &FXEQProperties,
		params,
		sizeof(FAPOFXEQParameters),
		0,
		customMalloc,
		customFree,
		customRealloc
	);

	/* Function table... */
	result->base.base.Initialize = (InitializeFunc)
		FAPOFXEQ_Initialize;
	result->base.base.Process = (ProcessFunc)
		FAPOFXEQ_Process;
	result->base.Destructor = FAPOFXEQ_Free;

	/* Finally. */
	*pEffect = &result->base.base;
	return 0;
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
