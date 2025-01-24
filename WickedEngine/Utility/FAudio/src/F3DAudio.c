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

#include "F3DAudio.h"
#include "FAudio_internal.h"

#include <math.h> /* ONLY USE THIS FOR isnan! */
#include <float.h> /* ONLY USE THIS FOR FLT_MIN/FLT_MAX! */

/* VS2010 doesn't define isnan (which is C99), so here it is. */
#if defined(_MSC_VER) && !defined(isnan)
#define isnan(x) _isnan(x)
#endif

/* UTILITY MACROS */

#define PARAM_CHECK_OK 1
#define PARAM_CHECK_FAIL (!PARAM_CHECK_OK)

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

#define LERP(a, x, y) ((1.0f - a) * x + a * y)

/* PARAMETER CHECK MACROS */

#define PARAM_CHECK(cond, msg) FAudio_assert(cond && msg)

#define POINTER_CHECK(p) \
	PARAM_CHECK(p != NULL, "Pointer " #p " must be != NULL")

#define FLOAT_BETWEEN_CHECK(f, a, b) \
	PARAM_CHECK(f >= a, "Value" #f " is too low"); \
	PARAM_CHECK(f <= b, "Value" #f " is too big")


/* Quote X3DAUDIO docs:
 * "To be considered orthonormal, a pair of vectors must have a magnitude of
 * 1 +- 1x10-5 and a dot product of 0 +- 1x10-5."
 * VECTOR_NORMAL_CHECK verifies that vectors are normal (i.e. have norm 1 +- 1x10-5)
 * VECTOR_BASE_CHECK verifies that a pair of vectors are orthogonal (i.e. their dot
 * product is 0 +- 1x10-5)
 */

/* TODO: Switch to square length (to save CPU) */
#define VECTOR_NORMAL_CHECK(v) \
	PARAM_CHECK( \
		FAudio_fabsf(VectorLength(v) - 1.0f) <= 1e-5f, \
		"Vector " #v " isn't normal" \
	)

#define VECTOR_BASE_CHECK(u, v) \
	PARAM_CHECK( \
		FAudio_fabsf(VectorDot(u, v)) <= 1e-5f, \
		"Vector u and v have non-negligible dot product" \
	)

/*************************************
 * F3DAudioInitialize Implementation *
 *************************************/

/* F3DAUDIO_HANDLE Structure */
#define SPEAKERMASK(Instance)		*((uint32_t*)	&Instance[0])
#define SPEAKERCOUNT(Instance)		*((uint32_t*)	&Instance[4])
#define SPEAKER_LF_INDEX(Instance)	*((uint32_t*)	&Instance[8])
#define SPEEDOFSOUND(Instance)		*((float*)	&Instance[12])
#define SPEEDOFSOUNDEPSILON(Instance)	*((float*)	&Instance[16])

/* Export for unit tests */
F3DAUDIOAPI uint32_t F3DAudioCheckInitParams(
	uint32_t SpeakerChannelMask,
	float SpeedOfSound,
	F3DAUDIO_HANDLE instance
) {
	const uint32_t kAllowedSpeakerMasks[] =
	{
		SPEAKER_MONO,
		SPEAKER_STEREO,
		SPEAKER_2POINT1,
		SPEAKER_QUAD,
		SPEAKER_SURROUND,
		SPEAKER_4POINT1,
		SPEAKER_5POINT1,
		SPEAKER_5POINT1_SURROUND,
		SPEAKER_7POINT1,
		SPEAKER_7POINT1_SURROUND,
	};
	uint8_t speakerMaskIsValid = 0;
	uint32_t i;

	POINTER_CHECK(instance);

	for (i = 0; i < ARRAY_COUNT(kAllowedSpeakerMasks); i += 1)
	{
		if (SpeakerChannelMask == kAllowedSpeakerMasks[i])
		{
			speakerMaskIsValid = 1;
			break;
		}
	}

	/* The docs don't clearly say it, but the debug dll does check that
	 * we're exactly in one of the allowed speaker configurations.
	 * -Adrien
	 */
	PARAM_CHECK(
		speakerMaskIsValid == 1,
		"SpeakerChannelMask is invalid. Needs to be one of"
		" MONO, STEREO, QUAD, 2POINT1, 4POINT1, 5POINT1, 7POINT1,"
		" SURROUND, 5POINT1_SURROUND, or 7POINT1_SURROUND."
	);

	PARAM_CHECK(SpeedOfSound >= FLT_MIN, "SpeedOfSound needs to be >= FLT_MIN");

	return PARAM_CHECK_OK;
}

void F3DAudioInitialize(
	uint32_t SpeakerChannelMask,
	float SpeedOfSound,
	F3DAUDIO_HANDLE Instance
) {
	F3DAudioInitialize8(SpeakerChannelMask, SpeedOfSound, Instance);
}

uint32_t F3DAudioInitialize8(
	uint32_t SpeakerChannelMask,
	float SpeedOfSound,
	F3DAUDIO_HANDLE Instance
) {
	union
	{
		float f;
		uint32_t i;
	} epsilonHack;
	uint32_t speakerCount = 0;

	if (!F3DAudioCheckInitParams(SpeakerChannelMask, SpeedOfSound, Instance))
	{
		return FAUDIO_E_INVALID_CALL;
	}

	SPEAKERMASK(Instance) = SpeakerChannelMask;
	SPEEDOFSOUND(Instance) = SpeedOfSound;

	/* "Convert" raw float to int... */
	epsilonHack.f = SpeedOfSound;
	/* ... Subtract epsilon value... */
	epsilonHack.i -= 1;
	/* ... Convert back to float. */
	SPEEDOFSOUNDEPSILON(Instance) = epsilonHack.f;

	SPEAKER_LF_INDEX(Instance) = 0xFFFFFFFF;
	if (SpeakerChannelMask & SPEAKER_LOW_FREQUENCY)
	{
		if (SpeakerChannelMask & SPEAKER_FRONT_CENTER)
		{
			SPEAKER_LF_INDEX(Instance) = 3;
		}
		else
		{
			SPEAKER_LF_INDEX(Instance) = 2;
		}
	}

	while (SpeakerChannelMask)
	{
		speakerCount += 1;
		SpeakerChannelMask &= SpeakerChannelMask - 1;
	}
	SPEAKERCOUNT(Instance) = speakerCount;

	return 0;
}


/************************************
 * F3DAudioCalculate Implementation *
 ************************************/

/* VECTOR UTILITIES */

static inline F3DAUDIO_VECTOR Vec(float x, float y, float z)
{
	F3DAUDIO_VECTOR res;
	res.x = x;
	res.y = y;
	res.z = z;
	return res;
}

#define VectorAdd(u, v) Vec(u.x + v.x, u.y + v.y, u.z + v.z)

#define VectorSub(u, v) Vec(u.x - v.x, u.y - v.y, u.z - v.z)

#define VectorScale(u, s) Vec(u.x * s, u.y * s, u.z * s)

#define VectorCross(u, v) Vec( \
	(u.y * v.z) - (u.z * v.y), \
	(u.z * v.x) - (u.x * v.z), \
	(u.x * v.y) - (u.y * v.x) \
)

#define VectorLength(v) FAudio_sqrtf( \
	(v.x * v.x) + (v.y * v.y) + (v.z * v.z) \
)

#define VectorDot(u, v) ((u.x * v.x) + (u.y * v.y) + (u.z * v.z))

/* This structure represent a tuple of vectors that form a left-handed basis.
 * That is, all vectors are normal, orthogonal to each other, and taken in the
 * order front, right, top they follow the left-hand rule.
 * (https://en.wikipedia.org/wiki/Right-hand_rule)
 */
typedef struct F3DAUDIO_BASIS
{
	F3DAUDIO_VECTOR front;
	F3DAUDIO_VECTOR right;
	F3DAUDIO_VECTOR top;
} F3DAUDIO_BASIS;

/* CHECK UTILITY FUNCTIONS */

static inline uint8_t CheckCone(F3DAUDIO_CONE *pCone)
{
	if (!pCone)
	{
		return PARAM_CHECK_OK;
	}

	FLOAT_BETWEEN_CHECK(pCone->InnerAngle, 0.0f, F3DAUDIO_2PI);
	FLOAT_BETWEEN_CHECK(pCone->OuterAngle, pCone->InnerAngle, F3DAUDIO_2PI);

	FLOAT_BETWEEN_CHECK(pCone->InnerVolume, 0.0f, 2.0f);
	FLOAT_BETWEEN_CHECK(pCone->OuterVolume, 0.0f, 2.0f);

	FLOAT_BETWEEN_CHECK(pCone->InnerLPF, 0.0f, 1.0f);
	FLOAT_BETWEEN_CHECK(pCone->OuterLPF, 0.0f, 1.0f);

	FLOAT_BETWEEN_CHECK(pCone->InnerReverb, 0.0f, 2.0f);
	FLOAT_BETWEEN_CHECK(pCone->OuterReverb, 0.0f, 2.0f);

	return PARAM_CHECK_OK;
}

static inline uint8_t CheckCurve(F3DAUDIO_DISTANCE_CURVE *pCurve)
{
	F3DAUDIO_DISTANCE_CURVE_POINT *points;
	uint32_t i;
	if (!pCurve)
	{
		return PARAM_CHECK_OK;
	}

	points = pCurve->pPoints;
	POINTER_CHECK(points);
	PARAM_CHECK(pCurve->PointCount >= 2, "Invalid number of points for curve");

	for (i = 0; i < pCurve->PointCount; i += 1)
	{
		FLOAT_BETWEEN_CHECK(points[i].Distance, 0.0f, 1.0f);
	}

	PARAM_CHECK(
		points[0].Distance == 0.0f,
		"First point in the curve must be at distance 0.0f"
	);
	PARAM_CHECK(
		points[pCurve->PointCount - 1].Distance == 1.0f,
		"Last point in the curve must be at distance 1.0f"
	);

	for (i = 0; i < (pCurve->PointCount - 1); i += 1)
	{
		PARAM_CHECK(
			points[i].Distance < points[i + 1].Distance,
			"Curve points must be in strict ascending order"
		);
	}

	return PARAM_CHECK_OK;
}

/* Export for unit tests */
F3DAUDIOAPI uint8_t F3DAudioCheckCalculateParams(
	const F3DAUDIO_HANDLE Instance,
	const F3DAUDIO_LISTENER *pListener,
	const F3DAUDIO_EMITTER *pEmitter,
	uint32_t Flags,
	F3DAUDIO_DSP_SETTINGS *pDSPSettings
) {
	uint32_t i, ChannelCount;

	POINTER_CHECK(Instance);
	POINTER_CHECK(pListener);
	POINTER_CHECK(pEmitter);
	POINTER_CHECK(pDSPSettings);

	if (Flags & F3DAUDIO_CALCULATE_MATRIX)
	{
		POINTER_CHECK(pDSPSettings->pMatrixCoefficients);
	}
	if (Flags & F3DAUDIO_CALCULATE_ZEROCENTER)
	{
		const uint32_t isCalculateMatrix = (Flags & F3DAUDIO_CALCULATE_MATRIX);
		const uint32_t hasCenter = SPEAKERMASK(Instance) & SPEAKER_FRONT_CENTER;
		PARAM_CHECK(
			isCalculateMatrix && hasCenter,
			"F3DAUDIO_CALCULATE_ZEROCENTER is only valid for matrix"
			" calculations with an output format that has a center channel"
		);
	}

	if (Flags & F3DAUDIO_CALCULATE_REDIRECT_TO_LFE)
	{
		const uint32_t isCalculateMatrix = (Flags & F3DAUDIO_CALCULATE_MATRIX);
		const uint32_t hasLF = SPEAKERMASK(Instance) & SPEAKER_LOW_FREQUENCY;
		PARAM_CHECK(
			isCalculateMatrix && hasLF,
			"F3DAUDIO_CALCULATE_REDIRECT_TO_LFE is only valid for matrix"
			" calculations with an output format that has a low-frequency"
			" channel"
		);
	}

	ChannelCount = SPEAKERCOUNT(Instance);
	PARAM_CHECK(
		pDSPSettings->DstChannelCount == ChannelCount,
		"Invalid channel count, DSP settings and speaker configuration must agree"
	);
	PARAM_CHECK(
		pDSPSettings->SrcChannelCount == pEmitter->ChannelCount,
		"Invalid channel count, DSP settings and emitter must agree"
	);

	if (pListener->pCone)
	{
		PARAM_CHECK(
			CheckCone(pListener->pCone) == PARAM_CHECK_OK,
			"Invalid listener cone"
		);
	}
	VECTOR_NORMAL_CHECK(pListener->OrientFront);
	VECTOR_NORMAL_CHECK(pListener->OrientTop);
	VECTOR_BASE_CHECK(pListener->OrientFront, pListener->OrientTop);

	if (pEmitter->pCone)
	{
		VECTOR_NORMAL_CHECK(pEmitter->OrientFront);
		PARAM_CHECK(
			CheckCone(pEmitter->pCone) == PARAM_CHECK_OK,
			"Invalid emitter cone"
		);
	}
	else if (Flags & F3DAUDIO_CALCULATE_EMITTER_ANGLE)
	{
		VECTOR_NORMAL_CHECK(pEmitter->OrientFront);
	}
	if (pEmitter->ChannelCount > 1)
	{
		/* Only used for multi-channel emitters */
		VECTOR_NORMAL_CHECK(pEmitter->OrientFront);
		VECTOR_NORMAL_CHECK(pEmitter->OrientTop);
		VECTOR_BASE_CHECK(pEmitter->OrientFront, pEmitter->OrientTop);
	}
	FLOAT_BETWEEN_CHECK(pEmitter->InnerRadius, 0.0f, FLT_MAX);
	FLOAT_BETWEEN_CHECK(pEmitter->InnerRadiusAngle, 0.0f, F3DAUDIO_2PI / 4.0f);
	PARAM_CHECK(
		pEmitter->ChannelCount > 0,
		"Invalid channel count for emitter"
	);
	PARAM_CHECK(
		pEmitter->ChannelRadius >= 0.0f,
		"Invalid channel radius for emitter"
	);
	if (pEmitter->ChannelCount > 1)
	{
		PARAM_CHECK(
			pEmitter->pChannelAzimuths != NULL,
			"Invalid channel azimuths for multi-channel emitter"
		);
		if (pEmitter->pChannelAzimuths)
		{
			for (i = 0; i < pEmitter->ChannelCount; i += 1)
			{
				float currentAzimuth = pEmitter->pChannelAzimuths[i];
				FLOAT_BETWEEN_CHECK(currentAzimuth, 0.0f, F3DAUDIO_2PI);
				if (currentAzimuth == F3DAUDIO_2PI)
				{
					PARAM_CHECK(
						!(Flags & F3DAUDIO_CALCULATE_REDIRECT_TO_LFE),
						"F3DAUDIO_CALCULATE_REDIRECT_TO_LFE valid only for"
						" matrix calculations with emitters that have no LFE"
						" channel"
					);
				}
			}
		}
	}
	FLOAT_BETWEEN_CHECK(pEmitter->CurveDistanceScaler, FLT_MIN, FLT_MAX);
	FLOAT_BETWEEN_CHECK(pEmitter->DopplerScaler, 0.0f, FLT_MAX);

	PARAM_CHECK(
		CheckCurve(pEmitter->pVolumeCurve) == PARAM_CHECK_OK,
		"Invalid Volume curve"
	);
	PARAM_CHECK(
		CheckCurve(pEmitter->pLFECurve) == PARAM_CHECK_OK,
		"Invalid LFE curve"
	);
	PARAM_CHECK(
		CheckCurve(pEmitter->pLPFDirectCurve) == PARAM_CHECK_OK,
		"Invalid LPFDirect curve"
	);
	PARAM_CHECK(
		CheckCurve(pEmitter->pLPFReverbCurve) == PARAM_CHECK_OK,
		"Invalid LPFReverb curve"
	);
	PARAM_CHECK(
		CheckCurve(pEmitter->pReverbCurve) == PARAM_CHECK_OK,
		"Invalid Reverb curve"
	);

	return PARAM_CHECK_OK;
}

/*
 * MATRIX CALCULATION
 */

/* This function computes the distance either according to a curve if pCurve
 * isn't NULL, or according to the inverse distance law 1/d otherwise.
 */
static inline float ComputeDistanceAttenuation(
	float normalizedDistance,
	F3DAUDIO_DISTANCE_CURVE *pCurve
) {
	float res;
	float alpha;
	uint32_t n_points;
	size_t i;
	if (pCurve)
	{
		F3DAUDIO_DISTANCE_CURVE_POINT* points = pCurve->pPoints;
		n_points = pCurve->PointCount;

		/* By definition, the first point in the curve must be 0.0f
		 * -Adrien
		 */

		/* We advance i up until our normalizedDistance lies between the distances of
		 * the i_th and (i-1)_th points, or we reach the last point.
		 */
		for (i = 1; (i < n_points) && (normalizedDistance >= points[i].Distance); i += 1);
		if (i == n_points)
		{
			/* We've reached the last point, so we use its value directly.
			 * Quote X3DAUDIO docs:
			 * "If an emitter moves beyond a distance of (CurveDistanceScaler × 1.0f),
			 * the last point on the curve is used to compute the volume output level."
			 */
			res = points[n_points - 1].DSPSetting;
		}
		else
		{
			/* We're between two points: the distance attenuation is the linear interpolation of the DSPSetting
			 * values defined by our points, according to the distance.
			 */
			alpha = (points[i].Distance - normalizedDistance) / (points[i].Distance - points[i - 1].Distance);
			res = LERP(alpha, points[i].DSPSetting, points[i - 1].DSPSetting);
		}
	}
	else
	{
		res = 1.0f;
		if (normalizedDistance >= 1.0f)
		{
			res /= normalizedDistance;
		}
	}
	return res;
}

static inline float ComputeConeParameter(
	float distance,
	float angle,
	float innerAngle,
	float outerAngle,
	float innerParam,
	float outerParam
) {
	/* When computing whether a point lies inside a cone, X3DAUDIO first determines
	 * whether the point is close enough to the apex of the cone.
	 * If it is, the innerParam is used.
	 * The following constant is the one that is used for this distance check;
	 * It is an approximation, found by manual binary search.
	 * TODO: find the exact value of the constant via automated binary search. */
	#define CONE_NULL_DISTANCE_TOLERANCE 1e-7

	float halfInnerAngle, halfOuterAngle, alpha;

	/* Quote X3DAudio.h:
	 * "Set both cone angles to 0 or X3DAUDIO_2PI for omnidirectionality using
	 * only the outer or inner values respectively."
	 */
	if (innerAngle == 0.0f && outerAngle == 0.0f)
	{
		return outerParam;
	}
	if (innerAngle == F3DAUDIO_2PI && outerAngle == F3DAUDIO_2PI)
	{
		return innerParam;
	}

	/* If we're within the inner angle, or close enough to the apex, we use
	 * the innerParam. */
	halfInnerAngle = innerAngle / 2.0f;
	if (distance <= CONE_NULL_DISTANCE_TOLERANCE || angle <= halfInnerAngle)
	{
		return innerParam;
	}

	/* If we're between the inner angle and the outer angle, we must use
	 * some interpolation of the innerParam and outerParam according to the
	 * distance between our angle and the inner and outer angles.
	 */
	halfOuterAngle = outerAngle / 2.0f;
	if (angle <= halfOuterAngle)
	{
		alpha = (angle - halfInnerAngle) / (halfOuterAngle - halfInnerAngle);

		/* Sooo... This is awkward. MSDN doesn't say anything, but
		 * X3DAudio.h says that this should be lerped. However in
		 * practice the behaviour of X3DAudio isn't a lerp at all. It's
		 * easy to see with big (InnerAngle / OuterAngle) values. If we
		 * want accurate emulation, we'll need to either find what
		 * formula they use, or use a more advanced interpolation, like
		 * tricubic.
		 *
		 * TODO: HIGH_ACCURACY version.
		 * -Adrien
		 */
		return LERP(alpha, innerParam, outerParam);
	}

	/* Otherwise, we're outside the outer angle, so we just return the outer param. */
	return outerParam;
}

/* X3DAudio.h declares something like this, but the default (if emitter is NULL)
 * volume curve is a *computed* inverse law, while on the other hand a curve
 * leads to a piecewise linear function. So a "default curve" like this is
 * pointless, not sure what X3DAudio does with it...
 * -Adrien
 */
#if 0
static F3DAUDIO_DISTANCE_CURVE_POINT DefaultVolumeCurvePoints[] =
{
	{ 0.0f, 1.0f },
	{ 1.0f, 0.0f }
};
static F3DAUDIO_DISTANCE_CURVE DefaultVolumeCurve =
{
	DefaultVolumeCurvePoints,
	ARRAY_COUNT(DefaultVolumeCurvePoints)
};
#endif

/* Here we declare the azimuths of every speaker for every speaker
 * configuration, ordered by increasing angle, as well as the index to which
 * they map in the final matrix for their respective configuration. It had to be
 * reverse engineered by looking at the data from various X3DAudioCalculate()
 * matrix results for the various speaker configurations; *in particular*, the
 * azimuths are different from the ones in F3DAudio.h (and X3DAudio.h) for
 * SPEAKER_STEREO (which is declared has having front L and R speakers in the
 * bit mask, but in fact has L and R *side* speakers). LF speakers are
 * deliberately not included in the SpeakerInfo list, rather, we store the index
 * into a separate field (with a -1 sentinel value if it has no LF speaker).
 * -Adrien
 */
typedef struct
{
	float azimuth;
	uint32_t matrixIdx;
} SpeakerInfo;

typedef struct
{
	uint32_t configMask;
	const SpeakerInfo *speakers;

	/* Not strictly necessary because it can be inferred from the
	 * SpeakerCount field of the F3DAUDIO_HANDLE, but makes code much
	 * cleaner and less error prone
	 */
	uint32_t numNonLFSpeakers;

	int32_t LFSpeakerIdx;
} ConfigInfo;

/* It is absolutely necessary that these are stored in increasing, *positive*
 * azimuth order (i.e. all angles between [0; 2PI]), as we'll do a linear
 * interval search inside FindSpeakerAzimuths.
 * -Adrien
 */

#define SPEAKER_AZIMUTH_CENTER			0.0f
#define SPEAKER_AZIMUTH_FRONT_RIGHT_OF_CENTER	(F3DAUDIO_PI *  1.0f / 8.0f)
#define SPEAKER_AZIMUTH_FRONT_RIGHT		(F3DAUDIO_PI *  1.0f / 4.0f)
#define SPEAKER_AZIMUTH_SIDE_RIGHT		(F3DAUDIO_PI *  1.0f / 2.0f)
#define SPEAKER_AZIMUTH_BACK_RIGHT		(F3DAUDIO_PI *  3.0f / 4.0f)
#define SPEAKER_AZIMUTH_BACK_CENTER		F3DAUDIO_PI
#define SPEAKER_AZIMUTH_BACK_LEFT		(F3DAUDIO_PI *  5.0f / 4.0f)
#define SPEAKER_AZIMUTH_SIDE_LEFT		(F3DAUDIO_PI *  3.0f / 2.0f)
#define SPEAKER_AZIMUTH_FRONT_LEFT		(F3DAUDIO_PI *  7.0f / 4.0f)
#define SPEAKER_AZIMUTH_FRONT_LEFT_OF_CENTER	(F3DAUDIO_PI * 15.0f / 8.0f)

const SpeakerInfo kMonoConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_CENTER, 0 },
};
const SpeakerInfo kStereoConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_SIDE_RIGHT,	1 },
	{ SPEAKER_AZIMUTH_SIDE_LEFT,	0 },
};
const SpeakerInfo k2Point1ConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_SIDE_RIGHT,	1 },
	{ SPEAKER_AZIMUTH_SIDE_LEFT,	0 },
};
const SpeakerInfo kSurroundConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_CENTER,	2 },
	{ SPEAKER_AZIMUTH_FRONT_RIGHT,	1 },
	{ SPEAKER_AZIMUTH_BACK_CENTER,	3 },
	{ SPEAKER_AZIMUTH_FRONT_LEFT,	0 },
};
const SpeakerInfo kQuadConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_FRONT_RIGHT, 1 },
	{ SPEAKER_AZIMUTH_BACK_RIGHT,  3 },
	{ SPEAKER_AZIMUTH_BACK_LEFT,   2 },
	{ SPEAKER_AZIMUTH_FRONT_LEFT,  0 },
};
const SpeakerInfo k4Point1ConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_FRONT_RIGHT,	1 },
	{ SPEAKER_AZIMUTH_BACK_RIGHT,	4 },
	{ SPEAKER_AZIMUTH_BACK_LEFT,	3 },
	{ SPEAKER_AZIMUTH_FRONT_LEFT,	0 },
};
const SpeakerInfo k5Point1ConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_CENTER,	2 },
	{ SPEAKER_AZIMUTH_FRONT_RIGHT,	1 },
	{ SPEAKER_AZIMUTH_BACK_RIGHT,	5 },
	{ SPEAKER_AZIMUTH_BACK_LEFT,	4 },
	{ SPEAKER_AZIMUTH_FRONT_LEFT,	0 },
};
const SpeakerInfo k7Point1ConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_CENTER,			2 },
	{ SPEAKER_AZIMUTH_FRONT_RIGHT_OF_CENTER,	7 },
	{ SPEAKER_AZIMUTH_FRONT_RIGHT,			1 },
	{ SPEAKER_AZIMUTH_BACK_RIGHT,			5 },
	{ SPEAKER_AZIMUTH_BACK_LEFT,			4 },
	{ SPEAKER_AZIMUTH_FRONT_LEFT,			0 },
	{ SPEAKER_AZIMUTH_FRONT_LEFT_OF_CENTER,		6 },
};
const SpeakerInfo k5Point1SurroundConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_CENTER,	2 },
	{ SPEAKER_AZIMUTH_FRONT_RIGHT,	1 },
	{ SPEAKER_AZIMUTH_SIDE_RIGHT,	5 },
	{ SPEAKER_AZIMUTH_SIDE_LEFT,	4 },
	{ SPEAKER_AZIMUTH_FRONT_LEFT,	0 },
};
const SpeakerInfo k7Point1SurroundConfigSpeakers[] =
{
	{ SPEAKER_AZIMUTH_CENTER,	2 },
	{ SPEAKER_AZIMUTH_FRONT_RIGHT,	1 },
	{ SPEAKER_AZIMUTH_SIDE_RIGHT,	7 },
	{ SPEAKER_AZIMUTH_BACK_RIGHT,	5 },
	{ SPEAKER_AZIMUTH_BACK_LEFT,	4 },
	{ SPEAKER_AZIMUTH_SIDE_LEFT,	6 },
	{ SPEAKER_AZIMUTH_FRONT_LEFT,	0 },
};

