#include "RenderPath2D.h"
#include "wiRenderer.h"
#include "wiPlatform.h"

void RenderPath::ResizeLayout()
{
	dpi = wiPlatform::GetDPI();
}

void RenderPath::Update(float dt)
{
	if (wiRenderer::ResolutionChanged() || !initial_resizebuffer)
	{
		ResizeBuffers();
		ResizeLayout();
		initial_resizebuffer = true;
	}
	if (dpi != wiPlatform::GetDPI())
	{
		ResizeLayout();
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