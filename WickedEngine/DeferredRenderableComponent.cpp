#include "DeferredRenderableComponent.h"
#include "WickedEngine.h"

DeferredRenderableComponent::DeferredRenderableComponent(){
}
DeferredRenderableComponent::~DeferredRenderableComponent(){
}
void DeferredRenderableComponent::Initialize()
{
	Renderable3DSceneComponent::Initialize();

	setSSREnabled(true);
	setSSAOEnabled(true);

	setPreferredThreadingCount(4);
}
void DeferredRenderableComponent::Load()
{
	Renderable3DSceneComponent::Load();

	rtGBuffer.Initialize(
		screenW, screenH
		, 3, true, 1, 0
		, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtDeferred.Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);
}
void DeferredRenderableComponent::Start()
{
	Renderable3DSceneComponent::Start();
}
void DeferredRenderableComponent::Render(){

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
		RenderSecondaryScene(rtGBuffer, rtDeferred);
		RenderLightShafts(rtGBuffer);
		RenderComposition1(rtDeferred);
		RenderBloom();
		RenderComposition2();
	}

}


void DeferredRenderableComponent::RenderScene(wiRenderer::DeviceContext context){
	static const int tessellationQuality = 0;

	wiRenderer::UpdatePerFrameCB(context);
	wiImageEffects fx(screenW, screenH);

	rtGBuffer.Activate(context); {
		wiRenderer::UpdatePerRenderCB(context, tessellationQuality);
		wiRenderer::UpdatePerViewCB(context, wiRenderer::getCamera()->View, wiRenderer::getCamera()->refView, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->Eye);


		wiRenderer::UpdatePerEffectCB(context, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::DrawWorld(wiRenderer::getCamera()->View, wiRenderer::DX11, tessellationQuality, context, false
			, wiRenderer::SHADED_DEFERRED, rtReflection.shaderResource.front(), true, 2);


	}


	rtLinearDepth.Activate(context); {
		wiImage::BatchBegin(context);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(rtGBuffer.depth->shaderResource, fx, context);
		fx.process.clear();
	}
	dtDepthCopy.CopyFrom(*rtGBuffer.depth, context);

	rtGBuffer.Set(context); {
		wiRenderer::DrawDecals(wiRenderer::getCamera()->View, context, dtDepthCopy.shaderResource);
	}

	rtLight.Activate(context, rtGBuffer.depth); {
		wiRenderer::DrawLights(wiRenderer::getCamera()->View, context,
			dtDepthCopy.shaderResource, rtGBuffer.shaderResource[1], rtGBuffer.shaderResource[2]);
	}



	if (getSSAOEnabled()){
		wiImage::BatchBegin(context, STENCILREF_DEFAULT);
		rtSSAO[0].Activate(context); {
			fx.process.setSSAO(true);
			fx.setDepthMap(rtLinearDepth.shaderResource.back());
			fx.setNormalMap(rtGBuffer.shaderResource[1]);
			fx.setMaskMap(wiTextureHelper::getInstance()->getRandom64x64());
			//fx.sampleFlag=SAMPLEMODE_CLAMP;
			fx.quality = QUALITY_BILINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, context);
			//fx.sampleFlag=SAMPLEMODE_CLAMP;
			fx.process.clear();
		}
		rtSSAO[1].Activate(context); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 0;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[0].shaderResource.back(), fx, context);
		}
		rtSSAO[2].Activate(context); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 1;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[1].shaderResource.back(), fx, context);
			fx.blur = 0;
		}
	}


	rtDeferred.Activate(context, rtGBuffer.depth); {
		wiImage::DrawDeferred(rtGBuffer.shaderResource[0]
			, rtLinearDepth.shaderResource.back(), rtLight.shaderResource.front(), rtGBuffer.shaderResource[1]
			, getSSAOEnabled() ? rtSSAO.back().shaderResource.back() : wiTextureHelper::getInstance()->getWhite()
			, context, STENCILREF_DEFAULT);
		wiRenderer::DrawSky(wiRenderer::getCamera()->Eye, context);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera()->View, context);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera()->View, context);
	}


	if (getSSREnabled()){
		rtSSR.Activate(context); {
			wiImage::BatchBegin(context);
			context->GenerateMips(rtDeferred.shaderResource[0]);
			fx.process.setSSR(true);
			fx.setDepthMap(dtDepthCopy.shaderResource);
			fx.setNormalMap(rtGBuffer.shaderResource[1]);
			fx.setVelocityMap(rtGBuffer.shaderResource[2]);
			fx.setMaskMap(rtLinearDepth.shaderResource.front());
			wiImage::Draw(rtDeferred.shaderResource.front(), fx, context);
			fx.process.clear();
		}
	}
}


void DeferredRenderableComponent::setPreferredThreadingCount(unsigned short value)
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
			RenderSecondaryScene(rtGBuffer, rtDeferred, wiRenderer::deferredContexts[1]);
			RenderLightShafts(rtGBuffer, wiRenderer::deferredContexts[1]);
			RenderComposition1(rtDeferred, wiRenderer::deferredContexts[1]);
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
			RenderSecondaryScene(rtGBuffer, rtDeferred, wiRenderer::deferredContexts[2]);
			RenderLightShafts(rtGBuffer, wiRenderer::deferredContexts[2]);
			RenderComposition1(rtDeferred, wiRenderer::deferredContexts[2]);
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
			RenderSecondaryScene(rtGBuffer, rtDeferred, wiRenderer::deferredContexts[3]);
			RenderLightShafts(rtGBuffer, wiRenderer::deferredContexts[3]);
			RenderComposition1(rtDeferred, wiRenderer::deferredContexts[3]);
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

