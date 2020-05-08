#pragma once
#include "CommonInclude.h"
#include "wiResourceManager.h"

#include <functional>

class RenderPath
{
private:
	uint32_t layerMask = 0xFFFFFFFF;
	bool initial_resizebuffer = false;
	int dpi = 0;

protected:
	// create resolution dependant resources, such as render targets
	virtual void ResizeBuffers() {}
	// update resolution dependent elements, such as elements dependent on current monitor DPI
	virtual void ResizeLayout();
public:
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
	virtual void Compose(wiGraphics::CommandList cmd) const {}

	inline uint32_t getLayerMask() const { return layerMask; }
	inline void setlayerMask(uint32_t value) { layerMask = value; }
};

