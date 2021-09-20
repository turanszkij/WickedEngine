#include "audio.h"

#include <FAudio.h>
#include <FAudioFX.h>
#include <SDL.h>

struct AudioContext 
{
	FAudio *faudio;
	bool output_5p1;
	AudioVoiceType effect_on_voice;

	FAudioMasteringVoice *mastering_voice;
	FAudioSubmixVoice *submix_voice;
	FAudioSourceVoice *source_voice;
	FAudioVoice *voices[3];

	unsigned int wav_channels;
	unsigned int wav_samplerate;
	drwav_uint64 wav_sample_count;
	float *		 wav_samples;

	FAudioBuffer      buffer;

	FAudioEffectDescriptor reverb_effect;
	FAudioEffectChain	   effect_chain;
	ReverbParameters	   reverb_params;
	bool				   reverb_enabled;
};

void faudio_destroy_context(AudioContext *context)
{
	if (context != NULL)
	{
		FAudioVoice_DestroyVoice(context->source_voice);
		FAudioVoice_DestroyVoice(context->submix_voice);
		FAudioVoice_DestroyVoice(context->mastering_voice);
		FAudio_Release(context->faudio);
		delete context;
	}
}

void faudio_create_voice(AudioContext *context, float *buffer, size_t buffer_size, int sample_rate, int num_channels)
{
	// create reverb effect
	FAPO *fapo = NULL;
	uint32_t hr = FAudioCreateReverb(&fapo, 0);

	if (hr != 0)
	{
		return;
	}

	// create effect chain
	context->reverb_effect.InitialState = context->reverb_enabled;
	context->reverb_effect.OutputChannels = (context->output_5p1) ? 6 : context->wav_channels;
	context->reverb_effect.pEffect = fapo;

	context->effect_chain.EffectCount = 1;
	context->effect_chain.pEffectDescriptors = &context->reverb_effect;

	FAudioEffectChain *voice_effect[3] = {NULL, NULL, NULL};
	voice_effect[context->effect_on_voice] = &context->effect_chain;

	// create a mastering voice
	uint32_t inChannels = context->wav_channels;
	if (context->output_5p1 && context->effect_on_voice != AudioVoiceType_Master)
	{
		inChannels = 6;
	}

	hr = FAudio_CreateMasteringVoice(
		context->faudio, 
		&context->mastering_voice, 
		inChannels, 
		FAUDIO_DEFAULT_SAMPLERATE,
		0, 
		0, 
		voice_effect[AudioVoiceType_Master]
	);
	if (hr != 0)
	{
		return;
	}
	context->voices[AudioVoiceType_Master] = context->mastering_voice;

	// create a submix voice
	inChannels = context->wav_channels;
	if (context->output_5p1 && context->effect_on_voice == AudioVoiceType_Source)
	{
		inChannels = 6;
	}

	hr = FAudio_CreateSubmixVoice(
		context->faudio,
		&context->submix_voice,
		inChannels,
		SAMPLERATE,
		0,
		0,
		NULL,
		voice_effect[AudioVoiceType_Submix]
	);
	context->voices[AudioVoiceType_Submix] = context->submix_voice;

	FAudioVoice_SetVolume(context->submix_voice, 1.0f, FAUDIO_COMMIT_NOW);

	// create a source voice
	FAudioSendDescriptor desc = {0, context->submix_voice};
	FAudioVoiceSends sends = {1, &desc};

	FAudioWaveFormatEx waveFormat;
	waveFormat.wFormatTag = 3;
	waveFormat.nChannels = num_channels;
	waveFormat.nSamplesPerSec = sample_rate;
	waveFormat.nAvgBytesPerSec = sample_rate * 4;
	waveFormat.nBlockAlign = num_channels * 4;
	waveFormat.wBitsPerSample = 32;
	waveFormat.cbSize = 0;

	hr = FAudio_CreateSourceVoice(
		context->faudio, 
		&context->source_voice, 
		&waveFormat, 
		0,
		FAUDIO_DEFAULT_FREQ_RATIO,
		NULL, 
		&sends, 
		voice_effect[AudioVoiceType_Source]
	);

	if (hr != 0) 
	{
		return;
	}
	context->voices[AudioVoiceType_Source] = context->source_voice;

	FAudioVoice_SetVolume(context->source_voice, 1.0f, FAUDIO_COMMIT_NOW);
	
	// submit the array
	SDL_zero(context->buffer);
	context->buffer.AudioBytes = 4 * buffer_size * num_channels;
	context->buffer.pAudioData = (uint8_t *)buffer;
	context->buffer.Flags = FAUDIO_END_OF_STREAM;
	context->buffer.PlayBegin = 0;
	context->buffer.PlayLength = buffer_size;
	context->buffer.LoopBegin = 0;
	context->buffer.LoopLength = 0;
	context->buffer.LoopCount = 0;
}

