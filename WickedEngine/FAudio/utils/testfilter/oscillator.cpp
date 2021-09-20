#include "oscillator.h"
#include <stddef.h>

#define _USE_MATH_DEFINES 
#include <math.h>

void oscillator_sine_wave(Oscillator *p_oscillator, int p_num_channels)
{
	if (p_oscillator->buffer != NULL)
		delete[] p_oscillator->buffer;

	p_oscillator->buffer = new float[Oscillator::CHANNEL_BUFFER_LENGTH * p_num_channels];

	for (int sample = 0; sample < Oscillator::CHANNEL_BUFFER_LENGTH; ++sample)
	{
		float value = (float) sin(2 * M_PI * sample / Oscillator::CHANNEL_BUFFER_LENGTH);

		for (int channel = 0; channel < p_num_channels; ++channel)
		{
			p_oscillator->buffer[sample * p_num_channels + channel] = value;
		}
	}
}

void oscillator_square_wave(Oscillator *p_oscillator, int p_num_channels)
{
	if (p_oscillator->buffer != NULL)
		delete[] p_oscillator->buffer;

	p_oscillator->buffer = new float[Oscillator::CHANNEL_BUFFER_LENGTH * p_num_channels];

	for (int sample = 0; sample < Oscillator::CHANNEL_BUFFER_LENGTH / 2; ++sample)
	for (int channel = 0; channel < p_num_channels; ++channel)
	{
		p_oscillator->buffer[sample * p_num_channels + channel] = 1.0f;
	}

	for (int sample = Oscillator::CHANNEL_BUFFER_LENGTH / 2; sample < Oscillator::CHANNEL_BUFFER_LENGTH; ++sample)
	for (int channel = 0; channel < p_num_channels; ++channel)
	{
		p_oscillator->buffer[sample * p_num_channels + channel] = -1.0f;
	}
}

void oscillator_saw_tooth(Oscillator *p_oscillator, int p_num_channels)
{
	if (p_oscillator->buffer != NULL)
		delete[] p_oscillator->buffer;

	p_oscillator->buffer = new float[Oscillator::CHANNEL_BUFFER_LENGTH * p_num_channels];

	for (int sample = 0; sample < Oscillator::CHANNEL_BUFFER_LENGTH; ++sample)
	{
		float value = (2.0f * sample / Oscillator::CHANNEL_BUFFER_LENGTH) - 1.0f;

		for (int channel = 0; channel < p_num_channels; ++channel)
		{
			p_oscillator->buffer[sample * p_num_channels + channel] = value;
		}
	}
}