/* With that organization, the index of the LF speaker into the matrix array
 * strangely looks *exactly* like the mystery field in the F3DAUDIO_HANDLE!!
 * We're keeping a separate field within ConfigInfo because it makes the code
 * much cleaner, though.
 * -Adrien
 */
const ConfigInfo kSpeakersConfigInfo[] =
{
	{ SPEAKER_MONO,			kMonoConfigSpeakers,		ARRAY_COUNT(kMonoConfigSpeakers),		-1 },
	{ SPEAKER_STEREO,		kStereoConfigSpeakers,		ARRAY_COUNT(kStereoConfigSpeakers),		-1 },
	{ SPEAKER_2POINT1,		k2Point1ConfigSpeakers,		ARRAY_COUNT(k2Point1ConfigSpeakers),		 2 },
	{ SPEAKER_SURROUND,		kSurroundConfigSpeakers,	ARRAY_COUNT(kSurroundConfigSpeakers),		-1 },
	{ SPEAKER_QUAD,			kQuadConfigSpeakers,		ARRAY_COUNT(kQuadConfigSpeakers),		-1 },
	{ SPEAKER_4POINT1,		k4Point1ConfigSpeakers,		ARRAY_COUNT(k4Point1ConfigSpeakers),		 2 },
	{ SPEAKER_5POINT1,		k5Point1ConfigSpeakers,		ARRAY_COUNT(k5Point1ConfigSpeakers),		 3 },
	{ SPEAKER_7POINT1,		k7Point1ConfigSpeakers,		ARRAY_COUNT(k7Point1ConfigSpeakers),		 3 },
	{ SPEAKER_5POINT1_SURROUND,	k5Point1SurroundConfigSpeakers,	ARRAY_COUNT(k5Point1SurroundConfigSpeakers),	 3 },
	{ SPEAKER_7POINT1_SURROUND,	k7Point1SurroundConfigSpeakers,	ARRAY_COUNT(k7Point1SurroundConfigSpeakers),	 3 },
};

