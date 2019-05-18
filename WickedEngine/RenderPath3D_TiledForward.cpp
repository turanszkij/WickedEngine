#include "RenderPath3D_TiledForward.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiProfiler.h"
#include "wiTextureHelper.h"

using namespace wiGraphics;

void RenderPath3D_TiledForward::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	const Texture2D* scene_read[] = { &rtMain[0], &rtMain[1] };
	if (getMSAASampleCount() > 1)
	{
		scene_read[0] = &rtMain_resolved[0];
		scene_read[1] = &rtMain_resolved[1];
	}

	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
	RenderShadows(GRAPHICSTHREAD_IMMEDIATE);
	RenderReflections(GRAPHICSTHREAD_IMMEDIATE);

	// Main scene:
	{
		GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE;

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

		const GPUResource* dsv[] = { &depthBuffer };
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

		// depth prepass
		{
			wiProfiler::BeginRange("Z-Prepass", wiProfiler::DOMAIN_GPU, threadID);

			device->BindRenderTargets(0, nullptr, &depthBuffer, threadID);
			device->ClearDepthStencil(&depthBuffer, CLEAR_DEPTH | CLEAR_STENCIL, 0, 0, threadID);

			ViewPort vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_DEPTHONLY, getHairParticlesEnabled(), true);

			device->BindRenderTargets(0, nullptr, nullptr, threadID);

			wiProfiler::EndRange(threadID);
		}

		if (getMSAASampleCount() > 1)
		{
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, threadID);
			wiRenderer::ResolveMSAADepthBuffer(&depthBuffer_Copy, &depthBuffer, threadID);
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_READ, threadID);
		}
		else
		{
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);
			device->CopyTexture2D(&depthBuffer_Copy, &depthBuffer, threadID);
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);
		}

		RenderLinearDepth(threadID);

		wiRenderer::BindDepthTextures(&depthBuffer_Copy, &rtLinearDepth, threadID);

		wiRenderer::ComputeTiledLightCulling(threadID);

		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);

		// Opaque scene:
		{
			wiProfiler::BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

			const Texture2D* rts[] = {
				&rtMain[0],
				&rtMain[1],
			};
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(rts[1], clear, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, threadID);
			device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, threadID);
			device->BindResource(PS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, threadID);
			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_TILEDFORWARD, true, true);
			wiRenderer::DrawSky(threadID);

			device->BindRenderTargets(0, nullptr, nullptr, threadID);

			wiProfiler::EndRange(threadID); // Opaque Scene
		}

		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(scene_read[0], &rtMain[0], threadID);
			device->MSAAResolve(scene_read[1], &rtMain[1], threadID);
		}
		wiRenderer::BindGBufferTextures(scene_read[0], scene_read[1], nullptr, threadID);

		RenderSSAO(threadID);

		RenderSSR(*scene_read[0], threadID);
	}

	DownsampleDepthBuffer(GRAPHICSTHREAD_IMMEDIATE);

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), GRAPHICSTHREAD_IMMEDIATE);

	RenderOutline(rtMain[0], GRAPHICSTHREAD_IMMEDIATE);

	RenderLightShafts(GRAPHICSTHREAD_IMMEDIATE);

	RenderVolumetrics(GRAPHICSTHREAD_IMMEDIATE);

	RenderParticles(false, GRAPHICSTHREAD_IMMEDIATE);

	RenderRefractionSource(*scene_read[0], GRAPHICSTHREAD_IMMEDIATE);

	RenderTransparents(rtMain[0], RENDERPASS_TILEDFORWARD, GRAPHICSTHREAD_IMMEDIATE);

	if (getMSAASampleCount() > 1)
	{
		device->MSAAResolve(scene_read[0], &rtMain[0], GRAPHICSTHREAD_IMMEDIATE);
	}

	TemporalAAResolve(*scene_read[0], *scene_read[1], GRAPHICSTHREAD_IMMEDIATE);

	RenderBloom(*scene_read[0], GRAPHICSTHREAD_IMMEDIATE);

	RenderPostprocessChain(*scene_read[0], *scene_read[1], GRAPHICSTHREAD_IMMEDIATE);

	RenderPath2D::Render();
}
