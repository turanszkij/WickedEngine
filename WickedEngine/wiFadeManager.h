#pragma once
#include "CommonInclude.h"
#include "wiColor.h"
#include "wiTimer.h"
#include "wiMath.h"

class wiFadeManager
{
public:

	float opacity;
	int frame, targetFrames;
	enum FADE_STATE
	{
		FADE_IN,	// no fade -> faded
		FADE_MID,	// completely faded
		FADE_OUT,	// faded -> no fade
		FADE_FINISHED,
	} state;
	wiColor color;
	function<void()> onFade;

	wiFadeManager()
	{
		Clear();
	}
	void Clear();
	void Start(int targetFrames, const wiColor& color, function<void()> onFadeFunction)
	{
		this->targetFrames = targetFrames;
		this->color = color;
		frame = 0;
		state = FADE_IN;
		onFade = onFadeFunction;
	}
	void Update();
	bool IsFaded()
	{
		return state == FADE_MID;
	}
	bool IsActive()
	{
		return state != FADE_FINISHED;
	}
};