/* A simple linear search is absolutely OK for 10 elements. */
static const ConfigInfo* GetConfigInfo(uint32_t speakerConfigMask)
{
	uint32_t i;
	for (i = 0; i < ARRAY_COUNT(kSpeakersConfigInfo); i += 1)
	{
		if (kSpeakersConfigInfo[i].configMask == speakerConfigMask)
		{
			return &kSpeakersConfigInfo[i];
		}
	}

	FAudio_assert(0 && "Config info not found!");
	return NULL;
}

/* Given a configuration, this function finds the azimuths of the two speakers
 * between which the emitter lies. All the azimuths here are relative to the
 * listener's base, since that's where the speakers are defined.
 */
static inline void FindSpeakerAzimuths(
	const ConfigInfo* config,
	float emitterAzimuth,
	uint8_t skipCenter,
	const SpeakerInfo **speakerInfo
) {
	uint32_t i, nexti = 0;
	float a0 = 0.0f, a1 = 0.0f;

	FAudio_assert(config != NULL);

	/* We want to find, given an azimuth, which speakers are the closest
	 * ones (in terms of angle) to that azimuth.
	 * This is done by iterating through the list of speaker azimuths, as
	 * given to us by the current ConfigInfo (which stores speaker azimuths
	 * in increasing order of azimuth for each possible speaker configuration;
	 * each speaker azimuth is defined to be between 0 and 2PI by construction).
	 */
	for (i = 0; i < config->numNonLFSpeakers; i += 1)
	{
		/* a0 and a1 are the azimuths of candidate speakers */
		a0 = config->speakers[i].azimuth;
		nexti = (i + 1) % config->numNonLFSpeakers;
		a1 = config->speakers[nexti].azimuth;

		if (a0 < a1)
		{
			if (emitterAzimuth >= a0 && emitterAzimuth < a1)
			{
				break;
			}
		}
		/* It is possible for a speaker pair to enclose the singulary at 0 == 2PI:
		 * consider for example the quad config, which has a front left speaker
		 * at 7PI/4 and a front right speaker at PI/4. In that case a0 = 7PI/4 and
		 * a1 = PI/4, and the way we know whether our current azimuth lies between
		 * that pair is by checking whether the azimuth is greather than 7PI/4 or
		 * whether it's less than PI/4. (By contract, currentAzimuth is always less
		 * than 2PI.)
		 */
		else
		{
			if (emitterAzimuth >= a0 || emitterAzimuth < a1)
			{
				break;
			}
		}
	}
	FAudio_assert(emitterAzimuth >= a0 || emitterAzimuth < a1);

	/* skipCenter means that we don't want to use the center speaker.
	 * The easiest way to deal with this is to check whether either of our candidate
	 * speakers are the center, which always has an azimuth of 0.0. If that is the case
	 * we just replace it with either the previous one or the next one.
	 */
	if (skipCenter)
	{
		if (a0 == 0.0f)
		{
			if (i == 0)
			{
				i = config->numNonLFSpeakers - 1;
			}
			else
			{
				i -= 1;
			}
		}
		else if (a1 == 0.0f)
		{
			nexti += 1;
			if (nexti >= config->numNonLFSpeakers)
			{
				nexti -= config->numNonLFSpeakers;
			}
		}
	}
	speakerInfo[0] = &config->speakers[i];
	speakerInfo[1] = &config->speakers[nexti];
}

