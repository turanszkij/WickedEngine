#include "wiFadeManager.h"

void wiFadeManager::Clear()
{
	opacity = 0;
	state = FADE_FINISHED;
}

void wiFadeManager::Update(float dt)
{
	if (!IsActive())
		return;

	if (targetFadeTimeInSeconds <= 0)
	{
		// skip fade, just launch the job
		onFade();
		state = FADE_FINISHED;
	}

	float t = timer / targetFadeTimeInSeconds;
	timer += wiMath::Clamp(dt, 0, 0.033f);

	if (state == FADE_IN)
	{
		opacity = wiMath::Lerp(0.0f, 1.0f, t);
		if (t >= 1.0f)
		{
			state = FADE_MID;
			opacity = 1.0f;
		}
	}
	else if (state == FADE_MID)
	{
		state = FADE_OUT;
		opacity = 1.0f;
		onFade();
		timer = 0;
	}
	else if (state == FADE_OUT)
	{
		opacity = wiMath::Lerp(1.0f, 0.0f, t);
		if (t >= 1.0f)
		{
			state = FADE_FINISHED;
			opacity = 0.0f;
		}
	}
	else if (state == FADE_FINISHED)
	{
		opacity = 0.0f;
		onFade = [] {};
	}

}
