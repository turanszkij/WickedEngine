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

	wiJobSystem::context ctx;

	wiJobSystem::Execute(ctx, [this, device] { 
		RenderFrameSetUp(device->BeginCommandList()); 
	});
	wiJobSystem::Execute(ctx, [this, device] { 
		RenderShadows(device->BeginCommandList()); 
	});
	wiJobSystem::Execute(ctx, [this, device] { 
		RenderReflections(device->BeginCommandList()); 
	});

	// Main scene:
	wiJobSystem::Execute(ctx, [&] 
	{
		GRAPHICSTHREAD threadID = device->BeginCommandList();

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

		const GPUResource* dsv[] = { &depthBuffer };
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

		// depth prepass
		{
			auto range = wiProfiler::BeginRange("Z-Prepass", wiProfiler::DOMAIN_GPU, threadID);

			device->BindRenderTargets(0, nullptr, &depthBuffer, threadID);
			device->ClearDepthStencil(&depthBuffer, CLEAR_DEPTH | CLEAR_STENCIL, 0, 0, threadID);

			ViewPort vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_DEPTHONLY, getHairParticlesEnabled(), true);

			device->BindRenderTargets(0, nullptr, nullptr, threadID);

			wiProfiler::EndRange(range);
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
	}
	);

	wiJobSystem::Execute(ctx, [&] 
	{
		GRAPHICSTHREAD threadID = device->BeginCommandList();

		wiRenderer::ComputeTiledLightCulling(threadID);

		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);

		// Opaque scene:
		{
			auto range = wiProfiler::BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

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

			wiProfiler::EndRange(range); // Opaque Scene
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
	);

	wiJobSystem::Execute(ctx, [&] 
	{
		GRAPHICSTHREAD threadID = device->BeginCommandList();

		wiRenderer::BindCommonResources(threadID);

		DownsampleDepthBuffer(threadID);

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

		RenderOutline(rtMain[0], threadID);

		RenderLightShafts(threadID);

		RenderVolumetrics(threadID);

		RenderParticles(false, threadID);

		RenderRefractionSource(*scene_read[0], threadID);

		RenderTransparents(rtMain[0], RENDERPASS_TILEDFORWARD, threadID);

		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(scene_read[0], &rtMain[0], threadID);
		}

		TemporalAAResolve(*scene_read[0], *scene_read[1], threadID);

		RenderBloom(*scene_read[0], threadID);

		RenderPostprocessChain(*scene_read[0], *scene_read[1], threadID);
	}
	);

	wiJobSystem::Wait(ctx);

	RenderPath2D::Render();
}
