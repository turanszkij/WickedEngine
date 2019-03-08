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

		desc.Format = wiRenderer::RTFormat_gbuffer_1;
		device->CreateTexture2D(&desc, nullptr, &rtMain[1]);

		if (getMSAASampleCount() > 1)
		{
			desc.SampleDesc.Count = 1;

			desc.Format = wiRenderer::RTFormat_hdr;
			device->CreateTexture2D(&desc, nullptr, &rtMain_resolved[0]);

			desc.Format = wiRenderer::RTFormat_gbuffer_1;
			device->CreateTexture2D(&desc, nullptr, &rtMain_resolved[1]);
		}
	}
}

void RenderPath3D_Forward::Initialize()
{
	RenderPath3D::Initialize();
}
void RenderPath3D_Forward::Load()
{
	RenderPath3D::Load();

}
void RenderPath3D_Forward::Start()
{
	RenderPath3D::Start();
}
void RenderPath3D_Forward::Render()
{
	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
	RenderShadows(GRAPHICSTHREAD_IMMEDIATE);
	RenderReflections(GRAPHICSTHREAD_IMMEDIATE);
	RenderScene(GRAPHICSTHREAD_IMMEDIATE);
	RenderSecondaryScene(rtMain[0], rtMain[0], GRAPHICSTHREAD_IMMEDIATE);
	RenderComposition(rtMain[0], rtMain_resolved[0], rtMain_resolved[1], GRAPHICSTHREAD_IMMEDIATE);

	RenderPath2D::Render();
}


void RenderPath3D_Forward::RenderScene(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

	const GPUResource* dsv[] = { &depthBuffer };
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	// Opaque Scene:
	{
		wiProfiler::BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

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

		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
		device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[2] : wiTextureHelper::getWhite(), TEXSLOT_RENDERABLECOMPONENT_SSAO, threadID);
		device->BindResource(PS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_SSR, threadID);
		
		wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_FORWARD, getHairParticlesEnabled(), true, getLayerMask());
		wiRenderer::DrawSky(threadID);

		device->BindRenderTargets(0, nullptr, nullptr, threadID);

		wiProfiler::EndRange(threadID); // Opaque Scene
	}

	if (getMSAASampleCount() > 1)
	{
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, threadID);
		wiRenderer::ResolveMSAADepthBuffer(&depthCopy, &depthBuffer, threadID);
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_READ, threadID);
		
		device->MSAAResolve(&rtMain_resolved[0], &rtMain[0], threadID);
		device->MSAAResolve(&rtMain_resolved[1], &rtMain[1], threadID);
		wiRenderer::BindGBufferTextures(&rtMain_resolved[0], &rtMain_resolved[1], nullptr, threadID);
	}
	else
	{
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);
		device->CopyTexture2D(&depthCopy, &depthBuffer, threadID);
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);

		wiRenderer::BindGBufferTextures(&rtMain[0], &rtMain[1], nullptr, threadID);
	}

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

	wiRenderer::BindDepthTextures(&depthCopy, &rtLinearDepth, threadID);


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
			wiImage::Draw(&rtMain[0], fx, threadID);
			fx.process.clear();
		}
		device->EventEnd(threadID);
	}
}

