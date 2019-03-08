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

		desc.Format = wiRenderer::RTFormat_gbuffer_1;
		device->CreateTexture2D(&desc, nullptr, &rtGBuffer[1]);

		desc.Format = wiRenderer::RTFormat_gbuffer_2;
		device->CreateTexture2D(&desc, nullptr, &rtGBuffer[2]);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtDeferred);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_deferred_lightbuffer;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &lightbuffer_diffuse);
		device->CreateTexture2D(&desc, nullptr, &lightbuffer_specular);
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtSSS[0]);
		device->CreateTexture2D(&desc, nullptr, &rtSSS[1]);
	}
}

void RenderPath3D_Deferred::Initialize()
{
	RenderPath3D::Initialize();
}
void RenderPath3D_Deferred::Load()
{
	RenderPath3D::Load();
}
void RenderPath3D_Deferred::Start()
{
	RenderPath3D::Start();
}
void RenderPath3D_Deferred::Render()
{
	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
	RenderShadows(GRAPHICSTHREAD_IMMEDIATE);
	RenderReflections(GRAPHICSTHREAD_IMMEDIATE);
	RenderScene(GRAPHICSTHREAD_IMMEDIATE);
	RenderSecondaryScene(rtGBuffer[0], GetFinalRT(), GRAPHICSTHREAD_IMMEDIATE);
	RenderComposition(GetFinalRT(), rtGBuffer[0], rtGBuffer[1], GRAPHICSTHREAD_IMMEDIATE);

	RenderPath2D::Render();
}


void RenderPath3D_Deferred::RenderScene(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

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

		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
		device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[2] : wiTextureHelper::getWhite(), TEXSLOT_RENDERABLECOMPONENT_SSAO, threadID);
		wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_DEFERRED, getHairParticlesEnabled(), true, getLayerMask());

		wiProfiler::EndRange(threadID); // Opaque Scene
	}

	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);
	device->CopyTexture2D(&depthCopy, &depthBuffer, threadID);
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);

	// Linear depth:
	{
		const Texture2D* rts[] = { &rtLinearDepth };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth();
		wiImage::Draw(&depthCopy, fx, threadID);
		fx.process.clear();

		device->BindRenderTargets(0, nullptr, nullptr, threadID);
	}

	device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);

	wiRenderer::BindDepthTextures(&depthCopy, &rtLinearDepth, threadID);

	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}


	// Draw decals:
	{
		const Texture2D* rts[] = { &rtGBuffer[0] };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		wiRenderer::DrawDecals(wiRenderer::GetCamera(), threadID);

		device->BindRenderTargets(0, nullptr, nullptr, threadID);
	}

	wiRenderer::BindGBufferTextures(&rtGBuffer[0], &rtGBuffer[1], &rtGBuffer[2], threadID);


	// Deferred lights:
	{
		Texture2D* rts[] = {
			&lightbuffer_diffuse,
			&lightbuffer_specular,
		};
		device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);
		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[2] : wiTextureHelper::getWhite(), TEXSLOT_RENDERABLECOMPONENT_SSAO, threadID);
		device->BindResource(PS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_SSR, threadID);
		wiRenderer::DrawLights(wiRenderer::GetCamera(), threadID);
	}


	if (getSSAOEnabled())
	{
		device->UnbindResources(TEXSLOT_RENDERABLECOMPONENT_SSAO, 1, threadID);
		device->EventBegin("SSAO", threadID);
		fx.stencilRef = STENCILREF_DEFAULT;
		fx.stencilComp = STENCILMODE_LESS;
		{
			const Texture2D* rts[] = { &rtSSAO[0] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.process.setSSAO(getSSAORange(), getSSAOSampleCount());
			fx.setMaskMap(wiTextureHelper::getRandom64x64());
			fx.quality = QUALITY_LINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, threadID);
			fx.process.clear();
		}
		{
			const Texture2D* rts[] = { &rtSSAO[1] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.process.setBlur(XMFLOAT2(getSSAOBlur(), 0));
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(&rtSSAO[0], fx, threadID);
		}
		{
			const Texture2D* rts[] = { &rtSSAO[2] };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.process.setBlur(XMFLOAT2(0, getSSAOBlur()));
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(&rtSSAO[1], fx, threadID);
			fx.process.clear();
		}
		fx.stencilRef = 0;
		fx.stencilComp = STENCILMODE_DISABLED;
		device->EventEnd(threadID);
	}

	if (getSSSEnabled())
	{
		device->EventBegin("SSS", threadID);

		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(&rtSSS[0], clear, threadID);
		device->ClearRenderTarget(&rtSSS[1], clear, threadID);

		ViewPort vp;
		vp.Width = (float)rtSSS[0].GetDesc().Width;
		vp.Height = (float)rtSSS[0].GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		fx.stencilRef = STENCILREF_SKIN;
		fx.stencilComp = STENCILMODE_LESS;
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
				wiImage::Draw(static_cast<Texture2D*>(&lightbuffer_diffuse), fx, threadID);
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
			fx.stencilRef = 0;
			fx.stencilComp = STENCILMODE_DISABLED;
			fx.enableFullScreen();
			fx.enableHDR();
			wiImage::Draw(&lightbuffer_diffuse, fx, threadID);
			fx.stencilRef = STENCILREF_SKIN;
			fx.stencilComp = STENCILMODE_LESS;
			wiImage::Draw(&rtSSS[1], fx, threadID);
		}

		fx.stencilRef = 0;
		fx.stencilComp = STENCILMODE_DISABLED;
		device->EventEnd(threadID);
	}

	// Deferred composition:
	{
		const Texture2D* rts[] = { &rtDeferred };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		wiImage::DrawDeferred((getSSSEnabled() ? &rtSSS[0] : &lightbuffer_diffuse),
			&lightbuffer_specular
			, getSSAOEnabled() ? &rtSSAO[2] : wiTextureHelper::getWhite()
			, threadID, STENCILREF_DEFAULT);
		wiRenderer::DrawSky(threadID);
	}

	if (getSSREnabled())
	{
		device->UnbindResources(TEXSLOT_RENDERABLECOMPONENT_SSR, 1, threadID);
		device->EventBegin("SSR", threadID);
		{
			const Texture2D* rts[] = { &rtSSR };
			device->BindRenderTargets(ARRAYSIZE(rts), rts, nullptr, threadID);

			ViewPort vp;
			vp.Width = (float)rts[0]->GetDesc().Width;
			vp.Height = (float)rts[0]->GetDesc().Height;
			device->BindViewports(1, &vp, threadID);

			fx.process.clear();
			fx.disableFullScreen();
			fx.process.setSSR();
			fx.setMaskMap(nullptr);
			wiImage::Draw(&rtDeferred, fx, threadID);
			fx.process.clear();
		}
		device->EventEnd(threadID);
	}

}

const Texture2D& RenderPath3D_Deferred::GetFinalRT()
{
	//if (getSSREnabled())
	//	return rtSSR;
	//else
		return rtDeferred;
}

