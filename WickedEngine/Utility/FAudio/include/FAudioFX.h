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
 * https://docs.microsoft.com/en-us/windows/desktop/api/xaudio2fx/
 *
 * Note, however, that FAudio's Reverb implementation does NOT support the new
 * parameters for XAudio 2.9's 7.1 Reverb effect!
 */

#ifndef FAUDIOFX_H
#define FAUDIOFX_H

#include "FAudio.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* GUIDs */

extern const FAudioGUID FAudioFX_CLSID_AudioVolumeMeter;
extern const FAudioGUID FAudioFX_CLSID_AudioReverb;

/* Structures */

#pragma pack(push, 1)

typedef struct FAudioFXVolumeMeterLevels
{
	float *pPeakLevels;
	float *pRMSLevels;
	uint32_t ChannelCount;
} FAudioFXVolumeMeterLevels;

typedef struct FAudioFXReverbParameters
{
	float WetDryMix;
	uint32_t ReflectionsDelay;
	uint8_t ReverbDelay;
	uint8_t RearDelay;
	uint8_t PositionLeft;
	uint8_t PositionRight;
	uint8_t PositionMatrixLeft;
	uint8_t PositionMatrixRight;
	uint8_t EarlyDiffusion;
	uint8_t LateDiffusion;
	uint8_t LowEQGain;
	uint8_t LowEQCutoff;
	uint8_t HighEQGain;
	uint8_t HighEQCutoff;
	float RoomFilterFreq;
	float RoomFilterMain;
	float RoomFilterHF;
	float ReflectionsGain;
	float ReverbGain;
	float DecayTime;
	float Density;
	float RoomSize;
} FAudioFXReverbParameters;

typedef struct FAudioFXReverbParameters9
{
	float WetDryMix;
	uint32_t ReflectionsDelay;
	uint8_t ReverbDelay;
	uint8_t RearDelay;
	uint8_t SideDelay;
	uint8_t PositionLeft;
	uint8_t PositionRight;
	uint8_t PositionMatrixLeft;
	uint8_t PositionMatrixRight;
	uint8_t EarlyDiffusion;
	uint8_t LateDiffusion;
	uint8_t LowEQGain;
	uint8_t LowEQCutoff;
	uint8_t HighEQGain;
	uint8_t HighEQCutoff;
	float RoomFilterFreq;
	float RoomFilterMain;
	float RoomFilterHF;
	float ReflectionsGain;
	float ReverbGain;
	float DecayTime;
	float Density;
	float RoomSize;
} FAudioFXReverbParameters9;

typedef struct FAudioFXReverbI3DL2Parameters
{
	float WetDryMix;
	int32_t Room;
	int32_t RoomHF;
	float RoomRolloffFactor;
	float DecayTime;
	float DecayHFRatio;
	int32_t Reflections;
	float ReflectionsDelay;
	int32_t Reverb;
	float ReverbDelay;
	float Diffusion;
	float Density;
	float HFReference;
} FAudioFXReverbI3DL2Parameters;

#pragma pack(pop)

/* Constants */

#define FAUDIOFX_DEBUG 1

#define FAUDIOFX_REVERB_MIN_FRAMERATE 20000
#define FAUDIOFX_REVERB_MAX_FRAMERATE 48000

#define FAUDIOFX_REVERB_MIN_WET_DRY_MIX		0.0f
#define FAUDIOFX_REVERB_MIN_REFLECTIONS_DELAY	0
#define FAUDIOFX_REVERB_MIN_REVERB_DELAY	0
#define FAUDIOFX_REVERB_MIN_REAR_DELAY		0
#define FAUDIOFX_REVERB_MIN_7POINT1_SIDE_DELAY	0
#define FAUDIOFX_REVERB_MIN_7POINT1_REAR_DELAY	0
#define FAUDIOFX_REVERB_MIN_POSITION		0
#define FAUDIOFX_REVERB_MIN_DIFFUSION		0
#define FAUDIOFX_REVERB_MIN_LOW_EQ_GAIN		0
#define FAUDIOFX_REVERB_MIN_LOW_EQ_CUTOFF	0
#define FAUDIOFX_REVERB_MIN_HIGH_EQ_GAIN	0
#define FAUDIOFX_REVERB_MIN_HIGH_EQ_CUTOFF	0
#define FAUDIOFX_REVERB_MIN_ROOM_FILTER_FREQ	20.0f
#define FAUDIOFX_REVERB_MIN_ROOM_FILTER_MAIN	-100.0f
#define FAUDIOFX_REVERB_MIN_ROOM_FILTER_HF	-100.0f
#define FAUDIOFX_REVERB_MIN_REFLECTIONS_GAIN	-100.0f
#define FAUDIOFX_REVERB_MIN_REVERB_GAIN		-100.0f
#define FAUDIOFX_REVERB_MIN_DECAY_TIME		0.1f
#define FAUDIOFX_REVERB_MIN_DENSITY		0.0f
#define FAUDIOFX_REVERB_MIN_ROOM_SIZE		0.0f

