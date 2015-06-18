#include "FrameRate.h"

Timer FrameRate::timer;
double FrameRate::dt=0.0;

void FrameRate::Initialize()
{
	timer = Timer();
}

void FrameRate::Frame(){
	dt=timer.elapsed();
	timer.record();
}

double FrameRate::FPS() {
	static const int NUM_FPS_SAMPLES = 60;
	static double fpsSamples[NUM_FPS_SAMPLES];
	static int currentSample = 0;


	fpsSamples[currentSample % NUM_FPS_SAMPLES] = 1000.0 / dt;
    double fps = 0;
    for (int i = 0; i < NUM_FPS_SAMPLES; i++){
        fps += fpsSamples[i];
	}
    fps /= NUM_FPS_SAMPLES;
	currentSample++;

    return fps;
}