/* Used to store diffusion factors */
/* See below for explanation. */
#define DIFFUSION_SPEAKERS_ALL		0
#define DIFFUSION_SPEAKERS_MATCHING	1
#define DIFFUSION_SPEAKERS_OPPOSITE	2
typedef float DiffusionSpeakerFactors[3];

/* ComputeInnerRadiusDiffusionFactors is a utility function that returns how
 * energy dissipates to the speakers, given the radial distance between the
 * emitter and the listener and the (optionally 0) InnerRadius distance. It
 * returns 3 floats, via the diffusionFactors array, that say how much energy
 * (after distance attenuation) will need to be distributed between each of the
 * following cases:
 *
 * - SPEAKERS_ALL for all (non-LF) speakers, _INCLUDING_ the MATCHING and OPPOSITE.
 * - SPEAKERS_OPPOSITE corresponds to the two speakers OPPOSITE the emitter.
 * - SPEAKERS_MATCHING corresponds to the two speakers closest to the emitter.
 *
 * For a distance below a certain threshold (DISTANCE_EQUAL_ENERGY), all
 * speakers receive equal energy.
 *
 * Above that, the amount that all speakers receive decreases linearly as radial
 * distance increases, up until InnerRadius / 2. (If InnerRadius is null, we use
 * MINIMUM_INNER_RADIUS.)
 *
 * At the same time, both opposite and matching speakers start to receive sound
 * (in addition to the energy they receive from the aforementioned "all
 * speakers" linear law) according to some unknown as of now law,
 * that is currently emulated with a LERP. This is true up until InnerRadius.
 *
 * Above InnerRadius, only the two matching speakers receive sound.
 *
 * For more detail, see the "Inner Radius and Inner Radius Angle" in the
 * MSDN docs for the X3DAUDIO_EMITTER structure.
 * https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.x3daudio.x3daudio_emitter(v=vs.85).aspx
 */
