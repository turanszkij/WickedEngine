#include "RenderPath3D_TiledDeferred.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphics;

void RenderPath3D_TiledDeferred::RenderScene(GRAPHICSTHREAD threadID)
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
		device->ClearRenderTarget(&rtGBuffer[1], clear, threadID);
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

	RenderLinearDepth(threadID);

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

	device->BindResource(CS, getSSAOEnabled() ? &rtSSAO[2] : wiTextureHelper::getWhite(), TEXSLOT_RENDERABLECOMPONENT_SSAO, threadID);
	device->BindResource(CS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_SSR, threadID);

	wiRenderer::ComputeTiledLightCulling(threadID, &lightbuffer_diffuse, &lightbuffer_specular);

	RenderSSAO(threadID);

	RenderSSS(threadID);

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

	RenderSSR(rtDeferred, threadID);

}
void RenderPath3D_TiledDeferred::RenderTransparentScene(const Texture2D& refractionRT, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiProfiler::BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
	device->BindResource(PS, &refractionRT, TEXSLOT_RENDERABLECOMPONENT_REFRACTION, threadID);
	device->BindResource(PS, &rtWaterRipple, TEXSLOT_RENDERABLECOMPONENT_WATERRIPPLES, threadID);
	wiRenderer::DrawScene_Transparent(wiRenderer::GetCamera(), RENDERPASS_TILEDFORWARD, threadID, getHairParticlesEnabled(), true, getLayerMask());

	wiProfiler::EndRange(); // Transparent Scene
}
