#pragma once
#include "WickedEngine.h"

class RenderableComponent
{
public:
	static int screenW, screenH;

	virtual void Load() = 0;
	virtual void Start() = 0;
	virtual void Update() = 0;
	//Render the composition by layers
	virtual void Render() = 0;
	//Compose the rendered layers (for example blend the layers together as Images)
	virtual void Compose() = 0;
};

