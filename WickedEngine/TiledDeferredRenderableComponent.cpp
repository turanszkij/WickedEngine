#include "TiledDeferredRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;


TiledDeferredRenderableComponent::TiledDeferredRenderableComponent()
{
	DeferredRenderableComponent::setProperties();
}


TiledDeferredRenderableComponent::~TiledDeferredRenderableComponent()
{
}



void TiledDeferredRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	wiImageEffects fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	GPUResource* dsv[] = { rtGBuffer.depth->GetTexture() };
	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

	rtGBuffer.Activate(threadID, 0, 0, 0, 0);
	{
		wiRenderer::GetDevice()->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_DEFERRED, getHairParticlesEnabled(), true, getLayerMask());
	}


	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);

	rtLinearDepth.Activate(threadID); {
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(rtGBuffer.depth->GetTexture(), fx, threadID);
		fx.process.clear();
	}
	rtLinearDepth.Deactivate(threadID);
	dtDepthCopy.CopyFrom(*rtGBuffer.depth, threadID);

	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);


	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);

	wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), threadID);

	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}


	rtGBuffer.Set(threadID); {
		wiRenderer::DrawDecals(wiRenderer::getCamera(), threadID);
	}
	rtGBuffer.Deactivate(threadID);

	wiRenderer::UpdateGBuffer(rtGBuffer.GetTexture(0), rtGBuffer.GetTexture(1), rtGBuffer.GetTexture(2), rtGBuffer.GetTexture(3), nullptr, threadID);


	wiRenderer::GetDevice()->BindResource(CS, getSSREnabled() ? rtSSR.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_SSR, threadID);

	wiRenderer::ComputeTiledLightCulling(true, threadID);


	if (getSSAOEnabled()) {
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_RENDERABLECOMPONENT_SSAO, 1, threadID);
		wiRenderer::GetDevice()->EventBegin("SSAO", threadID);
		fx.stencilRef = STENCILREF_DEFAULT;
		fx.stencilComp = STENCILMODE_LESS;
		rtSSAO[0].Activate(threadID); {
			fx.process.setSSAO(true);
			fx.setMaskMap(wiTextureHelper::getInstance()->getRandom64x64());
			fx.quality = QUALITY_BILINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, threadID);
			fx.process.clear();
		}
		rtSSAO[1].Activate(threadID); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 0;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[0].GetTexture(), fx, threadID);
		}
		rtSSAO[2].Activate(threadID); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 1;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[1].GetTexture(), fx, threadID);
			fx.blur = 0;
		}
		fx.stencilRef = 0;
		fx.stencilComp = STENCILMODE_DISABLED;
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getSSSEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("SSS", threadID);
		fx.stencilRef = STENCILREF_SKIN;
		fx.stencilComp = STENCILMODE_LESS;
		fx.quality = QUALITY_BILINEAR;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		rtSSS[1].Activate(threadID, 0, 0, 0, 0);
		rtSSS[0].Activate(threadID, 0, 0, 0, 0);
		static int sssPassCount = 6;
		for (int i = 0; i < sssPassCount; ++i)
		{
			wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
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
				wiImage::Draw(static_cast<Texture2D*>(wiRenderer::textures[TEXTYPE_2D_TILEDDEFERRED_DIFFUSEUAV]), fx, threadID);
			}
			else
			{
				wiImage::Draw(rtSSS[(i + 1) % 2].GetTexture(), fx, threadID);
			}
		}
		fx.process.clear();
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		rtSSS[0].Activate(threadID, rtGBuffer.depth); {
			fx.setMaskMap(nullptr);
			fx.quality = QUALITY_NEAREST;
			fx.sampleFlag = SAMPLEMODE_CLAMP;
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.stencilRef = 0;
			fx.stencilComp = STENCILMODE_DISABLED;
			fx.presentFullScreen = true;
			fx.hdr = true;
			wiImage::Draw(static_cast<Texture2D*>(wiRenderer::textures[TEXTYPE_2D_TILEDDEFERRED_DIFFUSEUAV]), fx, threadID);
			fx.stencilRef = STENCILREF_SKIN;
			fx.stencilComp = STENCILMODE_LESS;
			wiImage::Draw(rtSSS[1].GetTexture(), fx, threadID);
		}

		fx.stencilRef = 0;
		fx.stencilComp = STENCILMODE_DISABLED;
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	rtDeferred.Activate(threadID, rtGBuffer.depth); {
		wiImage::DrawDeferred((getSSSEnabled() ? rtSSS[0].GetTexture(0) : static_cast<Texture2D*>(wiRenderer::textures[TEXTYPE_2D_TILEDDEFERRED_DIFFUSEUAV])), 
			static_cast<Texture2D*>(wiRenderer::textures[TEXTYPE_2D_TILEDDEFERRED_SPECULARUAV])
			, getSSAOEnabled() ? rtSSAO.back().GetTexture() : wiTextureHelper::getInstance()->getWhite()
			, threadID, STENCILREF_DEFAULT);
		wiRenderer::DrawSky(threadID);
	}

	if (getSSREnabled()) {
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_RENDERABLECOMPONENT_SSR, 1, threadID);
		wiRenderer::GetDevice()->EventBegin("SSR", threadID);
		rtSSR.Activate(threadID); {
			fx.process.clear();
			fx.presentFullScreen = false;
			fx.process.setSSR(true);
			fx.setMaskMap(nullptr);
			wiImage::Draw(rtDeferred.GetTexture(), fx, threadID);
			fx.process.clear();
		}
		wiRenderer::GetDevice()->EventEnd(threadID);
	}


	wiProfiler::GetInstance().EndRange(threadID); // Opaque Scene
}
void TiledDeferredRenderableComponent::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::GetDevice()->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, refractionRT.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_REFRACTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, rtWaterRipple.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_WATERRIPPLES, threadID);
	wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), SHADERTYPE_TILEDFORWARD, threadID, getHairParticlesEnabled(), true, getLayerMask());

	wiProfiler::GetInstance().EndRange(); // Transparent Scene
}
