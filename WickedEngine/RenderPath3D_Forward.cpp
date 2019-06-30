#include "RenderPath3D_Forward.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiProfiler.h"
#include "wiTextureHelper.h"

using namespace wiGraphics;

void RenderPath3D_Forward::ResizeBuffers()
{
	RenderPath3D::ResizeBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();


	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleDesc.Count = getMSAASampleCount();

		desc.Format = wiRenderer::RTFormat_hdr;
		device->CreateTexture2D(&desc, nullptr, &rtMain[0]);
		device->SetName(&rtMain[0], "rtMain[0]");

		desc.Format = wiRenderer::RTFormat_gbuffer_1;
		device->CreateTexture2D(&desc, nullptr, &rtMain[1]);
		device->SetName(&rtMain[1], "rtMain[1]");

		if (getMSAASampleCount() > 1)
		{
			desc.SampleDesc.Count = 1;

			desc.Format = wiRenderer::RTFormat_hdr;
			device->CreateTexture2D(&desc, nullptr, &rtMain_resolved[0]);
			device->SetName(&rtMain_resolved[0], "rtMain_resolved[0]");

			desc.Format = wiRenderer::RTFormat_gbuffer_1;
			device->CreateTexture2D(&desc, nullptr, &rtMain_resolved[1]);
			device->SetName(&rtMain_resolved[1], "rtMain_resolved[1]");
		}
	}
}

void RenderPath3D_Forward::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiJobSystem::context& ctx = device->GetJobContext();
	GRAPHICSTHREAD threadID;

	const Texture2D* scene_read[] = { &rtMain[0], &rtMain[1] };
	if (getMSAASampleCount() > 1)
	{
		scene_read[0] = &rtMain_resolved[0];
		scene_read[1] = &rtMain_resolved[1];
	}

	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, threadID] { RenderFrameSetUp(threadID); });
	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, threadID] { RenderShadows(threadID); });
	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, threadID] { RenderReflections(threadID); });

	// Main scene:
	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, threadID, scene_read] {

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

		const GPUResource* dsv[] = { &depthBuffer };
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

		// Opaque Scene:
		{
			auto range = wiProfiler::BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

			const Texture2D* rts[] = {
				&rtMain[0],
				&rtMain[1],
			};
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(rts[1], clear, threadID);
			device->ClearDepthStencil(&depthBuffer, CLEAR_DEPTH | CLEAR_STENCIL, 0, 0, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, threadID);
			device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, threadID);
			device->BindResource(PS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, threadID);

			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_FORWARD, getHairParticlesEnabled(), true);
			wiRenderer::DrawSky(threadID);

			device->BindRenderTargets(0, nullptr, nullptr, threadID);

			wiProfiler::EndRange(range); // Opaque Scene
		}

		if (getMSAASampleCount() > 1)
		{
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, threadID);
			wiRenderer::ResolveMSAADepthBuffer(&depthBuffer_Copy, &depthBuffer, threadID);
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_READ, threadID);

			device->MSAAResolve(scene_read[0], &rtMain[0], threadID);
			device->MSAAResolve(scene_read[1], &rtMain[1], threadID);
		}
		else
		{
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);
			device->CopyTexture2D(&depthBuffer_Copy, &depthBuffer, threadID);
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);
		}
		wiRenderer::BindGBufferTextures(scene_read[0], scene_read[1], nullptr, threadID);

		RenderLinearDepth(threadID);

		wiRenderer::BindDepthTextures(&depthBuffer_Copy, &rtLinearDepth, threadID);

		RenderSSAO(threadID);

		RenderSSR(*scene_read[0], threadID);
	});

	threadID = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, threadID, scene_read] {

		wiRenderer::BindCommonResources(threadID);

		DownsampleDepthBuffer(threadID);

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

		RenderOutline(rtMain[0], threadID);

		RenderLightShafts(threadID);

		RenderVolumetrics(threadID);

		RenderParticles(false, threadID);

		RenderRefractionSource(*scene_read[0], threadID);

		RenderTransparents(rtMain[0], RENDERPASS_FORWARD, threadID);

		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(scene_read[0], &rtMain[0], threadID);
		}

		RenderParticles(true, threadID);

		TemporalAAResolve(*scene_read[0], *scene_read[1], threadID);

		RenderBloom(*scene_read[0], threadID);

		RenderPostprocessChain(*scene_read[0], *scene_read[1], threadID);

	});

	RenderPath2D::Render();
}
