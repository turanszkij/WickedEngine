
#include "audio.h"

#ifdef HAVE_XAUDIO2

#include <xaudio2.h>
#include <xaudio2fx.h>
#include <SDL.h>

struct AudioContext 
{
	IXAudio2 *xaudio2;
	uint32_t	output_5p1;
	AudioVoiceType effect_on_voice;

	IXAudio2MasteringVoice *mastering_voice;
	IXAudio2SubmixVoice *submix_voice;
	IXAudio2SourceVoice *source_voice;
	IXAudio2Voice *voices[3];

	unsigned int wav_channels;
	unsigned int wav_samplerate;
	drwav_uint64 wav_sample_count;
	float *		 wav_samples;

	XAUDIO2_BUFFER    buffer;

	XAUDIO2_EFFECT_DESCRIPTOR reverb_effect;
	XAUDIO2_EFFECT_CHAIN	  effect_chain;
	ReverbParameters		  reverb_params;
	bool					  reverb_enabled;
};

void xaudio_destroy_context(AudioContext *context)
{
	if (context != NULL)
	{
		context->source_voice->DestroyVoice();
		context->submix_voice->DestroyVoice();
		context->mastering_voice->DestroyVoice();
		context->xaudio2->Release();
		delete context;
	}
}

void xaudio_create_voice(AudioContext *context, float *buffer, size_t buffer_size, int sample_rate, int num_channels)
{
	// create reverb effect
	IUnknown *xapo = NULL;
	HRESULT hr = XAudio2CreateReverb(&xapo);

	if (FAILED(hr))
	{
		return;
	}

	// create effect chain
	context->reverb_effect.InitialState = context->reverb_enabled;
	context->reverb_effect.OutputChannels = (context->output_5p1) ? 6 : context->wav_channels;
	context->reverb_effect.pEffect = xapo;

	context->effect_chain.EffectCount = 1;
	context->effect_chain.pEffectDescriptors = &context->reverb_effect;

	XAUDIO2_EFFECT_CHAIN *voice_effect[3] = {NULL, NULL, NULL};
	voice_effect[context->effect_on_voice] = &context->effect_chain;

	// create a mastering voice
	uint32_t inChannels = context->wav_channels;
	if (context->output_5p1 && context->effect_on_voice != AudioVoiceType_Master)
	{
		inChannels = 6;
	}

	hr = context->xaudio2->CreateMasteringVoice(
		&context->mastering_voice, 
		inChannels,
		XAUDIO2_DEFAULT_SAMPLERATE,
		0,
		0,
		voice_effect[AudioVoiceType_Master]
	);
	if (FAILED(hr))
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

	hr = context->xaudio2->CreateSubmixVoice(
		&context->submix_voice,
		inChannels, 
		SAMPLERATE,
		0,
		0,
		NULL,
		voice_effect[AudioVoiceType_Submix]
	);
	if (FAILED(hr))
	{
		return;
	}
	context->voices[AudioVoiceType_Submix] = context->submix_voice;
	context->submix_voice->SetVolume(1.0f);

	// create a source voice
	XAUDIO2_SEND_DESCRIPTOR desc = {0, context->submix_voice};
	XAUDIO2_VOICE_SENDS sends = {1, &desc};

	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	waveFormat.nChannels = num_channels;
	waveFormat.nSamplesPerSec = sample_rate;
	waveFormat.nBlockAlign = num_channels * 4;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.wBitsPerSample = 32;
	waveFormat.cbSize = 0;

	hr = context->xaudio2->CreateSourceVoice(
		&context->source_voice, 
		&waveFormat, 
		0,
		XAUDIO2_DEFAULT_FREQ_RATIO,
		NULL, 
		&sends, 
		voice_effect[AudioVoiceType_Source]
	);
	if (FAILED(hr)) 
	{
		return;
	}
	context->voices[AudioVoiceType_Source] = context->source_voice;

	context->source_voice->SetVolume(1.0f);

	xapo->Release();

	// submit the array
	SDL_zero(context->buffer);
	context->buffer.AudioBytes = 4 * buffer_size * num_channels;
	context->buffer.pAudioData = (byte *)buffer;
	context->buffer.Flags = XAUDIO2_END_OF_STREAM;
	context->buffer.PlayBegin = 0;
	context->buffer.PlayLength = buffer_size;
	context->buffer.LoopBegin = 0;
	context->buffer.LoopLength = 0;
	context->buffer.LoopCount = 0;
}

