#include "ForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiCamera.h"
#include "wiStencilRef.h"
#include "wiHelper.h"

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

	setPreferredThreadingCount(0);
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
			, nullptr, true, GRAPHICSTHREAD_SCENE);
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
			RenderShadows(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SHADOWS));
			RenderReflections(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SHADOWS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_SHADOWS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderScene(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderSecondaryScene(rtMain, rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderLightShafts(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderComposition1(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderBloom(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderComposition2(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		break;
	case 3:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SHADOWS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_SHADOWS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderReflections(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderScene(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderSecondaryScene(rtMain, rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderLightShafts(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderComposition1(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderBloom(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderComposition2(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		break;
	case 4:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SHADOWS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_SHADOWS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderReflections(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderScene(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderSecondaryScene(rtMain, rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			RenderLightShafts(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			RenderComposition1(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			RenderBloom(wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			RenderComposition2(wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_MISC1);
		}));
		break;
	default:
		wiHelper::messageBox("You can assign a maximum of 4 rendering threads for graphics!\nFalling back to single threading!", "Caution");
		break;
	};
}
