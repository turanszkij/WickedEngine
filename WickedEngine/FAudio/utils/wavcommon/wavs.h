#ifndef WAVS_H
#define WAVS_H

#include <stdint.h>
#include "dr_wav.h"

typedef enum
{
	AudioWave_SnareDrum01 = 0,
	AudioWave_SnareDrum02,
	AudioWave_SnareDrum03,
} AudioSampleWave;

float* WAVS_Open(
	AudioSampleWave sample,
	bool stereo,
	unsigned int *wav_channels,
	unsigned int *wav_samplerate,
	drwav_uint64 *wav_sample_count
);

#endif /* WAVS_H */
