#ifndef FAUDIOFILTERDEMO_AUDIO_H
#define FAUDIOFILTERDEMO_AUDIO_H

#include <stddef.h>

#ifdef _MSC_VER
#define HAVE_XAUDIO2
#endif

const float PI = 3.14159265358979323846f;

// types
struct AudioContext;
struct AudioVoice;
struct AudioFilter;

enum AudioEngine {
	AudioEngine_XAudio2,
	AudioEngine_FAudio
};

typedef void(*PFN_AUDIO_DESTROY_CONTEXT)(AudioContext *p_context);

typedef AudioVoice *(*PFN_AUDIO_CREATE_VOICE)(AudioContext *p_context, float *p_buffer, size_t p_buffer_size, int p_sample_rate, int p_num_channels);
typedef void (*PFN_AUDIO_VOICE_DESTROY)(AudioVoice *p_voice);
typedef void (*PFN_AUDIO_VOICE_SET_VOLUME)(AudioVoice *p_voice, float p_volume);
typedef void (*PFN_AUDIO_VOICE_SET_FREQUENCY)(AudioVoice *p_vioce, float p_frequency);

typedef AudioFilter *(*PFN_AUDIO_CREATE_FILTER)(AudioContext *p_context);
typedef void (*PFN_AUDIO_FILTER_UPDATE)(AudioFilter *p_filter, int p_type, float p_cutoff_frequency, float p_q);
typedef void (*PFN_AUDIO_FILTER_APPLY)(AudioFilter *p_filter, AudioVoice *p_voice);

// API
AudioContext *audio_create_context(AudioEngine p_engine);

extern PFN_AUDIO_DESTROY_CONTEXT audio_destroy_context;
extern PFN_AUDIO_CREATE_VOICE audio_create_voice;
extern PFN_AUDIO_VOICE_DESTROY audio_voice_destroy;
extern PFN_AUDIO_VOICE_SET_VOLUME audio_voice_set_volume;
extern PFN_AUDIO_VOICE_SET_FREQUENCY audio_voice_set_frequency;

extern PFN_AUDIO_CREATE_FILTER audio_create_filter;
extern PFN_AUDIO_FILTER_UPDATE audio_filter_update;
extern PFN_AUDIO_FILTER_APPLY audio_filter_apply;
extern PFN_AUDIO_FILTER_APPLY audio_output_filter_apply;

#endif // FAUDIOFILTERDEMO_AUDIO_H
