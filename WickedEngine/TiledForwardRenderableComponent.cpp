#include "TiledForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiProfiler.h"
#include "wiTextureHelper.h"

using namespace wiGraphicsTypes;

TiledForwardRenderableComponent::TiledForwardRenderableComponent() 
{
	ForwardRenderableComponent::setProperties();
}
TiledForwardRenderableComponent::~TiledForwardRenderableComponent() 
{
}

void TiledForwardRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	GPUResource* dsv[] = { rtMain.depth->GetTexture() };
	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

	wiImageEffects fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	wiProfiler::GetInstance().BeginRange("Z-Prepass", wiProfiler::DOMAIN_GPU, threadID);
	rtMain.Activate(threadID, 0, 0, 0, 0, true); // depth prepass
	{
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_DEPTHONLY, getHairParticlesEnabled(), true, getLayerMask());
	}
	wiProfiler::GetInstance().EndRange(threadID);

	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);

	dtDepthCopy.CopyFrom(*rtMain.depth, threadID);

	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);

	rtLinearDepth.Activate(threadID); {
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(dtDepthCopy.GetTextureResolvedMSAA(threadID), fx, threadID);
		fx.process.clear();
	}
	rtLinearDepth.Deactivate(threadID);

	wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTextureResolvedMSAA(threadID), rtLinearDepth.GetTexture(), threadID);

	wiRenderer::ComputeTiledLightCulling(false, threadID);

	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);

	wiProfiler::GetInstance().BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);
	rtMain.Set(threadID);
	{
		wiRenderer::GetDevice()->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
		wiRenderer::GetDevice()->BindResource(PS, getSSAOEnabled() ? rtSSAO.back().GetTexture() : wiTextureHelper::getInstance()->getWhite(), TEXSLOT_RENDERABLECOMPONENT_SSAO, threadID);
		wiRenderer::GetDevice()->BindResource(PS, getSSREnabled() ? rtSSR.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_SSR, threadID);
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_TILEDFORWARD, true, true);
		wiRenderer::DrawSky(threadID);
	}
	rtMain.Deactivate(threadID);
	wiRenderer::UpdateGBuffer(rtMain.GetTextureResolvedMSAA(threadID, 0), rtMain.GetTextureResolvedMSAA(threadID, 1), nullptr, nullptr, nullptr, threadID);



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

	if (getSSREnabled()) {
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_RENDERABLECOMPONENT_SSR, 1, threadID);
		wiRenderer::GetDevice()->EventBegin("SSR", threadID);
		rtSSR.Activate(threadID); {
			fx.process.clear();
			fx.presentFullScreen = false;
			fx.process.setSSR(true);
			fx.setMaskMap(nullptr);
			wiImage::Draw(rtMain.GetTexture(), fx, threadID);
			fx.process.clear();
		}
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	wiProfiler::GetInstance().EndRange(threadID); // Opaque Scene
}
void TiledForwardRenderableComponent::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::GetDevice()->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, refractionRT.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_REFRACTION, threadID);
	wiRenderer::GetDevice()->BindResource(PS, rtWaterRipple.GetTexture(), TEXSLOT_RENDERABLECOMPONENT_WATERRIPPLES, threadID);
	wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), SHADERTYPE_TILEDFORWARD, threadID, getHairParticlesEnabled(), true);

	wiProfiler::GetInstance().EndRange(threadID); // Transparent Scene
}

