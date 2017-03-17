#ifndef _GENERATE_MIPCHAIN_HF_
#define _GENERATE_MIPCHAIN_HF_

static const float gaussWeight0 = 1.0f;
static const float gaussWeight1 = 0.9f;
static const float gaussWeight2 = 0.55f;
static const float gaussWeight3 = 0.18f;
static const float gaussWeight4 = 0.1f;
static const float gaussNormalization = 1.0f / (gaussWeight0 + 2.0f * (gaussWeight1 + gaussWeight2 + gaussWeight3 + gaussWeight4));
static const float gaussianWeightsNormalized[9] = {
	gaussWeight4 * gaussNormalization,
	gaussWeight3 * gaussNormalization,
	gaussWeight2 * gaussNormalization,
	gaussWeight1 * gaussNormalization,
	gaussWeight0 * gaussNormalization,
	gaussWeight1 * gaussNormalization,
	gaussWeight2 * gaussNormalization,
	gaussWeight3 * gaussNormalization,
	gaussWeight4 * gaussNormalization,
};
static const uint gaussianOffsets[9] = {
	-4, -3, -2, -1, 0, 1, 2, 3, 4
};

#endif