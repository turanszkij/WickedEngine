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

/* This file has no documentation since you are expected to already know how
 * XACT works if you are still using these APIs!
 */

#ifndef FACT3D_H
#define FACT3D_H

#include "F3DAudio.h"
#include "FACT.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Constants */

#define LEFT_AZIMUTH			(3.0f * F3DAUDIO_PI / 2.0f)
#define RIGHT_AZIMUTH			(F3DAUDIO_PI / 2.0f)
#define FRONT_LEFT_AZIMUTH		(7.0f * F3DAUDIO_PI / 4.0f)
#define FRONT_RIGHT_AZIMUTH		(F3DAUDIO_PI / 4.0f)
#define FRONT_CENTER_AZIMUTH		0.0f
#define LOW_FREQUENCY_AZIMUTH		F3DAUDIO_2PI
#define BACK_LEFT_AZIMUTH		(5.0f * F3DAUDIO_PI / 4.0f)
#define BACK_RIGHT_AZIMUTH		(3.0f * F3DAUDIO_PI / 4.0f)
#define BACK_CENTER_AZIMUTH		F3DAUDIO_PI
#define FRONT_LEFT_OF_CENTER_AZIMUTH	(15.0f * F3DAUDIO_PI / 8.0f)
#define FRONT_RIGHT_OF_CENTER_AZIMUTH	(F3DAUDIO_PI / 8.0f)

static const float aStereoLayout[] =
{
	LEFT_AZIMUTH,
	RIGHT_AZIMUTH
};
static const float a2Point1Layout[] =
{
	LEFT_AZIMUTH,
	RIGHT_AZIMUTH,
	LOW_FREQUENCY_AZIMUTH
};
static const float aQuadLayout[] =
{
	FRONT_LEFT_AZIMUTH,
	FRONT_RIGHT_AZIMUTH,
	BACK_LEFT_AZIMUTH,
	BACK_RIGHT_AZIMUTH
};
static const float a4Point1Layout[] =
{
	FRONT_LEFT_AZIMUTH,
	FRONT_RIGHT_AZIMUTH,
	LOW_FREQUENCY_AZIMUTH,
	BACK_LEFT_AZIMUTH,
	BACK_RIGHT_AZIMUTH
};
static const float a5Point1Layout[] =
{
	FRONT_LEFT_AZIMUTH,
	FRONT_RIGHT_AZIMUTH,
	FRONT_CENTER_AZIMUTH,
	LOW_FREQUENCY_AZIMUTH,
	BACK_LEFT_AZIMUTH,
	BACK_RIGHT_AZIMUTH
};
static const float a7Point1Layout[] =
{
	FRONT_LEFT_AZIMUTH,
	FRONT_RIGHT_AZIMUTH,
	FRONT_CENTER_AZIMUTH,
	LOW_FREQUENCY_AZIMUTH,
	BACK_LEFT_AZIMUTH,
	BACK_RIGHT_AZIMUTH,
	LEFT_AZIMUTH,
	RIGHT_AZIMUTH
};

/* Functions */

FACTAPI uint32_t FACT3DInitialize(
	FACTAudioEngine *pEngine,
	F3DAUDIO_HANDLE F3DInstance
);

FACTAPI uint32_t FACT3DCalculate(
	F3DAUDIO_HANDLE F3DInstance,
	const F3DAUDIO_LISTENER *pListener,
	F3DAUDIO_EMITTER *pEmitter,
	F3DAUDIO_DSP_SETTINGS *pDSPSettings
);

FACTAPI uint32_t FACT3DApply(
	F3DAUDIO_DSP_SETTINGS *pDSPSettings,
	FACTCue *pCue
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FACT3D_H */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
