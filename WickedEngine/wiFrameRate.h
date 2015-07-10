#pragma once
#include "CommonInclude.h"
#include "wiTimer.h"

class Timer;

static class wiFrameRate
{
protected:
	static Timer timer;
	static double dt;
public:
	static void Initialize();

	static void Frame();

	static double FPS();
};

