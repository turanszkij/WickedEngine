#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"

class RenderPath : public wiCanvas
{
private:
	uint32_t layerMask = 0xFFFFFFFF;

public:
	virtual ~RenderPath() = default;

	// load resources in background (for example behind loading screen)
	virtual void Load() {}
	// called when RenderPath gets activated
	virtual void Start() {}
	// called when RenderPath gets deactivated (for example when switching to an other RenderPath)
	virtual void Stop() {}
	// executed before Update()
	virtual void PreUpdate() {}
	// update with fixed frequency
	virtual void FixedUpdate() {}
	// update once per frame
	//	dt : elapsed time since last call in seconds
	virtual void Update(float dt) {}
	// executed after Update()
	virtual void PostUpdate() {}
	// Render to layers, rendertargets, etc
	// This will be rendered offscreen
	virtual void Render() const {}
	// Compose the rendered layers (for example blend the layers together as Images)
	// This will be rendered to the backbuffer
	virtual void Compose(wiGraphics::CommandList cmd) const {}

	inline uint32_t getLayerMask() const { return layerMask; }
	inline void setlayerMask(uint32_t value) { layerMask = value; }
};

