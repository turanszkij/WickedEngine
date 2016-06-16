#include "DeferredRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"

using namespace wiGraphicsTypes;

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
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, true, FORMAT_R8G8B8A8_UNORM);
	rtGBuffer.Add(FORMAT_R16G16_FLOAT);
	rtGBuffer.Add(FORMAT_R8G8B8A8_UNORM);
	rtGBuffer.Add(FORMAT_R8G8B8A8_UNORM);

	rtDeferred.Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R16G16B16A16_FLOAT, 0);
	rtLight.Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R11G11B10_FLOAT); // diffuse
	rtLight.Add(FORMAT_R11G11B10_FLOAT); // specular

	rtSSS[0].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R11G11B10_FLOAT);
	rtSSS[1].Initialize(
		wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight()
		, false, FORMAT_R11G11B10_FLOAT);

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

		wiRenderer::GetDevice()->ExecuteDeferredContexts();
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
	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());

	rtGBuffer.Activate(threadID, 0.5f, 0.5f, 0.5f, 1); // special clearcolor! background should write 0.5 in velocity buffer!
	{

		wiRenderer::SetClipPlane(XMFLOAT4(0, 0, 0, 0), threadID);

		wiRenderer::DrawWorld(wiRenderer::getCamera(), false, tessellationQuality, threadID, false
			, SHADERTYPE_DEFERRED, rtReflection.GetTexture(), true, GRAPHICSTHREAD_SCENE);

	}


	rtLinearDepth.Activate(threadID); {
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(rtGBuffer.depth->GetTexture(), fx, threadID);
		fx.process.clear();
	}
	rtLinearDepth.Deactivate(threadID);
	dtDepthCopy.CopyFrom(*rtGBuffer.depth, threadID);


	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, TEXSLOT_ONDEMAND_COUNT, threadID);

	wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), threadID);

	if (getStereogramEnabled())
	{
		// We don't need the following for stereograms...
		return;
	}


	rtGBuffer.Set(threadID); {
		wiRenderer::DrawDecals(wiRenderer::getCamera(), threadID);
	}
	rtGBuffer.Deactivate(threadID);

	wiRenderer::UpdateGBuffer(rtGBuffer.GetTexture(0), rtGBuffer.GetTexture(1), rtGBuffer.GetTexture(2), rtGBuffer.GetTexture(3), nullptr, threadID);

	rtLight.Activate(threadID, rtGBuffer.depth); {
		wiRenderer::DrawLights(wiRenderer::getCamera(), threadID);
	}



	if (getSSAOEnabled()){
		fx.stencilRef = STENCILREF_DEFAULT;
		fx.stencilComp = COMPARISON_LESS;
		rtSSAO[0].Activate(threadID); {
			fx.process.setSSAO(true);
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
			wiImage::Draw(rtSSAO[0].GetTexture(), fx, threadID);
		}
		rtSSAO[2].Activate(threadID); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 1;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[1].GetTexture(), fx, threadID);
			fx.blur = 0;
		}
		fx.stencilRef = 0;
		fx.stencilComp = 0;
	}

	if (getSSSEnabled())
	{
		fx.stencilRef = STENCILREF_SKIN;
		fx.stencilComp = COMPARISON_LESS;
		fx.quality = QUALITY_BILINEAR;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		static int sssPassCount = 6;
		for (int i = 0; i < sssPassCount; ++i) 
		{
			wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
			rtSSS[i % 2].Activate(threadID, rtGBuffer.depth, 0, 0, 0, 0);
			XMFLOAT2 dir = XMFLOAT2(0, 0);
			static float stren = 0.018f;
			if (i % 2 == 0)
			{
				dir.x = stren*((float)wiRenderer::GetDevice()->GetScreenHeight() / (float)wiRenderer::GetDevice()->GetScreenWidth());
			}
			else
			{
				dir.y = stren;
			}
			fx.process.setSSSS(dir);
			if (i == 0)
			{
				wiImage::Draw(rtLight.GetTexture(0), fx, threadID);
			}
			else
			{
				wiImage::Draw(rtSSS[(i + 1) % 2].GetTexture(), fx, threadID);
			}
		}
		fx.process.clear();
		wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
		rtSSS[0].Activate(threadID, rtGBuffer.depth); {
			fx.setMaskMap(nullptr);
			fx.quality = QUALITY_NEAREST;
			fx.sampleFlag = SAMPLEMODE_CLAMP;
			fx.blendFlag = BLENDMODE_OPAQUE;
			fx.stencilRef = 0;
			fx.stencilComp = 0;
			wiImage::Draw(rtLight.GetTexture(0), fx, threadID);
			fx.stencilRef = STENCILREF_SKIN;
			fx.stencilComp = COMPARISON_LESS;
			wiImage::Draw(rtSSS[1].GetTexture(), fx, threadID);
		}

		fx.stencilRef = 0;
		fx.stencilComp = 0;
	}

	rtDeferred.Activate(threadID, rtGBuffer.depth); {
		wiImage::DrawDeferred((getSSSEnabled() ? rtSSS[0].GetTexture(0) : rtLight.GetTexture(0)), rtLight.GetTexture(1)
			, getSSAOEnabled() ? rtSSAO.back().GetTexture() : wiTextureHelper::getInstance()->getWhite()
			, threadID, STENCILREF_DEFAULT); 
		wiRenderer::DrawSky(threadID);
		wiRenderer::DrawDebugEnvProbes(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugBoneLines(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera(), threadID);
		wiRenderer::DrawTranslators(wiRenderer::getCamera(), threadID);
	}


	if (getSSREnabled()){
		rtSSR.Activate(threadID); {
			wiRenderer::GetDevice()->GenerateMips(rtDeferred.GetTexture(0), threadID);
			fx.process.setSSR(true);
			fx.setMaskMap(nullptr);
			wiImage::Draw(rtDeferred.GetTexture(), fx, threadID);
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
			wiImage::Draw(rtSSR.GetTexture(), fx, threadID);
		}
		else{
			wiImage::Draw(rtDeferred.GetTexture(), fx, threadID);
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
	else
		return rtDeferred;
}

void DeferredRenderableComponent::setPreferredThreadingCount(unsigned short value)
{
	Renderable3DComponent::setPreferredThreadingCount(value);

	if (!wiRenderer::GetDevice()->GetMultithreadingSupport())
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
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_REFLECTIONS);
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
			wiRenderer::UpdateGBuffer(rtGBuffer.GetTexture(0), rtGBuffer.GetTexture(1), rtGBuffer.GetTexture(2), nullptr, nullptr, GRAPHICSTHREAD_MISC1);
			RenderSecondaryScene(rtGBuffer, GetFinalRT(), GRAPHICSTHREAD_MISC1);
			RenderLightShafts(rtGBuffer, GRAPHICSTHREAD_MISC1);
			RenderComposition1(GetFinalRT(), GRAPHICSTHREAD_MISC1);
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
			wiRenderer::UpdateGBuffer(rtGBuffer.GetTexture(0), rtGBuffer.GetTexture(1), rtGBuffer.GetTexture(2), nullptr, nullptr, GRAPHICSTHREAD_MISC1);
			RenderSecondaryScene(rtGBuffer, GetFinalRT(), GRAPHICSTHREAD_MISC1);
			RenderLightShafts(rtGBuffer, GRAPHICSTHREAD_MISC1);
			RenderComposition1(GetFinalRT(), GRAPHICSTHREAD_MISC1);
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC1); 
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), GRAPHICSTHREAD_MISC2);
			wiRenderer::UpdateGBuffer(rtGBuffer.GetTexture(0), rtGBuffer.GetTexture(1), rtGBuffer.GetTexture(2), nullptr, nullptr, GRAPHICSTHREAD_MISC2);
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

