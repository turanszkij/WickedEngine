#include "TiledForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"

using namespace wiGraphicsTypes;

TiledForwardRenderableComponent::TiledForwardRenderableComponent() 
{
	ForwardRenderableComponent::setProperties();
	setShadowsEnabled(true);
	setHairParticleAlphaCompositionEnabled(true);
}
TiledForwardRenderableComponent::~TiledForwardRenderableComponent() 
{
}

void TiledForwardRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	rtMain.Activate(threadID, 0, 0, 0, 0, true); // depth prepass
	{
		if (getHairParticleAlphaCompositionEnabled())
		{
			wiRenderer::SetAlphaRef(0.1f, threadID);
		}
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_ALPHATESTONLY, nullptr, true);
	}

	rtLinearDepth.Activate(threadID); {
		wiImageEffects fx;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(rtMain.depth->GetTexture(), fx, threadID);
	}
	rtLinearDepth.Deactivate(threadID);
	dtDepthCopy.CopyFrom(*rtMain.depth, threadID);

	wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), threadID);

	wiRenderer::ComputeTiledLightCulling(threadID);

	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);

	rtMain.Set(threadID);
	{
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_TILEDFORWARD, rtReflection.GetTexture(), true);
		wiRenderer::DrawSky(threadID);
	}
}
void TiledForwardRenderableComponent::RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID)
{
	wiRenderer::DrawWorldTransparent(wiRenderer::getCamera(), SHADERTYPE_TILEDFORWARD, refractionRT.GetTexture(), rtReflection.GetTexture()
		, rtWaterRipple.GetTexture(), threadID, getHairParticleAlphaCompositionEnabled());
}

