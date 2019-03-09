#include "RenderPath2D.h"
#include "wiRenderer.h"

void RenderPath::Update(float dt)
{
	if (wiRenderer::ResolutionChanged() || !initial_resizebuffer)
	{
		ResizeBuffers();
		initial_resizebuffer = true;
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