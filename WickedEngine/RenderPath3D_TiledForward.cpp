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
	wiJobSystem::context ctx;
	CommandList cmd;

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd] { RenderFrameSetUp(cmd); });
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd] { RenderShadows(cmd); });
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd] { RenderReflections(cmd); });

	// Main scene:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, cmd] {

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);

		const GPUResource* dsv[] = { &depthBuffer };
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, cmd);

		// depth prepass
		{
			auto range = wiProfiler::BeginRangeGPU("Z-Prepass", cmd);

			device->RenderPassBegin(&renderpass_depthprepass, cmd);

			ViewPort vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), cmd, RENDERPASS_DEPTHONLY, getHairParticlesEnabled(), true);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range);
		}

		if (getMSAASampleCount() > 1)
		{
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, cmd);
			wiRenderer::ResolveMSAADepthBuffer(depthBuffer_Copy, depthBuffer, cmd);
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_READ, cmd);
		}
		else
		{
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, cmd);
			device->CopyTexture2D(&depthBuffer_Copy, &depthBuffer, cmd);
			device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, cmd);
		}

		RenderLinearDepth(cmd);

		RenderSSAO(cmd);
	});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, cmd] {

		wiRenderer::ComputeTiledLightCulling(
			depthBuffer_Copy, 
			cmd
		);

		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);

		// Opaque scene:
		{
			auto range = wiProfiler::BeginRangeGPU("Opaque Scene", cmd);

			device->RenderPassBegin(&renderpass_main, cmd);

			ViewPort vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
			device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, cmd);
			device->BindResource(PS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);
			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), cmd, RENDERPASS_TILEDFORWARD, true, true);
			wiRenderer::DrawSky(cmd);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range); // Opaque Scene
		}
	});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, cmd] {

		wiRenderer::BindCommonResources(cmd);

		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(GetSceneRT_Read(0), &rtMain[0], cmd);
			device->MSAAResolve(GetSceneRT_Read(1), &rtMain[1], cmd);
		}

		RenderSSR(*GetSceneRT_Read(0), rtMain[1], cmd);

		DownsampleDepthBuffer(cmd);

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);

		RenderOutline(rtMain[0], cmd);

		RenderLightShafts(cmd);

		RenderVolumetrics(cmd);

		RenderParticles(false, cmd);

		RenderRefractionSource(*GetSceneRT_Read(0), cmd);

		RenderTransparents(renderpass_transparent, RENDERPASS_TILEDFORWARD, cmd);

		if (getMSAASampleCount() > 1)
		{
			device->MSAAResolve(GetSceneRT_Read(0), &rtMain[0], cmd);
		}

		TemporalAAResolve(*GetSceneRT_Read(0), *GetSceneRT_Read(1), cmd);

		RenderBloom(renderpass_bloom, cmd);

		RenderPostprocessChain(*GetSceneRT_Read(0), *GetSceneRT_Read(1), cmd);
	});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}
