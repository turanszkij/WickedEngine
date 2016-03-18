#pragma once
#include "Renderable3DComponent.h"


class ForwardRenderableComponent :
	public Renderable3DComponent
{
protected:
	wiRenderTarget rtMain;

	virtual void RenderScene(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);
public:
	ForwardRenderableComponent();
	~ForwardRenderableComponent();

	virtual void setPreferredThreadingCount(unsigned short value);

	virtual void Initialize();
	virtual void Load();
	virtual void Start();
	virtual void Render();
};

