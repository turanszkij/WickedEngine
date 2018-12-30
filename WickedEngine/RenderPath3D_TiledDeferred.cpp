#include "RenderPath3D_TiledDeferred.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;


RenderPath3D_TiledDeferred::RenderPath3D_TiledDeferred()
{
	RenderPath3D_Deferred::setProperties();
}


RenderPath3D_TiledDeferred::~RenderPath3D_TiledDeferred()
{
}



void RenderPath3D_TiledDeferred::RenderScene(GRAPHICSTHREAD threadID)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	wiProfiler::BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

	wiImageParams fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	GPUResource* dsv[] = { rtGBuffer.depth->GetTexture() };
	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

	{
		// We can't just call rtGbuffer.Activate() because we need to inject light buffers here too so gets a bit more complicated:
		Texture2D* rts[] = {
			rtGBuffer.GetTexture(0),
			rtGBuffer.GetTexture(1),
			rtGBuffer.GetTexture(2),
			lightbuffer_diffuse,
			lightbuffer_specular,
		};
		device->BindRenderTargets(ARRAYSIZE(rts), rts, rtGBuffer.depth->GetTexture(), threadID);
		device->ClearDepthStencil(rtGBuffer.depth->GetTexture(), CLEAR_DEPTH | CLEAR_STENCIL, 0, 0, threadID);
		float clear[4] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, threadID);
		device->ClearRenderTarget(rts[1], clear, threadID);
		device->ClearRenderTarget(rts[2], clear, threadID);
		device->ClearRenderTarget(rts[3], clear, threadID);
		device->ClearRenderTarget(rts[4], clear, threadID);
		ViewPort vp;
		vp.Width = (float)rts[0]->GetDesc().Width;
		vp.Height = (float)rts[0]->GetDesc().Height;
		device->BindViewports(1, &vp, threadID);

		device->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
		wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), threadID, SHADERTYPE_DEFERRED, getHairParticlesEnabled(), true, getLayerMask());
	}


	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);

	rtLinearDepth.Activate(threadID); {
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth();
		wiImage::Draw(rtGBuffer.depth->GetTexture(), fx, threadID);
		fx.process.clear();
	}
	rtLinearDepth.Deactivate(threadID);
	dtDepthCopy.CopyFrom(*rtGBuffer.depth, threadID);

	device->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);


	device->UnbindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);

	wiRenderer::BindDepthTextures(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), threadID);

	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}


	rtGBuffer.Set(threadID); {
		wiRenderer::DrawDecals(wiRenderer::GetCamera(), threadID);
	}
	rtGBuffer.Deactivate(threadID);

	wiRenderer::BindGBufferTextures(rtGBuffer.GetTexture(0), rtGBuffer.GetTexture(1), rtGBuffer.GetTexture(2), threadID);


	device->BindResource(CS, getSSAOEnabled() ? rtSSAO.back().GetTexture() : wiTextureHelper::getWhite(), TEXSLOT_RENDERABLECOMPONENT_SSAO, threadID);
	device->BindResource(CS, getSSREnabled() ? rtSSR.GetTexture() : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_SSR, threadID);

	wiRenderer::ComputeTiledLightCulling(threadID, lightbuffer_diffuse, lightbuffer_specular);


	if (getSSAOEnabled()) {
		device->UnbindResources(TEXSLOT_RENDERABLECOMPONENT_SSAO, 1, threadID);
		device->EventBegin("SSAO", threadID);
		fx.stencilRef = STENCILREF_DEFAULT;
		fx.stencilComp = STENCILMODE_LESS;
		rtSSAO[0].Activate(threadID); {
			fx.process.setSSAO();
			fx.setMaskMap(wiTextureHelper::getRandom64x64());
			fx.quality = QUALITY_LINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, threadID);
			fx.process.clear();
		}
		rtSSAO[1].Activate(threadID); {
			fx.process.setBlur(XMFLOAT2(getSSAOBlur(), 0));
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[0].GetTexture(), fx, threadID);
		}
		rtSSAO[2].Activate(threadID); {
			fx.process.setBlur(XMFLOAT2(0, getSSAOBlur()));
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[1].GetTexture(), fx, threadID);
			fx.process.clear();
		}
		fx.stencilRef = 0;
		fx.stencilComp = STENCILMODE_DISABLED;
		device->EventEnd(threadID);
	}

	if (getSSSEnabled())
	{
		device->EventBegin("SSS", threadID);
		fx.stencilRef = STENCILREF_SKIN;
		fx.stencilComp = STENCILMODE_LESS;
		fx.quality = QUALITY_LINEAR;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		rtSSS[1].Activate(threadID, 0, 0, 0, 0);
		rtSSS[0].Activate(threadID, 0, 0, 0, 0);
		static int sssPassCount = 6;
		for (int i = 0; i < sssPassCount; ++i)
		{
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
			rtSSS[i % 2].Set(threadID, rtGBuffer.depth);
			XMFLOAT2 dir = XMFLOAT2(0, 0);
			static float stren = 0.018f;
			if (i % 2 == 0)
			{
				dir.x = stren*((float)wiRenderer::GetInternalResolution().y / (float)wiRenderer::GetInternalResolution().x);
			}
			else
			{
				dir.y = stren;
			}
			fx.process.setSSSS(dir);
			if (i == 0)
			{
				wiImage::Draw(static_cast<Texture2D*>(lightbuffer_diffuse), fx, threadID);
			}
			else
			{
				wiImage::Draw(rtSSS[(i + 1) % 2].GetTexture(), fx, threadID);
			}
		}
		fx.process.clear();
		device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		rtSSS[0].Activate(threadID, rtGBuffer.depth); {
			fx.setMaskMap(nullptr);
			fx.quality = QUALITY_NEAREST;
			fx.sampleFlag = SAMPLEMODE_CLAMP;
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.stencilRef = 0;
			fx.stencilComp = STENCILMODE_DISABLED;
			fx.enableFullScreen();
			fx.enableHDR();
			wiImage::Draw(static_cast<Texture2D*>(lightbuffer_diffuse), fx, threadID);
			fx.stencilRef = STENCILREF_SKIN;
			fx.stencilComp = STENCILMODE_LESS;
			wiImage::Draw(rtSSS[1].GetTexture(), fx, threadID);
		}

		fx.stencilRef = 0;
		fx.stencilComp = STENCILMODE_DISABLED;
		device->EventEnd(threadID);
	}

	rtDeferred.Activate(threadID, rtGBuffer.depth); {
		wiImage::DrawDeferred((getSSSEnabled() ? rtSSS[0].GetTexture(0) : lightbuffer_diffuse), 
			lightbuffer_specular
			, getSSAOEnabled() ? rtSSAO.back().GetTexture() : wiTextureHelper::getWhite()
			, threadID, STENCILREF_DEFAULT);
		wiRenderer::DrawSky(threadID);
	}

	if (getSSREnabled()) {
		device->UnbindResources(TEXSLOT_RENDERABLECOMPONENT_SSR, 1, threadID);
		device->EventBegin("SSR", threadID);
		rtSSR.Activate(threadID); {
			fx.process.clear();
			fx.disableFullScreen();
			fx.process.setSSR();
			fx.setMaskMap(nullptr);
			wiImage::Draw(rtDeferred.GetTexture(), fx, threadID);
			fx.process.clear();
		}
		device->EventEnd(threadID);
	}


	wiProfiler::EndRange(threadID); // Opaque Scene
}
void RenderPath3D_TiledDeferred::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiProfiler::BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::GetDevice()->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, refractionRT.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_REFRACTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, rtWaterRipple.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_WATERRIPPLES, threadID);
	wiRenderer::DrawScene_Transparent(wiRenderer::GetCamera(), SHADERTYPE_TILEDFORWARD, threadID, getHairParticlesEnabled(), true, getLayerMask());

	wiProfiler::EndRange(); // Transparent Scene
}
