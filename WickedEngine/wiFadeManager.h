#pragma once
#include "CommonInclude.h"
#include "wiColor.h"
#include "wiTimer.h"
#include "wiMath.h"

#include <functional>

class wiFadeManager
{
public:

	float opacity = 0;
	float timer = 0;
	float targetFadeTimeInSeconds = 1.0f;
	enum FADE_STATE
	{
		FADE_IN,	// no fade -> faded
		FADE_MID,	// completely faded
		FADE_OUT,	// faded -> no fade
		FADE_FINISHED,
	} state = FADE_FINISHED;
	wiColor color = wiColor(0, 0, 0, 255);
	std::function<void()> onFade = [] {};

	wiFadeManager()
	{
		Clear();
	}
	void Clear();
	void Start(float seconds, wiColor color, std::function<void()> onFadeFunction)
	{
		targetFadeTimeInSeconds = seconds;
		this->color = color;
		timer = 0;
		state = FADE_IN;
		onFade = onFadeFunction;
	}
	void Update(float dt);
	bool IsFaded()
	{
		return state == FADE_MID;
	}
	bool IsActive()
	{
		return state != FADE_FINISHED;
	}
};

