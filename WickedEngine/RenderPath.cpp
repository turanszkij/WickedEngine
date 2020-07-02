#include "RenderPath2D.h"
#include "wiRenderer.h"
#include "wiPlatform.h"
#include "wiEvent.h"

void RenderPath::Load()
{
	if (!initial_resizebuffers)
	{
		initial_resizebuffers = true;
		ResizeBuffers();
		ResizeLayout();
		wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_RESOLUTION, [this](uint64_t userdata) {
			ResizeBuffers();
			ResizeLayout();
			});
		wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_DPI, [this](uint64_t userdata) {
			ResizeLayout();
			});
	}
}

void RenderPath::Start() 
{
	if (onStart != nullptr)
	{
		onStart();
	}
};

void RenderPath::Stop() 
{
	if (onStop != nullptr)
	{
		onStop();
	}
};