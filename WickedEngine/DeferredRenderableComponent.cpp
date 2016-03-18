#include "DeferredRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "TextureMapping.h"

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
void DeferredRenderableComponent::Render()
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

		wiRenderer::graphicsDevice->ExecuteDeferredContexts();
	}
	else
	{
		RenderFrameSetUp();
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


void DeferredRenderableComponent::RenderScene(GRAPHICSTHREAD threadID){
	static const int tessellationQuality = 0;
	wiImageEffects fx((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight());

	rtGBuffer.Activate(threadID); {

		wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), threadID);

		wiRenderer::DrawWorld(wiRenderer::getCamera(), false, tessellationQuality, threadID, false, false
			, SHADERTYPE_DEFERRED, rtReflection.shaderResource.front(), true, GRAPHICSTHREAD_SCENE);

		wiRenderer::DrawSky(threadID);

	}


	rtLinearDepth.Activate(threadID); {
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(rtGBuffer.depth->shaderResource, fx, threadID);
		fx.process.clear();
	}
	rtLinearDepth.Deactivate(threadID);
	dtDepthCopy.CopyFrom(*rtGBuffer.depth, threadID);


	wiRenderer::graphicsDevice->UnbindTextures(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);

	wiRenderer::UpdateDepthBuffer(dtDepthCopy.shaderResource, rtLinearDepth.shaderResource.front(), threadID);

	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}


	rtGBuffer.Set(threadID); {
		wiRenderer::DrawDecals(wiRenderer::getCamera(), threadID);
	}
	rtGBuffer.Deactivate(threadID);

	wiRenderer::UpdateGBuffer(rtGBuffer.shaderResource, threadID);

	rtLight.Activate(threadID, rtGBuffer.depth); {
		wiRenderer::DrawLights(wiRenderer::getCamera(), threadID);
	}



	if (getSSAOEnabled()){
		fx.stencilRef = STENCILREF_DEFAULT;
		fx.stencilComp = COMPARISON_LESS;
		rtSSAO[0].Activate(threadID); {
			fx.process.setSSAO(true);
			//fx.setDepthMap(rtLinearDepth.shaderResource.back());
			//fx.setNormalMap(rtGBuffer.shaderResource[1]);
			fx.setMaskMap(wiTextureHelper::getInstance()->getRandom64x64());
			fx.quality = QUALITY_BILINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, threadID);
			fx.process.clear();
		}
		rtSSAO[1].Activate(threadID); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 0;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[0].shaderResource.back(), fx, threadID);
		}
		rtSSAO[2].Activate(threadID); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 1;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[1].shaderResource.back(), fx, threadID);
			fx.blur = 0;
		}
		fx.stencilRef = 0;
		fx.stencilComp = 0;
	}


	rtDeferred.Activate(threadID); {
		wiImage::DrawDeferred(rtGBuffer.shaderResource[0]
			, rtLinearDepth.shaderResource.back(), rtLight.shaderResource.front(), rtGBuffer.shaderResource[1]
			, getSSAOEnabled() ? rtSSAO.back().shaderResource.back() : wiTextureHelper::getInstance()->getWhite()
			, threadID, 0);
		wiRenderer::DrawDebugBoneLines(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera(), threadID);
	}

	if (getSSSEnabled())
	{
		//wiRenderer::UnbindTextures(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);
		fx.stencilRef = STENCILREF_SKIN;
		fx.stencilComp = COMPARISON_LESS;
		fx.quality = QUALITY_BILINEAR;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		//fx.setDepthMap(rtLinearDepth.shaderResource.back());
		for (unsigned int i = 0; i<rtSSS.size() - 1; ++i){
			rtSSS[i].Activate(threadID, rtGBuffer.depth);
			XMFLOAT2 dir = XMFLOAT2(0, 0);
			static const float stren = 0.018f;
			if (i % 2)
				dir.x = stren*((float)wiRenderer::GetScreenHeight() / (float)wiRenderer::GetScreenWidth());
			else
				dir.y = stren;
			fx.process.setSSSS(dir);
			if (i == 0)
				wiImage::Draw(rtDeferred.shaderResource.back(), fx, threadID);
			else
				wiImage::Draw(rtSSS[i - 1].shaderResource.back(), fx, threadID);
		}
		fx.process.clear();
		rtSSS.back().Activate(threadID, rtGBuffer.depth); {
			fx.setMaskMap(nullptr);
			//fx.setNormalMap(nullptr);
			fx.quality = QUALITY_NEAREST;
			fx.sampleFlag = SAMPLEMODE_CLAMP;
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.stencilRef = 0;
			fx.stencilComp = 0;
			wiImage::Draw(rtDeferred.shaderResource.front(), fx, threadID);
			fx.stencilRef = STENCILREF_SKIN;
			fx.stencilComp = COMPARISON_LESS;
			wiImage::Draw(rtSSS[rtSSS.size() - 2].shaderResource.back(), fx, threadID);
		}

		fx.stencilRef = 0;
		fx.stencilComp = 0;
	}

	if (getSSREnabled()){
		rtSSR.Activate(threadID); {
			wiRenderer::graphicsDevice->GenerateMips(rtDeferred.shaderResource[0], threadID);
			fx.process.setSSR(true);
			//fx.setDepthMap(dtDepthCopy.shaderResource);
			//fx.setNormalMap(rtGBuffer.shaderResource[1]);
			//fx.setVelocityMap(rtGBuffer.shaderResource[2]);
			fx.setMaskMap(rtLinearDepth.shaderResource.front());
			if (getSSSEnabled())
				wiImage::Draw(rtSSS.back().shaderResource.front(), fx, threadID);
			else
				wiImage::Draw(rtDeferred.shaderResource.front(), fx, threadID);
			fx.process.clear();
		}
	}

	if (getMotionBlurEnabled()){ //MOTIONBLUR
		rtMotionBlur.Activate(threadID);
		fx.process.setMotionBlur(true);
		//fx.setVelocityMap(rtGBuffer.shaderResource.back());
		//fx.setDepthMap(rtLinearDepth.shaderResource.back());
		fx.blendFlag = BLENDMODE_OPAQUE;
		if (getSSREnabled()){
			wiImage::Draw(rtSSR.shaderResource.front(), fx, threadID);
		}
		else if (getSSSEnabled())
		{
			wiImage::Draw(rtSSS.back().shaderResource.front(), fx, threadID);
		}
		else{
			wiImage::Draw(rtDeferred.shaderResource.front(), fx, threadID);
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

	if (!wiRenderer::graphicsDevice->GetMultithreadingSupport())
	{
		if (value > 1)
			wiHelper::messageBox("Multithreaded rendering not supported by your hardware! Falling back to single threading!", "Caution");
		return;
	}

	switch (value){
	case 0:
	case 1:
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_IMMEDIATE);
		break;
	case 2:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			RenderSecondaryScene(rtGBuffer, GetFinalRT(), GRAPHICSTHREAD_SCENE);
			RenderLightShafts(rtGBuffer, GRAPHICSTHREAD_SCENE);
			RenderComposition1(GetFinalRT(), GRAPHICSTHREAD_SCENE);
			RenderBloom(GRAPHICSTHREAD_SCENE);
			RenderComposition2(GRAPHICSTHREAD_SCENE);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));

		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_SCENE);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_SCENE);
		wiRenderer::graphicsDevice->ExecuteDeferredContexts();
		break;
	case 3:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS); 
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_SCENE);
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderSecondaryScene(rtGBuffer, GetFinalRT(), GRAPHICSTHREAD_MISC1);
			RenderLightShafts(rtGBuffer, GRAPHICSTHREAD_MISC1);
			RenderComposition1(GetFinalRT(), GRAPHICSTHREAD_MISC1);
			RenderBloom(GRAPHICSTHREAD_MISC1);
			RenderComposition2(GRAPHICSTHREAD_MISC1);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_MISC1);
		}));

		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_SCENE);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_SCENE);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_MISC1);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_MISC1);
		wiRenderer::graphicsDevice->ExecuteDeferredContexts();
		break;
	case 4:
	default:
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderFrameSetUp(GRAPHICSTHREAD_REFLECTIONS);
			RenderReflections(GRAPHICSTHREAD_REFLECTIONS);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS); 
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderShadows(GRAPHICSTHREAD_SCENE);
			RenderScene(GRAPHICSTHREAD_SCENE);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_SCENE); 
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderSecondaryScene(rtGBuffer, GetFinalRT(), GRAPHICSTHREAD_MISC1);
			RenderLightShafts(rtGBuffer, GRAPHICSTHREAD_MISC1);
			RenderComposition1(GetFinalRT(), GRAPHICSTHREAD_MISC1);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_MISC1); 
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			RenderBloom(GRAPHICSTHREAD_MISC2);
			RenderComposition2(GRAPHICSTHREAD_MISC2);
			wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_MISC2);
		}));

		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_SCENE);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_SCENE);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_MISC1);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_MISC1);
		wiRenderer::RebindPersistentState(GRAPHICSTHREAD_MISC2);
		wiRenderer::graphicsDevice->FinishCommandList(GRAPHICSTHREAD_MISC2);
		wiRenderer::graphicsDevice->ExecuteDeferredContexts();
		break;
	};
}