static inline void ComputeInnerRadiusDiffusionFactors(
	float radialDistance,
	float InnerRadius,
	DiffusionSpeakerFactors diffusionFactors
) {

	/* Determined experimentally; this is the midpoint value, i.e. the
	 * value at 0.5 for the matching speakers, used for the standard
	 * diffusion curve.
	 *
	 * Note: It is SUSPICIOUSLY close to 1/sqrt(2), but I haven't figured out why.
	 * -Adrien
	 */
	#define DIFFUSION_LERP_MIDPOINT_VALUE 0.707107f

	/* X3DAudio always uses an InnerRadius-like behaviour (i.e. diffusing sound to more than
	 * a pair of speakers) even if InnerRadius is set to 0.0f.
	 * This constant determines the distance at which this behaviour is produced in that case. */
	/* This constant was determined by manual binary search. TODO: get a more accurate version
	 * via an automated binary search. */
	#define DIFFUSION_DISTANCE_MINIMUM_INNER_RADIUS 4e-7f
	float actualInnerRadius = FAudio_max(InnerRadius, DIFFUSION_DISTANCE_MINIMUM_INNER_RADIUS);
	float normalizedRadialDist;
	float a, ms, os;

	normalizedRadialDist = radialDistance / actualInnerRadius;

	/* X3DAudio does another check for small radial distances before applying any InnerRadius-like
	 * behaviour. This is the constant that determines the threshold: below this distance we simply
	 * diffuse to all speakers equally. */
	#define DIFFUSION_DISTANCE_EQUAL_ENERGY 1e-7f
	if (radialDistance <= DIFFUSION_DISTANCE_EQUAL_ENERGY)
	{
		a = 1.0f;
		ms = 0.0f;
		os = 0.0f;
	}
	else if (normalizedRadialDist <= 0.5f)
	{
		/* Determined experimentally that this is indeed a linear law,
		 * with 100% confidence.
		 * -Adrien
		 */
		a = 1.0f - 2.0f * normalizedRadialDist;

		/* Lerping here is an approximation.
		 * TODO: High accuracy version. Having stared at the curves long
		 * enough, I'm pretty sure this is a quadratic, but trying to
		 * polyfit with numpy didn't give nice, round polynomial
		 * coefficients...
		 * -Adrien
		 */
		ms = LERP(2.0f * normalizedRadialDist, 0.0f, DIFFUSION_LERP_MIDPOINT_VALUE);
		os = 1.0f - a - ms;
	}
	else if (normalizedRadialDist <= 1.0f)
	{
		a = 0.0f;

		/* Similarly, this is a lerp based on the midpoint value; the
		 * real, high-accuracy curve also looks like a quadratic.
		 * -Adrien
		 */
		ms = LERP(2.0f * (normalizedRadialDist - 0.5f), DIFFUSION_LERP_MIDPOINT_VALUE, 1.0f);
		os = 1.0f - a - ms;
	}
	else
	{
		a = 0.0f;
		ms = 1.0f;
		os = 0.0f;
	}
	diffusionFactors[DIFFUSION_SPEAKERS_ALL] = a;
	diffusionFactors[DIFFUSION_SPEAKERS_MATCHING] = ms;
	diffusionFactors[DIFFUSION_SPEAKERS_OPPOSITE] = os;
}

/* ComputeEmitterChannelCoefficients handles the coefficients calculation for 1
 * column of the matrix. It uses ComputeInnerRadiusDiffusionFactors to separate
 * into three discrete cases; and for each case does the right repartition of
 * the energy after attenuation to the right speakers, in particular in the
 * MATCHING and OPPOSITE cases, it gives each of the two speakers found a linear
 * amount of the energy, according to the angular distance between the emitter
 * and the speaker azimuth.
 */
