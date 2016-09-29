#include "TiledForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"

using namespace wiGraphicsTypes;

TiledForwardRenderableComponent::TiledForwardRenderableComponent() {
	ForwardRenderableComponent::setProperties();
}
TiledForwardRenderableComponent::~TiledForwardRenderableComponent() {
}

void TiledForwardRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	rtMain.Activate(threadID, 0, 0, 0, 0);
	{
		wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), threadID);

		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID
			, false, SHADERTYPE_ALPHATESTONLY
			, nullptr, false);
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
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID
			, false, SHADERTYPE_TILEDFORWARD
			, nullptr, true);
		wiRenderer::DrawSky(threadID);
	}
}