void faudio_reverb_set_params(AudioContext *context)
{
	FAudioVoice_SetEffectParameters(
		context->voices[context->effect_on_voice], 
		0, 
		&context->reverb_params, 
		sizeof(context->reverb_params), 
		FAUDIO_COMMIT_NOW
	);
}

void faudio_wave_load(AudioContext *context, AudioSampleWave sample, bool stereo)
{
	if (context->source_voice)
	{
		FAudioVoice_DestroyVoice(context->source_voice);
		FAudioVoice_DestroyVoice(context->submix_voice);
		FAudioVoice_DestroyVoice(context->mastering_voice);
	}

	context->wav_samples = WAVS_Open(
		sample,
		stereo,
		&context->wav_channels,
		&context->wav_samplerate,
		&context->wav_sample_count
	);

	context->wav_sample_count /= context->wav_channels;

	audio_create_voice(context, context->wav_samples, context->wav_sample_count, context->wav_samplerate, context->wav_channels);
	faudio_reverb_set_params(context);
}

void faudio_wave_play(AudioContext *context)
{
	FAudioSourceVoice_Stop(context->source_voice, 0, FAUDIO_COMMIT_NOW);
	FAudioSourceVoice_FlushSourceBuffers(context->source_voice);

	FAudioSourceVoice_SubmitSourceBuffer(context->source_voice, &context->buffer, NULL);
	FAudioSourceVoice_Start(context->source_voice, 0, FAUDIO_COMMIT_NOW);
}

void faudio_wave_stop(AudioContext *context)
{
	FAudioSourceVoice_Stop(context->source_voice, FAUDIO_PLAY_TAILS, FAUDIO_COMMIT_NOW);
}

void faudio_effect_change(AudioContext *context, bool enabled, ReverbParameters *params)
{
	if (context->reverb_enabled && !enabled)
	{
		FAudioVoice_DisableEffect(
			context->voices[context->effect_on_voice], 
			0, 
			FAUDIO_COMMIT_NOW
		);
		context->reverb_enabled = enabled;
	}
	else if (!context->reverb_enabled && enabled)
	{
		FAudioVoice_EnableEffect(
			context->voices[context->effect_on_voice], 
			0, 
			FAUDIO_COMMIT_NOW
		);
		context->reverb_enabled = enabled;
	}

	context->reverb_params = *params;
	faudio_reverb_set_params(context);
}

AudioContext *faudio_create_context(bool output_5p1, AudioVoiceType effect_on_voice)
{
	// setup function pointers
	audio_destroy_context = faudio_destroy_context;
	audio_create_voice = faudio_create_voice;
	audio_wave_load = faudio_wave_load;
	audio_wave_play = faudio_wave_play;
	audio_wave_stop = faudio_wave_stop;
	audio_effect_change = faudio_effect_change;

	// create Faudio object
	FAudio *faudio;

	uint32_t hr = FAudioCreate(&faudio, 0, FAUDIO_DEFAULT_PROCESSOR);
	if (hr != 0)
	{
		return NULL;
	}

	// return a context object
	AudioContext *context = new AudioContext();
	context->faudio = faudio;
	context->output_5p1 = output_5p1;
	context->effect_on_voice = effect_on_voice;

	context->source_voice = NULL;
	context->submix_voice = NULL;
	context->mastering_voice = NULL;
	context->wav_samples = NULL;
	SDL_zero(context->reverb_params);
	context->reverb_enabled = false;

	// load the first wave
	audio_wave_load(context, (AudioSampleWave) 0, false);

	return context;
}
