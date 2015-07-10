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

	setPreferredThreadingCount(4);

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

	if (getThreadingCount() > 1)
	{
		for (auto workerThread : workerThreads)
		{
			workerThread->wakeup();
		}

		for (auto workerThread : workerThreads)
		{
			workerThread->wait();
		}

		wiRenderer::ExecuteDeferredContexts();
	}
	else
	{
		RenderShadows();
		RenderReflections();
		RenderScene();
		RenderSecondaryScene(rtMain, rtMain);
		RenderLightShafts(rtMain);
		RenderComposition1(rtMain);
		RenderBloom();
		RenderComposition2();
	}

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


void ForwardRenderableComponent::setPreferredThreadingCount(unsigned short value)
{
	Renderable3DSceneComponent::setPreferredThreadingCount(value);

	if (!wiRenderer::getMultithreadingSupport())
	{
		return;
	}

	switch (value){
	case 0: break;
	case 2:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(wiRenderer::deferredContexts[0]);
			RenderReflections(wiRenderer::deferredContexts[0]);
			wiRenderer::FinishCommandList(0);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderScene(wiRenderer::deferredContexts[1]);
			RenderSecondaryScene(rtMain, rtMain, wiRenderer::deferredContexts[1]);
			RenderLightShafts(rtMain, wiRenderer::deferredContexts[1]);
			RenderComposition1(rtMain, wiRenderer::deferredContexts[1]);
			RenderBloom(wiRenderer::deferredContexts[1]);
			RenderComposition2(wiRenderer::deferredContexts[1]);
			wiRenderer::FinishCommandList(1);
		}));
		break;
	case 3:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(wiRenderer::deferredContexts[0]);
			wiRenderer::FinishCommandList(0);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderReflections(wiRenderer::deferredContexts[1]);
			wiRenderer::FinishCommandList(1);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderScene(wiRenderer::deferredContexts[2]);
			RenderSecondaryScene(rtMain, rtMain, wiRenderer::deferredContexts[2]);
			RenderLightShafts(rtMain, wiRenderer::deferredContexts[2]);
			RenderComposition1(rtMain, wiRenderer::deferredContexts[2]);
			RenderBloom(wiRenderer::deferredContexts[2]);
			RenderComposition2(wiRenderer::deferredContexts[2]);
			wiRenderer::FinishCommandList(2);
		}));
		break;
	case 4:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(wiRenderer::deferredContexts[0]);
			wiRenderer::FinishCommandList(0);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderReflections(wiRenderer::deferredContexts[1]);
			wiRenderer::FinishCommandList(1);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderScene(wiRenderer::deferredContexts[2]);
			wiRenderer::FinishCommandList(2);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderSecondaryScene(rtMain, rtMain, wiRenderer::deferredContexts[3]);
			RenderLightShafts(rtMain, wiRenderer::deferredContexts[3]);
			RenderComposition1(rtMain, wiRenderer::deferredContexts[3]);
			RenderBloom(wiRenderer::deferredContexts[3]);
			RenderComposition2(wiRenderer::deferredContexts[3]);
			wiRenderer::FinishCommandList(3);
		}));
		break;
	default:
		wiHelper::messageBox("You can assign a maximum of 4 rendering threads for graphics!\nFalling back to single threading!", "Caution");
		break;
	};
}
