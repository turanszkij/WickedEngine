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
 * https://docs.microsoft.com/en-us/windows/desktop/api/x3daudio/
 */

#ifndef F3DAUDIO_H
#define F3DAUDIO_H

#ifdef _WIN32
#define F3DAUDIOAPI __declspec(dllexport)
#else
#define F3DAUDIOAPI
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Constants */

#ifndef _SPEAKER_POSITIONS_
#define SPEAKER_FRONT_LEFT		0x00000001
#define SPEAKER_FRONT_RIGHT		0x00000002
#define SPEAKER_FRONT_CENTER		0x00000004
#define SPEAKER_LOW_FREQUENCY		0x00000008
#define SPEAKER_BACK_LEFT		0x00000010
#define SPEAKER_BACK_RIGHT		0x00000020
#define SPEAKER_FRONT_LEFT_OF_CENTER	0x00000040
#define SPEAKER_FRONT_RIGHT_OF_CENTER	0x00000080
#define SPEAKER_BACK_CENTER		0x00000100
#define SPEAKER_SIDE_LEFT		0x00000200
#define SPEAKER_SIDE_RIGHT		0x00000400
#define SPEAKER_TOP_CENTER		0x00000800
#define SPEAKER_TOP_FRONT_LEFT		0x00001000
#define SPEAKER_TOP_FRONT_CENTER	0x00002000
#define SPEAKER_TOP_FRONT_RIGHT		0x00004000
#define SPEAKER_TOP_BACK_LEFT		0x00008000
#define SPEAKER_TOP_BACK_CENTER		0x00010000
#define SPEAKER_TOP_BACK_RIGHT		0x00020000
#define _SPEAKER_POSITIONS_
#endif

#ifndef SPEAKER_MONO
#define SPEAKER_MONO	SPEAKER_FRONT_CENTER
#define SPEAKER_STEREO	(SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
#define SPEAKER_2POINT1 \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_LOW_FREQUENCY	)
#define SPEAKER_SURROUND \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_FRONT_CENTER	| \
		SPEAKER_BACK_CENTER	)
#define SPEAKER_QUAD \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_BACK_LEFT	| \
		SPEAKER_BACK_RIGHT	)
#define SPEAKER_4POINT1 \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_LOW_FREQUENCY	| \
		SPEAKER_BACK_LEFT	| \
		SPEAKER_BACK_RIGHT	)
#define SPEAKER_5POINT1 \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_FRONT_CENTER	| \
		SPEAKER_LOW_FREQUENCY	| \
		SPEAKER_BACK_LEFT	| \
		SPEAKER_BACK_RIGHT	)
#define SPEAKER_7POINT1 \
	(	SPEAKER_FRONT_LEFT		| \
		SPEAKER_FRONT_RIGHT		| \
		SPEAKER_FRONT_CENTER		| \
		SPEAKER_LOW_FREQUENCY		| \
		SPEAKER_BACK_LEFT		| \
		SPEAKER_BACK_RIGHT		| \
		SPEAKER_FRONT_LEFT_OF_CENTER	| \
		SPEAKER_FRONT_RIGHT_OF_CENTER	)
#define SPEAKER_5POINT1_SURROUND \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_FRONT_CENTER	| \
		SPEAKER_LOW_FREQUENCY	| \
		SPEAKER_SIDE_LEFT	| \
		SPEAKER_SIDE_RIGHT	)
#define SPEAKER_7POINT1_SURROUND \
	(	SPEAKER_FRONT_LEFT	| \
		SPEAKER_FRONT_RIGHT	| \
		SPEAKER_FRONT_CENTER	| \
		SPEAKER_LOW_FREQUENCY	| \
		SPEAKER_BACK_LEFT	| \
		SPEAKER_BACK_RIGHT	| \
		SPEAKER_SIDE_LEFT	| \
		SPEAKER_SIDE_RIGHT	)
#define SPEAKER_XBOX SPEAKER_5POINT1
#endif

#define F3DAUDIO_PI			3.141592654f
#define F3DAUDIO_2PI			6.283185307f

#define F3DAUDIO_CALCULATE_MATRIX		0x00000001
#define F3DAUDIO_CALCULATE_DELAY		0x00000002
#define F3DAUDIO_CALCULATE_LPF_DIRECT		0x00000004
#define F3DAUDIO_CALCULATE_LPF_REVERB		0x00000008
#define F3DAUDIO_CALCULATE_REVERB		0x00000010
#define F3DAUDIO_CALCULATE_DOPPLER		0x00000020
#define F3DAUDIO_CALCULATE_EMITTER_ANGLE	0x00000040
#define F3DAUDIO_CALCULATE_ZEROCENTER		0x00010000
#define F3DAUDIO_CALCULATE_REDIRECT_TO_LFE	0x00020000

