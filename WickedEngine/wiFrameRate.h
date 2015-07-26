#pragma once
#include "CommonInclude.h"
#include "wiTimer.h"

class wiTimer;

class wiFrameRate
{
protected:
	static wiTimer timer;
	static double dt;
public:
	static void Initialize();

	static void Frame();

	static double FPS();
};

