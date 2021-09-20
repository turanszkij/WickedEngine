#include "audio.h"

PFN_AUDIO_DESTROY_CONTEXT audio_destroy_context = NULL;
PFN_AUDIO_CREATE_VOICE audio_create_voice = NULL;
PFN_AUDIO_VOICE_DESTROY audio_voice_destroy = NULL;
PFN_AUDIO_VOICE_SET_VOLUME audio_voice_set_volume = NULL;
PFN_AUDIO_VOICE_SET_FREQUENCY audio_voice_set_frequency = NULL;

PFN_AUDIO_CREATE_FILTER audio_create_filter = NULL;
PFN_AUDIO_FILTER_UPDATE audio_filter_update = NULL;
PFN_AUDIO_FILTER_APPLY audio_filter_apply = NULL;
PFN_AUDIO_FILTER_APPLY audio_output_filter_apply = NULL;

extern AudioContext *xaudio_create_context();
extern AudioContext *faudio_create_context();

AudioContext *audio_create_context(AudioEngine p_engine)
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