/* Type Declarations */

#define F3DAUDIO_HANDLE_BYTESIZE 20
typedef uint8_t F3DAUDIO_HANDLE[F3DAUDIO_HANDLE_BYTESIZE];

/* Structures */

#pragma pack(push, 1)

typedef struct F3DAUDIO_VECTOR
{
	float x;
	float y;
	float z;
} F3DAUDIO_VECTOR;

typedef struct F3DAUDIO_DISTANCE_CURVE_POINT
{
	float Distance;
	float DSPSetting;
} F3DAUDIO_DISTANCE_CURVE_POINT;

typedef struct F3DAUDIO_DISTANCE_CURVE
{
	F3DAUDIO_DISTANCE_CURVE_POINT *pPoints;
	uint32_t PointCount;
} F3DAUDIO_DISTANCE_CURVE;

typedef struct F3DAUDIO_CONE
{
	float InnerAngle;
	float OuterAngle;
	float InnerVolume;
	float OuterVolume;
	float InnerLPF;
	float OuterLPF;
	float InnerReverb;
	float OuterReverb;
} F3DAUDIO_CONE;

typedef struct F3DAUDIO_LISTENER
{
	F3DAUDIO_VECTOR OrientFront;
	F3DAUDIO_VECTOR OrientTop;
	F3DAUDIO_VECTOR Position;
	F3DAUDIO_VECTOR Velocity;
	F3DAUDIO_CONE *pCone;
} F3DAUDIO_LISTENER;

typedef struct F3DAUDIO_EMITTER
{
	F3DAUDIO_CONE *pCone;
	F3DAUDIO_VECTOR OrientFront;
	F3DAUDIO_VECTOR OrientTop;
	F3DAUDIO_VECTOR Position;
	F3DAUDIO_VECTOR Velocity;
	float InnerRadius;
	float InnerRadiusAngle;
	uint32_t ChannelCount;
	float ChannelRadius;
	float *pChannelAzimuths;
	F3DAUDIO_DISTANCE_CURVE *pVolumeCurve;
	F3DAUDIO_DISTANCE_CURVE *pLFECurve;
	F3DAUDIO_DISTANCE_CURVE *pLPFDirectCurve;
	F3DAUDIO_DISTANCE_CURVE *pLPFReverbCurve;
	F3DAUDIO_DISTANCE_CURVE *pReverbCurve;
	float CurveDistanceScaler;
	float DopplerScaler;
} F3DAUDIO_EMITTER;

#ifndef F3DAUDIO_DSP_SETTINGS_DECL
#define F3DAUDIO_DSP_SETTINGS_DECL
typedef struct F3DAUDIO_DSP_SETTINGS F3DAUDIO_DSP_SETTINGS;
#endif /* F3DAUDIO_DSP_SETTINGS_DECL */

struct F3DAUDIO_DSP_SETTINGS
{
	float *pMatrixCoefficients;
	float *pDelayTimes;
	uint32_t SrcChannelCount;
	uint32_t DstChannelCount;
	float LPFDirectCoefficient;
	float LPFReverbCoefficient;
	float ReverbLevel;
	float DopplerFactor;
	float EmitterToListenerAngle;
	float EmitterToListenerDistance;
	float EmitterVelocityComponent;
	float ListenerVelocityComponent;
};

#pragma pack(pop)

/* Functions */

F3DAUDIOAPI void F3DAudioInitialize(
	uint32_t SpeakerChannelMask,
	float SpeedOfSound,
	F3DAUDIO_HANDLE Instance
);

F3DAUDIOAPI uint32_t F3DAudioInitialize8(
	uint32_t SpeakerChannelMask,
	float SpeedOfSound,
	F3DAUDIO_HANDLE Instance
);

F3DAUDIOAPI void F3DAudioCalculate(
	const F3DAUDIO_HANDLE Instance,
	const F3DAUDIO_LISTENER *pListener,
	const F3DAUDIO_EMITTER *pEmitter,
	uint32_t Flags,
	F3DAUDIO_DSP_SETTINGS *pDSPSettings
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* F3DAUDIO_H */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
