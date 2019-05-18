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

		desc.Format = wiRenderer::RTFormat_gbuffer_0;
		device->CreateTexture2D(&desc, nullptr, &rtGBuffer[0]);
		device->SetName(&rtGBuffer[0], "rtGBuffer[0]");

		desc.Format = wiRenderer::RTFormat_gbuffer_1;
		device->CreateTexture2D(&desc, nullptr, &rtGBuffer[1]);
		device->SetName(&rtGBuffer[1], "rtGBuffer[1]");

		desc.Format = wiRenderer::RTFormat_gbuffer_2;
		device->CreateTexture2D(&desc, nullptr, &rtGBuffer[2]);
		device->SetName(&rtGBuffer[2], "rtGBuffer[2]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtDeferred);
		device->SetName(&rtDeferred, "rtDeferred");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_deferred_lightbuffer;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &lightbuffer_diffuse);
		device->SetName(&lightbuffer_diffuse, "lightbuffer_diffuse");
		device->CreateTexture2D(&desc, nullptr, &lightbuffer_specular);
		device->SetName(&lightbuffer_specular, "lightbuffer_specular");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtSSS[0]);
		device->SetName(&rtSSS[0], "rtSSS[0]");
		device->CreateTexture2D(&desc, nullptr, &rtSSS[1]);
		device->SetName(&rtSSS[1], "rtSSS[1]");
	}
}

void RenderPath3D_Deferred::Render() const
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
			device->ClearRenderTarget(rts[1], clear, threadID);
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

		// Deferred lights:
		{
			const Texture2D* rts[] = {
				&lightbuffer_diffuse,
				&lightbuffer_specular,
			};
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);
			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, threadID);
			device->BindResource(PS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, threadID);
			wiRenderer::DrawLights(wiRenderer::GetCamera(), threadID);
		}

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

	RenderTransparents(rtDeferred, RENDERPASS_FORWARD, GRAPHICSTHREAD_IMMEDIATE);

	RenderParticles(true, GRAPHICSTHREAD_IMMEDIATE);

	TemporalAAResolve(rtDeferred, rtGBuffer[1], GRAPHICSTHREAD_IMMEDIATE);

	RenderBloom(rtDeferred, GRAPHICSTHREAD_IMMEDIATE);

	RenderPostprocessChain(rtDeferred, rtGBuffer[1], GRAPHICSTHREAD_IMMEDIATE);

	RenderPath2D::Render();
}

void RenderPath3D_Deferred::RenderSSS(GRAPHICSTHREAD threadID) const
{
	if (getSSSEnabled())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

		device->EventBegin("SSS", threadID);

		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(&rtSSS[0], clear, threadID);
		device->ClearRenderTarget(&rtSSS[1], clear, threadID);

		ViewPort vp;
		vp.Width = (float)rtSSS[0].GetDesc().Width;
		vp.Height = (float)rtSSS[0].GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		fx.stencilRef = STENCILREF_SKIN;
		fx.stencilComp = STENCILMODE_EQUAL;
		fx.quality = QUALITY_LINEAR;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		static int sssPassCount = 6;
		for (int i = 0; i < sssPassCount; ++i)
		{
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);

			const Texture2D* rts[] = { &rtSSS[i % 2] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);

			XMFLOAT2 dir = XMFLOAT2(0, 0);
			static float stren = 0.018f;
			if (i % 2 == 0)
			{
				dir.x = stren * ((float)wiRenderer::GetInternalResolution().y / (float)wiRenderer::GetInternalResolution().x);
			}
			else
			{
				dir.y = stren;
			}
			fx.process.setSSSS(dir);
			if (i == 0)
			{
				wiImage::Draw(&lightbuffer_diffuse, fx, threadID);
			}
			else
			{
				wiImage::Draw(&rtSSS[(i + 1) % 2], fx, threadID);
			}
		}
		fx.process.clear();
		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		{
			const Texture2D* rts[] = { &rtSSS[0] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);
			device->ClearRenderTarget(rts[0], clear, threadID);

			fx.setMaskMap(nullptr);
			fx.quality = QUALITY_NEAREST;
			fx.sampleFlag = SAMPLEMODE_CLAMP;
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.stencilRef = STENCILREF_SKIN;
			fx.stencilComp = STENCILMODE_NOT;
			fx.enableFullScreen();
			fx.enableHDR();
			wiImage::Draw(&lightbuffer_diffuse, fx, threadID);
			fx.stencilRef = STENCILREF_SKIN;
			fx.stencilComp = STENCILMODE_EQUAL;
			wiImage::Draw(&rtSSS[1], fx, threadID);
		}

		device->EventEnd(threadID);
	}
}
void RenderPath3D_Deferred::RenderDecals(GRAPHICSTHREAD threadID) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const Texture2D* rts[] = { &rtGBuffer[0] };
	device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

	ViewPort vp;
	vp.Width = (float)rts[0]->GetDesc().Width;
	vp.Height = (float)rts[0]->GetDesc().Height;
	device->BindViewports(1, &vp, threadID);

	wiRenderer::DrawDecals(wiRenderer::GetCamera(), threadID);

	device->BindRenderTargets(0, nullptr, nullptr, threadID);
}
void RenderPath3D_Deferred::RenderDeferredComposition(GRAPHICSTHREAD threadID) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const Texture2D* rts[] = { &rtDeferred };
	device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);

	ViewPort vp;
	vp.Width = (float)rts[0]->GetDesc().Width;
	vp.Height = (float)rts[0]->GetDesc().Height;
	device->BindViewports(1, &vp, threadID);

	wiImage::DrawDeferred((getSSSEnabled() ? &rtSSS[0] : &lightbuffer_diffuse),
		&lightbuffer_specular
		, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite()
		, threadID, STENCILREF_DEFAULT);
	wiRenderer::DrawSky(threadID);
}
