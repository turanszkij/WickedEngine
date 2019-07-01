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
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiJobSystem::context ctx;
	GRAPHICSTHREAD threadID;

	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, threadID] { RenderFrameSetUp(threadID); });
	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, threadID] { RenderShadows(threadID); });
	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, threadID] { RenderReflections(threadID); });

	// Main scene:
	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, threadID] {

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

		const GPUResource* dsv[] = { &depthBuffer };
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

		{
			auto range = wiProfiler::BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

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

			wiProfiler::EndRange(range); // Opaque Scene
		}

		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);
		device->CopyTexture2D(&depthBuffer_Copy, &depthBuffer, threadID);
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);

		RenderLinearDepth(threadID);
	});

	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, threadID] {

		wiRenderer::BindCommonResources(threadID);
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

		DownsampleDepthBuffer(threadID);

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

		RenderOutline(rtDeferred, threadID);

		RenderLightShafts(threadID);

		RenderVolumetrics(threadID);

		RenderParticles(false, threadID);

		RenderRefractionSource(rtDeferred, threadID);

		RenderTransparents(rtDeferred, RENDERPASS_TILEDFORWARD, threadID);

		RenderParticles(true, threadID);

		TemporalAAResolve(rtDeferred, rtGBuffer[1], threadID);

		RenderBloom(rtDeferred, threadID);

		RenderPostprocessChain(rtDeferred, rtGBuffer[1], threadID);

	});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}
