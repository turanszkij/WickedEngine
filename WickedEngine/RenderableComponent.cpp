#include "Renderable2DComponent.h"


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