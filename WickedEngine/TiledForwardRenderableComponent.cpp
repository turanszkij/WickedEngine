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
	wiProfiler::GetInstance().BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	wiProfiler::GetInstance().BeginRange("Z-Prepass", wiProfiler::DOMAIN_GPU, threadID);
	rtMain.Activate(threadID, 0, 0, 0, 0, true); // depth prepass
	{
		if (getHairParticleAlphaCompositionEnabled())
		{
			wiRenderer::SetAlphaRef(0.25f, threadID);
		}
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_ALPHATESTONLY, nullptr, true, true);
	}
	wiProfiler::GetInstance().EndRange(threadID);

	dtDepthCopy.CopyFrom(*rtMain.depth, threadID);

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

	wiRenderer::ComputeTiledLightCulling(threadID);

	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);

	rtMain.Set(threadID);
	{
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_TILEDFORWARD, rtReflection.GetTexture(), true, true);
		wiRenderer::DrawSky(threadID);
	}

	wiProfiler::GetInstance().EndRange(); // Opaque Scene
}
void TiledForwardRenderableComponent::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Transparent Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), SHADERTYPE_TILEDFORWARD, refractionRT.GetTexture(), rtReflection.GetTexture()
		, rtWaterRipple.GetTexture(), threadID, getHairParticleAlphaCompositionEnabled(), true);

	wiProfiler::GetInstance().EndRange(); // Transparent Scene
}

