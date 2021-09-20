#include "audio.h"

#include <FAudio.h>
#include <FAudioFX.h>
#include <FAPO.h>

struct AudioContext
{
	FAudio *faudio;
	FAudioMasteringVoice *mastering_voice;
	FAudioSourceVoice *source_voice;

	unsigned int	wav_channels;
	unsigned int	wav_samplerate;
	drwav_uint64	wav_sample_count;
	float		*wav_samples;
	FAudioBuffer	buffer;
};

void faudio_destroy_context(AudioContext *context)
{
	if (context != NULL)
	{
		FAudioVoice_DestroyVoice(context->source_voice);
		FAudioVoice_DestroyVoice(context->mastering_voice);
		FAudio_Release(context->faudio);
		delete context;
	}
}

void faudio_wave_load(AudioContext *context, AudioSampleWave sample, bool stereo)
{
	if (context->source_voice)
	{
		FAudioVoice_DestroyVoice(context->source_voice);
	}

	/* Buffer data... */
	context->wav_samples = WAVS_Open(
		sample,
		stereo,
		&context->wav_channels,
		&context->wav_samplerate,
		&context->wav_sample_count
	);
	context->wav_sample_count /= context->wav_channels;
	context->buffer.Flags = FAUDIO_END_OF_STREAM;
	context->buffer.AudioBytes = 4 * context->wav_sample_count * context->wav_channels;
	context->buffer.pAudioData = (uint8_t*) context->wav_samples;
	context->buffer.PlayBegin = 0;
	context->buffer.PlayLength = context->wav_sample_count;
	context->buffer.LoopBegin = 0;
	context->buffer.LoopLength = 0;
	context->buffer.LoopCount = 0;
	context->buffer.pContext = NULL;

	/* Volume meter... */
	FAPO *fapo = NULL;
	uint32_t hr = FAudioCreateVolumeMeter(&fapo, 0);
	if (hr != 0)
	{
		return;
	}

	/* Effect chain... */
	FAudioEffectDescriptor vmDesc;
	vmDesc.InitialState = 1;
	vmDesc.OutputChannels = context->wav_channels;
	vmDesc.pEffect = fapo;
	FAudioEffectChain vmChain;
	vmChain.EffectCount = 1;
	vmChain.pEffectDescriptors = &vmDesc;

	/* Wave format... */
	FAudioWaveFormatEx waveFormat;
	waveFormat.wFormatTag = 3;
	waveFormat.nChannels = context->wav_channels;
	waveFormat.nSamplesPerSec = context->wav_samplerate;
	waveFormat.nAvgBytesPerSec = context->wav_samplerate * 4;
	waveFormat.nBlockAlign = context->wav_channels * 4;
	waveFormat.wBitsPerSample = 32;
	waveFormat.cbSize = 0;

	/*... Source voice, finally. */
	hr = FAudio_CreateSourceVoice(
		context->faudio,
		&context->source_voice,
		&waveFormat,
		0,
		FAUDIO_DEFAULT_FREQ_RATIO,
		NULL,
		NULL,
		&vmChain
	);
	if (hr != 0)
	{
		return;
	}
	fapo->Release(fapo);
}

void faudio_wave_play(AudioContext *context)
{
	FAudioSourceVoice_Stop(context->source_voice, 0, FAUDIO_COMMIT_NOW);
	FAudioSourceVoice_FlushSourceBuffers(context->source_voice);

	FAudioSourceVoice_SubmitSourceBuffer(context->source_voice, &context->buffer, NULL);
	FAudioSourceVoice_Start(context->source_voice, 0, FAUDIO_COMMIT_NOW);
}

void faudio_update_volumemeter(AudioContext *context, float *peak, float *rms)
{
	FAudioFXVolumeMeterLevels levels;
	levels.pPeakLevels = peak;
	levels.pRMSLevels = rms;
	levels.ChannelCount = context->wav_channels;

	if (context->source_voice != NULL)
	{
		FAudioVoice_GetEffectParameters(
			context->source_voice,
			0,
			&levels,
			sizeof(levels)
		);
	}
}

AudioContext* faudio_create_context()
{
	// setup function pointers
	audio_destroy_context = faudio_destroy_context;
	audio_wave_load = faudio_wave_load;
	audio_wave_play = faudio_wave_play;
	audio_update_volumemeter = faudio_update_volumemeter;

	// create FAudio objects
	FAudio *faudio;
	FAudioMasteringVoice *master;
	uint32_t hr = FAudioCreate(&faudio, 0, FAUDIO_DEFAULT_PROCESSOR);
	if (hr != 0)
	{
		return NULL;
	}
	hr = FAudio_CreateMasteringVoice(
		faudio,
		&master,
		FAUDIO_DEFAULT_CHANNELS,
		FAUDIO_DEFAULT_SAMPLERATE,
		0,
		0,
		NULL
	);
	if (hr != 0)
	{
		return NULL;
	}

	// return a context object
	AudioContext *context = new AudioContext();
	context->faudio = faudio;
	context->mastering_voice = master;
	context->source_voice = NULL;
	context->wav_samples = NULL;

	// load the first wave
	audio_wave_load(context, (AudioSampleWave) 0, false);

	return context;
}
