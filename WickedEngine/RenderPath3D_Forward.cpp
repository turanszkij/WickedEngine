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

	RenderLinearDepth(threadID);

	wiRenderer::BindDepthTextures(&depthCopy, &rtLinearDepth, threadID);

	RenderSSAO(threadID);

	RenderSSR(rtMain_resolved[0], threadID);
}

