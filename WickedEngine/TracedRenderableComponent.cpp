#include "TracedRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;

TracedRenderableComponent::TracedRenderableComponent()
{
	Renderable3DComponent::setProperties();
}
TracedRenderableComponent::~TracedRenderableComponent()
{
	SAFE_DELETE(traceResult);
}


void TracedRenderableComponent::ResizeBuffers()
{
	Renderable3DComponent::ResizeBuffers();

	FORMAT defaultTextureFormat = wiRenderer::GetDevice()->GetBackBufferFormat();

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

	
	SAFE_DELETE(traceResult);
	
	TextureDesc desc;
	desc.Width = lastBufferResWidth;
	desc.Height = lastBufferResHeight;
	desc.Format = FORMAT_R32G32B32A32_FLOAT;
	desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	desc.Usage = USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.ArraySize = 1;
	desc.MipLevels = 1;
	desc.Depth = 1;

	wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &traceResult);


	rtAccumulation.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, FORMAT_R32G32B32A32_FLOAT); // needs full float for correct accumulation over long time period!

}

void TracedRenderableComponent::Initialize()
{
	ResizeBuffers();

	Renderable3DComponent::Initialize();
}
void TracedRenderableComponent::Load()
{
	Renderable3DComponent::Load();
}
void TracedRenderableComponent::Start()
{
	Renderable3DComponent::Start();
}
void TracedRenderableComponent::Render()
{
	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
	RenderScene(GRAPHICSTHREAD_IMMEDIATE);

	Renderable2DComponent::Render();
}


void TracedRenderableComponent::Update(float dt)
{

	if (wiRenderer::getCamera()->hasChanged)
	{
		sam = 0;
	}
}

void TracedRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Traced Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	wiRenderer::DrawTracedScene(wiRenderer::getCamera(), traceResult, sam == 0, threadID);




	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.hdr = true;



	fx.opacity = 1.0f / (sam + 1.0f);
	fx.blendFlag = BLENDMODE_ALPHA;

	rtAccumulation.Set(threadID);
	wiImage::Draw(traceResult, fx, threadID);



	sam++;

	//if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
	//{
	//	wiRenderer::GetDevice()->EventBegin("Temporal AA Resolve", threadID);
	//	wiProfiler::GetInstance().BeginRange("Temporal AA Resolve", wiProfiler::DOMAIN_GPU, threadID);
	//	fx.blendFlag = BLENDMODE_OPAQUE;
	//	int current = wiRenderer::GetDevice()->GetFrameCount() % 2 == 0 ? 0 : 1;
	//	int history = 1 - current;
	//	rtTemporalAA[current].Set(threadID); {
	//		wiRenderer::UpdateGBuffer(nullptr, nullptr, nullptr, nullptr, nullptr, threadID);
	//		fx.presentFullScreen = false;
	//		fx.process.setTemporalAAResolve(true);
	//		fx.setMaskMap(rtTemporalAA[history].GetTexture());
	//		wiImage::Draw(traceResult, fx, threadID);
	//		fx.process.clear();
	//	}
	//	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_GBUFFER0, 1, threadID);
	//	wiRenderer::GetDevice()->UnBindResources(TEXSLOT_ONDEMAND0, 1, threadID);
	//	wiProfiler::GetInstance().EndRange(threadID);
	//	wiRenderer::GetDevice()->EventEnd(threadID);
	//}



	wiProfiler::GetInstance().EndRange(threadID); // Traced Scene
}

void TracedRenderableComponent::Compose()
{
	wiRenderer::GetDevice()->EventBegin("TracedRenderableComponent::Compose", GRAPHICSTHREAD_IMMEDIATE);


	wiImageEffects fx;
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.presentFullScreen = true;



	//wiImage::Draw(traceResult, fx, GRAPHICSTHREAD_IMMEDIATE);
	
	
	
	
	
	
	wiImage::Draw(rtAccumulation.GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);



	//if (wiRenderer::GetTemporalAAEnabled() && !wiRenderer::GetTemporalAADebugEnabled())
	//{
	//	int current = wiRenderer::GetDevice()->GetFrameCount() % 2 == 0 ? 0 : 1;
	//	wiImage::Draw(rtTemporalAA[current].GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);
	//}
	//else
	//{
	//	wiImage::Draw(traceResult, fx, GRAPHICSTHREAD_IMMEDIATE);
	//}


	wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);

	Renderable2DComponent::Compose();
}


