#include "ForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"

using namespace wiGraphicsTypes;

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

	rtMain.Initialize(wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight(), true);

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
void ForwardRenderableComponent::Render()
{
	wiRenderer::UpdatePerFrameData();

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

		wiRenderer::GetDevice()->ExecuteDeferredContexts();
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


void ForwardRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	rtMain.Activate(threadID, 0.5f, 0.5f, 0.5f, 1);
	{

		wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), threadID);

		wiRenderer::DrawWorld(wiRenderer::getCamera(), false, 0, threadID
			, false, false, SHADERTYPE_FORWARD_SIMPLE
			, nullptr, true, GRAPHICSTHREAD_SCENE);
		wiRenderer::DrawSky(threadID);
		wiRenderer::DrawDebugBoneLines(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawTranslators(wiRenderer::getCamera(), threadID);
	}

	rtLinearDepth.Activate(threadID); {
		wiImageEffects fx;
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		//wiImage::BatchBegin(threadID);
		wiImage::Draw(rtMain.depth->GetTexture(), fx, threadID);
	}
	rtLinearDepth.Deactivate(threadID);
	dtDepthCopy.CopyFrom(*rtMain.depth, threadID);

	wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), threadID);
}


void ForwardRenderableComponent::setPreferredThreadingCount(unsigned short value)
{
	Renderable3DComponent::setPreferredThreadingCount(value);

	if (!wiRenderer::GetDevice()->GetMultithreadingSupport())
	{
		if (value > 1)
			wiHelper::messageBox("Multithreaded rendering not supported by your hardware! Falling back to single threading!", "Caution");
		return;
	}

	switch (value) {
	case 0:
	case 1:
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
		break;
	case 2:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			RenderSecondaryScene(rtMain, rtMain, GRAPHICSTHREAD_SCENE);
			RenderLightShafts(rtMain, GRAPHICSTHREAD_SCENE);
			RenderComposition1(rtMain, GRAPHICSTHREAD_SCENE);
			RenderBloom(GRAPHICSTHREAD_SCENE);
			RenderComposition2(GRAPHICSTHREAD_SCENE);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));

		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_SCENE);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		wiRenderer::GetDevice()->ExecuteDeferredContexts();
		break;
	case 3:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), GRAPHICSTHREAD_MISC1);
			RenderSecondaryScene(rtMain, rtMain, GRAPHICSTHREAD_MISC1);
			RenderLightShafts(rtMain, GRAPHICSTHREAD_MISC1);
			RenderComposition1(rtMain, GRAPHICSTHREAD_MISC1);
			RenderBloom(GRAPHICSTHREAD_MISC1);
			RenderComposition2(GRAPHICSTHREAD_MISC1);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC1);
		}));

		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_SCENE);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_MISC1);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC1);
		wiRenderer::GetDevice()->ExecuteDeferredContexts();
		break;
	case 4:
	default:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), GRAPHICSTHREAD_MISC1);
			RenderSecondaryScene(rtMain, rtMain, GRAPHICSTHREAD_MISC1);
			RenderLightShafts(rtMain, GRAPHICSTHREAD_MISC1);
			RenderComposition1(rtMain, GRAPHICSTHREAD_MISC1);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC1);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), GRAPHICSTHREAD_MISC2);
			RenderBloom(GRAPHICSTHREAD_MISC2);
			RenderComposition2(GRAPHICSTHREAD_MISC2);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC2);
		}));

		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_SCENE);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_SCENE);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_MISC1);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC1);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_MISC2);
		wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC2);
		wiRenderer::GetDevice()->ExecuteDeferredContexts();
		break;
	};
}
