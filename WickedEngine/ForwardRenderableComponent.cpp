#include "ForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiStencilRef.h"
#include "wiHelper.h"

ForwardRenderableComponent::ForwardRenderableComponent(){
	Renderable3DComponent::setProperties();

	setSSREnabled(false);
	setSSAOEnabled(false);
	setShadowsEnabled(false);

	setPreferredThreadingCount(0);
}
ForwardRenderableComponent::~ForwardRenderableComponent(){
}
void ForwardRenderableComponent::Initialize()
{
	Renderable3DComponent::Initialize();

	rtMain.Initialize(wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight(), 1, true);

	Renderable2DComponent::Initialize();
}
void ForwardRenderableComponent::Load()
{
	Renderable3DComponent::Load();

}
void ForwardRenderableComponent::Start()
{
	Renderable3DComponent::Start();
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
		RenderFrameSetUp();
		RenderShadows();
		RenderReflections();
		RenderScene();
		RenderSecondaryScene(rtMain, rtMain);
		RenderLightShafts(rtMain);
		RenderComposition1(rtMain);
		RenderBloom();
		RenderComposition2();
	}

	Renderable2DComponent::Render();
}


void ForwardRenderableComponent::RenderScene(DeviceContext context)
{
	rtMain.Activate(context, 0, 0, 0, 1);
	{
		wiRenderer::UpdatePerFrameCB(context);
		wiRenderer::UpdatePerRenderCB(context, 0);
		wiRenderer::UpdatePerEffectCB(context, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::UpdatePerViewCB(context, wiRenderer::getCamera(), wiRenderer::getRefCamera());
		wiRenderer::DrawWorld(wiRenderer::getCamera(), false, 0, context
			, false, SHADERTYPE_FORWARD_SIMPLE
			, nullptr, true, GRAPHICSTHREAD_SCENE);
		wiRenderer::DrawSky(context);
		wiRenderer::DrawDebugBoneLines(wiRenderer::getCamera(), context);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera(), context);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera(), context);
	}

	rtLinearDepth.Activate(context); {
		wiImageEffects fx;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		//wiImage::BatchBegin(context);
		wiImage::Draw(rtMain.depth->shaderResource, fx, context);
	}

}


void ForwardRenderableComponent::setPreferredThreadingCount(unsigned short value)
{
	Renderable3DComponent::setPreferredThreadingCount(value);

	if (!wiRenderer::getMultithreadingSupport())
	{
		if (value > 1)
			wiHelper::messageBox("Multithreaded rendering not supported by your hardware! Falling back to single threading!", "Caution");
		return;
	}

	switch (value) {
	case 0:
	case 1:
		break;
	case 2:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderReflections(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderScene(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderSecondaryScene(rtMain, rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderLightShafts(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderComposition1(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderBloom(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderComposition2(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		break;
	case 3:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderReflections(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
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
	case 4:
	default:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderReflections(wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderScene(wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderSecondaryScene(rtMain, rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			RenderLightShafts(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			RenderComposition1(rtMain, wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_MISC1);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderBloom(wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC2));
			RenderComposition2(wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC2));
			wiRenderer::FinishCommandList(GRAPHICSTHREAD_MISC2);
		}));
		break;
	};
}
