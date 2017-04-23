#include "Renderable2DComponent.h"
#include "wiRenderer.h"

void RenderableComponent::Update(float dt)
{
	if (wiRenderer::ResolutionChanged())
	{
		ResizeBuffers();
	}
}

void RenderableComponent::Start() {
	if (onStart != nullptr)
	{
		onStart();
	}
};

void RenderableComponent::Stop() {
	if (onStop != nullptr)
	{
		onStop();
	}
};