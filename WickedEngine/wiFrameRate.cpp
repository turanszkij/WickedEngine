#include "wiFrameRate.h"
#include "wiTimer.h"

namespace wiFrameRate
{
	static wiTimer timer;
	static float dt = 0.0;
	static const int NUM_FPS_SAMPLES = 20;
	static const float INV_NUM_FPS_SAMPLES = 1.0f / NUM_FPS_SAMPLES;
	static float fpsSamples[NUM_FPS_SAMPLES];
	static int currentSample = 0;

	void UpdateFrame() 
	{
		dt = (float)timer.elapsed();
		timer.record();
	}

	float GetFPS() 
	{
		fpsSamples[currentSample] = 1000.0f / dt;
		float fps = 0;
		for (int i = 0; i < NUM_FPS_SAMPLES; i++) {
			fps += fpsSamples[i];
		}
		fps *= INV_NUM_FPS_SAMPLES;
		currentSample = (currentSample + 1) % NUM_FPS_SAMPLES;

		return fps;
	}
}
