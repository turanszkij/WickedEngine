#include "Renderable2DComponent.h"
#include "wiRenderer.h"

void RenderableComponent::Update(float dt)
{
	if (wiRenderer::GetDevice()->ResolutionChanged())
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