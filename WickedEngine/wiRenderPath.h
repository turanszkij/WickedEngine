#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"

namespace wi
{
	class RenderPath : public wi::Canvas
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
		// after PostUpdate(), before Render()
		virtual void PreRender() {}
		// Render to layers, rendertargets, etc
		// This will be rendered offscreen
		virtual void Render() const {}
		// after Render(), before Compose()
		virtual void PostRender() {}
		// Compose the rendered layers (for example blend the layers together as Images)
		// This will be rendered to the backbuffer
		virtual void Compose(wi::graphics::CommandList cmd) const {}

		inline uint32_t getLayerMask() const { return layerMask; }
		inline void setlayerMask(uint32_t value) { layerMask = value; }

		wi::graphics::ColorSpace colorspace = wi::graphics::ColorSpace::SRGB;

		// This is an identifier of RenderPath subtype that is used for lua binding.
		static constexpr const char* script_check_identifier = relative_path(__FILE__);
		virtual const char* GetScriptBindingID() const { return script_check_identifier; }
	};
}
