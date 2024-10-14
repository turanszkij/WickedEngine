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

#include "FACT3D.h"

uint32_t FACT3DInitialize(
	FACTAudioEngine *pEngine,
	F3DAUDIO_HANDLE F3DInstance
) {
	float nSpeedOfSound;
	FAudioWaveFormatExtensible wfxFinalMixFormat;

	if (pEngine == NULL)
	{
		return 0;
	}

	FACTAudioEngine_GetGlobalVariable(
		pEngine,
		FACTAudioEngine_GetGlobalVariableIndex(
			pEngine,
			"SpeedOfSound"
		),
		&nSpeedOfSound
	);
	FACTAudioEngine_GetFinalMixFormat(
		pEngine,
		&wfxFinalMixFormat
	);
	F3DAudioInitialize(
		wfxFinalMixFormat.dwChannelMask,
		nSpeedOfSound,
		F3DInstance
	);
	return 0;
}

uint32_t FACT3DCalculate(
	F3DAUDIO_HANDLE F3DInstance,
	const F3DAUDIO_LISTENER *pListener,
	F3DAUDIO_EMITTER *pEmitter,
	F3DAUDIO_DSP_SETTINGS *pDSPSettings
) {
	static F3DAUDIO_DISTANCE_CURVE_POINT DefaultCurvePoints[2] =
	{
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f }
	};
	static F3DAUDIO_DISTANCE_CURVE DefaultCurve =
	{
		(F3DAUDIO_DISTANCE_CURVE_POINT*) &DefaultCurvePoints[0], 2
	};

	if (pListener == NULL || pEmitter == NULL || pDSPSettings == NULL)
	{
		return 0;
	}

	if (pEmitter->ChannelCount > 1 && pEmitter->pChannelAzimuths == NULL)
	{
		pEmitter->ChannelRadius = 1.0f;

		if (pEmitter->ChannelCount == 2)
		{
			pEmitter->pChannelAzimuths = (float*) &aStereoLayout[0];
		}
		else if (pEmitter->ChannelCount == 3)
		{
			pEmitter->pChannelAzimuths = (float*) &a2Point1Layout[0];
		}
		else if (pEmitter->ChannelCount == 4)
		{
			pEmitter->pChannelAzimuths = (float*) &aQuadLayout[0];
		}
		else if (pEmitter->ChannelCount == 5)
		{
			pEmitter->pChannelAzimuths = (float*) &a4Point1Layout[0];
		}
		else if (pEmitter->ChannelCount == 6)
		{
			pEmitter->pChannelAzimuths = (float*) &a5Point1Layout[0];
		}
		else if (pEmitter->ChannelCount == 8)
		{
			pEmitter->pChannelAzimuths = (float*) &a7Point1Layout[0];
		}
		else
		{
			return 0;
		}
	}

	if (pEmitter->pVolumeCurve == NULL)
	{
		pEmitter->pVolumeCurve = &DefaultCurve;
	}
	if (pEmitter->pLFECurve == NULL)
	{
		pEmitter->pLFECurve = &DefaultCurve;
	}

	F3DAudioCalculate(
		F3DInstance,
		pListener,
		pEmitter,
		(
			F3DAUDIO_CALCULATE_MATRIX |
			F3DAUDIO_CALCULATE_DOPPLER |
			F3DAUDIO_CALCULATE_EMITTER_ANGLE
		),
		pDSPSettings
	);
	return 0;
}

uint32_t FACT3DApply(
	F3DAUDIO_DSP_SETTINGS *pDSPSettings,
	FACTCue *pCue
) {
	if (pDSPSettings == NULL || pCue == NULL)
	{
		return 0;
	}

	FACTCue_SetMatrixCoefficients(
		pCue,
		pDSPSettings->SrcChannelCount,
		pDSPSettings->DstChannelCount,
		pDSPSettings->pMatrixCoefficients
	);
	FACTCue_SetVariable(
		pCue,
		FACTCue_GetVariableIndex(pCue, "Distance"),
		pDSPSettings->EmitterToListenerDistance
	);
	FACTCue_SetVariable(
		pCue,
		FACTCue_GetVariableIndex(pCue, "DopplerPitchScalar"),
		pDSPSettings->DopplerFactor
	);
	FACTCue_SetVariable(
		pCue,
		FACTCue_GetVariableIndex(pCue, "OrientationAngle"),
		pDSPSettings->EmitterToListenerAngle * (180.0f / F3DAUDIO_PI)
	);
	return 0;
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