#define FAUDIOFX_REVERB_MAX_WET_DRY_MIX		100.0f
#define FAUDIOFX_REVERB_MAX_REFLECTIONS_DELAY	300
#define FAUDIOFX_REVERB_MAX_REVERB_DELAY	85
#define FAUDIOFX_REVERB_MAX_REAR_DELAY		5
#define FAUDIOFX_REVERB_MAX_7POINT1_SIDE_DELAY	5
#define FAUDIOFX_REVERB_MAX_7POINT1_REAR_DELAY	20
#define FAUDIOFX_REVERB_MAX_POSITION		30
#define FAUDIOFX_REVERB_MAX_DIFFUSION		15
#define FAUDIOFX_REVERB_MAX_LOW_EQ_GAIN		12
#define FAUDIOFX_REVERB_MAX_LOW_EQ_CUTOFF	9
#define FAUDIOFX_REVERB_MAX_HIGH_EQ_GAIN	8
#define FAUDIOFX_REVERB_MAX_HIGH_EQ_CUTOFF	14
#define FAUDIOFX_REVERB_MAX_ROOM_FILTER_FREQ	20000.0f
#define FAUDIOFX_REVERB_MAX_ROOM_FILTER_MAIN	0.0f
#define FAUDIOFX_REVERB_MAX_ROOM_FILTER_HF	0.0f
#define FAUDIOFX_REVERB_MAX_REFLECTIONS_GAIN	20.0f
#define FAUDIOFX_REVERB_MAX_REVERB_GAIN		20.0f
#define FAUDIOFX_REVERB_MAX_DENSITY		100.0f
#define FAUDIOFX_REVERB_MAX_ROOM_SIZE		100.0f

#define FAUDIOFX_REVERB_DEFAULT_WET_DRY_MIX		100.0f
#define FAUDIOFX_REVERB_DEFAULT_REFLECTIONS_DELAY	5
#define FAUDIOFX_REVERB_DEFAULT_REVERB_DELAY		5
#define FAUDIOFX_REVERB_DEFAULT_REAR_DELAY		5
#define FAUDIOFX_REVERB_DEFAULT_7POINT1_SIDE_DELAY	5
#define FAUDIOFX_REVERB_DEFAULT_7POINT1_REAR_DELAY	20
#define FAUDIOFX_REVERB_DEFAULT_POSITION		6
#define FAUDIOFX_REVERB_DEFAULT_POSITION_MATRIX		27
#define FAUDIOFX_REVERB_DEFAULT_EARLY_DIFFUSION		8
#define FAUDIOFX_REVERB_DEFAULT_LATE_DIFFUSION		8
#define FAUDIOFX_REVERB_DEFAULT_LOW_EQ_GAIN		8
#define FAUDIOFX_REVERB_DEFAULT_LOW_EQ_CUTOFF		4
#define FAUDIOFX_REVERB_DEFAULT_HIGH_EQ_GAIN		8
#define FAUDIOFX_REVERB_DEFAULT_HIGH_EQ_CUTOFF		4
#define FAUDIOFX_REVERB_DEFAULT_ROOM_FILTER_FREQ	5000.0f
#define FAUDIOFX_REVERB_DEFAULT_ROOM_FILTER_MAIN	0.0f
#define FAUDIOFX_REVERB_DEFAULT_ROOM_FILTER_HF		0.0f
#define FAUDIOFX_REVERB_DEFAULT_REFLECTIONS_GAIN	0.0f
#define FAUDIOFX_REVERB_DEFAULT_REVERB_GAIN		0.0f
#define FAUDIOFX_REVERB_DEFAULT_DECAY_TIME		1.0f
#define FAUDIOFX_REVERB_DEFAULT_DENSITY			100.0f
#define FAUDIOFX_REVERB_DEFAULT_ROOM_SIZE		100.0f

