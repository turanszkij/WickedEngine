#ifndef FAUDIOTESTVOLUMEMETER_AUDIO_H
#define FAUDIOTESTVOLUMEMETER_AUDIO_H

#include <stddef.h>
#include <stdint.h>
#include "../wavcommon/wavs.h"

#ifdef _MSC_VER
#define HAVE_XAUDIO2
#endif

typedef struct AudioContext AudioContext;

typedef enum
{
	AudioEngine_XAudio2,
	AudioEngine_FAudio
} AudioEngine;

typedef void (*PFN_AUDIO_DESTROY_CONTEXT)(AudioContext *p_context);
typedef void (*PFN_AUDIO_WAVE_LOAD)(AudioContext *p_context, AudioSampleWave sample, bool stereo);
typedef void (*PFN_AUDIO_WAVE_PLAY)(AudioContext *p_context);
typedef void (*PFN_AUDIO_UPDATE_VOLUMEMETER)(AudioContext *p_context, float *peak, float *rms);

AudioContext *audio_create_context(AudioEngine p_engine);

extern PFN_AUDIO_DESTROY_CONTEXT audio_destroy_context;
extern PFN_AUDIO_WAVE_LOAD audio_wave_load;
extern PFN_AUDIO_WAVE_PLAY audio_wave_play;
extern PFN_AUDIO_UPDATE_VOLUMEMETER audio_update_volumemeter;

#endif /* FAUDIOTESTVOLUMEMETER_AUDIO_H */
