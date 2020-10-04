#include "RenderPath3D_TiledDeferred.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"
#include "wiBackLog.h"

using namespace wiGraphics;

void RenderPath3D_TiledDeferred::ResizeBuffers()
{
	RenderPath3D_Deferred::ResizeBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();

	// Workaround textures if R11G11B10 UAV loads are not supported by the GPU:
	if(!device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT))
	{
		wiBackLog::post("\nWARNING: GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT not supported, Tiled deferred will be using workaround slow path!\n");

		TextureDesc desc;
		desc = lightbuffer_diffuse.GetDesc();
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		device->CreateTexture(&desc, nullptr, &lightbuffer_diffuse_noR11G11B10supportavailable);
		device->SetName(&lightbuffer_diffuse_noR11G11B10supportavailable, "lightbuffer_diffuse_noR11G11B10supportavailable");

		desc = lightbuffer_specular.GetDesc();
		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		device->CreateTexture(&desc, nullptr, &lightbuffer_specular_noR11G11B10supportavailable);
		device->SetName(&lightbuffer_specular_noR11G11B10supportavailable, "lightbuffer_specular_noR11G11B10supportavailable");
	}
}

void RenderPath3D_TiledDeferred::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiJobSystem::context ctx;
	CommandList cmd;

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) { RenderFrameSetUp(cmd); });
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) { RenderShadows(cmd); });
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) { RenderReflections(cmd); });

	static const uint32_t drawscene_flags =
		wiRenderer::DRAWSCENE_OPAQUE |
		wiRenderer::DRAWSCENE_HAIRPARTICLE |
		wiRenderer::DRAWSCENE_TESSELLATION |
		wiRenderer::DRAWSCENE_OCCLUSIONCULLING
		;

	// Main scene:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);

		{
			auto range = wiProfiler::BeginRangeGPU("Opaque Scene", cmd);

			device->RenderPassBegin(&renderpass_gbuffer, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
			wiRenderer::DrawScene(wiRenderer::GetCamera(), RENDERPASS_DEFERRED, cmd, drawscene_flags);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range); // Opaque Scene
		}

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY, IMAGE_LAYOUT_COPY_SRC),
				GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_COPY_DST)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->CopyResource(&depthBuffer_Copy, &depthBuffer, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_COPY_SRC, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY),
				GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_COPY_DST, IMAGE_LAYOUT_SHADER_RESOURCE)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		RenderLinearDepth(cmd);

		RenderAO(cmd);
	});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);
		wiRenderer::BindCommonResources(cmd);

		RenderDecals(cmd);

		device->BindResource(CS, getAOEnabled() ? &rtAO : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_AO, cmd);
		device->BindResource(CS, getSSREnabled() || getRaytracedReflectionEnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);


		if (device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT))
		{
			wiRenderer::ComputeTiledLightCulling(
				depthBuffer_Copy,
				cmd,
				&rtGBuffer[0],
				&rtGBuffer[1],
				&rtGBuffer[2],
				&lightbuffer_diffuse,
				&lightbuffer_specular
			);
		}
		else
		{
			// This workaround if R11G11B10_FLOAT can't be used with UAV loads copies into R16G16B16A16_FLOAT, does the tiled deferred then copies back:
			device->EventBegin("WARNING: GRAPHICSDEVICE_CAPABILITY_UAV_LOAD_FORMAT_R11G11B10_FLOAT not supported workaround!", cmd);

			wiRenderer::CopyTexture2D(lightbuffer_diffuse_noR11G11B10supportavailable, 0, 0, 0, lightbuffer_diffuse, 0, cmd);
			wiRenderer::CopyTexture2D(lightbuffer_specular_noR11G11B10supportavailable, 0, 0, 0, lightbuffer_specular, 0, cmd);

			wiRenderer::ComputeTiledLightCulling(
				depthBuffer_Copy,
				cmd,
				&rtGBuffer[0],
				&rtGBuffer[1],
				&rtGBuffer[2],
				&lightbuffer_diffuse_noR11G11B10supportavailable,
				&lightbuffer_specular_noR11G11B10supportavailable
			);

			wiRenderer::CopyTexture2D(lightbuffer_diffuse, 0, 0, 0, lightbuffer_diffuse_noR11G11B10supportavailable, 0, cmd);
			wiRenderer::CopyTexture2D(lightbuffer_specular, 0, 0, 0, lightbuffer_specular_noR11G11B10supportavailable, 0, cmd);

			device->EventEnd(cmd);
		}

	});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);
		wiRenderer::BindCommonResources(cmd);

		RenderSSS(cmd);

		RenderDeferredComposition(cmd);

		DownsampleDepthBuffer(cmd);

		RenderLightShafts(cmd);

		RenderVolumetrics(cmd);

		RenderSceneMIPChain(rtDeferred, cmd);

		RenderSSR(rtGBuffer[1], rtGBuffer[2], cmd);

		RenderTransparents(renderpass_transparent, RENDERPASS_TILEDFORWARD, cmd);

		RenderPostprocessChain(rtDeferred, rtGBuffer[1], cmd);

	});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}
