#ifndef FAUDIOFILTERDEMO_AUDIO_PLAYER_H
#define FAUDIOFILTERDEMO_AUDIO_PLAYER_H

#include "oscillator.h"
#include "audio.h"
#include <SDL.h>

class AudioPlayer
{
	public :
		enum OscillatorType {
			SineWave = 0,
			SquareWave,
			SawTooth,
			Oscillator_Count
		};

		AudioPlayer() 
		{
			SDL_zero(m_oscillators);
			setup(AudioEngine_FAudio, 1);
		}

		void setup(AudioEngine p_engine, int p_channels) 
		{
			oscillator_sine_wave(&m_oscillators[SineWave], p_channels);
			oscillator_square_wave(&m_oscillators[SquareWave], p_channels);
			oscillator_saw_tooth(&m_oscillators[SawTooth], p_channels);

			m_context = audio_create_context(p_engine);
			for (int idx = 0; idx < Oscillator_Count; ++idx)
			{
				m_voices[idx] = audio_create_voice(m_context, m_oscillators[idx].buffer, Oscillator::CHANNEL_BUFFER_LENGTH, Oscillator::SAMPLE_RATE, p_channels);
			}
			m_filter = audio_create_filter(m_context);
			m_output_filter = audio_create_filter(m_context);
		}

		void shutdown()
		{
			if (m_context == NULL)
				return;

			for (int idx = 0; idx < Oscillator_Count; ++idx)
			{
				audio_voice_destroy(m_voices[idx]);
				m_voices[idx] = NULL;
			}

			audio_destroy_context(m_context);
			m_context = NULL;
		}

		void oscillator_change(OscillatorType p_type, float p_frequency, float p_volume)
		{
			audio_voice_set_frequency(m_voices[p_type], p_frequency / Oscillator::BASE_FREQ);
			audio_voice_set_volume(m_voices[p_type], p_volume);
		}

		void filter_change(int p_type, float p_cutoff_frequency, float p_q)
		{
			audio_filter_update(m_filter, p_type, p_cutoff_frequency, p_q);
			for (int idx = 0; idx < Oscillator_Count; ++idx)
			{
				audio_filter_apply(m_filter, m_voices[idx]);
			}
		}

		void output_filter_change(int p_type, float p_cutoff_frequency, float p_q)
		{
			audio_filter_update(m_output_filter, p_type, p_cutoff_frequency, p_q);
			for (int idx = 0; idx < Oscillator_Count; ++idx)
			{
				audio_output_filter_apply(m_output_filter, m_voices[idx]);
			}
		}

	private : 
		Oscillator		m_oscillators[Oscillator_Count];
		AudioContext *	m_context;
		AudioVoice *	m_voices[Oscillator_Count];
		AudioFilter *	m_filter;
		AudioFilter *	m_output_filter;
};

#endif // FAUDIOFILTERDEMO_AUDIO_PLAYER_H
