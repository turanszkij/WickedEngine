#ifndef FACTFILTERDEMO_OSCILLATOR_H
#define FACTFILTERDEMO_OSCILLATOR_H

struct Oscillator
{
	static const int BASE_FREQ = 20;
	static const int SAMPLE_RATE = 48000;
	static const int CHANNEL_BUFFER_LENGTH = SAMPLE_RATE / BASE_FREQ;

	float *buffer;
};

void oscillator_sine_wave(Oscillator *p_oscillator, int p_num_channels);
void oscillator_square_wave(Oscillator *p_oscillator, int p_num_channels);
void oscillator_saw_tooth(Oscillator *p_oscillator, int p_num_channels);

#endif // FACTFILTERDEMO_OSCILLATOR_H