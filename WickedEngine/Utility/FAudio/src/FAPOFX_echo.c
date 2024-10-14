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

/* FXEcho FAPO Implementation */

const FAudioGUID FAPOFX_CLSID_FXEcho =
{
	0x5039D740,
	0xF736,
	0x449A,
	{
		0x84,
		0xD3,
		0xA5,
		0x62,
		0x02,
		0x55,
		0x7B,
		0x87
	}
};

static FAPORegistrationProperties FXEchoProperties =
{
	/* .clsid = */ {0},
	/* .FriendlyName = */
	{
		'F', 'X', 'E', 'c', 'h', 'o', '\0'
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

const FAudioGUID FAPOFX_CLSID_FXEcho_LEGACY =
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
		0x03
	}
};

static FAPORegistrationProperties FXEchoProperties_LEGACY =
{
	/* .clsid = */ {0},
	/* .FriendlyName = */
	{
		'F', 'X', 'E', 'c', 'h', 'o', '\0'
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

typedef struct FAPOFXEcho
{
	FAPOBase base;

	/* TODO */
} FAPOFXEcho;

uint32_t FAPOFXEcho_Initialize(
	FAPOFXEcho *fapo,
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

void FAPOFXEcho_Process(
	FAPOFXEcho *fapo,
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

void FAPOFXEcho_Free(void* fapo)
{
	FAPOFXEcho *echo = (FAPOFXEcho*) fapo;
	echo->base.pFree(echo->base.m_pParameterBlocks);
	echo->base.pFree(fapo);
}

/* Public API */

uint32_t FAPOFXCreateEcho(
	FAPO **pEffect,
	const void *pInitData,
	uint32_t InitDataByteSize,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc,
	uint8_t legacy
) {
	const FAPOFXEchoParameters fxdefault =
	{
		FAPOFXECHO_DEFAULT_WETDRYMIX,
		FAPOFXECHO_DEFAULT_FEEDBACK,
		FAPOFXECHO_DEFAULT_DELAY
	};

	/* Allocate... */
	FAPOFXEcho *result = (FAPOFXEcho*) customMalloc(
		sizeof(FAPOFXEcho)
	);
	uint8_t *params = (uint8_t*) customMalloc(
		sizeof(FAPOFXEchoParameters) * 3
	);
	if (pInitData == NULL)
	{
		FAudio_zero(params, sizeof(FAPOFXEchoParameters) * 3);
		#define INITPARAMS(offset) \
			FAudio_memcpy( \
				params + sizeof(FAPOFXEchoParameters) * offset, \
				&fxdefault, \
				sizeof(FAPOFXEchoParameters) \
			);
		INITPARAMS(0)
		INITPARAMS(1)
		INITPARAMS(2)
		#undef INITPARAMS
	}
	else
	{
		FAudio_assert(InitDataByteSize == sizeof(FAPOFXEchoParameters));
		FAudio_memcpy(params, pInitData, InitDataByteSize);
		FAudio_memcpy(params + InitDataByteSize, pInitData, InitDataByteSize);
		FAudio_memcpy(params + (InitDataByteSize * 2), pInitData, InitDataByteSize);
	}

	/* Initialize... */
	FAudio_memcpy(
		&FXEchoProperties_LEGACY.clsid,
		&FAPOFX_CLSID_FXEcho_LEGACY,
		sizeof(FAudioGUID)
	);
	FAudio_memcpy(
		&FXEchoProperties.clsid,
		&FAPOFX_CLSID_FXEcho,
		sizeof(FAudioGUID)
	);
	CreateFAPOBaseWithCustomAllocatorEXT(
		&result->base,
		legacy ? &FXEchoProperties_LEGACY : &FXEchoProperties,
		params,
		sizeof(FAPOFXEchoParameters),
		0,
		customMalloc,
		customFree,
		customRealloc
	);

	/* Function table... */
	result->base.base.Initialize = (InitializeFunc)
		FAPOFXEcho_Initialize;
	result->base.base.Process = (ProcessFunc)
		FAPOFXEcho_Process;
	result->base.Destructor = FAPOFXEcho_Free;

	/* Finally. */
	*pEffect = &result->base.base;
	return 0;
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
