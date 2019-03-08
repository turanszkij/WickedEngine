#include "RenderPath3D_TiledForward.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiProfiler.h"
#include "wiTextureHelper.h"

using namespace wiGraphics;

void RenderPath3D_TiledForward::RenderScene(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

	const GPUResource* dsv[] = { &depthBuffer };
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	// depth prepass
	{
		wiProfiler::BeginRange("Z-Prepass", wiProfiler::DOMAIN_GPU, threadID);

		device->BindRenderTargets(0, nullptr, &depthBuffer, threadID);
		device->ClearDepthStencil(&depthBuffer, CLEAR_DEPTH | CLEAR_STENCIL, 0, 0, threadID);

		ViewPort vp;
		vp.Width = (float)depthBuffer.GetDesc().Width;
		vp.Height = (float)depthBuffer.GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_DEPTHONLY, getHairParticlesEnabled(), true, getLayerMask());

		wiProfiler::EndRange(threadID);
	}

	if (getMSAASampleCount() > 1)
	{
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, threadID);
		wiRenderer::ResolveMSAADepthBuffer(&depthCopy, &depthBuffer, threadID);
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, RESOURCE_STATE_DEPTH_READ, threadID);
	}
	else
	{
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);
		device->CopyTexture2D(&depthCopy, &depthBuffer, threadID);
		device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);
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

	wiRenderer::ComputeTiledLightCulling(threadID);

	device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);

	// Opaque scene:
	{
		wiProfiler::BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

		const Texture2D* rts[] = {
			&rtMain[0],
			&rtMain[1],
		};
		device->BindRenderTargets(ARRAYSIZE(rts), rts, &depthBuffer, threadID);
		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[1], clear, threadID);

		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
		device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[2] : wiTextureHelper::getWhite(), TEXSLOT_RENDERABLECOMPONENT_SSAO, threadID);
		device->BindResource(PS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_SSR, threadID);
		wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, RENDERPASS_TILEDFORWARD, true, true);
		wiRenderer::DrawSky(threadID);

		device->BindRenderTargets(0, nullptr, nullptr, threadID);

		wiProfiler::EndRange(threadID); // Opaque Scene
	}

	if (getMSAASampleCount() > 1)
	{
		device->MSAAResolve(&rtMain_resolved[0], &rtMain[0], threadID);
		device->MSAAResolve(&rtMain_resolved[1], &rtMain[1], threadID);
		wiRenderer::BindGBufferTextures(&rtMain_resolved[0], &rtMain_resolved[1], nullptr, threadID);
	}
	else
	{
		wiRenderer::BindGBufferTextures(&rtMain[0], &rtMain[1], nullptr, threadID);
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
void RenderPath3D_TiledForward::RenderTransparentScene(const Texture2D& refractionRT, GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiProfiler::BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
	device->BindResource(PS, &refractionRT, TEXSLOT_RENDERABLECOMPONENT_REFRACTION, threadID);
	device->BindResource(PS, &rtWaterRipple, TEXSLOT_RENDERABLECOMPONENT_WATERRIPPLES, threadID);
	wiRenderer::DrawScene_Transparent(wiRenderer::GetCamera(), RENDERPASS_TILEDFORWARD, threadID, getHairParticlesEnabled(), true);

	wiProfiler::EndRange(threadID); // Transparent Scene
}

