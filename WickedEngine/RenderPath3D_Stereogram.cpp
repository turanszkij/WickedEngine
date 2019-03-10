#include "RenderPath3D_Stereogram.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiTextureHelper.h"

using namespace wiGraphics;

void RenderPath3D_Stereogram::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), GRAPHICSTHREAD_IMMEDIATE);

	const GPUResource* dsv[] = { &depthBuffer };
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, GRAPHICSTHREAD_IMMEDIATE);

	// Render depth only buffer:

	device->BindRenderTargets(0, nullptr, &depthBuffer, GRAPHICSTHREAD_IMMEDIATE);
	device->ClearDepthStencil(&depthBuffer, CLEAR_DEPTH | CLEAR_STENCIL, 0, 0, GRAPHICSTHREAD_IMMEDIATE);

	ViewPort vp;
	vp.Width = (float)depthBuffer.GetDesc().Width;
	vp.Height = (float)depthBuffer.GetDesc().Height;
	device->BindViewports(1, &vp, GRAPHICSTHREAD_IMMEDIATE);

	wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), GRAPHICSTHREAD_IMMEDIATE, RENDERPASS_DEPTHONLY, getHairParticlesEnabled(), true, getLayerMask());

	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, GRAPHICSTHREAD_IMMEDIATE);
	device->CopyTexture2D(&depthBuffer_Copy, &depthBuffer, GRAPHICSTHREAD_IMMEDIATE);

	RenderLinearDepth(GRAPHICSTHREAD_IMMEDIATE);

	wiRenderer::BindDepthTextures(&depthBuffer_Copy, &rtLinearDepth, GRAPHICSTHREAD_IMMEDIATE);
}

void RenderPath3D_Stereogram::Compose() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiImageParams fx((float)device->GetScreenWidth(), (float)device->GetScreenHeight());
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;
	fx.quality = QUALITY_LINEAR;

	device->EventBegin("Stereogram", GRAPHICSTHREAD_IMMEDIATE);
	fx.disableFullScreen();
	fx.process.clear();
	fx.process.setStereogram();
	wiImage::Draw(wiTextureHelper::getRandom64x64(), fx, GRAPHICSTHREAD_IMMEDIATE);
	device->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
}
