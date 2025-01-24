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

#ifndef FAPOFX_H
#define FAPOFX_H

#include "FAPO.h"

#define FAPOFXAPI FAUDIOAPI

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* GUIDs */

/* "Legacy" GUIDs are from XAPOFX <= 1.5. They were removed in XAudio 2.8 and later. */
extern const FAudioGUID FAPOFX_CLSID_FXEQ, FAPOFX_CLSID_FXEQ_LEGACY;
extern const FAudioGUID FAPOFX_CLSID_FXMasteringLimiter, FAPOFX_CLSID_FXMasteringLimiter_LEGACY;
extern const FAudioGUID FAPOFX_CLSID_FXReverb, FAPOFX_CLSID_FXReverb_LEGACY;
extern const FAudioGUID FAPOFX_CLSID_FXEcho, FAPOFX_CLSID_FXEcho_LEGACY;

/* Structures */

#pragma pack(push, 1)

/* See FAPOFXEQ_* constants below.
 * FrequencyCenter is in Hz, Gain is amplitude ratio, Bandwidth is Q factor.
 */
typedef struct FAPOFXEQParameters
{
	float FrequencyCenter0;
	float Gain0;
	float Bandwidth0;
	float FrequencyCenter1;
	float Gain1;
	float Bandwidth1;
	float FrequencyCenter2;
	float Gain2;
	float Bandwidth2;
	float FrequencyCenter3;
	float Gain3;
	float Bandwidth3;
} FAPOFXEQParameters;

/* See FAPOFXMASTERINGLIMITER_* constants below. */
typedef struct FAPOFXMasteringLimiterParameters
{
	uint32_t Release;	/* In milliseconds */
	uint32_t Loudness;	/* In... uh, MSDN doesn't actually say what. */
} FAPOFXMasteringLimiterParameters;

/* See FAPOFXREVERB_* constants below.
 * Both parameters are arbitrary and should be treated subjectively.
 */
typedef struct FAPOFXReverbParameters
{
	float Diffusion;
	float RoomSize;
} FAPOFXReverbParameters;

/* See FAPOFXECHO_* constants below. */
typedef struct FAPOFXEchoParameters
{
	float WetDryMix;	/* Percentage of processed signal vs original */
	float Feedback;		/* Percentage to feed back into input */
	float Delay;		/* In milliseconds */
} FAPOFXEchoParameters;

#pragma pack(pop)

/* Constants */

#define FAPOFXEQ_MIN_FRAMERATE 22000
#define FAPOFXEQ_MAX_FRAMERATE 48000

#define FAPOFXEQ_MIN_FREQUENCY_CENTER		20.0f
#define FAPOFXEQ_MAX_FREQUENCY_CENTER		20000.0f
#define FAPOFXEQ_DEFAULT_FREQUENCY_CENTER_0	100.0f
#define FAPOFXEQ_DEFAULT_FREQUENCY_CENTER_1	800.0f
#define FAPOFXEQ_DEFAULT_FREQUENCY_CENTER_2	2000.0f
#define FAPOFXEQ_DEFAULT_FREQUENCY_CENTER_3	10000.0f

#define FAPOFXEQ_MIN_GAIN	0.126f
#define FAPOFXEQ_MAX_GAIN	7.94f
#define FAPOFXEQ_DEFAULT_GAIN	1.0f

#define FAPOFXEQ_MIN_BANDWIDTH		0.1f
#define FAPOFXEQ_MAX_BANDWIDTH		2.0f
#define FAPOFXEQ_DEFAULT_BANDWIDTH	1.0f

#define FAPOFXMASTERINGLIMITER_MIN_RELEASE	1
#define FAPOFXMASTERINGLIMITER_MAX_RELEASE	20
#define FAPOFXMASTERINGLIMITER_DEFAULT_RELEASE	6

#define FAPOFXMASTERINGLIMITER_MIN_LOUDNESS	1
#define FAPOFXMASTERINGLIMITER_MAX_LOUDNESS	1800
#define FAPOFXMASTERINGLIMITER_DEFAULT_LOUDNESS	1000

#define FAPOFXREVERB_MIN_DIFFUSION	0.0f
#define FAPOFXREVERB_MAX_DIFFUSION	1.0f
#define FAPOFXREVERB_DEFAULT_DIFFUSION	0.9f

#define FAPOFXREVERB_MIN_ROOMSIZE	0.0001f
#define FAPOFXREVERB_MAX_ROOMSIZE	1.0f
#define FAPOFXREVERB_DEFAULT_ROOMSIZE	0.6f

#define FAPOFXECHO_MIN_WETDRYMIX	0.0f
#define FAPOFXECHO_MAX_WETDRYMIX	1.0f
#define FAPOFXECHO_DEFAULT_WETDRYMIX	0.5f

#define FAPOFXECHO_MIN_FEEDBACK		0.0f
#define FAPOFXECHO_MAX_FEEDBACK		1.0f
#define FAPOFXECHO_DEFAULT_FEEDBACK	0.5f

#define FAPOFXECHO_MIN_DELAY		1.0f
#define FAPOFXECHO_MAX_DELAY		2000.0f
#define FAPOFXECHO_DEFAULT_DELAY	500.0f

/* Functions */

/* Creates an effect from the pre-made FAPOFX effect library.
 *
 * clsid:		A reference to one of the FAPOFX_CLSID_* GUIDs
 * pEffect:		Filled with the resulting FAPO object
 * pInitData:		Starting parameters, pass NULL to use the default values
 * InitDataByteSize:	Parameter struct size, pass 0 if pInitData is NULL
 *
 * Returns 0 on success.
 */
FAPOFXAPI uint32_t FAPOFX_CreateFX(
	const FAudioGUID *clsid,
	FAPO **pEffect,
	const void *pInitData,
	uint32_t InitDataByteSize
);

/* See "extensions/CustomAllocatorEXT.txt" for more details. */
FAPOFXAPI uint32_t FAPOFX_CreateFXWithCustomAllocatorEXT(
	const FAudioGUID *clsid,
	FAPO **pEffect,
	const void *pInitData,
	uint32_t InitDataByteSize,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FAPOFX_H */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
