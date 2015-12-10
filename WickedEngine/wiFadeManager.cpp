#include "wiFadeManager.h"

void wiFadeManager::Clear()
{
	opacity = 0;
	frame = 0;
	state = FADE_FINISHED;
	color = wiColor(0, 0, 0, 1);
}

void wiFadeManager::Update()
{
	if (!IsActive())
		return;

	opacity = wiMath::Lerp(0.0f, 1.0f, (float)frame / (float)targetFrames);

	if (state == FADE_IN)
	{
		frame++;
		if (frame >= targetFrames)
		{
			state = FADE_MID;
			opacity = 1.0f;
		}
	}
	else if (state == FADE_OUT)
	{
		frame--;
		if (frame <= 0)
		{
			state = FADE_FINISHED;
			opacity = 0.0f;
		}
	}
	else if (state == FADE_MID)
	{
		state = FADE_OUT;
		opacity = 1.0f;
		onFade();
	}
	else if (state == FADE_FINISHED)
	{
		opacity = 0.0f;
	}

}
