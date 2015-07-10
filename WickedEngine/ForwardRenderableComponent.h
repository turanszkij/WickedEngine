#pragma once
#include "Renderable3DSceneComponent.h"


class ForwardRenderableComponent :
	public Renderable3DSceneComponent
{
protected:
	wiRenderTarget rtMain;

	virtual void RenderScene(wiRenderer::DeviceContext context = wiRenderer::immediateContext);
public:
	ForwardRenderableComponent();
	~ForwardRenderableComponent();

	virtual void setPreferredThreadingCount(unsigned short value);

	virtual void Initialize();
	virtual void Load();
	virtual void Start();
	virtual void Render();
};

