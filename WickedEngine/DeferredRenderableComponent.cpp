#include "DeferredRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiStencilRef.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"

DeferredRenderableComponent::DeferredRenderableComponent(){
	Renderable3DComponent::setProperties();

	setSSREnabled(true);
	setSSAOEnabled(true);

	setPreferredThreadingCount(0);
}
DeferredRenderableComponent::~DeferredRenderableComponent(){
}
void DeferredRenderableComponent::Initialize()
{
	Renderable3DComponent::Initialize();

	rtGBuffer.Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 3, true, 1, 0
		, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtDeferred.Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);
	rtLight.Initialize(
		wiRenderer::GetScreenWidth(), wiRenderer::GetScreenHeight()
		, 1, false, 1, 0
		, DXGI_FORMAT_R11G11B10_FLOAT
		);

	Renderable2DComponent::Initialize();
}
void DeferredRenderableComponent::Load()
{
	Renderable3DComponent::Load();
}
void DeferredRenderableComponent::Start()
{
	Renderable3DComponent::Start();
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
		RenderSecondaryScene(rtGBuffer, GetFinalRT());
		RenderLightShafts(rtGBuffer);
		RenderComposition1(GetFinalRT());
		RenderBloom();
		RenderComposition2();
	}

	Renderable2DComponent::Render();
}


void DeferredRenderableComponent::RenderScene(wiRenderer::DeviceContext context){
	static const int tessellationQuality = 0;

	wiRenderer::UpdatePerFrameCB(context);
	wiImageEffects fx((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight());

	rtGBuffer.Activate(context); {
		wiRenderer::UpdatePerRenderCB(context, tessellationQuality);
		wiRenderer::UpdatePerViewCB(context, wiRenderer::getCamera(), wiRenderer::getRefCamera());
		wiRenderer::UpdatePerEffectCB(context, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));

		wiRenderer::DrawWorld(wiRenderer::getCamera(), wiRenderer::DX11, tessellationQuality, context, false
			, wiRenderer::SHADED_DEFERRED, rtReflection.shaderResource.front(), true, GRAPHICSTHREAD_SCENE);

		wiRenderer::DrawSky(context);

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
		wiRenderer::DrawDecals(wiRenderer::getCamera(), context, dtDepthCopy.shaderResource);
	}

	rtLight.Activate(context, rtGBuffer.depth); {
		wiRenderer::DrawLights(wiRenderer::getCamera(), context,
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


	rtDeferred.Activate(context); {
		wiImage::DrawDeferred(rtGBuffer.shaderResource[0]
			, rtLinearDepth.shaderResource.back(), rtLight.shaderResource.front(), rtGBuffer.shaderResource[1]
			, getSSAOEnabled() ? rtSSAO.back().shaderResource.back() : wiTextureHelper::getInstance()->getWhite()
			, context, 0);
		wiRenderer::DrawDebugBoneLines(wiRenderer::getCamera(), context);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera(), context);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera(), context);
	}

	if (getSSSEnabled())
	{
		wiImage::BatchBegin(context, STENCILREF_SKIN);
		fx.quality = QUALITY_BILINEAR;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.setDepthMap(rtLinearDepth.shaderResource.back());
		for (unsigned int i = 0; i<rtSSS.size() - 1; ++i){
			rtSSS[i].Activate(context, rtGBuffer.depth);
			XMFLOAT2 dir = XMFLOAT2(0, 0);
			static const float stren = 0.018f;
			if (i % 2)
				dir.x = stren*((float)wiRenderer::GetScreenHeight() / (float)wiRenderer::GetScreenWidth());
			else
				dir.y = stren;
			fx.process.setSSSS(dir);
			if (i == 0)
				wiImage::Draw(rtDeferred.shaderResource.back(), fx, context);
			else
				wiImage::Draw(rtSSS[i - 1].shaderResource.back(), fx, context);
		}
		fx.process.clear();
		rtSSS.back().Activate(context, rtGBuffer.depth); {
			wiImage::BatchBegin(context);
			fx.quality = QUALITY_NEAREST;
			fx.sampleFlag = SAMPLEMODE_CLAMP;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtDeferred.shaderResource.front(), fx, context);
			wiImage::BatchBegin(context, STENCILREF_SKIN);
			wiImage::Draw(rtSSS[rtSSS.size() - 2].shaderResource.back(), fx, context);
		}
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
			if (getSSSEnabled())
				wiImage::Draw(rtSSS.back().shaderResource.front(), fx, context);
			else
				wiImage::Draw(rtDeferred.shaderResource.front(), fx, context);
			fx.process.clear();
		}
	}

	if (getMotionBlurEnabled()){ //MOTIONBLUR
		wiImage::BatchBegin(context);
		rtMotionBlur.Activate(context);
		fx.process.setMotionBlur(true);
		fx.setVelocityMap(rtGBuffer.shaderResource.back());
		fx.setDepthMap(rtLinearDepth.shaderResource.back());
		fx.blendFlag = BLENDMODE_OPAQUE;
		if (getSSREnabled()){
			wiImage::Draw(rtSSR.shaderResource.front(), fx, context);
		}
		else if (getSSSEnabled())
		{
			wiImage::Draw(rtSSS.back().shaderResource.front(), fx, context);
		}
		else{
			wiImage::Draw(rtDeferred.shaderResource.front(), fx, context);
		}
		fx.process.clear();
	}
}

wiRenderTarget& DeferredRenderableComponent::GetFinalRT()
{
	if (getMotionBlurEnabled())
		return rtMotionBlur;
	else if (getSSREnabled())
		return rtSSR;
	else if (getSSSEnabled())
		return rtSSS.back();
	else
		return rtDeferred;
}

void DeferredRenderableComponent::setPreferredThreadingCount(unsigned short value)
{
	Renderable3DComponent::setPreferredThreadingCount(value);

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
			RenderSecondaryScene(rtGBuffer, GetFinalRT(), wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderLightShafts(rtGBuffer, wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
			RenderComposition1(GetFinalRT(), wiRenderer::getDeferredContext(GRAPHICSTHREAD_REFLECTIONS));
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
			RenderSecondaryScene(rtGBuffer, GetFinalRT(), wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderLightShafts(rtGBuffer, wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
			RenderComposition1(GetFinalRT(), wiRenderer::getDeferredContext(GRAPHICSTHREAD_SCENE));
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
			RenderSecondaryScene(rtGBuffer, GetFinalRT(), wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			RenderLightShafts(rtGBuffer, wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
			RenderComposition1(GetFinalRT(), wiRenderer::getDeferredContext(GRAPHICSTHREAD_MISC1));
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

