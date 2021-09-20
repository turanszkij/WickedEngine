#include "audio.h"

PFN_AUDIO_DESTROY_CONTEXT audio_destroy_context = NULL;
PFN_AUDIO_WAVE_LOAD audio_wave_load = NULL;
PFN_AUDIO_WAVE_PLAY audio_wave_play = NULL;
PFN_AUDIO_UPDATE_VOLUMEMETER audio_update_volumemeter = NULL;

extern AudioContext* xaudio_create_context();
extern AudioContext* faudio_create_context();

AudioContext* audio_create_context(AudioEngine p_engine)
{
	switch (p_engine)
	{
		#ifdef HAVE_XAUDIO2
		case AudioEngine_XAudio2:
			return xaudio_create_context();
		#endif

		case AudioEngine_FAudio:
			return faudio_create_context();

		default:
			return NULL;
	}

}
