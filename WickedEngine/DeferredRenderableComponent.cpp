#include "DeferredRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;

DeferredRenderableComponent::DeferredRenderableComponent()
{
	Renderable3DComponent::setProperties();

	setSSREnabled(true);
	setSSAOEnabled(true);
	setHairParticleAlphaCompositionEnabled(false);

	setPreferredThreadingCount(0);
}
DeferredRenderableComponent::~DeferredRenderableComponent()
{
}


static wiRenderTarget rtGBuffer, rtDeferred, rtLight, rtSSS[2];
void DeferredRenderableComponent::ResizeBuffers()
{
	Renderable3DComponent::ResizeBuffers();

	FORMAT defaultTextureFormat = GraphicsDevice::GetBackBufferFormat();

	// Protect against multiple buffer resizes when there is no change!
	static UINT lastBufferResWidth = 0, lastBufferResHeight = 0, lastBufferMSAA = 0;
	static FORMAT lastBufferFormat = FORMAT_UNKNOWN;
	if (lastBufferResWidth == wiRenderer::GetInternalResolution().x &&
		lastBufferResHeight == wiRenderer::GetInternalResolution().y &&
		lastBufferMSAA == getMSAASampleCount() &&
		lastBufferFormat == defaultTextureFormat)
	{
		return;
	}
	else
	{
		lastBufferResWidth = wiRenderer::GetInternalResolution().x;
		lastBufferResHeight = wiRenderer::GetInternalResolution().y;
		lastBufferMSAA = getMSAASampleCount();
		lastBufferFormat = defaultTextureFormat;
	}

	rtGBuffer.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, true, FORMAT_R8G8B8A8_UNORM);
	rtGBuffer.Add(FORMAT_R16G16B16A16_FLOAT);
	rtGBuffer.Add(FORMAT_R8G8B8A8_UNORM);
	rtGBuffer.Add(FORMAT_R8G8B8A8_UNORM);

	// NOTE: Light buffer precision seems OK when using FORMAT_R11G11B10_FLOAT format
	// But the environmental light now also writes the AO to ALPHA so it has been changed to FORMAT_R16G16B16A16_FLOAT

	rtDeferred.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtLight.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, FORMAT_R16G16B16A16_FLOAT); // diffuse
	rtLight.Add(FORMAT_R16G16B16A16_FLOAT); // specular

	rtSSS[0].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, FORMAT_R16G16B16A16_FLOAT);
	rtSSS[1].Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, FORMAT_R16G16B16A16_FLOAT);
}

void DeferredRenderableComponent::Initialize()
{
	ResizeBuffers();

	Renderable3DComponent::Initialize();
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
		RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
		RenderShadows(GRAPHICSTHREAD_IMMEDIATE);
		RenderReflections(GRAPHICSTHREAD_IMMEDIATE);
		RenderScene(GRAPHICSTHREAD_IMMEDIATE);
		RenderSecondaryScene(rtGBuffer, GetFinalRT(), GRAPHICSTHREAD_IMMEDIATE);
		RenderComposition(GetFinalRT(), rtGBuffer, GRAPHICSTHREAD_IMMEDIATE);
	}

	Renderable2DComponent::Render();
}


wiRenderTarget DeferredRenderableComponent::rtGBuffer, DeferredRenderableComponent::rtDeferred, DeferredRenderableComponent::rtLight, DeferredRenderableComponent::rtSSS[2];
void DeferredRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	wiImageEffects fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	rtGBuffer.Activate(threadID, 0, 0, 0, 0);
	{
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_DEFERRED, rtReflection.GetTexture(), getHairParticlesEnabled(), true);
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
		wiRenderer::GetDevice()->EventBegin("SSAO", threadID);
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
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getSSSEnabled())
	{
		wiRenderer::GetDevice()->EventBegin("SSS", threadID);
		fx.stencilRef = STENCILREF_SKIN;
		fx.stencilComp = COMPARISON_LESS;
		fx.quality = QUALITY_BILINEAR;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		rtSSS[1].Activate(threadID, 0, 0, 0, 0);
		rtSSS[0].Activate(threadID, 0, 0, 0, 0);
		static int sssPassCount = 6;
		for (int i = 0; i < sssPassCount; ++i) 
		{
			wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
			rtSSS[i % 2].Set(threadID, rtGBuffer.depth);
			XMFLOAT2 dir = XMFLOAT2(0, 0);
			static float stren = 0.018f;
			if (i % 2 == 0)
			{
				dir.x = stren*((float)wiRenderer::GetInternalResolution().y / (float)wiRenderer::GetInternalResolution().x);
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
			fx.presentFullScreen = true;
			wiImage::Draw(rtLight.GetTexture(0), fx, threadID);
			fx.stencilRef = STENCILREF_SKIN;
			fx.stencilComp = COMPARISON_LESS;
			wiImage::Draw(rtSSS[1].GetTexture(), fx, threadID);
		}

		fx.stencilRef = 0;
		fx.stencilComp = 0;
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	rtDeferred.Activate(threadID, rtGBuffer.depth); {
		wiImage::DrawDeferred((getSSSEnabled() ? rtSSS[0].GetTexture(0) : rtLight.GetTexture(0)), rtLight.GetTexture(1)
			, getSSAOEnabled() ? rtSSAO.back().GetTexture() : wiTextureHelper::getInstance()->getWhite()
			, threadID, STENCILREF_DEFAULT);
		wiRenderer::DrawSky(threadID);
	}


	if (getSSREnabled()){
		wiRenderer::GetDevice()->EventBegin("SSR", threadID);
		rtSSR.Activate(threadID); {
			wiRenderer::GetDevice()->GenerateMips(rtDeferred.GetTexture(0), threadID);
			fx.process.setSSR(true);
			fx.setMaskMap(nullptr);
			wiImage::Draw(rtDeferred.GetTexture(), fx, threadID);
			fx.process.clear();
		}
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	wiProfiler::GetInstance().EndRange(); // Opaque Scene
}

wiRenderTarget& DeferredRenderableComponent::GetFinalRT()
{
	if (getSSREnabled())
		return rtSSR;
	else
		return rtDeferred;
}

wiDepthTarget* DeferredRenderableComponent::GetDepthBuffer()
{
	return rtGBuffer.depth;
}

void DeferredRenderableComponent::setPreferredThreadingCount(unsigned short value)
{
	Renderable3DComponent::setPreferredThreadingCount(value);

	if (!wiRenderer::GetDevice()->CheckCapability(GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_MULTITHREADED_RENDERING))
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
			RenderComposition(GetFinalRT(), rtGBuffer, GRAPHICSTHREAD_SCENE);
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
			RenderComposition(GetFinalRT(), rtGBuffer, GRAPHICSTHREAD_MISC1);
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
			wiRenderer::GetDevice()->FinishCommandList(GRAPHICSTHREAD_MISC1); 
		}));
		workerThreads.push_back(new wiTaskThread([&]
		{
			wiRenderer::UpdateDepthBuffer(dtDepthCopy.GetTexture(), rtLinearDepth.GetTexture(), GRAPHICSTHREAD_MISC2);
			wiRenderer::UpdateGBuffer(rtGBuffer.GetTexture(0), rtGBuffer.GetTexture(1), rtGBuffer.GetTexture(2), nullptr, nullptr, GRAPHICSTHREAD_MISC2);
			RenderComposition(GetFinalRT(), rtGBuffer, GRAPHICSTHREAD_MISC2);
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

