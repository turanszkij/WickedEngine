#include "audio.h"

#ifdef HAVE_XAUDIO2

#include <xaudio2.h>
#include <math.h>

struct AudioContext 
{
	IXAudio2 *xaudio2;
	IXAudio2MasteringVoice *mastering_voice;
};

struct AudioVoice
{
	AudioContext *context;
	IXAudio2SourceVoice *voice;
};

struct AudioFilter
{
	AudioContext *context;
	XAUDIO2_FILTER_PARAMETERS params;
};

void xaudio_destroy_context(AudioContext *p_context)
{
	p_context->mastering_voice->DestroyVoice();
	p_context->xaudio2->Release();
	delete p_context;
}

AudioVoice *xaudio_create_voice(AudioContext *p_context, float *p_buffer, size_t p_buffer_size, int p_sample_rate, int p_num_channels)
{
	// create a source voice
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	waveFormat.nChannels = p_num_channels;
	waveFormat.nSamplesPerSec = p_sample_rate;
	waveFormat.nBlockAlign = p_num_channels * 4;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.wBitsPerSample = 32;
	waveFormat.cbSize = 0;

	IXAudio2SourceVoice *voice;
	XAUDIO2_SEND_DESCRIPTOR send;
	XAUDIO2_VOICE_SENDS sends;
	sends.SendCount = 1;
	sends.pSends = &send;
	send.Flags = XAUDIO2_SEND_USEFILTER;
	send.pOutputVoice = p_context->mastering_voice;
	HRESULT hr = p_context->xaudio2->CreateSourceVoice(&voice, &waveFormat, XAUDIO2_VOICE_USEFILTER, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, &sends);

	if (FAILED(hr)) {
		return NULL;
	}
	
	voice->SetVolume(0.0f);

	// submit the array
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = 4 * p_buffer_size * p_num_channels;
	buffer.pAudioData = (byte *)p_buffer;
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.PlayBegin = 0;
	buffer.PlayLength = 0;
	buffer.LoopBegin = 0;
	buffer.LoopLength = 0;
	buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

	hr = voice->SubmitSourceBuffer(&buffer);

	if (FAILED(hr)) {
		return NULL;
	}

	// start the voice playing
	voice->Start();

	// return a voice struct
	AudioVoice *result = new AudioVoice();
	result->context = p_context;
	result->voice = voice;
	return result;
}

void xaudio_voice_destroy(AudioVoice *p_voice)
{

}

void xaudio_voice_set_volume(AudioVoice *p_voice, float p_volume)
{
	p_voice->voice->SetVolume(p_volume);
}

void xaudio_voice_set_frequency(AudioVoice *p_voice, float p_frequency)
{
	p_voice->voice->SetFrequencyRatio(p_frequency);
}

AudioFilter *xaudio_create_filter(AudioContext *p_context)
{
	AudioFilter *filter = new AudioFilter();
	filter->context = p_context;
	return filter;
}

void xaudio_filter_update(AudioFilter *p_filter, int p_type, float p_cutoff_frequency, float p_q)
{
	if (p_type != -1)
	{
		p_filter->params.Type = XAUDIO2_FILTER_TYPE(p_type);
		p_filter->params.Frequency = (float) (2 * sin(PI * p_cutoff_frequency / 44100));
		p_filter->params.OneOverQ = (float)(1.0 / p_q);
	}
	else 
	{
		// documentation of XAUDIO2_FILTER_PARAMETERS: 
		// Setting XAUDIO2_FILTER_PARAMETERS with the following values is acoustically equivalent to the filter being fully bypassed.
		p_filter->params.Type = LowPassFilter;
		p_filter->params.Frequency = 1.0f;
		p_filter->params.OneOverQ = 1.0f;
	}
}

void xaudio_filter_apply(AudioFilter *p_filter, AudioVoice *p_voice)
{
	p_voice->voice->SetFilterParameters(&p_filter->params, XAUDIO2_COMMIT_ALL);
}

void xaudio_output_filter_apply(AudioFilter *p_filter, AudioVoice *p_voice)
{
	p_voice->voice->SetOutputFilterParameters(p_voice->context->mastering_voice, &p_filter->params, XAUDIO2_COMMIT_ALL);
}

AudioContext *xaudio_create_context()
{
	// setup function pointers
	audio_destroy_context = xaudio_destroy_context;
	audio_create_voice = xaudio_create_voice;
	audio_voice_destroy = xaudio_voice_destroy;
	audio_voice_set_volume = xaudio_voice_set_volume;
	audio_voice_set_frequency = xaudio_voice_set_frequency;
	audio_create_filter = xaudio_create_filter;
	audio_filter_update = xaudio_filter_update;
	audio_filter_apply = xaudio_filter_apply;
	audio_output_filter_apply = xaudio_output_filter_apply;

	// create XAudio object
	IXAudio2 *xaudio2;

	HRESULT hr = XAudio2Create(&xaudio2);
	if (FAILED(hr))
		return NULL;

	// create a mastering voice
	IXAudio2MasteringVoice *mastering_voice;

	hr = xaudio2->CreateMasteringVoice(&mastering_voice);
	if (FAILED(hr))
		return NULL;

	// return a context object
	AudioContext *context = new AudioContext();
	context->xaudio2 = xaudio2;
	context->mastering_voice = mastering_voice;

	return context;
}

#endif // HAVE_XAUDIO2
