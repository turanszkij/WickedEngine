#include "RenderPath3D_Deferred.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphics;


void RenderPath3D_Deferred::ResizeBuffers()
{
	RenderPath3D::ResizeBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();

	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;

		desc.Format = FORMAT_R8G8B8A8_UNORM;
		device->CreateTexture(&desc, nullptr, &rtGBuffer[0]);
		device->SetName(&rtGBuffer[0], "rtGBuffer[0]");

		desc.Format = FORMAT_R16G16B16A16_FLOAT;
		device->CreateTexture(&desc, nullptr, &rtGBuffer[1]);
		device->SetName(&rtGBuffer[1], "rtGBuffer[1]");

		desc.Format = FORMAT_R8G8B8A8_UNORM;
		device->CreateTexture(&desc, nullptr, &rtGBuffer[2]);
		device->SetName(&rtGBuffer[2], "rtGBuffer[2]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtDeferred);
		device->SetName(&rtDeferred, "rtDeferred");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &lightbuffer_diffuse);
		device->SetName(&lightbuffer_diffuse, "lightbuffer_diffuse");
		device->CreateTexture(&desc, nullptr, &lightbuffer_specular);
		device->SetName(&lightbuffer_specular, "lightbuffer_specular");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = FORMAT_R11G11B10_FLOAT;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtSSS[0]);
		device->SetName(&rtSSS[0], "rtSSS[0]");
		device->CreateTexture(&desc, nullptr, &rtSSS[1]);
		device->SetName(&rtSSS[1], "rtSSS[1]");
	}

	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGBuffer[0], RenderPassAttachment::LOADOP_DONTCARE));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGBuffer[1], RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGBuffer[2], RenderPassAttachment::LOADOP_DONTCARE));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&lightbuffer_diffuse, RenderPassAttachment::LOADOP_DONTCARE));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&lightbuffer_specular, RenderPassAttachment::LOADOP_DONTCARE));
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

		device->CreateRenderPass(&desc, &renderpass_gbuffer);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&lightbuffer_diffuse, RenderPassAttachment::LOADOP_LOAD));
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&lightbuffer_specular, RenderPassAttachment::LOADOP_LOAD));
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

		device->CreateRenderPass(&desc, &renderpass_lights);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtGBuffer[0], RenderPassAttachment::LOADOP_LOAD));
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

		device->CreateRenderPass(&desc, &renderpass_decals);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtDeferred, RenderPassAttachment::LOADOP_LOAD));
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

		device->CreateRenderPass(&desc, &renderpass_deferredcomposition);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&lightbuffer_diffuse, RenderPassAttachment::LOADOP_LOAD));
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

		device->CreateRenderPass(&desc, &renderpass_SSS[0]);

		desc.attachments[0].texture = &rtSSS[0];
		device->CreateRenderPass(&desc, &renderpass_SSS[1]);

		desc.attachments[0].texture = &rtSSS[1];
		device->CreateRenderPass(&desc, &renderpass_SSS[2]);
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtDeferred, RenderPassAttachment::LOADOP_LOAD));
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

		device->CreateRenderPass(&desc, &renderpass_transparent);
	}
}

void RenderPath3D_Deferred::Render() const
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

		// Deferred lights:
		{
			device->RenderPassBegin(&renderpass_lights, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->BindResource(PS, getAOEnabled() ? &rtAO : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_AO, cmd);
			device->BindResource(PS, getSSREnabled() || getRaytracedReflectionEnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);
			wiRenderer::DrawDeferredLights(wiRenderer::GetCamera(), depthBuffer_Copy, rtGBuffer[0], rtGBuffer[1], rtGBuffer[2], cmd);

			device->RenderPassEnd(cmd);
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

		RenderTransparents(renderpass_transparent, RENDERPASS_FORWARD, cmd);

		RenderPostprocessChain(rtDeferred, rtGBuffer[1], cmd);

	});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}

void RenderPath3D_Deferred::RenderSSS(CommandList cmd) const
{
	if (getSSSEnabled())
	{
		wiRenderer::Postprocess_SSS(
			rtLinearDepth,
			rtGBuffer[0],
			renderpass_SSS[0],
			renderpass_SSS[1],
			renderpass_SSS[2],
			cmd
		);
	}
}
void RenderPath3D_Deferred::RenderDecals(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->RenderPassBegin(&renderpass_decals, cmd);

	Viewport vp;
	vp.Width = (float)depthBuffer.GetDesc().Width;
	vp.Height = (float)depthBuffer.GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	wiRenderer::DrawDeferredDecals(wiRenderer::GetCamera(), depthBuffer_Copy, cmd);

	device->RenderPassEnd(cmd);
}
void RenderPath3D_Deferred::RenderDeferredComposition(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->RenderPassBegin(&renderpass_deferredcomposition, cmd);

	Viewport vp;
	vp.Width = (float)depthBuffer.GetDesc().Width;
	vp.Height = (float)depthBuffer.GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	wiRenderer::DeferredComposition(
		rtGBuffer[0],
		rtGBuffer[1],
		rtGBuffer[2],
		lightbuffer_diffuse,
		lightbuffer_specular,
		getAOEnabled() ? rtAO : *wiTextureHelper::getWhite(),
		depthBuffer_Copy,
		cmd
	);
	wiRenderer::DrawSky(cmd);

	RenderOutline(cmd);

	device->RenderPassEnd(cmd);
}
