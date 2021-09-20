#ifndef FAUDIOFILTERDEMO_AUDIO_H
#define FAUDIOFILTERDEMO_AUDIO_H

#include <stddef.h>
#include <stdint.h>
#include "../wavcommon/wavs.h"

#ifdef _MSC_VER
#define HAVE_XAUDIO2
#endif

const uint32_t SAMPLERATE = 44100;

// types
struct AudioContext;

enum AudioEngine {
	AudioEngine_XAudio2,
	AudioEngine_FAudio
};

enum AudioVoiceType {
	AudioVoiceType_Source = 0,
	AudioVoiceType_Submix,
	AudioVoiceType_Master
};

#pragma pack(push, 1)

struct ReverbI3DL2Parameters
{
	float WetDryMix;
	int Room; 
	int RoomHF;
	float RoomRolloffFactor;
	float DecayTime;
	float DecayHFRatio;
	int Reflections;
	float ReflectionsDelay;
	int Reverb;
	float ReverbDelay;
	float Diffusion;
	float Density;
	float HFReference;
};

struct ReverbParameters
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
};

#pragma pack(pop)

extern const char *audio_voice_type_names[3];

extern const char *audio_reverb_preset_names[];
extern const ReverbI3DL2Parameters audio_reverb_presets_i3dl2[];
extern const ReverbParameters	   *audio_reverb_presets;
extern const size_t				   audio_reverb_preset_count;

typedef void (*PFN_AUDIO_DESTROY_CONTEXT)(AudioContext *p_context);
typedef void (*PFN_AUDIO_CREATE_VOICE)(AudioContext *p_context, float *p_buffer, size_t p_buffer_size, int p_sample_rate, int p_num_channels);

typedef void (*PFN_AUDIO_WAVE_LOAD)(AudioContext *p_context, AudioSampleWave sample, bool stereo);
typedef void (*PFN_AUDIO_WAVE_PLAY)(AudioContext *p_context);
typedef void (*PFN_AUDIO_WAVE_STOP)(AudioContext *p_context);

typedef void(*PFN_AUDIO_EFFECT_CHANGE)(AudioContext *p_context, bool p_enabled, ReverbParameters *p_params);

// API
AudioContext *audio_create_context(AudioEngine p_engine, bool output_5p1, AudioVoiceType effect_on_voice);

extern PFN_AUDIO_DESTROY_CONTEXT audio_destroy_context;
extern PFN_AUDIO_CREATE_VOICE audio_create_voice;

extern PFN_AUDIO_WAVE_LOAD audio_wave_load;
extern PFN_AUDIO_WAVE_PLAY audio_wave_play;
extern PFN_AUDIO_WAVE_STOP audio_wave_stop;

extern PFN_AUDIO_EFFECT_CHANGE audio_effect_change;

#endif // FAUDIOFILTERDEMO_AUDIO_H
