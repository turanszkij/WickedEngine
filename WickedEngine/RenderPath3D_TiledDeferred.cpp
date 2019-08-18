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

		{
			auto range = wiProfiler::BeginRangeGPU("Opaque Scene", cmd);

			const Texture2D* rts[] = {
				&rtGBuffer[0],
				&rtGBuffer[1],
				&rtGBuffer[2],
				&lightbuffer_diffuse,
				&lightbuffer_specular,
			};
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, cmd);
			float clear[] = { 0,0,0,0 };
			device->ClearRenderTarget(&rtGBuffer[1], clear, cmd);
			device->ClearDepthStencil(&depthBuffer, CLEAR_DEPTH | CLEAR_STENCIL, 0, 0, cmd);
			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
			device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, cmd);
			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), cmd, RENDERPASS_DEFERRED, getHairParticlesEnabled(), true);

			wiProfiler::EndRange(range); // Opaque Scene
		}

		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, cmd);
		device->CopyTexture2D(&depthBuffer_Copy, &depthBuffer, cmd);
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, cmd);

		RenderLinearDepth(cmd);
	});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, cmd] {

		wiRenderer::BindCommonResources(cmd);

		RenderDecals(cmd);

		device->BindResource(CS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, cmd);
		device->BindResource(CS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);

		wiRenderer::ComputeTiledLightCulling(
			depthBuffer_Copy,
			cmd, 
			&rtGBuffer[0],
			&rtGBuffer[1],
			&rtGBuffer[2],
			&lightbuffer_diffuse,
			&lightbuffer_specular
		);

		RenderSSAO(cmd);

		RenderSSS(cmd);

		RenderDeferredComposition(cmd);

		RenderSSR(rtDeferred, rtGBuffer[1], cmd);

		DownsampleDepthBuffer(cmd);

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);

		RenderOutline(rtDeferred, cmd);

		RenderLightShafts(cmd);

		RenderVolumetrics(cmd);

		RenderParticles(false, cmd);

		RenderRefractionSource(rtDeferred, cmd);

		RenderTransparents(rtDeferred, RENDERPASS_TILEDFORWARD, cmd);

		RenderParticles(true, cmd);

		TemporalAAResolve(rtDeferred, rtGBuffer[1], cmd);

		RenderBloom(rtDeferred, cmd);

		RenderPostprocessChain(rtDeferred, rtGBuffer[1], cmd);

	});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}
