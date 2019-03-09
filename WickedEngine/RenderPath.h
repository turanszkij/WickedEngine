#pragma once
#include "CommonInclude.h"
#include "wiResourceManager.h"

class RenderPath
{
private:
	uint32_t layerMask = 0xFFFFFFFF;
	bool initial_resizebuffer = false;

protected:
	// create resolution dependant resources
	virtual void ResizeBuffers() {}
public:
	wiResourceManager Content;

	std::function<void()> onStart;
	std::function<void()> onStop;

	virtual ~RenderPath() { Unload(); }

	// initialize component
	virtual void Initialize() {}
	// load resources
	virtual void Load() {}
	// delete resources
	virtual void Unload() {}
	// start component, load temporary resources
	virtual void Start();
	// unload temporary resources
	virtual void Stop();
	// update with fixed frequency
	virtual void FixedUpdate() {}
	// update once per frame
	//	dt : elapsed time since last call in seconds
	virtual void Update(float dt);
	// Render to layers, rendertargets, etc
	// This will be rendered offscreen
	virtual void Render() const {}
	// Compose the rendered layers (for example blend the layers together as Images)
	// This will be rendered to the backbuffer
	virtual void Compose() const {}

	inline uint32_t getLayerMask() const { return layerMask; }
	inline void setlayerMask(uint32_t value) { layerMask = value; }
};