static inline void ComputeEmitterChannelCoefficients(
	const ConfigInfo *curConfig,
	const F3DAUDIO_BASIS *listenerBasis,
	float innerRadius,
	F3DAUDIO_VECTOR channelPosition,
	float attenuation,
	float LFEattenuation,
	uint32_t flags,
	uint32_t currentChannel,
	uint32_t numSrcChannels,
	float *pMatrixCoefficients
) {
	float elevation, radialDistance;
	F3DAUDIO_VECTOR projTopVec, projPlane;
	uint8_t skipCenter = (flags & F3DAUDIO_CALCULATE_ZEROCENTER) ? 1 : 0;
	DiffusionSpeakerFactors diffusionFactors = { 0.0f };

	float x, y;
	float emitterAzimuth;
	float energyPerChannel;
	float totalEnergy;
	uint32_t nChannelsToDiffuseTo;
	uint32_t iS, centerChannelIdx = -1;
	const SpeakerInfo* infos[2];
	float a0, a1, val;
	uint32_t i0, i1;

	/* We project against the listener basis' top vector to get the elevation of the
	 * current emitter channel position.
	 */
	elevation = VectorDot(listenerBasis->top, channelPosition);

	/* To obtain the projection in the front-right plane of the listener's basis of the
	 * emitter channel position, we simply remove the projection against the top vector.
	 * The radial distance is then the length of the projected vector.
	 */
	projTopVec = VectorScale(listenerBasis->top, elevation);
	projPlane = VectorSub(channelPosition, projTopVec);
	radialDistance = VectorLength(projPlane);

	ComputeInnerRadiusDiffusionFactors(
		radialDistance,
		innerRadius,
		diffusionFactors
	);

	/* See the ComputeInnerRadiusDiffusionFactors comment above for more context. */
	/* DIFFUSION_SPEAKERS_ALL corresponds to diffusing part of the sound to all of the
	 * speakers, equally. The amount of sound is determined by the float value
	 * diffusionFactors[DIFFUSION_SPEAKERS_ALL]. */
	if (diffusionFactors[DIFFUSION_SPEAKERS_ALL] > 0.0f)
	{
		nChannelsToDiffuseTo = curConfig->numNonLFSpeakers;
		totalEnergy = diffusionFactors[DIFFUSION_SPEAKERS_ALL] * attenuation;

		if (skipCenter)
		{
			nChannelsToDiffuseTo -= 1;
			FAudio_assert(curConfig->speakers[0].azimuth == SPEAKER_AZIMUTH_CENTER);
			centerChannelIdx = curConfig->speakers[0].matrixIdx;
		}

		energyPerChannel = totalEnergy / nChannelsToDiffuseTo;

		for (iS = 0; iS < curConfig->numNonLFSpeakers; iS += 1)
		{
			const uint32_t curSpeakerIdx = curConfig->speakers[iS].matrixIdx;
			if (skipCenter && curSpeakerIdx == centerChannelIdx)
			{
				continue;
			}

			pMatrixCoefficients[curSpeakerIdx * numSrcChannels + currentChannel] += energyPerChannel;
		}
	}

	/* DIFFUSION_SPEAKERS_MATCHING corresponds to sending part of the sound to the speakers closest
	 * (in terms of azimuth) to the current position of the emitter. The amount of sound we shoud send
	 * corresponds here to diffusionFactors[DIFFUSION_SPEAKERS_MATCHING].
	 * We use the FindSpeakerAzimuths function to find the speakers that match. */
	if (diffusionFactors[DIFFUSION_SPEAKERS_MATCHING] > 0.0f)
	{
		const float totalEnergy = diffusionFactors[DIFFUSION_SPEAKERS_MATCHING] * attenuation;

		x = VectorDot(listenerBasis->front, projPlane);
		y = VectorDot(listenerBasis->right, projPlane);

		/* Now, a critical point: We shouldn't be sending sound to
		 * matching speakers when x and y are close to 0. That's the
		 * contract we get from ComputeInnerRadiusDiffusionFactors,
		 * which checks that we're not too close to the zero distance.
		 * This allows the atan2 calculation to give good results.
		 */

		/* atan2 returns [-PI, PI], but we want [0, 2PI] */
		emitterAzimuth = FAudio_atan2f(y, x);
		if (emitterAzimuth < 0.0f)
		{
			emitterAzimuth += F3DAUDIO_2PI;
		}

		FindSpeakerAzimuths(curConfig, emitterAzimuth, skipCenter, infos);
		a0 = infos[0]->azimuth;
		a1 = infos[1]->azimuth;

		/* The following code is necessary to handle the singularity in
		 * (0 == 2PI). It'll give us a nice, well ordered interval.
		 */
		if (a0 > a1)
		{
			if (emitterAzimuth >= a0)
			{
				emitterAzimuth -= F3DAUDIO_2PI;
			}
			a0 -= F3DAUDIO_2PI;
		}
		FAudio_assert(emitterAzimuth >= a0 && emitterAzimuth <= a1);

		val = (emitterAzimuth - a0) / (a1 - a0);

		i0 = infos[0]->matrixIdx;
		i1 = infos[1]->matrixIdx;

		pMatrixCoefficients[i0 * numSrcChannels + currentChannel] += (1.0f - val) * totalEnergy;
		pMatrixCoefficients[i1 * numSrcChannels + currentChannel] += (       val) * totalEnergy;
	}

	/* DIFFUSION_SPEAKERS_OPPOSITE corresponds to sending part of the sound to the speakers
	 * _opposite_ the ones that are the closest to the current emitter position.
	 * To find these, we simply find the ones that are closest to the current emitter's azimuth + PI
	 * using the FindSpeakerAzimuth function. */
	if (diffusionFactors[DIFFUSION_SPEAKERS_OPPOSITE] > 0.0f)
	{
		/* This code is similar to the matching speakers code above. */
		const float totalEnergy = diffusionFactors[DIFFUSION_SPEAKERS_OPPOSITE] * attenuation;

		x = VectorDot(listenerBasis->front, projPlane);
		y = VectorDot(listenerBasis->right, projPlane);

		/* Similarly, we expect atan2 to be well behaved here. */
		emitterAzimuth = FAudio_atan2f(y, x);

		/* Opposite speakers lie at azimuth + PI */
		emitterAzimuth += F3DAUDIO_PI;

		/* Normalize to [0; 2PI) range. */
		if (emitterAzimuth < 0.0f)
		{
			emitterAzimuth += F3DAUDIO_2PI;
		}
		else if (emitterAzimuth > F3DAUDIO_2PI)
		{
			emitterAzimuth -= F3DAUDIO_2PI;
		}

		FindSpeakerAzimuths(curConfig, emitterAzimuth, skipCenter, infos);
		a0 = infos[0]->azimuth;
		a1 = infos[1]->azimuth;

		/* The following code is necessary to handle the singularity in
		 * (0 == 2PI). It'll give us a nice, well ordered interval.
		 */
		if (a0 > a1)
		{
			if (emitterAzimuth >= a0)
			{
				emitterAzimuth -= F3DAUDIO_2PI;
			}
			a0 -= F3DAUDIO_2PI;
		}
		FAudio_assert(emitterAzimuth >= a0 && emitterAzimuth <= a1);

		val = (emitterAzimuth - a0) / (a1 - a0);

		i0 = infos[0]->matrixIdx;
		i1 = infos[1]->matrixIdx;

		pMatrixCoefficients[i0 * numSrcChannels + currentChannel] += (1.0f - val) * totalEnergy;
		pMatrixCoefficients[i1 * numSrcChannels + currentChannel] += (       val) * totalEnergy;
	}

	if (flags & F3DAUDIO_CALCULATE_REDIRECT_TO_LFE)
	{
		FAudio_assert(curConfig->LFSpeakerIdx != -1);
		pMatrixCoefficients[curConfig->LFSpeakerIdx * numSrcChannels + currentChannel] += LFEattenuation / numSrcChannels;
	}
}

