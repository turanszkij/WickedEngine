#include "RenderPath3D_TiledDeferred.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphics;

void RenderPath3D_TiledDeferred::Render() const
{
	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
	RenderShadows(GRAPHICSTHREAD_IMMEDIATE);
	RenderReflections(GRAPHICSTHREAD_IMMEDIATE);

	// Main scene:
	{
		GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE;
		GraphicsDevice* device = wiRenderer::GetDevice();

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

		const GPUResource* dsv[] = { &depthBuffer };
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

		{
			wiProfiler::BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

			const Texture2D* rts[] = {
				&rtGBuffer[0],
				&rtGBuffer[1],
				&rtGBuffer[2],
				&lightbuffer_diffuse,
				&lightbuffer_specular,
			};
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(&rtGBuffer[1], clear, threadID);
			device->ClearDepthStencil(&depthBuffer, CLEAR_DEPTH | CLEAR_STENCIL, 0, 0, threadID);
			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, threadID);
			device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, threadID);
			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_DEFERRED, getHairParticlesEnabled(), true);

			wiProfiler::EndRange(threadID); // Opaque Scene
		}

		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);
		device->CopyTexture2D(&depthBuffer_Copy, &depthBuffer, threadID);
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);

		RenderLinearDepth(threadID);

		device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);

		wiRenderer::BindDepthTextures(&depthBuffer_Copy, &rtLinearDepth, threadID);

		RenderDecals(threadID);

		wiRenderer::BindGBufferTextures(&rtGBuffer[0], &rtGBuffer[1], &rtGBuffer[2], threadID);

		device->BindResource(CS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, threadID);
		device->BindResource(CS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, threadID);

		wiRenderer::ComputeTiledLightCulling(threadID, &lightbuffer_diffuse, &lightbuffer_specular);

		RenderSSAO(threadID);

		RenderSSS(threadID);

		RenderDeferredComposition(threadID);

		RenderSSR(rtDeferred, threadID);
	}

	DownsampleDepthBuffer(GRAPHICSTHREAD_IMMEDIATE);

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), GRAPHICSTHREAD_IMMEDIATE);

	RenderOutline(rtDeferred, GRAPHICSTHREAD_IMMEDIATE);

	RenderLightShafts(GRAPHICSTHREAD_IMMEDIATE);

	RenderVolumetrics(GRAPHICSTHREAD_IMMEDIATE);

	RenderParticles(false, GRAPHICSTHREAD_IMMEDIATE);

	RenderRefractionSource(rtDeferred, GRAPHICSTHREAD_IMMEDIATE);

	RenderTransparents(rtDeferred, RENDERPASS_TILEDFORWARD, GRAPHICSTHREAD_IMMEDIATE);

	RenderParticles(true, GRAPHICSTHREAD_IMMEDIATE);

	TemporalAAResolve(rtDeferred, rtGBuffer[1], GRAPHICSTHREAD_IMMEDIATE);

	RenderBloom(rtDeferred, GRAPHICSTHREAD_IMMEDIATE);

	RenderPostprocessChain(rtDeferred, rtGBuffer[1], GRAPHICSTHREAD_IMMEDIATE);

	RenderPath2D::Render();
}