#define FAUDIOFX_I3DL2_PRESET_DEFAULT \
	{100,-10000,    0,0.0f, 1.00f,0.50f,-10000,0.020f,-10000,0.040f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_GENERIC \
	{100, -1000, -100,0.0f, 1.49f,0.83f, -2602,0.007f,   200,0.011f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_PADDEDCELL \
	{100, -1000,-6000,0.0f, 0.17f,0.10f, -1204,0.001f,   207,0.002f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_ROOM \
	{100, -1000, -454,0.0f, 0.40f,0.83f, -1646,0.002f,    53,0.003f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_BATHROOM \
	{100, -1000,-1200,0.0f, 1.49f,0.54f,  -370,0.007f,  1030,0.011f,100.0f, 60.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_LIVINGROOM \
	{100, -1000,-6000,0.0f, 0.50f,0.10f, -1376,0.003f, -1104,0.004f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_STONEROOM \
	{100, -1000, -300,0.0f, 2.31f,0.64f,  -711,0.012f,    83,0.017f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_AUDITORIUM \
	{100, -1000, -476,0.0f, 4.32f,0.59f,  -789,0.020f,  -289,0.030f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_CONCERTHALL \
	{100, -1000, -500,0.0f, 3.92f,0.70f, -1230,0.020f,    -2,0.029f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_CAVE \
	{100, -1000,    0,0.0f, 2.91f,1.30f,  -602,0.015f,  -302,0.022f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_ARENA \
	{100, -1000, -698,0.0f, 7.24f,0.33f, -1166,0.020f,    16,0.030f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_HANGAR \
	{100, -1000,-1000,0.0f,10.05f,0.23f,  -602,0.020f,   198,0.030f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_CARPETEDHALLWAY \
	{100, -1000,-4000,0.0f, 0.30f,0.10f, -1831,0.002f, -1630,0.030f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_HALLWAY \
	{100, -1000, -300,0.0f, 1.49f,0.59f, -1219,0.007f,   441,0.011f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_STONECORRIDOR \
	{100, -1000, -237,0.0f, 2.70f,0.79f, -1214,0.013f,   395,0.020f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_ALLEY \
	{100, -1000, -270,0.0f, 1.49f,0.86f, -1204,0.007f,    -4,0.011f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_FOREST \
	{100, -1000,-3300,0.0f, 1.49f,0.54f, -2560,0.162f,  -613,0.088f, 79.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_CITY \
	{100, -1000, -800,0.0f, 1.49f,0.67f, -2273,0.007f, -2217,0.011f, 50.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_MOUNTAINS \
	{100, -1000,-2500,0.0f, 1.49f,0.21f, -2780,0.300f, -2014,0.100f, 27.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_QUARRY \
	{100, -1000,-1000,0.0f, 1.49f,0.83f,-10000,0.061f,   500,0.025f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_PLAIN \
	{100, -1000,-2000,0.0f, 1.49f,0.50f, -2466,0.179f, -2514,0.100f, 21.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_PARKINGLOT \
	{100, -1000,    0,0.0f, 1.65f,1.50f, -1363,0.008f, -1153,0.012f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_SEWERPIPE \
	{100, -1000,-1000,0.0f, 2.81f,0.14f,   429,0.014f,   648,0.021f, 80.0f, 60.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_UNDERWATER \
	{100, -1000,-4000,0.0f, 1.49f,0.10f,  -449,0.007f,  1700,0.011f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_SMALLROOM \
	{100, -1000, -600,0.0f, 1.10f,0.83f,  -400,0.005f,   500,0.010f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_MEDIUMROOM \
	{100, -1000, -600,0.0f, 1.30f,0.83f, -1000,0.010f,  -200,0.020f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_LARGEROOM \
	{100, -1000, -600,0.0f, 1.50f,0.83f, -1600,0.020f, -1000,0.040f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_MEDIUMHALL \
	{100, -1000, -600,0.0f, 1.80f,0.70f, -1300,0.015f,  -800,0.030f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_LARGEHALL \
	{100, -1000, -600,0.0f, 1.80f,0.70f, -2000,0.030f, -1400,0.060f,100.0f,100.0f,5000.0f}
#define FAUDIOFX_I3DL2_PRESET_PLATE \
	{100, -1000, -200,0.0f, 1.30f,0.90f,     0,0.002f,     0,0.010f,100.0f, 75.0f,5000.0f}

/* Functions */

FAUDIOAPI uint32_t FAudioCreateVolumeMeter(FAPO** ppApo, uint32_t Flags);
FAUDIOAPI uint32_t FAudioCreateReverb(FAPO** ppApo, uint32_t Flags);
FAUDIOAPI uint32_t FAudioCreateReverb9(FAPO** ppApo, uint32_t Flags);

/* See "extensions/CustomAllocatorEXT.txt" for more information. */
FAUDIOAPI uint32_t FAudioCreateVolumeMeterWithCustomAllocatorEXT(
	FAPO** ppApo,
	uint32_t Flags,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
);
FAUDIOAPI uint32_t FAudioCreateReverbWithCustomAllocatorEXT(
	FAPO** ppApo,
	uint32_t Flags,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
);
FAUDIOAPI uint32_t FAudioCreateReverb9WithCustomAllocatorEXT(
	FAPO** ppApo,
	uint32_t Flags,
	FAudioMallocFunc customMalloc,
	FAudioFreeFunc customFree,
	FAudioReallocFunc customRealloc
);

FAUDIOAPI void ReverbConvertI3DL2ToNative(
	const FAudioFXReverbI3DL2Parameters *pI3DL2,
	FAudioFXReverbParameters *pNative
);
FAUDIOAPI void ReverbConvertI3DL2ToNative9(
	const FAudioFXReverbI3DL2Parameters *pI3DL2,
	FAudioFXReverbParameters9 *pNative,
	int32_t sevenDotOneReverb
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FAUDIOFX_H */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
