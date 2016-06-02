#include "wiFrameRate.h"

wiTimer wiFrameRate::timer;
double wiFrameRate::dt=0.0;

void wiFrameRate::Initialize()
{
	timer = wiTimer();
}

void wiFrameRate::Frame(){
	dt=timer.elapsed();
	timer.record();
}

double wiFrameRate::FPS() {
	static const int NUM_FPS_SAMPLES = 20;
	static const double INV_NUM_FPS_SAMPLES = 1.0 / NUM_FPS_SAMPLES;
	static double fpsSamples[NUM_FPS_SAMPLES];
	static int currentSample = 0;


	fpsSamples[currentSample] = 1000.0 / dt;
    double fps = 0;
    for (int i = 0; i < NUM_FPS_SAMPLES; i++){
        fps += fpsSamples[i];
	}
    fps *= INV_NUM_FPS_SAMPLES;
	currentSample = (currentSample + 1) % NUM_FPS_SAMPLES;

    return fps;
}
