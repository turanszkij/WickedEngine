#include "ForwardRenderableComponent.h"
#include "WickedEngine.h"

ForwardRenderableComponent::ForwardRenderableComponent(){
}
ForwardRenderableComponent::~ForwardRenderableComponent(){
}
void ForwardRenderableComponent::Initialize()
{
	Renderable3DSceneComponent::Initialize();

	setSSREnabled(false);
	setSSAOEnabled(false);
	setShadowsEnabled(false);

}
void ForwardRenderableComponent::Load()
{
	Renderable3DSceneComponent::Load();

	rtMain.Initialize(screenW, screenH, 1, true);
}
void ForwardRenderableComponent::Start()
{
	Renderable3DSceneComponent::Start();
}
void ForwardRenderableComponent::Render(){
	RenderReflections();
	RenderShadows();
	RenderScene();
	RenderSecondaryScene(rtMain, rtMain);
	RenderLightShafts(rtMain);
	RenderComposition1(rtMain);
	RenderBloom();
	RenderComposition2();
}


void ForwardRenderableComponent::RenderScene(wiRenderer::DeviceContext context)
{
	rtMain.Activate(context, 0, 0, 0, 1);
	{
		wiRenderer::UpdatePerFrameCB(context);
		wiRenderer::UpdatePerRenderCB(context, 0);
		wiRenderer::UpdatePerEffectCB(context, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::UpdatePerViewCB(context, wiRenderer::getCamera()->View, wiRenderer::getCamera()->refView, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->Eye, XMFLOAT4(0, 0, 0, 0));
		wiRenderer::DrawWorld(wiRenderer::getCamera()->View, false, 0, context
			, false, wiRenderer::SHADED_FORWARD_SIMPLE
			, nullptr, true);
		wiRenderer::DrawSky(wiRenderer::getCamera()->Eye, context);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera()->View, context);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera()->View, context);
	}

	rtLinearDepth.Activate(context); {
		wiImageEffects fx;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::BatchBegin(context);
		wiImage::Draw(rtMain.depth->shaderResource, fx, context);
	}

}
