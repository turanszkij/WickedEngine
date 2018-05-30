#pragma once
#include "CommonInclude.h"
#include "wiRenderTarget.h"
#include "wiDepthTarget.h"
#include "wiResourceManager.h"
#include "wiCVars.h"

class RenderableComponent
{
private:
	uint32_t layerMask = 0xFFFFFFFF;
protected:
	// create resolution dependant resources
	virtual void ResizeBuffers() {}
public:
	wiCVars Params;
	wiResourceManager Content;

	std::function<void()> onStart;
	std::function<void()> onStop;

	RenderableComponent(){}
	virtual ~RenderableComponent() { Unload(); }

	// initialize component
	virtual void Initialize() {}
	// load (graphics) resources
	virtual void Load() {}
	// delete resources
	virtual void Unload() {}
	// start component, load temporary resources
	virtual void Start();
	// unload temporary resources
	virtual void Stop();
	// update logic
	virtual void FixedUpdate() {}
	// update physics
	virtual void Update(float dt);
	// Render to layers, rendertargets, etc
	// This will be rendered offscreen
	virtual void Render() {}
	// Compose the rendered layers (for example blend the layers together as Images)
	// This will be rendered to the backbuffer
	virtual void Compose() {}


	inline uint32_t getLayerMask() { return layerMask; }
	inline void setlayerMask(uint32_t value) { layerMask = value; }
};

