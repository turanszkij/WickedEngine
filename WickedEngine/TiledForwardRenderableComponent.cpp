#include "TiledForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;

TiledForwardRenderableComponent::TiledForwardRenderableComponent() 
{
	ForwardRenderableComponent::setProperties();
	setShadowsEnabled(true);
	setHairParticleAlphaCompositionEnabled(true);
	//setMSAASampleCount(8);
}
TiledForwardRenderableComponent::~TiledForwardRenderableComponent() 
{
}

void TiledForwardRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	GPUResource* dsv[] = { rtMain.depth->GetTexture() };
	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

	wiProfiler::GetInstance().BeginRange("Z-Prepass", wiProfiler::DOMAIN_GPU, threadID);
	rtMain.Activate(threadID, 0, 0, 0, 0, true); // depth prepass
	{
		if (getHairParticleAlphaCompositionEnabled())
		{
			wiRenderer::SetAlphaRef(0.25f, threadID);
		}
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_DEPTHONLY, nullptr, getHairParticlesEnabled(), true);
	}
	wiProfiler::GetInstance().EndRange(threadID);

	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);

	dtDepthCopy.CopyFrom(*rtMain.depth, threadID);

	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);

	rtLinearDepth.Activate(threadID); {
		wiImageEffects fx;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(dtDepthCopy.GetTextureResolvedMSAA(threadID), fx, threadID);
	}
	rtLinearDepth.Deactivate(threadID);

	wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTextureResolvedMSAA(threadID), rtLinearDepth.GetTexture(), threadID);

	wiRenderer::ComputeTiledLightCulling(false, threadID);

	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);

	wiProfiler::GetInstance().BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);
	rtMain.Set(threadID);
	{
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_TILEDFORWARD, rtReflection.GetTexture(), true, true);
		wiRenderer::DrawSky(threadID);
	}
	rtMain.Deactivate(threadID);
	wiRenderer::UpdateGBuffer(rtMain.GetTextureResolvedMSAA(threadID, 0), rtMain.GetTextureResolvedMSAA(threadID, 1), nullptr, nullptr, nullptr, threadID);

	wiProfiler::GetInstance().EndRange(threadID); // Opaque Scene
}
void TiledForwardRenderableComponent::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), SHADERTYPE_TILEDFORWARD, refractionRT.GetTexture(), rtReflection.GetTexture()
		, rtWaterRipple.GetTexture(), threadID, getHairParticlesEnabled() && getHairParticleAlphaCompositionEnabled(), true);

	wiProfiler::GetInstance().EndRange(threadID); // Transparent Scene
}