/* Calculations consist of several orthogonal steps that compose multiplicatively:
 *
 * First, we compute the attenuations (volume and LFE) due to distance, which
 * may involve an optional volume and/or LFE volume curve.
 *
 * Then, we compute those due to optional cones.
 *
 * We then compute how much energy is diffuse w.r.t InnerRadius. If InnerRadius
 * is 0.0f, this step is computed as if it was InnerRadius was
 * NON_NULL_DISTANCE_DISK_RADIUS. The way this works is, we look at the radial
 * distance of the current emitter channel to the listener, with regard to the
 * listener's top orientation (i.e. this distance is independant of the
 * emitter's elevation!). If this distance is less than NULL_DISTANCE_RADIUS,
 * energy is diffused equally between all channels. If it's greater than
 * InnerRadius (or NON_NULL_DISTANCE_RADIUS, if InnerRadius is 0.0f, as
 * mentioned above), the two closest speakers, by azimuth, receive all the
 * energy. Between InnerRadius/2.0f and InnerRadius, the energy starts bleeding
 * into the opposite speakers. Once we go below InnerRadius/2.0f, the energy
 * also starts to bleed into the other (non-opposite) channels, if there are
 * any. This computation is handled by the ComputeInnerRadiusDiffusionFactors
 * function. (TODO: High-accuracy version of this.)
 *
 * Finally, if we're not in the equal diffusion case, we find out the azimuths
 * of the two closest speakers (with azimuth being defined with respect to the
 * listener's front orientation, in the plane normal to the listener's top
 * vector), as well as the azimuths of the two opposite speakers, if necessary,
 * and linearly interpolate with respect to the angular distance. In the equal
 * diffusion case, each channel receives the same value.
 *
 * Note: in the case of multi-channel emitters, the distance attenuation is only
 * compted once, but all the azimuths and InnerRadius calculations are done per
 * emitter channel.
 *
 * TODO: Handle InnerRadiusAngle. But honestly the X3DAudio default behaviour is
 * so wacky that I wonder if anybody has ever used it.
 * -Adrien
 */
static inline void CalculateMatrix(
	uint32_t ChannelMask,
	uint32_t Flags,
	const F3DAUDIO_LISTENER *pListener,
	const F3DAUDIO_EMITTER *pEmitter,
	uint32_t SrcChannelCount,
	uint32_t DstChannelCount,
	F3DAUDIO_VECTOR emitterToListener,
	float eToLDistance,
	float normalizedDistance,
	float* MatrixCoefficients
) {
	uint32_t iEC;
	float curEmAzimuth;
	const ConfigInfo* curConfig = GetConfigInfo(ChannelMask);
	float attenuation = ComputeDistanceAttenuation(
		normalizedDistance,
		pEmitter->pVolumeCurve
	);
	/* TODO: this could be skipped if the destination has no LFE */
	float LFEattenuation = ComputeDistanceAttenuation(
		normalizedDistance,
		pEmitter->pLFECurve
	);

	F3DAUDIO_VECTOR listenerToEmitter;
	F3DAUDIO_VECTOR listenerToEmChannel;
	F3DAUDIO_BASIS listenerBasis;

	/* Note: For both cone calculations, the angle might be NaN or infinite
	 * if distance == 0... ComputeConeParameter *does* check for this
	 * special case. It is necessary that we still go through the
	 * ComputeConeParameter function, because omnidirectional cones might
	 * give either InnerVolume or OuterVolume.
	 * -Adrien
	 */
	if (pListener->pCone)
	{
		/* Negate the dot product because we need listenerToEmitter in
		 * this case
		 * -Adrien
		 */
		const float angle = -FAudio_acosf(
			VectorDot(pListener->OrientFront, emitterToListener) /
			eToLDistance
		);

		const float listenerConeParam = ComputeConeParameter(
			eToLDistance,
			angle,
			pListener->pCone->InnerAngle,
			pListener->pCone->OuterAngle,
			pListener->pCone->InnerVolume,
			pListener->pCone->OuterVolume
		);
		attenuation *= listenerConeParam;
		LFEattenuation *= listenerConeParam;
	}

	/* See note above. */
	if (pEmitter->pCone && pEmitter->ChannelCount == 1)
	{
		const float angle = FAudio_acosf(
			VectorDot(pEmitter->OrientFront, emitterToListener) /
			eToLDistance
		);

		const float emitterConeParam = ComputeConeParameter(
			eToLDistance,
			angle,
			pEmitter->pCone->InnerAngle,
			pEmitter->pCone->OuterAngle,
			pEmitter->pCone->InnerVolume,
			pEmitter->pCone->OuterVolume
		);
		attenuation *= emitterConeParam;
	}

	FAudio_zero(MatrixCoefficients, sizeof(float) * SrcChannelCount * DstChannelCount);

	/* In the SPEAKER_MONO case, we can skip all energy diffusion calculation. */
	if (DstChannelCount == 1)
	{
		for (iEC = 0; iEC < pEmitter->ChannelCount; iEC += 1)
		{
			curEmAzimuth = 0.0f;
			if (pEmitter->pChannelAzimuths)
			{
				curEmAzimuth = pEmitter->pChannelAzimuths[iEC];
			}

			/* The MONO setup doesn't have an LFE speaker. */
			if (curEmAzimuth != F3DAUDIO_2PI)
			{
				MatrixCoefficients[iEC] = attenuation;
			}
		}
	}
	else if (curConfig != NULL)
	{
		listenerToEmitter = VectorScale(emitterToListener, -1.0f);

		/* Remember here that the coordinate system is Left-Handed. */
		listenerBasis.front = pListener->OrientFront;
		listenerBasis.right = VectorCross(pListener->OrientTop, pListener->OrientFront);
		listenerBasis.top = pListener->OrientTop;


		/* Handling the mono-channel emitter case separately is easier
		 * than having it as a separate case of a for-loop; indeed, in
		 * this case, we need to ignore the non-relevant values from the
		 * emitter, _even if they're set_.
		 */
		if (pEmitter->ChannelCount == 1)
		{
			listenerToEmChannel = listenerToEmitter;

			ComputeEmitterChannelCoefficients(
				curConfig,
				&listenerBasis,
				pEmitter->InnerRadius,
				listenerToEmChannel,
				attenuation,
				LFEattenuation,
				Flags,
				0 /* currentChannel */,
				1 /* numSrcChannels */,
				MatrixCoefficients
			);
		}
		else /* Multi-channel emitter case. */
		{
			const F3DAUDIO_VECTOR emitterRight = VectorCross(pEmitter->OrientTop, pEmitter->OrientFront);

			for (iEC = 0; iEC < pEmitter->ChannelCount; iEC += 1)
			{
				const float emChAzimuth = pEmitter->pChannelAzimuths[iEC];

				/* LFEs are easy enough to deal with; we can
				 * just do them separately.
				 */
				if (emChAzimuth == F3DAUDIO_2PI)
				{
					MatrixCoefficients[curConfig->LFSpeakerIdx * pEmitter->ChannelCount + iEC] = LFEattenuation;
				}
				else
				{
					/* First compute the emitter channel
					 * vector relative to the emitter base...
					 */
					const F3DAUDIO_VECTOR emitterBaseToChannel = VectorAdd(
						VectorScale(pEmitter->OrientFront, pEmitter->ChannelRadius * FAudio_cosf(emChAzimuth)),
						VectorScale(emitterRight, pEmitter->ChannelRadius * FAudio_sinf(emChAzimuth))
					);
					/* ... then translate. */
					listenerToEmChannel = VectorAdd(
						listenerToEmitter,
						emitterBaseToChannel
					);

					ComputeEmitterChannelCoefficients(
						curConfig,
						&listenerBasis,
						pEmitter->InnerRadius,
						listenerToEmChannel,
						attenuation,
						LFEattenuation,
						Flags,
						iEC,
						pEmitter->ChannelCount,
						MatrixCoefficients
					);
				}
			}
		}
	}
	else
	{
		FAudio_assert(0 && "Config info not found!");
	}

	/* TODO: add post check to validate values
	 * (sum < 1, all values > 0, no Inf / NaN..
	 * Sum can be >1 when cone or curve is set to a gain!
	 * Perhaps under a paranoid check disabled by default.
	 */
}