void xaudio_reverb_set_params(AudioContext *context)
{
	XAUDIO2FX_REVERB_PARAMETERS native_params = { 0 };

	native_params.WetDryMix = context->reverb_params.WetDryMix;
	native_params.ReflectionsDelay = context->reverb_params.ReflectionsDelay;
	native_params.ReverbDelay = context->reverb_params.ReverbDelay;
	native_params.RearDelay = context->reverb_params.RearDelay;
	native_params.PositionLeft = context->reverb_params.PositionLeft;
	native_params.PositionRight = context->reverb_params.PositionRight;
	native_params.PositionMatrixLeft = context->reverb_params.PositionMatrixLeft;
	native_params.PositionMatrixRight = context->reverb_params.PositionMatrixRight;
	native_params.EarlyDiffusion = context->reverb_params.EarlyDiffusion;
	native_params.LateDiffusion = context->reverb_params.LateDiffusion;
	native_params.LowEQGain = context->reverb_params.LowEQGain;
	native_params.LowEQCutoff = context->reverb_params.LowEQCutoff;
	native_params.HighEQGain = context->reverb_params.HighEQGain;
	native_params.HighEQCutoff = context->reverb_params.HighEQCutoff;
	native_params.RoomFilterFreq = context->reverb_params.RoomFilterFreq;
	native_params.RoomFilterMain = context->reverb_params.RoomFilterMain;
	native_params.RoomFilterHF = context->reverb_params.RoomFilterHF;
	native_params.ReflectionsGain = context->reverb_params.ReflectionsGain;
	native_params.ReverbGain = context->reverb_params.ReverbGain;
	native_params.DecayTime = context->reverb_params.DecayTime;
	native_params.Density = context->reverb_params.Density;
	native_params.RoomSize = context->reverb_params.RoomSize;

	/* 2.8+ only but zero-initialization catches this 
	native_params.DisableLateField = 0; */

	HRESULT hr = context->voices[context->effect_on_voice]->SetEffectParameters(
		0, 
		&native_params,
		sizeof(XAUDIO2FX_REVERB_PARAMETERS)
	);
}

void xaudio_wave_load(AudioContext *context, AudioSampleWave sample, bool stereo)
{
	if (context->source_voice)
	{
		context->source_voice->DestroyVoice();
		context->submix_voice->DestroyVoice();
		context->mastering_voice->DestroyVoice();
	}

	context->wav_samples = WAVS_Open(
		sample,
		stereo,
		&context->wav_channels,
		&context->wav_samplerate,
		&context->wav_sample_count);

	context->wav_sample_count /= context->wav_channels;

	audio_create_voice(context, context->wav_samples, context->wav_sample_count, context->wav_samplerate, context->wav_channels);
	xaudio_reverb_set_params(context);
}

void xaudio_wave_play(AudioContext *context)
{
	context->source_voice->Stop();
	context->source_voice->FlushSourceBuffers();

	HRESULT hr = context->source_voice->SubmitSourceBuffer(&context->buffer);

	if (FAILED(hr)) 
	{
		return;
	}

	context->source_voice->Start();
}

void xaudio_wave_stop(AudioContext *context)
{
	context->source_voice->Stop(XAUDIO2_PLAY_TAILS);
}

void xaudio_effect_change(AudioContext *context, bool enabled, ReverbParameters *params)
{
	HRESULT hr;

	if (context->reverb_enabled && !enabled)
	{
		hr = context->voices[context->effect_on_voice]->DisableEffect(0);
		context->reverb_enabled = enabled;
	}
	else if (!context->reverb_enabled && enabled)
	{
		hr = context->voices[context->effect_on_voice]->EnableEffect(0);
		context->reverb_enabled = enabled;
	}

	memcpy(&context->reverb_params, params, sizeof(ReverbParameters));
	xaudio_reverb_set_params(context);
}

AudioContext *xaudio_create_context(bool output_5p1, AudioVoiceType effect_on_voice)
{
	// setup function pointers
	audio_destroy_context = xaudio_destroy_context;
	audio_create_voice = xaudio_create_voice;
	audio_wave_load = xaudio_wave_load;
	audio_wave_play = xaudio_wave_play;
	audio_wave_stop = xaudio_wave_stop;
	audio_effect_change = xaudio_effect_change;

	// create XAudio object
	IXAudio2 *xaudio2;

	HRESULT hr = XAudio2Create(&xaudio2);
	if (FAILED(hr))
	{
		return NULL;
	}

	// return a context object
	AudioContext *context = new AudioContext();
	context->xaudio2 = xaudio2;
	context->output_5p1 = output_5p1;
	context->effect_on_voice = effect_on_voice;
	context->mastering_voice = NULL;
	context->submix_voice = NULL;
	context->source_voice = NULL;
	context->wav_samples = NULL;
	context->reverb_params = audio_reverb_presets[0];
	context->reverb_enabled = false;

	// load the first wave
	audio_wave_load(context, (AudioSampleWave) 0, false);

	return context;
}

#endif // HAVE_XAUDIO2
