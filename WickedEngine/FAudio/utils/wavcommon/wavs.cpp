#define DR_WAV_IMPLEMENTATION
#include "wavs.h"

#ifndef RESOURCE_PATH
#ifdef _MSC_VER
#define RESOURCE_PATH "../../utils/wavcommon/resources"
#else
#define RESOURCE_PATH "utils/wavcommon/resources"
#endif
#endif

const char *audio_sample_filenames[] =
{
	RESOURCE_PATH"/snaredrum_forte.wav",
	RESOURCE_PATH"/snaredrum_fortissimo.wav",
	RESOURCE_PATH"/snaredrum_mezzoforte.wav",
};

const char *audio_stereo_filenames[] =
{
	RESOURCE_PATH"/snaredrum_forte_stereo.wav",
	RESOURCE_PATH"/snaredrum_fortissimo_stereo.wav",
	RESOURCE_PATH"/snaredrum_mezzoforte_stereo.wav",
};

float* WAVS_Open(
	AudioSampleWave sample,
	bool stereo,
	unsigned int *wav_channels,
	unsigned int *wav_samplerate,
	drwav_uint64 *wav_sample_count
) {
	return drwav_open_file_and_read_pcm_frames_f32(
		(!stereo) ?
			audio_sample_filenames[sample] :
			audio_stereo_filenames[sample],
		wav_channels,
		wav_samplerate,
		wav_sample_count,
		NULL
	);
}