/*
 * OTHER CALCULATIONS
 */

/* DopplerPitchScalar
 * Adapted from algorithm published as a part of the webaudio specification:
 * https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html#Spatialization-doppler-shift
 * -Chad
 */
static inline void CalculateDoppler(
	float SpeedOfSound,
	const F3DAUDIO_LISTENER* pListener,
	const F3DAUDIO_EMITTER* pEmitter,
	F3DAUDIO_VECTOR emitterToListener,
	float eToLDistance,
	float* listenerVelocityComponent,
	float* emitterVelocityComponent,
	float* DopplerFactor
) {
	float scaledSpeedOfSound;
	*DopplerFactor = 1.0f;

	/* Project... */
	if (eToLDistance != 0.0f)
	{
		*listenerVelocityComponent =
			VectorDot(emitterToListener, pListener->Velocity) / eToLDistance;
		*emitterVelocityComponent =
			VectorDot(emitterToListener, pEmitter->Velocity) / eToLDistance;
	}
	else
	{
		*listenerVelocityComponent = 0.0f;
		*emitterVelocityComponent = 0.0f;
	}

	if (pEmitter->DopplerScaler > 0.0f)
	{
		scaledSpeedOfSound = SpeedOfSound / pEmitter->DopplerScaler;

		/* Clamp... */
		*listenerVelocityComponent = FAudio_min(
			*listenerVelocityComponent,
			scaledSpeedOfSound
		);
		*emitterVelocityComponent = FAudio_min(
			*emitterVelocityComponent,
			scaledSpeedOfSound
		);

		/* ... then Multiply. */
		*DopplerFactor = (
			SpeedOfSound - pEmitter->DopplerScaler * *listenerVelocityComponent
		) / (
			SpeedOfSound - pEmitter->DopplerScaler * *emitterVelocityComponent
		);
		if (isnan(*DopplerFactor)) /* If emitter/listener are at the same pos... */
		{
			*DopplerFactor = 1.0f;
		}

		/* Limit the pitch shifting to 2 octaves up and 1 octave down */
		*DopplerFactor = FAudio_clamp(
			*DopplerFactor,
			0.5f,
			4.0f
		);
	}
}

void F3DAudioCalculate(
	const F3DAUDIO_HANDLE Instance,
	const F3DAUDIO_LISTENER *pListener,
	const F3DAUDIO_EMITTER *pEmitter,
	uint32_t Flags,
	F3DAUDIO_DSP_SETTINGS *pDSPSettings
) {
	uint32_t i;
	F3DAUDIO_VECTOR emitterToListener;
	float eToLDistance, normalizedDistance, dp;

	#define DEFAULT_POINTS(name, x1, y1, x2, y2) \
		static F3DAUDIO_DISTANCE_CURVE_POINT name##Points[2] = \
		{ \
			{ x1, y1 }, \
			{ x2, y2 } \
		}; \
		static F3DAUDIO_DISTANCE_CURVE name##Default = \
		{ \
			(F3DAUDIO_DISTANCE_CURVE_POINT*) &name##Points[0], 2 \
		};
	DEFAULT_POINTS(lpfDirect, 0.0f, 1.0f, 1.0f, 0.75f)
	DEFAULT_POINTS(lpfReverb, 0.0f, 0.75f, 1.0f, 0.75f)
	DEFAULT_POINTS(reverb, 0.0f, 1.0f, 1.0f, 0.0f)
	#undef DEFAULT_POINTS

	/* For XACT, this calculates "Distance" */
	emitterToListener = VectorSub(pListener->Position, pEmitter->Position);
	eToLDistance = VectorLength(emitterToListener);
	pDSPSettings->EmitterToListenerDistance = eToLDistance;

	F3DAudioCheckCalculateParams(Instance, pListener, pEmitter, Flags, pDSPSettings);

	/* This is used by MATRIX, LPF, and REVERB */
	normalizedDistance = eToLDistance / pEmitter->CurveDistanceScaler;

	if (Flags & F3DAUDIO_CALCULATE_MATRIX)
	{
		CalculateMatrix(
			SPEAKERMASK(Instance),
			Flags,
			pListener,
			pEmitter,
			pDSPSettings->SrcChannelCount,
			pDSPSettings->DstChannelCount,
			emitterToListener,
			eToLDistance,
			normalizedDistance,
			pDSPSettings->pMatrixCoefficients
		);
	}

	if (Flags & F3DAUDIO_CALCULATE_LPF_DIRECT)
	{
		pDSPSettings->LPFDirectCoefficient = ComputeDistanceAttenuation(
			normalizedDistance,
			(pEmitter->pLPFDirectCurve != NULL) ?
				pEmitter->pLPFDirectCurve :
				&lpfDirectDefault
		);
	}

	if (Flags & F3DAUDIO_CALCULATE_LPF_REVERB)
	{
		pDSPSettings->LPFReverbCoefficient = ComputeDistanceAttenuation(
			normalizedDistance,
			(pEmitter->pLPFReverbCurve != NULL) ?
				pEmitter->pLPFReverbCurve :
				&lpfReverbDefault
		);
	}

	if (Flags & F3DAUDIO_CALCULATE_REVERB)
	{
		pDSPSettings->ReverbLevel = ComputeDistanceAttenuation(
			normalizedDistance,
			(pEmitter->pReverbCurve != NULL) ?
				pEmitter->pReverbCurve :
				&reverbDefault
		);
	}

	/* For XACT, this calculates "DopplerPitchScalar" */
	if (Flags & F3DAUDIO_CALCULATE_DOPPLER)
	{
		CalculateDoppler(
			SPEEDOFSOUND(Instance),
			pListener,
			pEmitter,
			emitterToListener,
			eToLDistance,
			&pDSPSettings->ListenerVelocityComponent,
			&pDSPSettings->EmitterVelocityComponent,
			&pDSPSettings->DopplerFactor
		);
	}

	/* For XACT, this calculates "OrientationAngle" */
	if (Flags & F3DAUDIO_CALCULATE_EMITTER_ANGLE)
	{
		/* Determined roughly.
		 * Below that distance, the emitter angle is considered to be PI/2.
		 */
		#define EMITTER_ANGLE_NULL_DISTANCE 1.2e-7
		if (eToLDistance < EMITTER_ANGLE_NULL_DISTANCE)
		{
			pDSPSettings->EmitterToListenerAngle = F3DAUDIO_PI / 2.0f;
		}
		else
		{
			/* Note: pEmitter->OrientFront is normalized. */
			dp = VectorDot(emitterToListener, pEmitter->OrientFront) / eToLDistance;
			pDSPSettings->EmitterToListenerAngle = FAudio_acosf(dp);
		}
	}

	/* Unimplemented Flags */
	if (	(Flags & F3DAUDIO_CALCULATE_DELAY) &&
		SPEAKERMASK(Instance) == SPEAKER_STEREO	)
	{
		for (i = 0; i < pDSPSettings->DstChannelCount; i += 1)
		{
			pDSPSettings->pDelayTimes[i] = 0.0f;
		}
		FAudio_assert(0 && "DELAY not implemented!");
	}
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
