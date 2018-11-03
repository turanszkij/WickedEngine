#include "RenderPath2D.h"
#include "wiRenderer.h"

void RenderPath::Update(float dt)
{
	if (wiRenderer::ResolutionChanged())
	{
		ResizeBuffers();
	}
}

void RenderPath::Start() {
	if (onStart != nullptr)
	{
		onStart();
	}
};

void RenderPath::Stop() {
	if (onStop != nullptr)
	{
		onStop();
	}
};