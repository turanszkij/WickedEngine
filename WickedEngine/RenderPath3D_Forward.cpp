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
		if (getMSAASampleCount() == 1)
		{
			desc.BindFlags |= BIND_UNORDERED_ACCESS;
		}
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		desc.SampleCount = getMSAASampleCount();

		desc.Format = FORMAT_R11G11B10_FLOAT;
		device->CreateTexture(&desc, nullptr, &rtMain[0]);
		device->SetName(&rtMain[0], "rtMain[0]");

		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		device->CreateTexture(&desc, nullptr, &rtMain[1]);
		device->SetName(&rtMain[1], "rtMain[1]");

		desc.Format = FORMAT_R8_UNORM;
		device->CreateTexture(&desc, nullptr, &rtMain[2]);
		device->SetName(&rtMain[2], "rtMain[2]");

		if (getMSAASampleCount() > 1)
		{
			desc.SampleCount = 1;
			desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

			desc.Format = FORMAT_R11G11B10_FLOAT;
			device->CreateTexture(&desc, nullptr, &rtMain_resolved[0]);
			device->SetName(&rtMain_resolved[0], "rtMain_resolved[0]");

			desc.Format = FORMAT_R16G16B16A16_FLOAT;
			device->CreateTexture(&desc, nullptr, &rtMain_resolved[1]);
			device->SetName(&rtMain_resolved[1], "rtMain_resolved[1]");

			desc.Format = FORMAT_R8_UNORM;
			device->CreateTexture(&desc, nullptr, &rtMain_resolved[2]);
			device->SetName(&rtMain_resolved[2], "rtMain_resolved[2]");
		}
	}

	{
		RenderPassDesc desc;
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_CLEAR,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		device->CreateRenderPass(&desc, &renderpass_depthprepass);

		desc.attachments.clear();
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtMain[0], RenderPassAttachment::LOADOP_DONTCARE));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtMain[1], RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtMain[2], RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetSceneRT_Read(0)));
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetSceneRT_Read(1)));
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetSceneRT_Read(2)));
		}
		device->CreateRenderPass(&desc, &renderpass_main);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtMain[0], RenderPassAttachment::LOADOP_LOAD));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				&depthBuffer,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);
		if (getMSAASampleCount() > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(GetSceneRT_Read(0)));
		}
		device->CreateRenderPass(&desc, &renderpass_transparent);
	}
}

void RenderPath3D_Forward::Render() const
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

		// depth prepass
		{
			auto range = wiProfiler::BeginRangeGPU("Z-Prepass", cmd);

			device->RenderPassBegin(&renderpass_depthprepass, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			wiRenderer::DrawScene(wiRenderer::GetCamera(), RENDERPASS_DEPTHONLY, cmd, drawscene_flags);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range);
		}

		if (getMSAASampleCount() > 1)
		{
			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY, IMAGE_LAYOUT_SHADER_RESOURCE),
					GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_GENERAL)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			wiRenderer::ResolveMSAADepthBuffer(depthBuffer_Copy, depthBuffer, cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY),
					GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_GENERAL, IMAGE_LAYOUT_SHADER_RESOURCE)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}
		}
		else
		{
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
		}

		RenderLinearDepth(cmd);

		RenderAO(cmd);
	});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);

		if (wiRenderer::GetVariableRateShadingClassification() && device->CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2))
		{
			wiRenderer::ComputeShadingRateClassification(*GetSceneRT_Read(1), rtLinearDepth, rtShadingRate, cmd);
			device->BindShadingRate(SHADING_RATE_1X1, cmd);
			device->BindShadingRateImage(&rtShadingRate, cmd);
		}

		// Opaque Scene:
		{
			auto range = wiProfiler::BeginRangeGPU("Opaque Scene", cmd);

			device->RenderPassBegin(&renderpass_main, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
			device->BindResource(PS, getAOEnabled() ? &rtAO : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_AO, cmd);
			device->BindResource(PS, getSSREnabled() || getRaytracedReflectionEnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);
			wiRenderer::DrawScene(wiRenderer::GetCamera(), RENDERPASS_FORWARD, cmd, drawscene_flags);
			wiRenderer::DrawSky(cmd);

			RenderOutline(cmd);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range); // Opaque Scene
		}

		device->BindShadingRateImage(nullptr, cmd);
	});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd](wiJobArgs args) {

		GraphicsDevice* device = wiRenderer::GetDevice();
		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);
		wiRenderer::BindCommonResources(cmd);

		DownsampleDepthBuffer(cmd);

		RenderLightShafts(cmd);

		RenderVolumetrics(cmd);

		RenderSceneMIPChain(*GetSceneRT_Read(0), cmd);

		RenderSSR(*GetSceneRT_Read(1), *GetSceneRT_Read(2), cmd);

		RenderTransparents(renderpass_transparent, RENDERPASS_FORWARD, cmd);

		RenderPostprocessChain(*GetSceneRT_Read(0), *GetSceneRT_Read(1), cmd);

	});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}
